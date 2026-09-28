# ppsspp-trace — Phase 11.5 PPSSPP-vs-runtime HLE behavioral diff

## Purpose

Phase 11.5 ground-truths the PSP HLE call sequence against PPSSPP via behavioral diff.
Four prior phases (11.1–11.4) hit the same dead-end speculative-hypothesis pattern.
This tool captures every HLE call PPSSPP makes during the boot window so that
`scripts/diff_traces.sh` can diff against `PSPRECOMP_HLE_TRACE=1` runtime output and
pinpoint the **first divergent call**.

Diagnostic mechanism (RESEARCH §3.2): attach 237 logging-only breakpoints
(`enabled: false, log: true, logFormat: "..."`) to every PSP NID import stub inside
PPSSPP via the WebSocket debugger API. PPSSPP pushes a structured `log` event per
HLE call without halting execution.

## Prerequisite

PPSSPP must already be running with a game loaded. **DO NOT spawn a new PPSSPP** —
the live session (PID 10398, port 49477 per `/tmp/p11_5_handoff.md`) is the canonical
target. This tool auto-detects the PPSSPP listening port via `lsof` so the literal
port number is never hardcoded:

```sh
PPSSPP_PORT=$(lsof -i -P 2>/dev/null | grep -i PPSSPP | grep LISTEN | awk '{print $9}' | sed 's/.*://')
```

## Usage

```sh
cd tools/ppsspp_trace
pnpm install
pnpm run build

# Self-test (no PPSSPP needed — exercises the boundary-marker termination logic):
node --import tsx capture.ts --self-test

# Live capture (Plan 02 / Wave 1 — connects to running PPSSPP):
node --import tsx capture.ts --out-raw=/tmp/trace_ppsspp.raw.jsonl
```

## Entry Points

| File | Role |
|------|------|
| `capture.ts` | CLI orchestrator — auto-detects PPSSPP port, installs breakpoints, captures `log` events, exits at boundary marker (user_main + sceGeListEnQueue, soft-cap 12s wall / 5000 events). |
| `install_breakpoints.ts` | Library — idempotent installer that drains pre-existing breakpoints at NID-stub addresses and re-adds 237 new ones with `enabled: false, log: true`. |
| `ppsspp_log_client.ts` | WebSocket client class — ticket bookkeeping for request/response PLUS push-event surface (`onLog` callback) for `log` frames that lack a ticket. |
| `nid_table.ts` | 237-entry NID stub table extracted from `runtime/include/hle/psp_hle_syscall_table.h`. Single source of truth. |

## Troubleshooting

- **Zero log events captured in 12s** — PPSSPP's HLE intercept may sit C++-side rather
  than at the import-stub address (RESEARCH §10 OQ1). Fall back to call-site breakpoints
  using `output/generated/batch_*.cpp` for the JAL targets. The capture orchestrator
  prints `[CAPTURE] events=0 elapsed_ms=12000 terminated_by=wall_clock_12000` in that case.
- **`lsof` returns empty** — PPSSPP died. Check `ps -p 10398 -o pid,stat,command`.
  Plan 02 Task 1 has the recovery procedure.
- **WebSocket `connection refused`** — port mismatch or PPSSPP debugger disabled.
  Restart PPSSPP with the debugger enabled and re-run `lsof` detection.
- **Push events dropped silently** — verify `ppsspp_log_client.ts` registers
  `onLog` BEFORE sending breakpoint requests; PPSSPP starts pushing events on the first
  breakpoint hit. Ticket-only PPSSPP debugger clients drop un-ticketed frames;
  this client must NOT inherit that pattern.

## Carry-Forward Constraint

This tool MUST NOT modify `runtime/src/`, `output/generated/`, or
`scripts/test_phase11_2_gatekeeper.sh`. It is a pure host-side diagnostic. PPSSPP
process management is out of scope — the existing PID 10398 session is the target.

## Design References

- `runtime/include/hle/psp_hle_syscall_table.h` (237 NID stubs — single source of truth)
