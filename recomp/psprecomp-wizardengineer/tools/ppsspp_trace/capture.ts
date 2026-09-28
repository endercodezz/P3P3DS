/**
 * PPSSPP HLE trace capture orchestrator.
 *
 * Connects to a LIVE PPSSPP debugger WebSocket (PID 10398 / port auto-detected
 * via lsof per RESEARCH §1 + CLAUDE.md §"PSP ReClass"), installs 237 logging-only
 * breakpoints, registers an onLog handler that appends each `log` push event
 * to a JSONL file, and exits when the boot-window boundary marker fires:
 *
 *   (a) sceKernelCreateThread has been called with name="user_main"  AND
 *   (b) sceGeListEnQueue has been called at least once
 *
 *   Soft cap: 12000 ms wall-clock OR 5000 captured log events (whichever first).
 *
 * Self-test (`--self-test`): synthesize four log frames in memory and run them
 * through the onLog handler logic, asserting the boundary detection wires up
 * correctly without needing a live PPSSPP. Useful for CI / Plan 02 pre-flight.
 *
 * IMPORTANT: this tool MUST NOT spawn a PPSSPP process. PID 10398 is the
 * canonical target per /tmp/p11_5_handoff.md. The hardcoded literal port number
 * recorded in the handoff doc never appears in this source — only via lsof
 * auto-detect at runtime.
 */

import { execSync } from "node:child_process";
import * as fs from "node:fs";

import { PpssppLogClient, type LogFrame } from "./ppsspp_log_client.js";
import {
  installBreakpoints,
  removeBreakpoints,
} from "./install_breakpoints.js";

// ────────────────────────────────────────────────────────────────────────────
// CLI argv parsing — simple --flag=value pattern
// (simple Map-based positional + --flags parser; no third-party CLI deps).
// ────────────────────────────────────────────────────────────────────────────

function parseArgs(argv: string[]): {
  positional: string[];
  flags: Map<string, string>;
} {
  const positional: string[] = [];
  const flags = new Map<string, string>();
  for (const arg of argv) {
    if (arg.startsWith("--")) {
      const eq = arg.indexOf("=");
      if (eq >= 0) {
        flags.set(arg.slice(2, eq), arg.slice(eq + 1));
      } else {
        flags.set(arg.slice(2), "true");
      }
    } else {
      positional.push(arg);
    }
  }
  return { positional, flags };
}

// ────────────────────────────────────────────────────────────────────────────
// Boundary-marker state — RESEARCH §4.3 deterministic capture window
// ────────────────────────────────────────────────────────────────────────────

interface CaptureState {
  userMainSeen: boolean;
  geEnQueueSeen: boolean;
  eventCount: number;
  startMs: number;
  eventCap: number;
  softCapMs: number;
  raw: fs.WriteStream | null;
  exited: boolean;
}

/**
 * Evaluate one log frame against boundary markers. Returns the termination
 * reason if BOTH markers have fired or the event cap has been hit; null otherwise.
 */
export function evaluateLogFrame(
  state: CaptureState,
  frame: LogFrame
): "user_main+geEnQueue" | "event_cap_5000" | null {
  state.eventCount++;
  // Persist the raw frame if a writer is wired (self-test mode has raw=null).
  if (state.raw != null) {
    state.raw.write(JSON.stringify(frame) + "\n");
  }
  const msg = frame.message;
  // user_main boundary: PPSSPP logs `sceKernelCreateThread` with the string-form
  // thread name visible in the args. We accept either a `=user_main` style
  // logFormat embed OR a raw `user_main` substring (defensive — PPSSPP's
  // {a0:s} pointer-deref may or may not be enabled).
  if (
    !state.userMainSeen &&
    msg.indexOf("sceKernelCreateThread") >= 0 &&
    /user_main/.test(msg)
  ) {
    state.userMainSeen = true;
  }
  if (!state.geEnQueueSeen && msg.indexOf("sceGeListEnQueue") >= 0) {
    state.geEnQueueSeen = true;
  }
  if (state.userMainSeen && state.geEnQueueSeen) {
    return "user_main+geEnQueue";
  }
  if (state.eventCount >= state.eventCap) {
    return "event_cap_5000";
  }
  return null;
}

// ────────────────────────────────────────────────────────────────────────────
// Port auto-detect — lsof-driven; no hardcoded port literal may appear in source
// (CLAUDE.md §"PSP ReClass" + RESEARCH §1 "Locked Decisions").
// ────────────────────────────────────────────────────────────────────────────

function autoDetectPpssppPort(): number | null {
  const cmd =
    "lsof -i -P 2>/dev/null | grep -i PPSSPP | grep LISTEN | " +
    "awk '{print $9}' | sed 's/.*://'";
  let out: string;
  try {
    out = execSync(cmd, { encoding: "utf8", timeout: 3000 });
  } catch {
    return null;
  }
  const lines = out
    .split(/\r?\n/)
    .map((s) => s.trim())
    .filter((s) => s.length > 0);
  if (lines.length === 0) return null;
  // Prefer the first numeric line. PPSSPP may also expose UDP entries via lsof
  // when discovery is active; we filtered to LISTEN only, so a numeric value
  // here is the TCP debugger port.
  for (const line of lines) {
    const n = parseInt(line, 10);
    if (Number.isFinite(n) && n > 0 && n < 65536) {
      return n;
    }
  }
  return null;
}

// ────────────────────────────────────────────────────────────────────────────
// Self-test — exercises the onLog dispatch + boundary detection logic without
// a live PPSSPP. Synthesizes four log frames in memory.
// ────────────────────────────────────────────────────────────────────────────

function makeFrame(message: string): LogFrame {
  return {
    event: "log",
    timestamp: "00:00:00",
    header: "Breakpoints.cpp:289",
    message,
    level: 1,
    channel: "G3D",
  };
}

function runSelfTest(): void {
  const state: CaptureState = {
    userMainSeen: false,
    geEnQueueSeen: false,
    eventCount: 0,
    startMs: Date.now(),
    eventCap: 5000,
    softCapMs: 12000,
    raw: null,
    exited: false,
  };

  // Frame 1: unrelated call — neither marker fires
  const r1 = evaluateLogFrame(
    state,
    makeFrame(
      "[PPSSPP-HLE] pc=089D7780 sceKernelDcacheWritebackAll " +
        "a0=0 a1=0 a2=0 a3=0 v0=0"
    )
  );
  if (r1 !== null) {
    process.stderr.write(
      `FAIL capture.ts self-test: frame 1 returned ${r1}, expected null\n`
    );
    process.exit(1);
  }

  // Frame 2: a sceKernelCreateThread for a DIFFERENT thread — userMainSeen stays false
  const r2 = evaluateLogFrame(
    state,
    makeFrame(
      "[PPSSPP-HLE] pc=089D76F8 sceKernelCreateThread a0=08AAA000 a1=0 " +
        "a2=0 a3=0 v0=42  thread_name=loader"
    )
  );
  if (r2 !== null || state.userMainSeen) {
    process.stderr.write(
      `FAIL capture.ts self-test: frame 2 falsely tripped user_main ` +
        `(userMainSeen=${state.userMainSeen}, returned=${r2})\n`
    );
    process.exit(1);
  }

  // Frame 3: sceKernelCreateThread WITH user_main token — userMainSeen flips true
  const r3 = evaluateLogFrame(
    state,
    makeFrame(
      "[PPSSPP-HLE] pc=089D76F8 sceKernelCreateThread a0=08BBB000 a1=0 " +
        "a2=0 a3=0 v0=43  thread_name=user_main"
    )
  );
  if (r3 !== null || !state.userMainSeen || state.geEnQueueSeen) {
    process.stderr.write(
      `FAIL capture.ts self-test: frame 3 should set userMainSeen but not ` +
        `geEnQueueSeen (userMainSeen=${state.userMainSeen}, ` +
        `geEnQueueSeen=${state.geEnQueueSeen}, returned=${r3})\n`
    );
    process.exit(1);
  }

  // Frame 4: sceGeListEnQueue — second marker fires, exit reason returned
  const r4 = evaluateLogFrame(
    state,
    makeFrame(
      "[PPSSPP-HLE] pc=089D78E8 sceGeListEnQueue a0=09000000 a1=09000400 " +
        "a2=0 a3=0 v0=1"
    )
  );
  if (r4 !== "user_main+geEnQueue") {
    process.stderr.write(
      `FAIL capture.ts self-test: frame 4 should return ` +
        `'user_main+geEnQueue' but returned ${r4}\n`
    );
    process.exit(1);
  }

  if (state.eventCount !== 4) {
    process.stderr.write(
      `FAIL capture.ts self-test: expected eventCount=4 got ${state.eventCount}\n`
    );
    process.exit(1);
  }

  process.stdout.write("PASS capture.ts self-test\n");
  process.exit(0);
}

// ────────────────────────────────────────────────────────────────────────────
// Main — live capture mode
// ────────────────────────────────────────────────────────────────────────────

async function main(): Promise<void> {
  const { flags } = parseArgs(process.argv.slice(2));

  if (flags.get("self-test") === "true") {
    runSelfTest();
    return;
  }

  // Soft-cap config — these defaults match RESEARCH §4.3.
  const softCapMs = parseInt(flags.get("soft-cap-ms") ?? "12000", 10);
  const eventCap = parseInt(flags.get("event-cap") ?? "5000", 10);
  const outRaw = flags.get("out-raw") ?? "/tmp/trace_ppsspp.raw.jsonl";

  // Port: explicit override OR lsof auto-detect. Never hardcoded.
  const portFlag = flags.get("port");
  const detectedPort =
    portFlag != null && portFlag !== "true"
      ? parseInt(portFlag, 10)
      : autoDetectPpssppPort();
  if (
    detectedPort == null ||
    !Number.isFinite(detectedPort) ||
    detectedPort <= 0
  ) {
    process.stderr.write(
      "[CAPTURE] FAIL: lsof did not detect a listening PPSSPP — " +
        "confirm PID 10398 is alive per /tmp/p11_5_handoff.md\n"
    );
    process.exit(1);
  }

  const host = flags.get("host") ?? "127.0.0.1";
  const client = new PpssppLogClient(host, detectedPort);

  process.stderr.write(
    `[CAPTURE] connecting to ws://${host}:${detectedPort}/debugger\n`
  );
  await client.connect();

  // Open the raw output file (truncate if exists).
  const raw = fs.createWriteStream(outRaw, { flags: "w" });
  const state: CaptureState = {
    userMainSeen: false,
    geEnQueueSeen: false,
    eventCount: 0,
    startMs: Date.now(),
    eventCap,
    softCapMs,
    raw,
    exited: false,
  };

  const finalize = async (reason: string): Promise<void> => {
    if (state.exited) return;
    state.exited = true;
    const elapsedMs = Date.now() - state.startMs;
    process.stderr.write(
      `[CAPTURE] events=${state.eventCount} elapsed_ms=${elapsedMs} ` +
        `terminated_by=${reason}\n`
    );
    try {
      await removeBreakpoints(client);
    } catch {
      /* best-effort */
    }
    try {
      raw.end();
    } catch {
      /* best-effort */
    }
    try {
      client.disconnect();
    } catch {
      /* best-effort */
    }
    // Give the WebSocket close + file flush a moment before exit.
    setTimeout(() => process.exit(0), 50);
  };

  // Register the onLog handler BEFORE installing breakpoints — otherwise the
  // first hits could fire before our handler is wired.
  client.setOnLog((frame: LogFrame) => {
    const reason = evaluateLogFrame(state, frame);
    if (reason != null) {
      void finalize(reason);
    }
  });

  // Install the 237 logging-only breakpoints (enabled: false, log: true — Pitfall 3).
  await installBreakpoints(client);

  // Wall-clock soft cap (RESEARCH §4.3).
  setTimeout(() => {
    void finalize("wall_clock_12000");
  }, softCapMs);
}

// Only run main() when executed as a script (not when imported for tests).
const isMain = (() => {
  try {
    // import.meta.url is the script URL when run via tsx/node ESM.
    const url = import.meta.url;
    return (
      url.endsWith("/capture.ts") ||
      url.endsWith("/capture.js") ||
      url.endsWith("capture.ts") ||
      url.endsWith("capture.js")
    );
  } catch {
    return true;
  }
})();

if (isMain) {
  main().catch((err: unknown) => {
    process.stderr.write(
      `[CAPTURE] FATAL: ${err instanceof Error ? err.message : String(err)}\n`
    );
    process.exit(1);
  });
}
