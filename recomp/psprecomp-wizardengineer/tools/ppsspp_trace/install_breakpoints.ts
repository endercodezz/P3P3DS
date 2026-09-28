/**
 * Install logging-only breakpoints on every PSP NID import stub address.
 *
 * Library module (no top-level execution). Plan 02 / capture.ts imports
 * `installBreakpoints(client)` and calls it after `client.connect()` to wire
 * all 237 breakpoints. The mechanism is documented in RESEARCH §3.2:
 *
 *   {
 *     event: "cpu.breakpoint.add",
 *     ticket: "tN",
 *     address: 0x089D7608,
 *     enabled: false,             // MANDATORY — PPSSPP default true; would halt Patapon
 *     log: true,
 *     logFormat: "[PPSSPP-HLE] pc={pc:x} <funcName> a0={a0:x} a1={a1:x} a2={a2:x} a3={a3:x} v0={v0:x}"
 *   }
 *
 * When such a breakpoint trips, PPSSPP evaluates `logFormat` against current MIPS
 * register state and pushes the result as a `log` event without pausing execution.
 *
 * Idempotency (RESEARCH §3.4 / Pitfall 3): on startup, query the existing
 * breakpoint list and `cpu.breakpoint.remove` any pre-existing breakpoint at one
 * of the 237 NID-stub addresses BEFORE re-adding. Then send a single `cpu.resume`
 * to recover from any prior `stepping` state left by a failed previous run.
 */

import { PpssppLogClient, type PpssppResponse } from "./ppsspp_log_client.js";
import { PSP_NID_STUBS, PSP_NID_STUB_COUNT } from "./nid_table.js";

/** Type of the response we expect from `cpu.breakpoint.list`. */
interface BreakpointListEntry {
  address: number;
  enabled?: boolean;
  log?: boolean;
}
interface BreakpointListResponse extends PpssppResponse {
  breakpoints?: BreakpointListEntry[];
}

/** Build the `logFormat` string for a given function name. */
function buildLogFormat(funcName: string): string {
  // The function name is embedded literally in the format so the normalizer
  // can extract it by token position. Argument and pc values are templated.
  return (
    `[PPSSPP-HLE] pc={pc:x} ${funcName} ` +
    `a0={a0:x} a1={a1:x} a2={a2:x} a3={a3:x} v0={v0:x}`
  );
}

/**
 * Idempotent install: drain existing breakpoints at NID addresses, then add 237
 * new logging-only breakpoints, then send `cpu.resume` to recover stepping state.
 *
 * @param client connected PpssppLogClient
 * @param log    where to write the banner line (default: stderr)
 */
export async function installBreakpoints(
  client: PpssppLogClient,
  log: (msg: string) => void = (msg) => process.stderr.write(msg)
): Promise<void> {
  // (1) Build a Set of all 237 NID-stub addresses for fast membership lookup.
  const nidAddrs = new Set<number>();
  for (const stub of PSP_NID_STUBS) {
    nidAddrs.add(stub.stubAddr);
  }

  // (2) Query existing breakpoints and remove any at our stub addresses
  //     (idempotency — recover from previous failed runs).
  let removedCount = 0;
  try {
    const listResp = await client.send<BreakpointListResponse>({
      event: "cpu.breakpoint.list",
    });
    const existing = listResp.breakpoints ?? [];
    for (const bp of existing) {
      if (typeof bp.address === "number" && nidAddrs.has(bp.address)) {
        // cpu.breakpoint.remove — fire-and-forget OK; PPSSPP responds with a
        // simple ack and we don't need correlation. Awaiting send() ensures
        // sequencing so the subsequent add() doesn't race with stale state.
        await client.send({
          event: "cpu.breakpoint.remove",
          address: bp.address,
        });
        removedCount++;
      }
    }
  } catch (err) {
    // If list/remove fails (e.g. game in stepping mode), continue with add —
    // PPSSPP will treat re-adds as updates on most paths.
    log(
      `[INSTALL] cpu.breakpoint.list pass failed (${String(err)}); ` +
        `proceeding with add-only (may produce duplicates on PPSSPP side)\n`
    );
  }

  // (3) Add 237 logging-only breakpoints.
  //     CRITICAL: `enabled: false, log: true` — RESEARCH §3.4 Pitfall 3.
  //     Without `enabled: false` the first NID-stub hit pauses Patapon.
  let addedCount = 0;
  for (const stub of PSP_NID_STUBS) {
    await client.send({
      event: "cpu.breakpoint.add",
      address: stub.stubAddr,
      enabled: false,
      log: true,
      logFormat: buildLogFormat(stub.funcName),
    });
    addedCount++;
  }

  // (4) Send a single cpu.resume to recover from any prior `stepping` state.
  //     If PPSSPP isn't paused this is a no-op; if it is, this is the fix.
  try {
    client.fireAndForget({ event: "cpu.resume" });
  } catch {
    // best-effort
  }

  // (5) Banner line on stderr.
  log(
    `[INSTALL] wired N=${addedCount} breakpoints ` +
      `(removed=${removedCount} pre-existing; ` +
      `PSP_NID_STUB_COUNT=${PSP_NID_STUB_COUNT})\n`
  );
}

/**
 * Best-effort cleanup: remove all 237 NID-stub breakpoints before disconnecting.
 * Called from capture.ts on graceful shutdown.
 */
export async function removeBreakpoints(
  client: PpssppLogClient,
  log: (msg: string) => void = (msg) => process.stderr.write(msg)
): Promise<void> {
  let removedCount = 0;
  for (const stub of PSP_NID_STUBS) {
    try {
      await client.send({
        event: "cpu.breakpoint.remove",
        address: stub.stubAddr,
      });
      removedCount++;
    } catch {
      // PPSSPP may already have shut down or the breakpoint may already be gone;
      // continue iterating so we make progress on the rest.
    }
  }
  log(`[INSTALL] removed ${removedCount} breakpoints on shutdown\n`);
}
