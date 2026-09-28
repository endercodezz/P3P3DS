/**
 * PPSSPP WebSocket client with **both** ticket bookkeeping (request/response)
 * and push-event surface (`onLog` callback for un-ticketed `log` frames).
 *
 * A typical ticket-only PPSSPP debugger client early-returns whenever the
 * incoming frame lacks a ticket field, which
 * silently DROPS push events. Phase 11.5 ground-truths
 * against the PPSSPP HLE call stream by emitting every breakpoint hit as a `log`
 * push event — which such a client would discard. This client
 * preserves the ticket plumbing pattern but adds an `onLog` registration that
 * dispatches every un-ticketed (or `event: "log"`) frame to the registered handler.
 *
 * Reference: RESEARCH §3.1 (WebSocket URL), §3.2 (logging breakpoint payload),
 * §3.3 (event table), §3.4 (Pitfall 3 — enabled:false).
 */

import WebSocket from "ws";

const REQUEST_TIMEOUT_MS = 10_000;

/** A `log` push-event frame (per LogBroadcaster.cpp:99-107 in PPSSPP source). */
export interface LogFrame {
  event: "log";
  timestamp: string;
  header: string;
  message: string;
  level: number;
  channel: string;
}

/** Generic request body — caller fills in the event-specific fields. */
export type PpssppRequest = Record<string, unknown> & { event: string; ticket?: string };

/** Generic response shape — `ticket` ties it back to a pending send(). */
export type PpssppResponse = Record<string, unknown> & { event?: string; ticket?: string };

type PendingRequest = {
  resolve: (resp: PpssppResponse) => void;
  reject: (err: Error) => void;
  timer: ReturnType<typeof setTimeout>;
};

export type LogHandler = (frame: LogFrame) => void;

/**
 * PPSSPP debugger WebSocket client.
 *
 * Connect, optionally register an `onLog` handler before installing logging-only
 * breakpoints, then either fire request/response calls via `send()` or simply
 * wait for `onLog` to receive push events. Push-event delivery is the primary
 * mechanism for Phase 11.5 HLE trace capture.
 */
export class PpssppLogClient {
  readonly url: string;
  private ws: WebSocket | null = null;
  private ticketCounter = 0;
  private pending = new Map<string, PendingRequest>();
  private logHandler: LogHandler | null = null;

  constructor(host: string, port: number) {
    this.url = `ws://${host}:${port}/debugger`;
  }

  /** Generate the next ticket id ("t1", "t2", ...). */
  nextTicket(): string {
    return `t${++this.ticketCounter}`;
  }

  /**
   * Register the push-event handler. MUST be called BEFORE installing breakpoints
   * if you want to capture the first hit. Replacing an existing handler is allowed.
   */
  setOnLog(handler: LogHandler): void {
    this.logHandler = handler;
  }

  /** Alias for setOnLog — matches the spec wording "onLog" registration. */
  onLog(handler: LogHandler): void {
    this.setOnLog(handler);
  }

  /** Open the WebSocket and install the dispatching message handler. */
  connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      // PPSSPP advertises subprotocol "debugger.ppsspp.org" — passing it as an
      // option makes the handshake explicit. PPSSPP also accepts no subprotocol,
      // so this is belt-and-suspenders.
      const ws = new WebSocket(this.url, ["debugger.ppsspp.org"]);
      this.ws = ws;

      ws.once("open", () => resolve());
      ws.once("error", (err: Error) => reject(err));

      ws.on("message", (data: WebSocket.RawData) => {
        let resp: PpssppResponse;
        try {
          resp = JSON.parse(data.toString()) as PpssppResponse;
        } catch {
          return;
        }
        // Dispatch order matters: a frame with both `ticket` AND `event:"log"`
        // is theoretically possible but the canonical `log` push event has no
        // ticket (LogBroadcaster.cpp). We dispatch to `onLog` whenever the frame
        // is a log event, regardless of ticket presence — capturing it is more
        // useful than the request/response correlation for that frame.
        if (resp.event === "log") {
          this.dispatchLog(resp);
          // Fall through — if there ALSO happens to be a pending ticket
          // (unusual), resolve it. This is defensive; PPSSPP does not produce
          // such frames in practice.
        }
        const ticket = resp.ticket;
        if (ticket == null) {
          // Un-ticketed AND not a log event — could be a cpu.stepping push or
          // similar. We still dispatch to onLog so the capture orchestrator
          // can observe state changes that surface as raw event payloads.
          if (resp.event !== "log" && this.logHandler != null) {
            // Convert non-log push events into a synthetic log frame so the
            // capture file remains JSONL with uniform shape. The capture
            // orchestrator filters by message content.
            this.dispatchLog({
              event: "log",
              timestamp: "",
              header: `push:${String(resp.event ?? "unknown")}`,
              message: JSON.stringify(resp),
              level: 0,
              channel: "PUSH",
            });
          }
          return;
        }
        const pending = this.pending.get(ticket);
        if (pending == null) return;
        this.pending.delete(ticket);
        clearTimeout(pending.timer);
        pending.resolve(resp);
      });
    });
  }

  private dispatchLog(resp: PpssppResponse): void {
    if (this.logHandler == null) return;
    const frame: LogFrame = {
      event: "log",
      timestamp: String(resp.timestamp ?? ""),
      header: String(resp.header ?? ""),
      message: String(resp.message ?? ""),
      level: typeof resp.level === "number" ? resp.level : 0,
      channel: String(resp.channel ?? ""),
    };
    try {
      this.logHandler(frame);
    } catch (err) {
      // Never let handler exceptions propagate into the socket callback —
      // they would tear down the WebSocket and lose the rest of the trace.
      process.stderr.write(
        `[ppsspp_log_client] onLog handler threw: ${String(err)}\n`
      );
    }
  }

  /** Send a request and await the ticket-correlated response. */
  send<T extends PpssppResponse = PpssppResponse>(
    req: PpssppRequest
  ): Promise<T> {
    return new Promise((resolve, reject) => {
      if (this.ws == null || this.ws.readyState !== WebSocket.OPEN) {
        reject(new Error("not connected"));
        return;
      }
      const ticket = this.nextTicket();
      req.ticket = ticket;
      const timer = setTimeout(() => {
        this.pending.delete(ticket);
        reject(new Error(`request timed out: ${req.event}`));
      }, REQUEST_TIMEOUT_MS);

      this.pending.set(ticket, {
        resolve: resolve as (resp: PpssppResponse) => void,
        reject,
        timer,
      });

      this.ws.send(JSON.stringify(req));
    });
  }

  /** Fire-and-forget (no response expected; useful for cpu.resume cleanup). */
  fireAndForget(req: PpssppRequest): void {
    if (this.ws == null || this.ws.readyState !== WebSocket.OPEN) return;
    this.ws.send(JSON.stringify(req));
  }

  /** Close the WebSocket and reject any in-flight requests. */
  disconnect(): void {
    this.ws?.close();
    this.ws = null;
    for (const [, req] of this.pending) {
      clearTimeout(req.timer);
      req.reject(new Error("disconnected"));
    }
    this.pending.clear();
  }
}
