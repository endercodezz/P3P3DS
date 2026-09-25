#include "dx12_presenter.hpp"
#include "vcs_runtime_log.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

namespace vcs {

#if defined(_WIN32)
namespace {

using Microsoft::WRL::ComPtr;
constexpr UINT kFrameCount = 2u;
constexpr DXGI_FORMAT kSwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

[[nodiscard]] std::string hr_text(HRESULT hr, const char *where) {
    std::ostringstream out;
    out << where << " failed (HRESULT=0x" << std::hex << std::uppercase
        << static_cast<unsigned long>(hr) << ')';
    LPSTR message = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS;
    if (FormatMessageA(flags, nullptr, static_cast<DWORD>(hr),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPSTR>(&message), 0u, nullptr) != 0u && message != nullptr) {
        std::string text(message);
        LocalFree(message);
        while (!text.empty() && (text.back() == '\r' || text.back() == '\n')) text.pop_back();
        if (!text.empty()) out << ": " << text;
    }
    return out.str();
}

struct FrameResources {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12Resource> source_texture;
    ComPtr<ID3D12Resource> upload_buffer;
    std::byte *mapped_upload{};
    UINT64 upload_capacity{};
    UINT64 fence_value{};
    D3D12_RESOURCE_STATES source_state{D3D12_RESOURCE_STATE_COPY_DEST};
    std::uint32_t source_width{};
    std::uint32_t source_height{};
    UINT source_row_pitch{};

    void release_source() noexcept {
        if (upload_buffer && mapped_upload != nullptr) upload_buffer->Unmap(0u, nullptr);
        mapped_upload = nullptr;
        upload_buffer.Reset();
        source_texture.Reset();
        upload_capacity = 0u;
        source_width = 0u;
        source_height = 0u;
        source_row_pitch = 0u;
        source_state = D3D12_RESOURCE_STATE_COPY_DEST;
    }
};

struct PresenterState {
    std::mutex mutex;
    HWND hwnd{};
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12DescriptorHeap> srv_heap;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12RootSignature> root_signature;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12Fence> fence;
    HANDLE fence_event{};
    std::array<ComPtr<ID3D12Resource>, kFrameCount> backbuffers;
    std::array<FrameResources, kFrameCount> frames;
    UINT rtv_stride{};
    UINT srv_stride{};
    UINT backbuffer_index{};
    std::uint32_t client_width{};
    std::uint32_t client_height{};
    UINT64 next_fence_value{1u};
    bool tearing_supported{};
    bool initialized{};
    bool hardware_adapter{};
    std::string adapter_name;
    std::string last_error;
};

PresenterState &state() {
    static PresenterState instance;
    return instance;
}

[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle(PresenterState &s, UINT index) noexcept {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s.rtv_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.rtv_stride;
    return handle;
}

[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle(PresenterState &s, UINT index) noexcept {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s.srv_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s.srv_stride;
    return handle;
}

[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle(PresenterState &s, UINT index) noexcept {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = s.srv_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * s.srv_stride;
    return handle;
}

[[nodiscard]] bool wait_for_fence(PresenterState &s, UINT64 value, std::string &error) noexcept {
    if (value == 0u || s.fence->GetCompletedValue() >= value) return true;
    const HRESULT hr = s.fence->SetEventOnCompletion(value, s.fence_event);
    if (FAILED(hr)) {
        error = hr_text(hr, "ID3D12Fence::SetEventOnCompletion");
        return false;
    }
    const DWORD wait = WaitForSingleObject(s.fence_event, 5000u);
    if (wait != WAIT_OBJECT_0) {
        std::ostringstream out;
        out << "DirectX 12 fence wait failed/timed out (wait=" << wait << ')';
        if (s.device) {
            const HRESULT removed = s.device->GetDeviceRemovedReason();
            if (FAILED(removed)) out << "; device removed reason=0x" << std::hex
                                     << static_cast<unsigned long>(removed);
        }
        error = out.str();
        return false;
    }
    return true;
}

[[nodiscard]] bool wait_idle(PresenterState &s, std::string &error) noexcept {
    if (!s.queue || !s.fence) return true;
    const UINT64 value = s.next_fence_value++;
    const HRESULT signal = s.queue->Signal(s.fence.Get(), value);
    if (FAILED(signal)) {
        error = hr_text(signal, "ID3D12CommandQueue::Signal");
        return false;
    }
    return wait_for_fence(s, value, error);
}

void release_backbuffers(PresenterState &s) noexcept {
    for (auto &buffer : s.backbuffers) buffer.Reset();
}

[[nodiscard]] bool create_backbuffers(PresenterState &s, std::string &error) noexcept {
    for (UINT i = 0u; i < kFrameCount; ++i) {
        HRESULT hr = s.swapchain->GetBuffer(i, IID_PPV_ARGS(&s.backbuffers[i]));
        if (FAILED(hr)) {
            error = hr_text(hr, "IDXGISwapChain::GetBuffer");
            return false;
        }
        s.device->CreateRenderTargetView(s.backbuffers[i].Get(), nullptr, rtv_handle(s, i));
    }
    s.backbuffer_index = s.swapchain->GetCurrentBackBufferIndex();
    return true;
}

[[nodiscard]] bool resize_swapchain_if_needed(PresenterState &s, std::string &error) noexcept {
    RECT client{};
    if (!GetClientRect(s.hwnd, &client)) {
        error = "GetClientRect failed for DirectX 12 window";
        return false;
    }
    const std::uint32_t width = static_cast<std::uint32_t>(std::max<LONG>(1, client.right - client.left));
    const std::uint32_t height = static_cast<std::uint32_t>(std::max<LONG>(1, client.bottom - client.top));
    if (width == s.client_width && height == s.client_height && s.backbuffers[0]) return true;
    if (!wait_idle(s, error)) return false;
    release_backbuffers(s);
    const UINT flags = s.tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;
    const HRESULT hr = s.swapchain->ResizeBuffers(kFrameCount, width, height,
                                                   kSwapchainFormat, flags);
    if (FAILED(hr)) {
        error = hr_text(hr, "IDXGISwapChain::ResizeBuffers");
        return false;
    }
    s.client_width = width;
    s.client_height = height;
    return create_backbuffers(s, error);
}

[[nodiscard]] bool select_adapter(PresenterState &s, std::string &error) noexcept {
    ComPtr<IDXGIAdapter1> fallback;
    for (UINT index = 0u;; ++index) {
        ComPtr<IDXGIAdapter1> candidate;
        HRESULT hr = s.factory->EnumAdapterByGpuPreference(
            index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate));
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(hr)) continue;
        DXGI_ADAPTER_DESC1 desc{};
        candidate->GetDesc1(&desc);
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0u) continue;
        if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0,
                                         __uuidof(ID3D12Device), nullptr))) {
            s.adapter = candidate;
            s.hardware_adapter = true;
            break;
        }
        if (!fallback) fallback = candidate;
    }
    if (!s.adapter && fallback) s.adapter = fallback;
    if (!s.adapter) {
        error = "No Direct3D 12-capable hardware adapter was found";
        return false;
    }
    DXGI_ADAPTER_DESC1 desc{};
    s.adapter->GetDesc1(&desc);
    char utf8[512]{};
    const int count = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                                           utf8, static_cast<int>(sizeof(utf8)), nullptr, nullptr);
    s.adapter_name = count > 0 ? utf8 : "Direct3D 12 adapter";
    return true;
}

[[nodiscard]] bool create_pipeline(PresenterState &s, std::string &error) noexcept {
    const bool linear = vcs_configuration().display.upscale_filter == DisplayUpscaleFilter::Bilinear;
    const char *shader = R"HLSL(
Texture2D<float4> SourceTexture : register(t0);
SamplerState SourceSampler : register(s0);
struct VsOut { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
VsOut VSMain(uint id : SV_VertexID) {
    VsOut o;
    if (id == 0) { o.position=float4(-1.0,-1.0,0.0,1.0); o.uv=float2(0.0,1.0); }
    else if (id == 1) { o.position=float4(-1.0,3.0,0.0,1.0); o.uv=float2(0.0,-1.0); }
    else { o.position=float4(3.0,-1.0,0.0,1.0); o.uv=float2(2.0,1.0); }
    return o;
}
float4 PSMain(VsOut input) : SV_TARGET { return SourceTexture.Sample(SourceSampler, input.uv); }
)HLSL";
    UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS;
    ComPtr<ID3DBlob> vs, ps, errors;
    HRESULT hr = D3DCompile(shader, std::strlen(shader), "VCSNativeDx12Presenter", nullptr, nullptr,
                            "VSMain", "vs_5_1", compile_flags, 0u, &vs, &errors);
    if (FAILED(hr)) {
        error = errors ? std::string(static_cast<const char *>(errors->GetBufferPointer()), errors->GetBufferSize())
                       : hr_text(hr, "D3DCompile(VSMain)");
        return false;
    }
    errors.Reset();
    hr = D3DCompile(shader, std::strlen(shader), "VCSNativeDx12Presenter", nullptr, nullptr,
                    "PSMain", "ps_5_1", compile_flags, 0u, &ps, &errors);
    if (FAILED(hr)) {
        error = errors ? std::string(static_cast<const char *>(errors->GetBufferPointer()), errors->GetBufferSize())
                       : hr_text(hr, "D3DCompile(PSMain)");
        return false;
    }

    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1u;
    range.BaseShaderRegister = 0u;
    range.RegisterSpace = 0u;
    range.OffsetInDescriptorsFromTableStart = 0u;
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable.NumDescriptorRanges = 1u;
    parameter.DescriptorTable.pDescriptorRanges = &range;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = linear ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1u;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0u;
    sampler.RegisterSpace = 0u;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC root{};
    root.NumParameters = 1u;
    root.pParameters = &parameter;
    root.NumStaticSamplers = 1u;
    root.pStaticSamplers = &sampler;
    root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> root_blob;
    errors.Reset();
    hr = D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1,
                                     &root_blob, &errors);
    if (FAILED(hr)) {
        error = errors ? std::string(static_cast<const char *>(errors->GetBufferPointer()), errors->GetBufferSize())
                       : hr_text(hr, "D3D12SerializeRootSignature");
        return false;
    }
    hr = s.device->CreateRootSignature(0u, root_blob->GetBufferPointer(), root_blob->GetBufferSize(),
                                       IID_PPV_ARGS(&s.root_signature));
    if (FAILED(hr)) {
        error = hr_text(hr, "ID3D12Device::CreateRootSignature");
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = s.root_signature.Get();
    pso.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    pso.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = FALSE;
    D3D12_RENDER_TARGET_BLEND_DESC rt{};
    rt.BlendEnable = FALSE;
    rt.LogicOpEnable = FALSE;
    rt.SrcBlend = D3D12_BLEND_ONE;
    rt.DestBlend = D3D12_BLEND_ZERO;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.BlendState.RenderTarget[0] = rt;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.RasterizerState.FrontCounterClockwise = FALSE;
    pso.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    pso.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    pso.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.RasterizerState.MultisampleEnable = FALSE;
    pso.RasterizerState.AntialiasedLineEnable = FALSE;
    pso.RasterizerState.ForcedSampleCount = 0u;
    pso.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    pso.DepthStencilState.DepthEnable = FALSE;
    pso.DepthStencilState.StencilEnable = FALSE;
    pso.InputLayout = {nullptr, 0u};
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1u;
    pso.RTVFormats[0] = kSwapchainFormat;
    pso.SampleDesc.Count = 1u;
    hr = s.device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&s.pipeline));
    if (FAILED(hr)) {
        error = hr_text(hr, "ID3D12Device::CreateGraphicsPipelineState");
        return false;
    }
    return true;
}

[[nodiscard]] bool ensure_source(PresenterState &s, UINT frame_index,
                                 std::uint32_t width, std::uint32_t height,
                                 std::string &error) noexcept {
    FrameResources &frame = s.frames[frame_index];
    if (frame.source_texture && frame.source_width == width && frame.source_height == height)
        return true;
    if (!wait_for_fence(s, frame.fence_value, error)) return false;
    frame.release_source();

    D3D12_RESOURCE_DESC texture{};
    texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width = width;
    texture.Height = height;
    texture.DepthOrArraySize = 1u;
    texture.MipLevels = 1u;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.SampleDesc.Count = 1u;
    texture.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture.Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_PROPERTIES default_heap{};
    default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    HRESULT hr = s.device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
                                                    &texture, D3D12_RESOURCE_STATE_COPY_DEST,
                                                    nullptr, IID_PPV_ARGS(&frame.source_texture));
    if (FAILED(hr)) {
        error = hr_text(hr, "CreateCommittedResource(DX12 source texture)");
        return false;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rows = 0u;
    UINT64 row_size = 0u, total = 0u;
    s.device->GetCopyableFootprints(&texture, 0u, 1u, 0u, &footprint, &rows, &row_size, &total);
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = std::max<UINT64>(total, 256u);
    buffer.Height = 1u;
    buffer.DepthOrArraySize = 1u;
    buffer.MipLevels = 1u;
    buffer.Format = DXGI_FORMAT_UNKNOWN;
    buffer.SampleDesc.Count = 1u;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES upload_heap{};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    hr = s.device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE,
                                            &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                                            nullptr, IID_PPV_ARGS(&frame.upload_buffer));
    if (FAILED(hr)) {
        error = hr_text(hr, "CreateCommittedResource(DX12 upload buffer)");
        frame.release_source();
        return false;
    }
    void *mapped = nullptr;
    const D3D12_RANGE no_read{0u, 0u};
    hr = frame.upload_buffer->Map(0u, &no_read, &mapped);
    if (FAILED(hr) || mapped == nullptr) {
        error = hr_text(hr, "ID3D12Resource::Map(DX12 upload buffer)");
        frame.release_source();
        return false;
    }
    frame.mapped_upload = static_cast<std::byte *>(mapped);
    frame.upload_capacity = total;
    frame.source_width = width;
    frame.source_height = height;
    frame.source_row_pitch = footprint.Footprint.RowPitch;
    frame.source_state = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1u;
    s.device->CreateShaderResourceView(frame.source_texture.Get(), &srv,
                                       srv_cpu_handle(s, frame_index));
    return true;
}

void barrier(ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
             D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) noexcept {
    if (before == after) return;
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = resource;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = after;
    list->ResourceBarrier(1u, &b);
}

void shutdown_unlocked(PresenterState &s) noexcept {
    std::string ignored;
    if (s.initialized || s.queue) (void)wait_idle(s, ignored);
    for (FrameResources &frame : s.frames) {
        frame.release_source();
        frame.allocator.Reset();
        frame.fence_value = 0u;
    }
    release_backbuffers(s);
    if (s.fence_event != nullptr) CloseHandle(s.fence_event);
    s.fence_event = nullptr;
    s.fence.Reset();
    s.pipeline.Reset();
    s.root_signature.Reset();
    s.command_list.Reset();
    s.srv_heap.Reset();
    s.rtv_heap.Reset();
    s.swapchain.Reset();
    s.queue.Reset();
    s.device.Reset();
    s.adapter.Reset();
    s.factory.Reset();
    s.hwnd = nullptr;
    s.client_width = s.client_height = 0u;
    s.next_fence_value = 1u;
    s.tearing_supported = false;
    s.initialized = false;
    s.hardware_adapter = false;
    s.adapter_name.clear();
}

} // namespace

bool dx12_presenter_initialize(void *native_window, std::string &error) noexcept {
    PresenterState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    if (s.initialized && s.hwnd == static_cast<HWND>(native_window)) return true;
    if (native_window == nullptr) {
        error = "DirectX 12 presenter received a null HWND";
        return false;
    }
    shutdown_unlocked(s);
    s.hwnd = static_cast<HWND>(native_window);

    UINT factory_flags = 0u;
    if (const char *debug = std::getenv("PSPRECOMP_DX12_DEBUG");
        debug != nullptr && *debug != '\0' && *debug != '0') {
        ComPtr<ID3D12Debug> debug_layer;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_layer)))) {
            debug_layer->EnableDebugLayer();
            factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
    HRESULT hr = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&s.factory));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateDXGIFactory2"); return false; }
    if (!select_adapter(s, error)) { s.last_error = error; return false; }
    hr = D3D12CreateDevice(s.adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&s.device));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "D3D12CreateDevice"); return false; }

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    hr = s.device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&s.queue));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateCommandQueue"); return false; }

    BOOL tearing = FALSE;
    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(s.factory.As(&factory5))) {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                 &tearing, sizeof(tearing)))) tearing = FALSE;
    }
    s.tearing_supported = tearing == TRUE;

    RECT client{};
    GetClientRect(s.hwnd, &client);
    s.client_width = static_cast<std::uint32_t>(std::max<LONG>(1, client.right - client.left));
    s.client_height = static_cast<std::uint32_t>(std::max<LONG>(1, client.bottom - client.top));
    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = s.client_width;
    swap_desc.Height = s.client_height;
    swap_desc.Format = kSwapchainFormat;
    swap_desc.SampleDesc.Count = 1u;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = kFrameCount;
    swap_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swap_desc.Flags = s.tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;
    ComPtr<IDXGISwapChain1> swap1;
    hr = s.factory->CreateSwapChainForHwnd(s.queue.Get(), s.hwnd, &swap_desc,
                                            nullptr, nullptr, &swap1);
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateSwapChainForHwnd"); return false; }
    (void)s.factory->MakeWindowAssociation(s.hwnd, DXGI_MWA_NO_ALT_ENTER);
    hr = swap1.As(&s.swapchain);
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "Query IDXGISwapChain3"); return false; }

    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{};
    rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc.NumDescriptors = kFrameCount;
    hr = s.device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&s.rtv_heap));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateDescriptorHeap(RTV)"); return false; }
    s.rtv_stride = s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC srv_desc{};
    srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srv_desc.NumDescriptors = kFrameCount;
    srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = s.device->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&s.srv_heap));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateDescriptorHeap(SRV)"); return false; }
    s.srv_stride = s.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (FrameResources &frame : s.frames) {
        hr = s.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              IID_PPV_ARGS(&frame.allocator));
        if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateCommandAllocator"); return false; }
    }
    hr = s.device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     s.frames[0].allocator.Get(), nullptr,
                                     IID_PPV_ARGS(&s.command_list));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateCommandList"); return false; }
    s.command_list->Close();
    hr = s.device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s.fence));
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CreateFence"); return false; }
    s.fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (s.fence_event == nullptr) { error = s.last_error = "CreateEventW failed for DirectX 12 fence"; return false; }
    if (!create_backbuffers(s, error)) { s.last_error = error; return false; }
    if (!create_pipeline(s, error)) { s.last_error = error; return false; }
    s.initialized = true;
    s.last_error.clear();
    std::ostringstream log;
    log << "dx12 presenter initialized adapter=" << s.adapter_name
        << " frames_in_flight=" << kFrameCount
        << (s.tearing_supported ? " tearing=1" : " tearing=0")
        << " size=" << s.client_width << 'x' << s.client_height;
    runtime_log_line(log.str());
    return true;
}

bool dx12_presenter_present_rgba(std::span<const std::byte> rgba,
                                 std::uint32_t source_width,
                                 std::uint32_t source_height,
                                 const DisplayConfiguration &display,
                                 bool force_preserve_aspect,
                                 std::string &error) noexcept {
    PresenterState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    if (!s.initialized || !s.swapchain || source_width == 0u || source_height == 0u) {
        error = "DirectX 12 presenter is not initialized";
        return false;
    }
    const std::size_t required = static_cast<std::size_t>(source_width) * source_height * 4u;
    if (rgba.size() < required) { error = "DirectX 12 presenter received a short RGBA frame"; return false; }
    if (!resize_swapchain_if_needed(s, error)) { s.last_error = error; return false; }

    const UINT frame_index = s.swapchain->GetCurrentBackBufferIndex();
    FrameResources &frame = s.frames[frame_index];
    if (!wait_for_fence(s, frame.fence_value, error)) { s.last_error = error; return false; }
    if (!ensure_source(s, frame_index, source_width, source_height, error)) {
        s.last_error = error; return false;
    }

    const std::size_t source_row = static_cast<std::size_t>(source_width) * 4u;
    for (std::uint32_t y = 0u; y < source_height; ++y) {
        std::memcpy(frame.mapped_upload + static_cast<std::size_t>(y) * frame.source_row_pitch,
                    rgba.data() + static_cast<std::size_t>(y) * source_row, source_row);
    }
    const D3D12_RANGE written{0u, static_cast<SIZE_T>(frame.source_row_pitch) * source_height};
    (void)written; // Persistently mapped UPLOAD heaps are coherent for CPU writes.

    HRESULT hr = frame.allocator->Reset();
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "CommandAllocator::Reset"); return false; }
    hr = s.command_list->Reset(frame.allocator.Get(), s.pipeline.Get());
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "GraphicsCommandList::Reset"); return false; }

    if (frame.source_state != D3D12_RESOURCE_STATE_COPY_DEST) {
        barrier(s.command_list.Get(), frame.source_texture.Get(), frame.source_state,
                D3D12_RESOURCE_STATE_COPY_DEST);
        frame.source_state = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    D3D12_RESOURCE_DESC texture_desc = frame.source_texture->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rows = 0u;
    UINT64 row_size = 0u, total = 0u;
    s.device->GetCopyableFootprints(&texture_desc, 0u, 1u, 0u,
                                     &footprint, &rows, &row_size, &total);
    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = frame.source_texture.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0u;
    D3D12_TEXTURE_COPY_LOCATION src{};
    src.pResource = frame.upload_buffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;
    s.command_list->CopyTextureRegion(&dst, 0u, 0u, 0u, &src, nullptr);
    barrier(s.command_list.Get(), frame.source_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    frame.source_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    ID3D12Resource *backbuffer = s.backbuffers[frame_index].Get();
    barrier(s.command_list.Get(), backbuffer, D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtv_handle(s, frame_index);
    s.command_list->OMSetRenderTargets(1u, &rtv, FALSE, nullptr);
    constexpr float black[4]{0.0f, 0.0f, 0.0f, 1.0f};
    s.command_list->ClearRenderTargetView(rtv, black, 0u, nullptr);

    const DisplayAspectMode mode = force_preserve_aspect ? DisplayAspectMode::Preserve
                                                         : display.aspect_mode;
    const PresentationRectangle rect = calculate_presentation_rectangle(
        s.client_width, s.client_height, source_width, source_height,
        mode, display.integer_scale);
    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = static_cast<float>(rect.x);
    viewport.TopLeftY = static_cast<float>(rect.y);
    viewport.Width = static_cast<float>(std::max(1, rect.width));
    viewport.Height = static_cast<float>(std::max(1, rect.height));
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor{rect.x, rect.y, rect.x + std::max(1, rect.width),
                       rect.y + std::max(1, rect.height)};
    s.command_list->RSSetViewports(1u, &viewport);
    s.command_list->RSSetScissorRects(1u, &scissor);
    ID3D12DescriptorHeap *heaps[]{s.srv_heap.Get()};
    s.command_list->SetDescriptorHeaps(1u, heaps);
    s.command_list->SetGraphicsRootSignature(s.root_signature.Get());
    s.command_list->SetGraphicsRootDescriptorTable(0u, srv_gpu_handle(s, frame_index));
    s.command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    s.command_list->DrawInstanced(3u, 1u, 0u, 0u);
    barrier(s.command_list.Get(), backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    hr = s.command_list->Close();
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "GraphicsCommandList::Close"); return false; }
    ID3D12CommandList *lists[]{s.command_list.Get()};
    s.queue->ExecuteCommandLists(1u, lists);

    const UINT present_flags = s.tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0u;
    hr = s.swapchain->Present(0u, present_flags);
    if (FAILED(hr)) {
        std::ostringstream out;
        out << hr_text(hr, "IDXGISwapChain::Present");
        const HRESULT removed = s.device->GetDeviceRemovedReason();
        if (FAILED(removed)) out << "; device removed reason=0x" << std::hex
                                 << static_cast<unsigned long>(removed);
        error = s.last_error = out.str();
        runtime_log_error("dx12 present", error);
        return false;
    }
    frame.fence_value = s.next_fence_value++;
    hr = s.queue->Signal(s.fence.Get(), frame.fence_value);
    if (FAILED(hr)) { error = s.last_error = hr_text(hr, "ID3D12CommandQueue::Signal"); return false; }
    s.backbuffer_index = s.swapchain->GetCurrentBackBufferIndex();
    s.last_error.clear();
    return true;
}

void dx12_presenter_shutdown() noexcept {
    PresenterState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    if (s.initialized) runtime_log_line("dx12 presenter shutdown");
    shutdown_unlocked(s);
}

bool dx12_presenter_active() noexcept {
    PresenterState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    return s.initialized;
}

Dx12PresenterStatus dx12_presenter_status() {
    PresenterState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    return {s.initialized, s.hardware_adapter, s.tearing_supported, kFrameCount,
            s.adapter_name, s.last_error};
}

#else

bool dx12_presenter_initialize(void *, std::string &error) noexcept {
    error = "DirectX 12 is available only on Windows";
    return false;
}
bool dx12_presenter_present_rgba(std::span<const std::byte>, std::uint32_t, std::uint32_t,
                                 const DisplayConfiguration &, bool, std::string &error) noexcept {
    error = "DirectX 12 is available only on Windows";
    return false;
}
void dx12_presenter_shutdown() noexcept {}
bool dx12_presenter_active() noexcept { return false; }
Dx12PresenterStatus dx12_presenter_status() { return {}; }

#endif

} // namespace vcs
