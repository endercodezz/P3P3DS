#pragma once
#include <cstdint>

// Generated NID import binding table (issue #40).
//
// DEFINED in <output>/syscall_table.cpp, which `psprecomp recompile` emits
// from analysis.json imports[] (stub address -> module + NID -> resolved
// name) and which links in via psp::recomp. The runtime carries no per-game
// stub addresses: psp_hle_init() walks this table and binds its HLE handlers
// BY NAME at the recorded stub addresses. Rows whose name has no registered
// handler get a loud unimplemented stub (see psp_hle_dispatch.cpp).
//
// Unresolved NIDs carry the walker's canonical fallback name "NID_0x%08X"
// (psp-parser nid::fallback_name) — a handler for such an import registers
// under exactly that name.
extern "C" {
typedef struct RecompNidStub {
    uint32_t stub_addr;
    uint32_t nid;
    const char* func_name;
    const char* module_name;
} RecompNidStub;
extern const RecompNidStub recomp_nid_stubs[];
extern const int recomp_nid_stub_count;
}
