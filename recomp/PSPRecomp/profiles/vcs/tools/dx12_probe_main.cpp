#include "dx12_presenter.hpp"
#include "vcs_config.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main() {
#if !defined(_WIN32)
    std::cout << "{\"ok\":false,\"reason\":\"windows-only\"}\n";
    return 0;
#else
    const wchar_t *class_name = L"VCSNativeDx12Probe";
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = class_name;
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0u, class_name, L"VCSNative DX12 probe",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                320, 240, nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::cout << "{\"ok\":false,\"reason\":\"CreateWindowExW\"}\n";
        return 2;
    }
    ShowWindow(hwnd, SW_HIDE);
    std::string error;
    if (!vcs::dx12_presenter_initialize(hwnd, error)) {
        std::cout << "{\"ok\":false,\"reason\":\"" << error << "\"}\n";
        DestroyWindow(hwnd);
        return 3;
    }
    constexpr std::uint32_t width = 64u, height = 64u;
    std::vector<std::byte> pixels(static_cast<std::size_t>(width) * height * 4u);
    for (std::uint32_t y = 0u; y < height; ++y) {
        for (std::uint32_t x = 0u; x < width; ++x) {
            const bool white = ((x / 8u) ^ (y / 8u)) & 1u;
            const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 4u;
            pixels[i + 0u] = white ? std::byte{0xFF} : std::byte{0x20};
            pixels[i + 1u] = white ? std::byte{0xFF} : std::byte{0x40};
            pixels[i + 2u] = white ? std::byte{0xFF} : std::byte{0x80};
            pixels[i + 3u] = std::byte{0xFF};
        }
    }
    vcs::DisplayConfiguration display{};
    display.aspect_mode = vcs::DisplayAspectMode::Preserve;
    display.upscale_filter = vcs::DisplayUpscaleFilter::Nearest;
    const bool presented = vcs::dx12_presenter_present_rgba(
        pixels, width, height, display, false, error);
    const vcs::Dx12PresenterStatus status = vcs::dx12_presenter_status();
    std::cout << "{\"ok\":" << (presented ? "true" : "false")
              << ",\"adapter\":\"" << status.adapter_name
              << "\",\"frames_in_flight\":" << status.frames_in_flight
              << ",\"tearing\":" << (status.tearing_supported ? "true" : "false")
              << ",\"error\":\"" << error << "\"}\n";
    vcs::dx12_presenter_shutdown();
    DestroyWindow(hwnd);
    UnregisterClassW(class_name, wc.hInstance);
    return presented ? 0 : 4;
#endif
}
