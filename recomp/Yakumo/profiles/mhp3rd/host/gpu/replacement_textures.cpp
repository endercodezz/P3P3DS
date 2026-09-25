#include "replacement_textures.hpp"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace mhp3rd::gpu {
namespace {

// Replacements on the GPU at once, at most; each holds one descriptor set.
constexpr std::uint32_t kMaxResident = 4096u;
// Pixels copied to the GPU per frame, at most, past the first image: a scene
// change can bring in dozens of large images, and copying them all in one
// frame would stall it.
constexpr std::uint64_t kUploadBytesPerFrame = 32ull * 1024u * 1024u;
// A replacement drawn within this many frames is never dropped for the budget.
// When a frame begins, the renderer has waited only for the frame two back:
// the frame before may still be drawing, and so may the presents between the
// flips before it, which draw the frame before that again with frame
// interpolation. Those reach three frames back; a fourth is margin. (Two was
// enough while each frame waited for the one before.)
constexpr std::uint64_t kKeepFrames = 4u;

std::uint64_t budget_from_environment() {
    // MHP3RD_TEXTURE_PACK_MEMORY: megabytes of GPU memory for replacements.
    std::uint64_t megabytes = 1024u;
    if (const char *value = std::getenv("MHP3RD_TEXTURE_PACK_MEMORY"); value != nullptr && *value != '\0') {
        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(value, &end, 10);
        if (end != value && parsed >= 64u) megabytes = parsed;
    }
    return megabytes * 1024u * 1024u;
}

bool check(VkResult result, const char *what, std::string &error) {
    if (result == VK_SUCCESS) return true;
    error = std::string(what) + " failed (" + std::to_string(static_cast<int>(result)) + ")";
    return false;
}

void barrier(VkCommandBuffer commands, VkImage image, std::uint32_t level, std::uint32_t levels, VkImageLayout from,
             VkImageLayout to, VkAccessFlags source_access, VkAccessFlags destination_access,
             VkPipelineStageFlags source_stage, VkPipelineStageFlags destination_stage) {
    VkImageMemoryBarrier info{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    info.oldLayout = from;
    info.newLayout = to;
    info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.image = image;
    info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, level, levels, 0u, 1u};
    info.srcAccessMask = source_access;
    info.dstAccessMask = destination_access;
    vkCmdPipelineBarrier(commands, source_stage, destination_stage, 0u, 0u, nullptr, 0u, nullptr, 1u, &info);
}

} // namespace

bool ReplacementTextures::initialize(const Device &device, std::string &error) {
    device_ = device;
    budget_bytes_ = budget_from_environment();
    vkGetPhysicalDeviceMemoryProperties(device.physical_device, &memory_properties_);

    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = device.queue_family;
    if (!check(vkCreateCommandPool(device.device, &pool_info, nullptr, &command_pool_), "vkCreateCommandPool", error))
        return false;

    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxResident};
    VkDescriptorPoolCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_info.maxSets = kMaxResident;
    descriptor_info.poolSizeCount = 1u;
    descriptor_info.pPoolSizes = &size;
    if (!check(vkCreateDescriptorPool(device.device, &descriptor_info, nullptr, &descriptor_pool_),
               "vkCreateDescriptorPool", error))
        return false;

    // Replacements are usually several times the original's size and are
    // drawn at the original's size on screen, so they are mip-mapped;
    // repeat addressing matches the renderer's own samplers.
    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    if (!check(vkCreateSampler(device.device, &sampler_info, nullptr, &linear_), "vkCreateSampler", error))
        return false;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    return check(vkCreateSampler(device.device, &sampler_info, nullptr, &nearest_), "vkCreateSampler", error);
}

void ReplacementTextures::shutdown() {
    if (device_.device == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(device_.device);
    clear();
    if (linear_ != VK_NULL_HANDLE) vkDestroySampler(device_.device, linear_, nullptr);
    if (nearest_ != VK_NULL_HANDLE) vkDestroySampler(device_.device, nearest_, nullptr);
    if (descriptor_pool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_.device, descriptor_pool_, nullptr);
    if (command_pool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_.device, command_pool_, nullptr);
    *this = ReplacementTextures{};
}

std::uint32_t ReplacementTextures::memory_type(std::uint32_t mask, VkMemoryPropertyFlags flags) const {
    for (std::uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i)
        if ((mask & (1u << i)) != 0u && (memory_properties_.memoryTypes[i].propertyFlags & flags) == flags) return i;
    return 0u;
}

VkSampler ReplacementTextures::sampler_for(ReplacementFilter filter) const {
    switch (filter) {
    case ReplacementFilter::Nearest: return nearest_;
    case ReplacementFilter::Linear: return linear_;
    default: return sharp_ ? nearest_ : linear_;
    }
}

void ReplacementTextures::write_descriptor(const Resident &resident) {
    VkDescriptorImageInfo image_info{sampler_for(resident.owner->filter), resident.view,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = resident.descriptor;
    write.descriptorCount = 1u;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;
    vkUpdateDescriptorSets(device_.device, 1u, &write, 0u, nullptr);
}

VkDescriptorSet ReplacementTextures::descriptor(Replacement &replacement, TexturePack &pack, std::uint64_t frame) {
    if (replacement.gpu != nullptr) {
        auto &resident = *static_cast<Resident *>(replacement.gpu);
        resident.last_used = frame;
        return resident.descriptor;
    }
    switch (replacement.state.load(std::memory_order_acquire)) {
    case Replacement::State::Unloaded:
        pack.request(replacement);
        return VK_NULL_HANDLE;
    case Replacement::State::Decoded: break;
    default: return VK_NULL_HANDLE;
    }
    if (frame != frame_) {
        frame_ = frame;
        uploaded_this_frame_ = 0u;
    }
    if (uploaded_this_frame_ != 0u && uploaded_this_frame_ + replacement.pixels.size() > kUploadBytesPerFrame)
        return VK_NULL_HANDLE;  // next frame; the original is drawn meanwhile
    if (resident_.size() >= kMaxResident) return VK_NULL_HANDLE;

    auto resident = std::make_unique<Resident>();
    resident->owner = &replacement;
    if (!upload(replacement, *resident)) {
        destroy(*resident);
        // Retrying every draw would only fail again; the original stays.
        pack.consumed(replacement);
        replacement.state.store(Replacement::State::Failed, std::memory_order_release);
        return VK_NULL_HANDLE;
    }
    if (texture_pack_trace())
        std::cout << "[texpack] frame " << frame << ": uploaded " << replacement.name << " (" << (resident->bytes >> 10u)
                  << " KiB with mips)\n";
    uploaded_this_frame_ += replacement.pixels.size();
    resident->last_used = frame;
    resident_bytes_ += resident->bytes;
    pack.consumed(replacement);
    replacement.gpu = resident.get();
    const VkDescriptorSet descriptor = resident->descriptor;
    resident_.emplace(&replacement, std::move(resident));
    return descriptor;
}

bool ReplacementTextures::upload(Replacement &replacement, Resident &resident) {
    std::string error;
    const VkDevice device = device_.device;
    const std::uint32_t width = replacement.width;
    const std::uint32_t height = replacement.height;
    const std::uint32_t levels = static_cast<std::uint32_t>(std::bit_width(std::max(width, height)));

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = {width, height, 1u};
    image_info.mipLevels = levels;
    image_info.arrayLayers = 1u;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!check(vkCreateImage(device, &image_info, nullptr, &resident.image), "vkCreateImage", error)) return false;
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, resident.image, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!check(vkAllocateMemory(device, &allocate, nullptr, &resident.memory), "vkAllocateMemory", error)) {
        std::cerr << "[texpack] no GPU memory for " << replacement.name << ": " << error << "\n";
        return false;
    }
    vkBindImageMemory(device, resident.image, resident.memory, 0u);
    resident.bytes = requirements.size;

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = resident.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, levels, 0u, 1u};
    if (!check(vkCreateImageView(device, &view_info, nullptr, &resident.view), "vkCreateImageView", error))
        return false;

    VkDescriptorSetAllocateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    set_info.descriptorPool = descriptor_pool_;
    set_info.descriptorSetCount = 1u;
    set_info.pSetLayouts = &device_.layout;
    if (!check(vkAllocateDescriptorSets(device, &set_info, &resident.descriptor), "vkAllocateDescriptorSets", error))
        return false;
    write_descriptor(resident);

    Upload upload{};
    const VkDeviceSize bytes = replacement.pixels.size();
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    const auto release = [&] {
        if (upload.staging != VK_NULL_HANDLE) vkDestroyBuffer(device, upload.staging, nullptr);
        if (upload.staging_memory != VK_NULL_HANDLE) vkFreeMemory(device, upload.staging_memory, nullptr);
        if (upload.commands != VK_NULL_HANDLE) vkFreeCommandBuffers(device, command_pool_, 1u, &upload.commands);
        if (upload.fence != VK_NULL_HANDLE) vkDestroyFence(device, upload.fence, nullptr);
    };
    if (!check(vkCreateBuffer(device, &buffer_info, nullptr, &upload.staging), "vkCreateBuffer", error)) {
        release();
        return false;
    }
    vkGetBufferMemoryRequirements(device, upload.staging, &requirements);
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void *mapped = nullptr;
    if (!check(vkAllocateMemory(device, &allocate, nullptr, &upload.staging_memory), "vkAllocateMemory", error) ||
        !check(vkBindBufferMemory(device, upload.staging, upload.staging_memory, 0u), "vkBindBufferMemory", error) ||
        !check(vkMapMemory(device, upload.staging_memory, 0u, bytes, 0u, &mapped), "vkMapMemory", error)) {
        release();
        return false;
    }
    std::memcpy(mapped, replacement.pixels.data(), static_cast<std::size_t>(bytes));
    vkUnmapMemory(device, upload.staging_memory);

    VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = command_pool_;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1u;
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (!check(vkAllocateCommandBuffers(device, &command_info, &upload.commands), "vkAllocateCommandBuffers", error) ||
        !check(vkCreateFence(device, &fence_info, nullptr, &upload.fence), "vkCreateFence", error)) {
        release();
        return false;
    }
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(upload.commands, &begin);
    const VkImage image = resident.image;
    barrier(upload.commands, image, 0u, levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0u,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {width, height, 1u};
    vkCmdCopyBufferToImage(upload.commands, upload.staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
    // Each level is the previous one halved with a linear filter.
    std::int32_t level_width = static_cast<std::int32_t>(width);
    std::int32_t level_height = static_cast<std::int32_t>(height);
    for (std::uint32_t level = 1; level < levels; ++level) {
        barrier(upload.commands, image, level - 1u, 1u, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        const std::int32_t next_width = std::max(level_width / 2, 1);
        const std::int32_t next_height = std::max(level_height / 2, 1);
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1u, 0u, 1u};
        blit.srcOffsets[1] = {level_width, level_height, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0u, 1u};
        blit.dstOffsets[1] = {next_width, next_height, 1};
        vkCmdBlitImage(upload.commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_LINEAR);
        barrier(upload.commands, image, level - 1u, 1u, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        level_width = next_width;
        level_height = next_height;
    }
    barrier(upload.commands, image, levels - 1u, 1u, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkEndCommandBuffer(upload.commands);
    // Submitted now, ahead of the frame being recorded: queue order puts the
    // copy before every draw that samples the image, with no wait here.
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &upload.commands;
    if (!check(vkQueueSubmit(device_.queue, 1u, &submit, upload.fence), "vkQueueSubmit", error)) {
        release();
        return false;
    }
    resident.upload_fence = upload.fence;
    uploads_.push_back(upload);
    return true;
}

void ReplacementTextures::destroy(Resident &resident) {
    const VkDevice device = device_.device;
    if (resident.descriptor != VK_NULL_HANDLE) vkFreeDescriptorSets(device, descriptor_pool_, 1u, &resident.descriptor);
    if (resident.view != VK_NULL_HANDLE) vkDestroyImageView(device, resident.view, nullptr);
    if (resident.image != VK_NULL_HANDLE) vkDestroyImage(device, resident.image, nullptr);
    if (resident.memory != VK_NULL_HANDLE) vkFreeMemory(device, resident.memory, nullptr);
    resident.descriptor = VK_NULL_HANDLE;
    resident.view = VK_NULL_HANDLE;
    resident.image = VK_NULL_HANDLE;
    resident.memory = VK_NULL_HANDLE;
}

void ReplacementTextures::begin_frame(std::uint64_t frame) {
    if (device_.device == VK_NULL_HANDLE) return;
    const VkDevice device = device_.device;
    // Finished uploads give back their staging memory.
    std::erase_if(uploads_, [&](Upload &upload) {
        if (vkGetFenceStatus(device, upload.fence) != VK_SUCCESS) return false;
        for (auto &[owner, resident] : resident_)
            if (resident->upload_fence == upload.fence) resident->upload_fence = VK_NULL_HANDLE;
        vkDestroyBuffer(device, upload.staging, nullptr);
        vkFreeMemory(device, upload.staging_memory, nullptr);
        vkFreeCommandBuffers(device, command_pool_, 1u, &upload.commands);
        vkDestroyFence(device, upload.fence, nullptr);
        return true;
    });

    if (resident_bytes_ <= budget_bytes_ && resident_.size() < kMaxResident) return;
    // Over budget: drop the least recently drawn replacements. The frame
    // fence has signalled, so no recorded work still reads them; the texture
    // that used one draws its original until the file is loaded again.
    std::vector<Resident *> candidates;
    for (auto &[owner, resident] : resident_)
        if (resident->upload_fence == VK_NULL_HANDLE && resident->last_used + kKeepFrames < frame)
            candidates.push_back(resident.get());
    std::sort(candidates.begin(), candidates.end(),
              [](const Resident *a, const Resident *b) { return a->last_used < b->last_used; });
    for (Resident *resident : candidates) {
        if (resident_bytes_ <= budget_bytes_ * 7u / 8u && resident_.size() < kMaxResident) break;
        Replacement *owner = resident->owner;
        resident_bytes_ -= resident->bytes;
        destroy(*resident);
        owner->gpu = nullptr;
        owner->state.store(Replacement::State::Unloaded, std::memory_order_release);
        resident_.erase(owner);
    }
    if (resident_bytes_ > budget_bytes_ && !warned_budget_) {
        warned_budget_ = true;
        std::cerr << "[texpack] the replacements drawn right now need " << (resident_bytes_ >> 20u)
                  << " MB, over the " << (budget_bytes_ >> 20u) << " MB budget (MHP3RD_TEXTURE_PACK_MEMORY)\n";
    }
}

void ReplacementTextures::clear() {
    if (device_.device == VK_NULL_HANDLE) return;
    for (Upload &upload : uploads_) {
        vkWaitForFences(device_.device, 1u, &upload.fence, VK_TRUE, UINT64_MAX);
        vkDestroyBuffer(device_.device, upload.staging, nullptr);
        vkFreeMemory(device_.device, upload.staging_memory, nullptr);
        vkFreeCommandBuffers(device_.device, command_pool_, 1u, &upload.commands);
        vkDestroyFence(device_.device, upload.fence, nullptr);
    }
    uploads_.clear();
    for (auto &[owner, resident] : resident_) {
        destroy(*resident);
        owner->gpu = nullptr;
        owner->state.store(Replacement::State::Unloaded, std::memory_order_release);
    }
    resident_.clear();
    resident_bytes_ = 0u;
}

void ReplacementTextures::set_sharp(bool sharp) {
    sharp_ = sharp;
    for (auto &[owner, resident] : resident_) write_descriptor(*resident);
}

} // namespace mhp3rd::gpu
