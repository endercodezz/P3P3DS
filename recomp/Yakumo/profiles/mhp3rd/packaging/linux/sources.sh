# Third-party sources a Linux release bundles, pinned by version and SHA-256.
# Sourced by build_in_sdk.sh and scripts/release_linux.sh. When a version
# changes here or in cmake/FFmpeg.cmake, update THIRD_PARTY_NOTICES.md to
# match; the release script refuses to package when they disagree.

SDL3_VERSION=3.4.16
SDL3_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"
SDL3_SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68

# FFmpeg is not pinned here: the build itself downloads, checks and builds the
# LGPL-only FFmpeg it bundles (cmake/FFmpeg.cmake, MHP3RD_FFMPEG=bundled).

# Japanese text needs a CJK font. Releases carry one as a fallback for systems
# (and Flatpak runtimes) without one.
NOTO_CJK_TAG=Sans2.004
NOTO_CJK_URL="https://github.com/notofonts/noto-cjk/raw/${NOTO_CJK_TAG}/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf"
NOTO_CJK_SHA256=68a3fc98800b2a27b371f2fb79991daf3633bd89309d4ffaa6946fd587f375b5
NOTO_CJK_LICENSE_URL="https://github.com/notofonts/noto-cjk/raw/${NOTO_CJK_TAG}/LICENSE"
NOTO_CJK_LICENSE_SHA256=6a73f9541c2de74158c0e7cf6b0a58ef774f5a780bf191f2d7ec9cc53efe2bf2

# The build environment: the Steam Runtime 3 "sniper" SDK (Debian 11, glibc
# 2.31), so the result runs on SteamOS and on most distributions of the last
# few years, pinned by digest (the 3.0.20260805 SDK) so a rebuild uses the
# same compilers; override with YAKUMO_SDK_IMAGE.
SDK_IMAGE="${YAKUMO_SDK_IMAGE:-registry.gitlab.steamos.cloud/steamrt/sniper/sdk@sha256:1c33c507bc75d012e77df5727f93b0d5b8c3f7c8d4142ba5f7a16882cc92e014}"

# The Flatpak runtime the bundle targets.
FLATPAK_RUNTIME_VERSION=25.08
