# Static Recompilation Technical References

This directory contains vendored, read-only technical reference snapshots of modern static recompilation engines, runtimes, and emulators evaluated during the P3P3DS architectural audit (`docs/STATIC_RECOMP_ARCHITECTURE_AUDIT.md`).

These repositories are stored as normal tracked source files in the P3P3DS repository tree (no submodules, no gitlinks, no nested `.git`).

Authoritative provenance and licensing registry is maintained in `docs/UPSTREAMS.md`.

## Inventory

| Project | Upstream URL | Imported SHA | Branch | License | Purpose in P3P3DS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **XenonRecomp** | `https://github.com/hedge-dev/XenonRecomp.git` | `ddd128bcca99fe8bfbb99bea583c972351fa6ace` | `main` | MIT | Xbox 360 PPC-to-C++ static recompilation architecture, CFG recovery, and jump-table analysis reference |
| **rexglue-sdk** | `https://github.com/rexglue/rexglue-sdk.git` | `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518` | `main` | BSD-3-Clause | Xbox 360 AOT runtime SDK, function dispatchers, guest memory layout, and kernel HLE reference |
| **xenia** | `https://github.com/xenia-project/xenia.git` | `95a5c3ee250f80c3b9d139658649d9ffb6db3eec` | `master` | BSD-3-Clause | Kernel HLE, memory subsystems, and GPU translation architecture reference for Xbox recompilation ecosystem |
| **N64ModernRuntime** | `https://github.com/N64Recomp/N64ModernRuntime.git` | `cdf5abbd5026fef5c364c676e4667c45e42b6863` | `main` | GPL-3.0-or-later | Dynamic overlays, modular C patch injection system, and function address mapping reference |
| **Zelda64Recomp** | `https://github.com/Zelda64Recomp/Zelda64Recomp.git` | `b65c482ed672258eb684ab94a4e8c8aa662615ee` | `dev` | GPL-3.0-or-later | Production N64 recompiled game runtime, RT64 graphics translation bridge, and mod patch system |
| **UnleashedRecomp** | `https://github.com/hedge-dev/UnleashedRecomp.git` | `cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c` | `main` | GPL-3.0-or-later | Production XenonRecomp runtime, game memory mapping, and graphics pipeline integration reference |

*Note: N64Recomp is vendored in `recomp/N64Recomp` and reused directly without duplication.*
