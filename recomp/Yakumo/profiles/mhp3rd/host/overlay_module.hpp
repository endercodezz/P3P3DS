#pragma once

// ABI between Yakumo and the recompiled overlay libraries it loads at run
// time. One library holds one corpus: keeping them out of the executable means
// a new overlay costs a compile of its own sources and nothing else.
//
// The libraries resolve psprecomp symbols against the executable that loaded
// them, so a library only ever matches the build it was produced with. The ABI
// version below guards the metadata layout; nothing else is checked.

#include <cstdint>

namespace psprecomp {
class Runtime;
}

namespace mhp3rd {

inline constexpr std::uint32_t kOverlayAbiVersion = 1u;

// An overlay image starts with "MWo3" and a 64-byte header: id, load address,
// code size, data size, bss size, two end-of-image addresses and a 32-byte
// name. The header and the code after it are the part the game never writes to,
// so they are what identifies the image; its data section drifts as it runs.
inline constexpr std::uint32_t kOverlayHeaderBytes = 64u;

// Identification of the image the corpus was generated from, exactly as the
// host recomputes it from guest memory: slot base plus the FNV-1a hash of the
// first kOverlayHeaderBytes + code_size bytes.
struct OverlayModuleInfo {
    std::uint32_t abi_version;
    std::uint32_t base;
    std::uint32_t size;
    std::uint32_t code_size;
    std::uint64_t hash;
    const char *name;
};

} // namespace mhp3rd

// Only the library defines these; the host resolves them by name and uses the
// declarations for the types alone.
#if defined(MHP3RD_OVERLAY_MODULE)
#if defined(_WIN32)
#define MHP3RD_OVERLAY_EXPORT __declspec(dllexport)
#else
#define MHP3RD_OVERLAY_EXPORT __attribute__((visibility("default")))
#endif
#else
#define MHP3RD_OVERLAY_EXPORT
#endif

extern "C" {
MHP3RD_OVERLAY_EXPORT const mhp3rd::OverlayModuleInfo *mhp3rd_overlay_info();
MHP3RD_OVERLAY_EXPORT void mhp3rd_register_overlay(psprecomp::Runtime &runtime);
}
