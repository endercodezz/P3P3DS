# Source lists shared by the host build (CMakeLists.txt) and the New 3DS
# build (platform/3ds/CMakeLists.txt). Paths are relative to the repository root.

set(P3P3DS_CORE_SOURCES
    core/src/frontier.cpp
    core/src/interpreter.cpp
    core/src/vfs.cpp
    core/src/hle/iofilemgr.cpp
    core/src/hle/modulemgr.cpp
    core/src/hle/sascore.cpp
    core/src/hle/mpeg.cpp
    core/src/hle/utils.cpp
    core/src/hle/utility.cpp
    core/src/hle/savedata.cpp
    core/src/hle/ctrl.cpp
    core/src/input.cpp
    core/src/ge/async_renderer.cpp
    core/src/ge/geometry.cpp
    core/src/ge/software_renderer.cpp
    core/src/hle/sysmem.cpp
    core/src/hle/hle_modules.cpp
    core/src/hle/threadman.cpp
    core/src/hle/threadman_hle.cpp
    core/src/hle/display.cpp
    core/src/hle/ge.cpp
    core/src/hle/umd.cpp
    core/src/hle/audio.cpp
)

# PSPRecomp runtime sources needed at run time (no host-only tools).
set(PSPRECOMP_RUNTIME_SOURCES
    recomp/PSPRecomp/src/decoder.cpp
    recomp/PSPRecomp/src/deflate.cpp
    recomp/PSPRecomp/src/elf32.cpp
    recomp/PSPRecomp/src/guest_memory.cpp
    recomp/PSPRecomp/src/nid_registry.cpp
    recomp/PSPRecomp/src/program_analysis.cpp
    recomp/PSPRecomp/src/runtime.cpp
    recomp/PSPRecomp/src/sha256.cpp
)
