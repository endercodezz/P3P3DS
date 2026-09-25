# Third-party sources the macOS release bundles, pinned by version and SHA-256.
# Sourced by scripts/release_macos.sh. SDL3 and the fallback font are shared
# with the Linux release and pinned in ../linux/sources.sh; FFmpeg comes from
# the build itself (cmake/FFmpeg.cmake). When a version changes here, update
# THIRD_PARTY_NOTICES.md to match; the release script refuses to package when
# they disagree.

# shellcheck source=../linux/sources.sh
source "$(dirname "${BASH_SOURCE[0]}")/../linux/sources.sh"

# The oldest macOS the release runs on. Everything bundled is built for it or
# rewritten to it, after checking that every system symbol the binaries import
# exists in that release's SDK (see release_macos.sh).
MACOS_DEPLOYMENT_TARGET=13.0

# The Vulkan loader, built from source for the deployment target above. It
# finds MoltenVK through the driver manifest in the app bundle's
# Contents/Resources/vulkan/icd.d.
VULKAN_SDK_TAG=vulkan-sdk-1.4.357.0
VULKAN_LOADER_URL="https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/${VULKAN_SDK_TAG}.tar.gz"
VULKAN_LOADER_SHA256=54f2537df22313768da0317dda2abdaaab7711b4081c48c869a79db343d0ae70
VULKAN_HEADERS_URL="https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/${VULKAN_SDK_TAG}.tar.gz"
VULKAN_HEADERS_SHA256=e87dce08116151f6b6d7de6b6faf41498e87e6cf848ff16fa3bd5402190ad4a3

# MoltenVK, Vulkan on Metal: the Khronos release build, unmodified except that
# only its arm64 slice is kept.
MOLTENVK_VERSION=1.4.2
MOLTENVK_URL="https://github.com/KhronosGroup/MoltenVK/releases/download/v${MOLTENVK_VERSION}/MoltenVK-macos.tar"
MOLTENVK_SHA256=f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e
