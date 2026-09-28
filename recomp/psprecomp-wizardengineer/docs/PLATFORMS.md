# Platform Support

This page states, factually, where psprecomp is known to work today. The
project is developed and tested on macOS only; Linux and Windows have never
been run. See [ARCHITECTURE.md](../ARCHITECTURE.md) for the structure these
notes refer to and [README.md](../README.md) for build/run instructions.

## Summary

| Platform      | Rust pipeline (analyze/recompile) | C++ runtime (build + run)        |
|---------------|-----------------------------------|----------------------------------|
| macOS         | Verified                          | Verified                         |
| Linux         | Expected to work (untested)       | Likely portable, untested        |
| Windows       | Untested                          | Untested                         |

"Verified" means it is the platform on which the project is actively built and
run. "Untested" means exactly that — it has not been tried, not that it is
known to fail.

## What is platform-independent

The two halves of the project have very different platform exposure.

- **Rust pipeline** (`crates/psp-*`): pure Rust with no OS-specific code paths
  in the hot path. `analyze` shells out to a Ghidra headless install (the only
  external dependency); `recompile` reads `analysis.json` and emits C++ text.
  Continuous integration builds and unit-tests this half on `ubuntu-latest`
  (see [`.github/workflows/ci.yml`](../.github/workflows/ci.yml)), so the Rust
  crates are exercised on Linux on every push, even though the project is
  otherwise macOS-only.
- **Generated C++ output** (`output/`): standard C++17. It carries no
  platform assumptions of its own; portability is determined by the runtime
  that links it.

## macOS (verified)

This is the reference platform. Both the Rust pipeline and the C++ runtime are
built and run here.

- **SDL2** is located via `pkg-config` (`pkg_check_modules(SDL2 REQUIRED ...)`
  in `runtime/CMakeLists.txt`), matching a Homebrew `sdl2` install. This is a
  deliberate choice over CMake's `find_package(SDL2)` because the Homebrew
  package ships a `.pc` file but not always SDL2's CMake config.
- **OpenGL 3.3 core profile**: the runtime requests a 3.3 core context
  (`SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR/MINOR_VERSION, 3, 3)` in
  `runtime/src/psp_event_loop.cpp`) and loads function pointers with GLAD2.
- **Main-thread GL via the render queue**: macOS requires all OpenGL calls to
  happen on the thread that created the context (the main thread). The runtime
  is built around this: game threads never call GL directly — they post
  requests onto a condvar-based render queue that the main thread drains
  (`runtime/src/psp_render_queue.cpp`, `runtime/src/psp_event_loop.cpp`). A
  debug-build assertion `GL_THREAD_CHECK()` (`runtime/include/psp_runtime.h`)
  aborts if a GL call is ever made off the main thread.

## Linux (likely portable, untested)

Nothing in the design is intentionally macOS-only, but the runtime has never
been built or run on Linux, so this is unverified.

- **Rust pipeline**: expected to build and test the same as on macOS; CI
  already builds the workspace on Linux. The `analyze` step still needs a
  Ghidra install with the ghidra-allegrex processor extension.
- **SDL2 / OpenGL**: `pkg_check_modules(SDL2 ...)` and `find_package(OpenGL)`
  should resolve against a distro `libsdl2-dev` + Mesa/driver GL the same way
  they do on macOS; `zlib` is already discovered via `find_package(ZLIB)`,
  noted in the CMake as pkg-config-discoverable on Linux.
- **Assumptions to revisit before claiming Linux support**:
  - The main-thread-GL constraint that shaped the render queue is a macOS
    requirement; on Linux it is not strictly necessary, but the render-queue
    architecture is correct there too — it would simply be stricter than
    required, not wrong.
  - `GL_SILENCE_DEPRECATION` is defined to quiet macOS's OpenGL deprecation
    warnings; it is a no-op elsewhere but signals the macOS focus.
  - The GL 3.3 core context request must be honored by the Linux GL driver;
    Mesa supports it, but this has not been exercised.

## Windows (untested)

The runtime has never been built on Windows and several assumptions would need
review first:

- The build links `pthread` directly (`runtime/CMakeLists.txt`), which has no
  native Windows equivalent without a compatibility layer.
- SDL2 discovery via `pkg-config` is uncommon on a stock MSVC toolchain and
  would likely need a different discovery path.
- The threading model (`runtime/src/psp_scheduler.cpp`) and debug socket
  (`runtime/src/psp_debug_socket.cpp`) use POSIX-style primitives that have not
  been checked against Windows.

No effort has been spent on Windows; treat it as unsupported until someone
tries it.

## Contributing platform support

Porting to Linux or Windows is welcome. The Rust pipeline is the easy half (it
already runs in Linux CI); the work is in the C++ runtime's SDL2/GL/threading
assumptions listed above. The honest state is that only macOS has been
exercised end to end.
