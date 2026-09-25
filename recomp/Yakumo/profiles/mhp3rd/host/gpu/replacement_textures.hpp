#pragma once

#include "texture_pack.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mhp3rd::gpu {

// The GPU side of a texture pack: uploads decoded replacement images with a
// full mip chain, hands out their descriptors, and keeps their memory within
// a budget by dropping the least recently drawn ones.
//
// Uploads never wait for the GPU: each goes into its own command buffer,
// submitted ahead of the frame that first draws it, and its staging memory is
// freed once its fence has signalled. Everything here runs on the renderer's
// thread.
class ReplacementTextures {
public:
    struct Device {
        VkPhysicalDevice physical_device{};
        VkDevice device{};
        VkQueue queue{};
        std::uint32_t queue_family{};
        VkDescriptorSetLayout layout{};  // one combined image sampler
    };

    bool initialize(const Device &device, std::string &error);
    void shutdown();

    // The descriptor to sample instead of the original texture, or null while
    // the replacement is not on the GPU yet: then it asks the pack to decode
    // it, or uploads it once decoded. `frame` counts presented frames.
    VkDescriptorSet descriptor(Replacement &replacement, TexturePack &pack, std::uint64_t frame);

    // After the frame fence: frees finished uploads and, while over budget,
    // drops replacements no draw has used for a few frames.
    void begin_frame(std::uint64_t frame);
    // Drops every replacement. The GPU must be idle.
    void clear();
    // The texture filter setting, for replacements the pack leaves on auto.
    // The GPU must be idle.
    void set_sharp(bool sharp);

    [[nodiscard]] std::uint64_t resident_bytes() const noexcept { return resident_bytes_; }
    [[nodiscard]] std::size_t resident_count() const noexcept { return resident_.size(); }

private:
    struct Resident {
        Replacement *owner{};
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
        VkDescriptorSet descriptor{};
        VkDeviceSize bytes{};
        std::uint64_t last_used{};
        VkFence upload_fence{};  // until the upload has finished
    };
    struct Upload {
        VkFence fence{};
        VkCommandBuffer commands{};
        VkBuffer staging{};
        VkDeviceMemory staging_memory{};
    };

    bool upload(Replacement &replacement, Resident &resident);
    void destroy(Resident &resident);
    void write_descriptor(const Resident &resident);
    [[nodiscard]] std::uint32_t memory_type(std::uint32_t mask, VkMemoryPropertyFlags flags) const;
    [[nodiscard]] VkSampler sampler_for(ReplacementFilter filter) const;

    Device device_{};
    VkPhysicalDeviceMemoryProperties memory_properties_{};
    VkCommandPool command_pool_{};
    VkDescriptorPool descriptor_pool_{};
    VkSampler linear_{};
    VkSampler nearest_{};
    bool sharp_{};
    std::unordered_map<Replacement *, std::unique_ptr<Resident>> resident_;
    std::vector<Upload> uploads_;
    std::uint64_t resident_bytes_{};
    std::uint64_t budget_bytes_{};
    std::uint64_t frame_{};
    std::uint64_t uploaded_this_frame_{};
    bool warned_budget_{};
};

} // namespace mhp3rd::gpu
