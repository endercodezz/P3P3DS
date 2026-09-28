# Scheduler Design — PSP-faithful preemption rework

**Status:** design-before-code. Tracks GitHub issue #66. Nothing here is implemented.
No scheduler code lands until this doc is reviewed.

**Driving case:** .hack//Link (#63) freezes on ~43% of boots in a nondeterministic CRI
streaming-ring producer/consumer convergence race that the current cooperative scheduler
cannot resolve. Event-flag and semaphore HLE semantics were independently verified correct
(`.planning/research/dothack-eventflag-probe.md`); the gap is scheduling/timing, not HLE.

**Load-bearing constraint:** Patapon boots and renders on the current scheduler today. Every
locus this rework touches is on Patapon's critical path. Patapon-regression risk is HIGH.

---

## 1. Current model (precise)

### 1.1 PSP thread = real OS thread

Each PSP thread is a real `std::thread`. The thread pool is a fixed array of 64
`PspThread` slots (`runtime/include/psp_scheduler.h:43`, `runtime/src/psp_scheduler.cpp:12`),
each owning its own `std::condition_variable cv`, its own `std::thread host_thread`, and its
own `recomp_context ctx` (`psp_scheduler.h:52-68`). Register state is isolated per thread by
construction — no save/restore. `psp_thread_start` flips the slot to `READY` and launches the
OS thread (`psp_scheduler.cpp:83-96`). The host OS — not our scheduler — decides which of these
threads actually runs on a CPU at any instant.

This is the inverse of PPSSPP's model (§3): PPSSPP runs all guest threads on **one** host
thread, interleaving them by an emulated cycle budget. We run N guest threads as N concurrent
OS threads and try to *constrain* their concurrency back toward PSP semantics with a global
lock and explicit yields.

### 1.2 `g_sched_mutex` — held only transiently

`g_sched_mutex` (`psp_scheduler.cpp:9`) is a single global lock guarding all `PspThread.status`
transitions and `cv` operations. It is acquired at the top of each scheduler function and
released on return. It is **never** held across recompiled guest code, and **never** held
across a kernel-object `cv.wait_for` — those waits are on per-object mutexes
(`ef->mtx`, `s->mtx`), disjoint from `g_sched_mutex`. Consequence: while a thread runs guest
code or parks in a kernel wait, every other OS thread is free to run guest code concurrently.
There is no global "only one guest thread executes at a time" invariant.

### 1.3 `thread_local g_current`

`static thread_local PspThread* g_current` (`psp_scheduler.cpp:16`) is set once at the top of
`thread_entry_wrapper` (`:102`). Each OS thread knows its own `PspThread` with no shared lookup.
`psp_get_current_thread()` returns it (`:304`).

### 1.4 Where/when the runtime yields today

The only voluntary yield is `sched_yield_point()` (`psp_scheduler.cpp:161-216`). It:

1. scans all 64 slots for the highest-priority `READY` thread (lowest priority number)
   that is not `g_current` (`:172-184`);
2. if none exists, or `g_current`'s priority ≤ the best candidate's, **returns without
   yielding** (`:187-189`) — a higher-or-equal-priority running thread never yields to a
   lower-priority ready thread;
3. otherwise marks itself `READY`, marks the best `RUNNING`, `notify_one()`s it, then parks on
   its own `cv` with a **50 ms safety valve** (`SCHED_TIMEOUT_MS`, `psp_scheduler.h:47`) until
   re-scheduled (`:191-215`).

`sched_yield_point()` is **not** emitted into recompiled guest code. It is called only from HLE
stubs, at hand-chosen boundaries: `sceKernelWaitEventFlag` (`psp_hle_kernel_eventflag.cpp:164`),
`sceKernelWaitSema` / `WaitSemaCB` (`psp_hle_kernel_sema.cpp:236`, `:441-449`),
`sceKernelDelayThread` (`psp_hle_kernel_thread.cpp:306`), `sceKernelSleepThread`
(`:321`), `sceKernelLockMutex` (`psp_hle_kernel_mutex.cpp:58`), the GE / display / ctrl HLE
(`psp_hle_ge.cpp:58,69`, `psp_hle_display.cpp:54`, `psp_hle_ctrl.cpp:74`), the IO HLE
(many sites in `psp_hle_io.cpp`), and the render queue (`psp_render_queue.cpp:119`).

**A guest thread that busy-polls a memory location in a loop that calls none of these syscalls
never yields and never gives the scheduler a decision point.** It just runs as an OS thread at
host-decided concurrency. This is the .hack failure shape (§2, §4).

### 1.5 How `SetEventFlag` / wait wakes a thread

`sceKernelSetEventFlag` (`psp_hle_kernel_eventflag.cpp:109-134`) takes `ef->mtx`, ORs the bits
into `ef->pattern`, and `ef->cv.notify_all()`. `sceKernelWaitEventFlag` (`:156-219`) takes
`ef->mtx`, then `ef->cv.wait_for(lock, 5s, pred)` where `pred` re-checks `pattern_matches`. The
predicate is evaluated **before** parking (textbook condvar use), so a Set that already
satisfied the pattern returns immediately; a Set arriving during the wait wakes the waiter.
This path is correct and was verified correct (`dothack-eventflag-probe.md` (a)-(c)).

**The critical limitation:** `notify_all()` only wakes threads that are *parked in
`cv.wait_for`*. It does nothing for a thread that is **busy-polling guest memory** and not
inside any kernel wait. There is no mechanism by which a producer's `SetEventFlag` (or its ring
write) forces a busy-polling consumer to re-read after the write becomes visible. The consumer
re-reads only when the host scheduler next runs it — an unconstrained timing event.

(The header constant inversion `PSP_EVENT_WAITAND`/`WAITOR` noted in the probe is a real,
separate HLE bug tracked in #64; it is single-bit-immune and not the .hack mechanism. Out of
scope here.)

### 1.6 `sceKernelDelayThread` / sleep / sema behavior

- **DelayThread** (`psp_hle_kernel_thread.cpp:296-309`): dispatches pending IO callbacks,
  calls `sched_yield_point()`, then **`std::this_thread::sleep_for(usec)`**. The host OS thread
  literally sleeps for the requested microseconds; it does not model a PSP scheduler quantum.
- **SleepThread** (`:318-325`): `sched_yield_point()` then `psp_thread_sleep_current()`
  (`psp_scheduler.cpp:325-374`), which uses PPSSPP-style `wakeup_count` semantics (pre-woken →
  return immediately; else `WAIT_SLEEP` until `psp_thread_wakeup` sets it back to `READY`),
  with a **5 s safety valve**.
- **WaitSema** (`psp_hle_kernel_sema.cpp:229-367`): FIFO direct-handoff (issue #29). Fast path
  acquires only if no waiter is queued ahead (no queue-jumping). Slow path enqueues a
  stack-allocated `SemaWaiter` and parks on `s->cv` (timeout or 5 s safety valve + a 30 s
  rate-limited STUCK warning). `SignalSema` (`:126-227`) transfers count to FIFO waiters and
  `notify_all()`. Same busy-poll blind spot as event flags: a thread that never calls
  `WaitSema` is never woken by `SignalSema`.

### 1.7 Summary of the current model

It is **not** cooperative in the classic sense (one runnable at a time, switching only at yield
points) and **not** preemptive. It is **host-concurrent with cooperative yield hints**: real OS
threads run in true parallelism, the host kernel preempts them on wall-clock, and our explicit
yields only redistribute turns *among threads that reach a yield-bearing syscall*. There is no
timeslice, no priority-driven preemption of a running thread, no round-robin within a priority,
and no preemption point inside guest loops.

---

## 2. The gap vs real PSP

Real PSP (PSPThreadMan) is a **priority-preemptive** scheduler on a single CPU:

- **Timer-based preemption.** A periodic timer interrupt can preempt the running thread.
  A lower-priority CPU hog cannot starve a higher-priority ready thread indefinitely.
- **Strict priority.** The highest-priority READY thread runs. A wake that readies a
  higher-priority thread preempts the current one (PPSSPP: `__KernelReSchedule`,
  `pop_first_better`, §3).
- **Round-robin / timeslice within a priority.** Equal-priority threads rotate;
  `sceKernelRotateThreadReadyQueue` and the quantum rotate same-priority peers so none hogs.
- **Single CPU.** Only one thread runs at a time; memory effects of one thread are fully
  ordered before the next thread is dispatched. There is no true parallel write/read race
  between guest threads — interleaving happens only at well-defined reschedule points.

Our model has none of timer preemption, priority preemption of a running thread, or
within-priority rotation, and — unlike PSP — runs guest threads in **genuine OS parallelism**,
so guest code that assumes single-CPU ordering can observe interleavings impossible on hardware.

### Guest patterns that break without faithful scheduling

1. **Busy-poll consumer (the .hack `user_main` CRI ring).** A thread spins reading a guest
   memory flag/ring pointer in a loop that issues no yield-bearing syscall. On real PSP the
   timer preempts it, the producer runs, advances the ring, and on resume the consumer sees the
   new value — guaranteed forward progress. On ours the spinning OS thread never yields and the
   producer's progress becomes visible only when the host scheduler happens to run the consumer
   after the write — nondeterministic, and on ~43% of boots it lands in a state the consumer's
   poll never re-satisfies. **This is the .hack freeze.**
2. **Spin-wait synchronization.** A guest spinlock / "wait until field != 0" guarding a handoff,
   with the setter on another thread. Same class as (1): without preemption the spinner can
   monopolize its CPU turn; with true OS parallelism it can also observe stale values longer
   than hardware would.
3. **Priority inversion / starvation.** A high-priority thread spinning on a result a
   low-priority thread must produce. PSP's priority-preemptive rotation guarantees the
   low-priority producer eventually runs; ours offers no guarantee a high-priority spinner
   yields to the lower-priority producer (`sched_yield_point` explicitly refuses to yield to a
   lower-priority thread, `psp_scheduler.cpp:187`) — even if the spinner *did* reach a yield.
4. **Any-thread CPU hogging.** A compute loop with no syscalls runs unbounded on its OS thread.
   The 50 ms / 5 s safety valves only rescue threads that already parked at a yield/wait; they
   do not preempt a thread that never yields.

---

## 3. Reference: PPSSPP scheduler model

PPSSPP is the behavioral oracle. Its model (`~/GitDownloads/ppsspp/Core/HLE/sceKernelThread.cpp`,
`Core/CoreTiming.cpp`):

- **One host execution context, many guest threads.** Guest threads are interleaved on a single
  emulated CPU; only one runs at a time. No real parallel guest memory races.
- **Priority-preemptive selection.** `__KernelNextThread` (`sceKernelThread.cpp:1589`) pops the
  best ready thread: `threadReadyQueue.pop_first_better(cur->currentPriority)` (`:1595`) — switch
  only to a strictly-better priority — else `pop_first()`. `threadReadyQueue` (`:512`) is a
  per-priority queue with `push_front`/`push_back` for round-robin within a level (`:911-923`).
- **Preemption via CoreTiming.** Delays/timeouts schedule wakeups in *cycles*:
  `CoreTiming::ScheduleEvent(usToCycles(usFromNow), eventScheduledWakeup, threadID)`
  (`:1446-1447`). `__KernelReSchedule` (`:1617-1636`) runs pending events
  (`CoreTiming::Advance()`), checks callbacks, and switches context to the next thread. Reschedule
  is invoked at every syscall boundary and timed event — it is the universal preemption point.
- **Key consequence for us:** PPSSPP never has a "busy-poll consumer that never yields" problem,
  because its single-CPU interleaving advances time and reschedules on a cycle budget regardless
  of whether the guest loop calls a syscall. The guest loop *is* executed on the one CPU in
  bounded slices, so the producer always gets a turn and the ring always converges
  (`dothack-loadprog.md` ORACLE diff: the same loading scene is interactive on PPSSPP).

We will not copy PPSSPP's full cycle-accurate CoreTiming; we need the *behavioral* property
(bounded forward progress + ordered handoff), at the lowest-risk locus.

---

## 4. Candidate approaches

Four candidates, each evaluated on **fidelity** (closeness to PSP behavior), **perf**,
**locus** (emitter vs runtime), and **risk** (especially Patapon regression).

### (a) Instruction-budget preemption points emitted at loop back-edges / function entries

The emitter injects a periodic preemption check — e.g. `if (--ctx->budget <= 0)
sched_preempt();` — at loop back-edges and/or function entries. `sched_preempt()` is a runtime
yield that any thread reaches even inside a syscall-free busy loop.

- **Fidelity:** HIGH. This is the closest analog to PSP timer preemption — every thread,
  including a pure busy-poll, reaches a reschedule point on a bounded instruction budget.
- **Perf:** MEDIUM-LOW cost if the check is a cheap counter decrement + predicted-not-taken
  branch; back-edge-only placement keeps it off straight-line code. Still touches every hot loop.
- **Locus:** EMITTER (`psp-emitter`) + a small runtime entry point. This is the only candidate
  that changes generated code. It requires regenerating `output/` and re-running the staleness
  gate, and it is the only one that can preempt a loop that touches no kernel object.
- **Risk:** HIGH-but-bounded. It is the most invasive (every batch file changes) and the most
  general fix. Budget tuning is delicate: too-frequent preemption tanks perf; too-coarse leaves
  races. Mitigable by emitting the points but defaulting `sched_preempt()` to a no-op behind a
  flag (rollout, §6).

### (b) Runtime yield-on-no-progress / spin detection

The runtime watches a thread for "no forward progress" (e.g. the PC-trace ring cycling a small
address window — already observed in `dothack-verdict.md`, or repeated identical reads) and
forces it to yield once a spin is detected.

- **Fidelity:** LOW-MEDIUM. It is a heuristic, not a model; it reacts to a stuck spin rather than
  preventing monopolization. Detection threshold is a guess and title-dependent.
- **Perf:** LOW overhead (sampling/instrumentation), but detection latency adds stall before the
  yield kicks in.
- **Locus:** RUNTIME only (extend the existing PC-trace ring in `psp_dispatch.cpp`). No emitter
  change, no `output/` regen.
- **Risk:** MEDIUM. False positives could yield a legitimately-busy thread; false negatives
  leave the race. Heuristic fragility makes it hard to *prove* a race is fixed (§5). Reasonable
  as a diagnostic or a backstop, weak as the primary mechanism.

### (c) Cooperative yield injected at common busy-wait syscalls

Add/strengthen `sched_yield_point()` at more HLE entry points the busy-poll touches — e.g. force
a yield + memory fence in `sceKernelReferEventFlagStatus` / `PollEventFlag` / `PollSema`, which
a polling loop *does* call (the .hack worker pump's release path polls
`sceKernelReferEventFlagStatus`, per the comment at `psp_hle_kernel_eventflag.cpp:264-266`).

- **Fidelity:** LOW. Only helps loops that happen to call an instrumented syscall; a pure
  memory-only spin (no syscall) is untouched. The .hack `user_main` ring pump *does* go through
  `FUN_08839670` and CRI poll syscalls, so this may reach it — but it is poll-shaped, not modeled.
- **Perf:** LOW (a few more yields).
- **Locus:** RUNTIME only (HLE stubs). No emitter change.
- **Risk:** MEDIUM. Lowest-effort, and the yield ordering it changes is exactly Patapon's
  critical path (Patapon's PCM/sema handoff, GE signal flag). Easy to land, easy to regress
  Patapon, and does not generalize to syscall-free spins.

### (d) Fuller preemptive timeslice model (host-thread based)

Restructure toward PSP's single-CPU priority-preemptive model: a global "only one guest thread
runs at a time" token (a global run lock or a per-thread gate held across guest execution) plus a
timer that revokes the token on a quantum and hands it to the highest-priority ready thread, with
round-robin within a priority. This brings genuine OS parallelism back under a single-runnable
discipline that matches PSP ordering.

- **Fidelity:** HIGHEST. Directly models timer preemption + strict priority + within-priority
  rotation + single-CPU ordering. Eliminates the true-parallel-race class entirely (pattern (1)
  and the stale-read half of (2)).
- **Perf:** Serializes guest execution (one runnable at a time) — gives up the OS parallelism we
  currently exploit; the quantum timer adds context-switch overhead. On a cooperative-ish
  workload this is acceptable; for compute-heavy multi-thread titles it is a real cost.
- **Locus:** RUNTIME (`psp_scheduler.cpp`) — but a deep rewrite, plus a preemption point so a
  syscall-free loop can be interrupted to revoke the token (which still wants emitter help, i.e.
  (a), unless the quantum timer can asynchronously signal the running OS thread to yield).
- **Risk:** HIGHEST. It rewrites the load-bearing core wholesale. Highest Patapon-regression
  surface of all four. Highest fidelity ceiling, but the worst single-step risk profile.

---

## 5. How each resolves the .hack CRI-ring race

The race (`dothack-loadprog.md`): the `CRI ADX File` worker fills the streaming ring (advances
the write pointer in the `0x08C90000` control block) and `SetEventFlag(CriCond)`; `user_main`'s
pump (`FUN_08839670` / `FUN_08838FB4`) busy-polls the ring's available-bytes
(`writePtr − readPtr`) directly, having abandoned `WaitEventFlag` after its first few waits
(last Wait@296 vs ~8M later Sets, `dothack-eventflag-probe.md`). The fix must make the consumer
**deterministically re-read the ring after the producer's write becomes visible** — i.e. give the
producer a guaranteed turn before the consumer's next poll, and order the producer's
write+signal before that poll.

- **(a) instruction-budget preemption:** Resolves it directly and generally. The consumer's poll
  loop hits an emitted preemption point on a bounded budget regardless of syscalls; it yields,
  the worker runs, advances the ring, and on the consumer's resume the new write pointer is
  visible (with a fence at the preemption point). Forward progress becomes guaranteed, not
  timing-dependent. Strongest mechanistic fit.
- **(b) spin detection:** Resolves it reactively. Once the consumer's PC-window is detected
  stuck, the runtime forces a yield, the worker runs, the ring advances. Works only after the
  stall is detected (added latency) and only if the detector fires reliably — hard to prove.
- **(c) syscall yield:** *May* resolve it. The pump touches CRI poll syscalls
  (`ReferEventFlagStatus`/poll); forcing a yield+fence there gives the worker a turn before the
  next poll. But it depends on the loop calling an instrumented syscall every iteration; a tight
  memory-only inner spin would slip through. Partial, title-shaped.
- **(d) timeslice model:** Resolves it by construction. With a single-runnable token + quantum
  timer, the consumer cannot monopolize; the worker is scheduled, its ring write + SetEventFlag
  complete and are ordered (single-CPU) before the consumer is re-dispatched, which then re-reads
  the advanced ring. Eliminates the race class, at the cost of the full rewrite.

---

## 6. Validation harness (critical — this is a race)

**A single green run does NOT prove a race fix.** A ~43% freeze means a "passing" boot has a
~57% prior probability under the *unchanged* code. Validation must measure a **rate** across
many boots, on both titles, and pair it with the existing correctness gates.

### 6.1 Many-boot determinism harness (new)

A headless N-boot runner (model it on `scripts/verify_geometry.sh`, which already does bounded
SIGTERM-first runs, log aggregation, and a single verdict) extended to:

- Boot **.hack** N× (N ≥ 30; the more boots, the tighter the freeze-rate confidence interval)
  and **Patapon** N×, one runtime at a time, SIGKILL between, `pgrep`-confirmed clean (per the
  research provenance discipline).
- Classify each .hack boot as **frozen** vs **progressed** by an objective signal, not eyeballing:
  e.g. frame/enqueue count crossing the FAST-boot threshold within the window, or `user_main`
  exiting the `0x0883xxxx` spin window (debug socket `I` recent-funcs ring), or reaching the
  steady render loop (prims climbing past the MEDIUM ceiling). Frozen boots in the research were
  ≤~52 frames; progressed boots climbed past 470 — a clean separation to threshold on.
- Report **freeze-rate** with the boot count (e.g. "2/40 frozen = 5%") for baseline vs candidate.
  Acceptance is a statistically meaningful drop (baseline ~43% → target near-0), demonstrated
  over enough boots that the drop is not noise. State the boot count and the classifier in the
  result; do not report a single run.
- Run **Patapon** through the same N-boot loop to confirm the change did not *introduce*
  nondeterminism on the title that works today.

### 6.2 Patapon CLEANROOM band (existing — must stay green)

After any candidate, run the canonical CLEANROOM gate (DEBUGGING.md §2):

```
PSPRECOMP_CROSS_MID=1 PSPRECOMP_CLEANROOM=1 timeout 120 \
  ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1
```

Gates: exit 124; `real_nonsprite` in **13.8k–15.7k** (baseline ~15618); exactly ONE
degenerate-matrix fallback notice; `[GE_GEOM_HEARTBEAT]` monotonic; LOOKUP_MISS sentinel-only;
no "bus error" / "marking dead". (`scripts/verify_geometry.sh 3 120` aggregates the
`real_nonsprite` verdict across runs.)

### 6.3 Purity gate (existing — must stay green)

`runtime/tools/purity_gate.sh runtime/build` (and a `build-none` build): no game-specific code
in runtime core. A scheduler change in `runtime/src` must remain title-agnostic — any
.hack-specific fix belongs in `games/dothack/`, never in `psp_scheduler.cpp`. If candidate (a)
emits preemption points, confirm the emitter change is generic (no per-title budget literals in
core).

### 6.4 Visible-pixel gate (existing, NEW since #27 — must stay green)

`real_nonsprite` in band does **not** prove a visible frame: a broken transform can submit
in-band prims that collapse to a sub-pixel sliver, leaving the screen black (#27 regressed dev
exactly this way). After a Patapon run, grab the title frame and assert it is visible:

```
PSPRECOMP_DISC0=./disc0 ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1 &
sleep 22 && printf 'S /tmp/title.tga\n' | nc -w 15 127.0.0.1 9999 && kill %1
python3 runtime/tools/visible_output_gate.py /tmp/title.tga
```

PASS == distinct ≥ 8 AND non-black ≥ 1% (baseline ~100 distinct, ~14% non-black). A scheduler
change that perturbs frame pacing must not turn the title black.

### 6.5 Acceptance summary

A candidate is accepted only when **all** hold: (i) .hack freeze-rate drops to near-0 over N≥30
boots with a stated classifier; (ii) Patapon N-boot loop introduces no new freezes;
(iii) CLEANROOM band green; (iv) purity green; (v) visible-pixel gate green. (i) alone is not
sufficient (could be luck); (ii)-(v) alone are not sufficient (don't exercise the race).

---

## 7. Patapon-regression risk

**HIGH.** The scheduler is load-bearing and Patapon boots and renders on it today. The shared
loci a candidate touches are exactly Patapon's critical paths:

- `psp_scheduler.cpp` (`sched_yield_point` ordering, run-queue selection) — every Patapon thread
  switch.
- The CriCond/event-flag and sema wake paths — Patapon uses event flags (incl. the GE
  `SceGuSignal` flag, `psp_hle_kernel_eventflag.cpp`) and the FIFO sema handoff (issue #29,
  `sgx-psp-sas-attrsema` PCM thread).
- Yield-point placement / cooperative yield ordering — Patapon's GE/display/IO pacing.
- For candidate (a): every generated batch file gains preemption points — a global change to all
  recompiled Patapon code.

### Flag-gated, incremental rollout

1. **Behind a flag, OFF by default.** New behavior gates on an env var (e.g.
   `PSPRECOMP_PREEMPT=1`) and/or a CMake option, defaulting to the current behavior. The default
   build is byte-for-byte the current scheduler until the flag flips.
2. **For candidate (a):** emit the preemption *points* unconditionally (so generated code is
   stable) but make `sched_preempt()` a no-op unless the flag is set. The emitter change and the
   behavior change land separately; the no-op default must pass CLEANROOM + visible-pixel
   unchanged before the behavior is enabled.
3. **Prove on .hack first, then re-prove Patapon.** With the flag ON: run the .hack N-boot
   harness (must drop the freeze rate) AND the full Patapon gate set (must stay green). Only after
   both does the flag's default flip.
4. **One candidate at a time, banked.** Implement → validate (independent verifier) → commit
   before the next builds on it. Do not stack speculative scheduler changes.
5. **Revertibility.** Each step is a single flag flip away from the proven-good default, so a
   Patapon regression discovered later is a one-line rollback, not a code excavation.

---

## 8. Recommendation

**Pursue candidate (a) — emitter-injected instruction-budget preemption points — first.**

Rationale:

- It is the only candidate that mechanistically *guarantees* forward progress for a
  **syscall-free busy-poll** (the exact .hack `user_main` ring spin) and is the closest analog to
  PSP timer preemption, so it generalizes to the whole pattern class (busy-poll consumers,
  spin-wait sync), not just .hack. (b) is a heuristic that is hard to prove; (c) misses
  memory-only spins; (d) has the highest fidelity but the worst single-step risk and still wants
  (a) to interrupt syscall-free loops.
- Its risk is **bounded by construction** via the rollout: emit the points, default
  `sched_preempt()` to no-op, prove the no-op default leaves Patapon byte-identical-behaving
  (CLEANROOM + visible-pixel green), then enable the behavior behind a flag and prove it on the
  .hack N-boot harness while re-proving Patapon. The dangerous part (changing run-time behavior)
  is decoupled from the invasive part (changing generated code) and is flag-gated off by default.
- It keeps the fix **generic** (in the emitter + runtime core, no per-title code), satisfying the
  purity gate, and leaves (d) available as a future fidelity upgrade once (a) proves the
  preemption-point plumbing.

### First implementation step

Add the preemption-point primitive with **behavior disabled**, and prove it is a no-op:

1. Define `sched_preempt()` in the runtime as a no-op unless `PSPRECOMP_PREEMPT=1` (when set, it
   does a budget-reset + a `sched_yield_point()`-style fair yield with a memory fence).
2. Have the emitter inject `if (--ctx->preempt_budget <= 0) sched_preempt();` at **loop
   back-edges only** (smallest footprint; covers the busy-poll spin), behind a CMake/emitter
   flag, with the budget a runtime-tuned constant (not a per-title literal — keep purity).
3. Regenerate `output/`, rebuild, and run the staleness/fingerprint gate.

**Acceptance test for this first step (no behavior change yet):**

- With `PSPRECOMP_PREEMPT` unset, the CLEANROOM gate (`real_nonsprite` 13.8k–15.7k, one matrix
  fallback, heartbeat monotonic, LOOKUP_MISS sentinel-only), the purity gate, and the
  visible-pixel gate all stay **green** — i.e. injecting the (no-op) preemption points does not
  change Patapon behavior or output.
- The Patapon N-boot loop (§6.1) shows no new freezes vs the pre-change baseline.

Only after this no-op step is banked does the *next* unit turn the behavior on
(`PSPRECOMP_PREEMPT=1`) and validate against the .hack N-boot freeze-rate harness (§6.1) — the
true test of the fix — while re-running the full Patapon gate set.

---

## References

- #66 (this design), #63 (.hack driving case), #64 (separate WAITAND/WAITOR HLE bug — not this).
- `.planning/research/dothack-loadprog.md`, `.planning/research/dothack-verdict.md`,
  `.planning/research/dothack-eventflag-probe.md`.
- Current model: `runtime/src/psp_scheduler.cpp`, `runtime/include/psp_scheduler.h`,
  `runtime/src/hle/psp_hle_kernel_eventflag.cpp`, `runtime/src/hle/psp_hle_kernel_sema.cpp`,
  `runtime/src/hle/psp_hle_kernel_thread.cpp`.
- Reference: `~/GitDownloads/ppsspp/Core/HLE/sceKernelThread.cpp`, `Core/CoreTiming.cpp`.
- Validation: `scripts/verify_geometry.sh`, `runtime/tools/visible_output_gate.py`,
  `runtime/tools/purity_gate.sh`, DEBUGGING.md §2 (CLEANROOM gate).
