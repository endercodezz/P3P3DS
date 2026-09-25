# Build system: why incremental state gets lost, and what to do about it

Every Ninja behaviour described here was reproduced in small synthetic CMake
projects; numbers for the real build come from its Ninja logs and from a
proxy for the generated units (section 5). Tools: Ninja 1.13.2 and CMake
4.3.3 (both Homebrew), Apple clang 21, an 8-core Mac with 8 GB of memory.
`ccache` 4.14 was installed with Homebrew to take the measurements.

## Summary

| Observed failure | Root cause | Confidence |
|---|---|---|
| 1. `premature end of file; recovering`, then a full rebuild | Two Ninja processes appended to the same `.ninja_deps`. Ninja detects this and truncates the log at the first bad record, so every dependency record after that point is lost. Ninja 1.13.2 also has a recovery bug (upstream #2703, fixed on master and not yet released) that keeps the bad record, so the damage comes back on **every** build — which is how a checkout ends up rebuilding everything on every run. | High: reproduced exactly. |
| 2. Several Ninjas in one directory, load average above 100 | (a) `add_overlay.py` calls `cmake --build` without `-j`, so Ninja uses its default of 10 jobs. (b) On macOS every overlay target depends on `Yakumo`, so building one overlay against a stale host compiles all 89 generated units at `-j10`. (c) Killing `cmake --build` or a Python wrapper leaves its Ninja running as an orphan, and the next build then runs alongside it. | High: (a) and (b) come from the build files and a live sample, (c) was reproduced. |
| 3. `no work to do` after a real edit | It is **not** the edit-during-build race: Ninja ≥ 1.12 catches that, and the synthetic tests confirm it. It is **not** timestamp resolution either: APFS and Ninja both use nanoseconds. Two causes produce it. Another Ninja — for example one building overlays, which depend on the host — may already have compiled the edit, in which case `no work to do` is correct. Or the file arrived with an older mtime than its object, through a copy that preserves timestamps (`cp -p`, `rsync -a`, `tar`). | Medium: both reproduced; which one caused a given case cannot always be told from the logs. |
| 4. "Rebuilds from zero after an error" | Interrupting Ninja does **not** do this: after SIGKILL, SIGINT, SIGTERM or a killed compiler job, only the unfinished edges are rebuilt. The full rebuilds come from (i) the deps-log truncation in item 1, which happens when the build after an interrupted one runs alongside the orphaned Ninja from item 2(c); (ii) the remedy of deleting `.ninja_deps` and `.ninja_log`; and (iii) `generate.sh`, which runs `rm -rf generated` and so gives all 89 units new mtimes even when their content has not changed. | High for (ii) and (iii). (i) is reproduced but not proven for each past case. |

**Recommendation: keep Ninja.** Everything observed can be fixed with a lock, a
compiler cache and a Ninja upgrade. None of the alternatives fixes an observed
problem that these do not. Ranked by payoff over effort:

1. **One lock per build directory, taken by Ninja itself.** Set
   `CMAKE_MAKE_PROGRAM` to a 30-line wrapper (Appendix A) that takes an exclusive
   `flock` on the build directory and then `exec`s Ninja. Every
   `cmake --build`, whether from a script, `add_overlay.py` or the command line,
   then waits for the build already running instead of racing it. The lock is
   released when Ninja exits, not when its parent does. Effort: one file plus
   one cache variable. This removes the cause of items 1 and 2(c) and the
   concurrency behind item 4.
2. **ccache.** A full rebuild of the 89 units from a warm cache took **1.2 s**
   in the measurement below; a cold rebuild takes about **14 minutes** on the
   development machine. Lost logs, `ninja -t clean`, regenerated sources and
   the item-3 `touch` then cost seconds. Effort: `brew install ccache` and one
   CMake line. Miss overhead was not measurable.
3. **Bound the parallelism of the heavy compiles.** Pass an explicit `-j` in
   `add_overlay.py`, or better, put the generated units in a Ninja job pool of
   depth 2 so that no invocation can start 10 of them at once. Effort: a few
   lines. This fixes the load half of item 2.
4. **Make a corrupted deps log a one-time cost.** Use Ninja from master
   (1.14.0.git, `brew install --HEAD ninja`) until 1.14.0 is released, or
   check the log before each build and run `ninja -t recompact` when it is bad.
   Effort: minutes.
5. **Drop the per-overlay reconfigure, and build all overlays in one Ninja
   run.** The `CONFIGURE_DEPENDS` glob already picks up new overlays. This
   saves about 1.15 s × 355 ≈ 7 min per full overlay run, and it gives one
   scheduler with one `-j`. Effort: split `add_overlay.py` into a recompile
   step and a build step.
6. **Header coupling: low priority.** The data does not show what was
   assumed. The generated units depend on only four framework headers, not on
   the host HLE headers, and host changes rebuild 3–8 edges. A precompiled
   header would save about 8 % of a full rebuild.

**First change:** the locking `CMAKE_MAKE_PROGRAM` wrapper, together with an
explicit `-j 2` in `add_overlay.py`. That stops new deps-log corruption and the
memory stalls. ccache should follow the same day, because it turns the cost of
any remaining lost state from 14 minutes into seconds.

## Mitigations in place

The build now implements ranks 1 to 5 and the `generate.sh` fix from item 4.

| Mitigation | Where | Default | Turn off or tune |
|---|---|---|---|
| One Ninja per build directory (rank 1) | `cmake/BuildLock.cmake`, `cmake/ninja_locked.py.in` | on with a Ninja generator on macOS and Linux | `-DPSPRECOMP_BUILD_LOCK=OFF` |
| Repair of a damaged `.ninja_deps` before each build (rank 4) | the same wrapper | on together with the lock | as above |
| Warning for Ninja 1.13.2 (rank 4) | `cmake/BuildLock.cmake` | once per build directory | — |
| ccache (rank 2) | top-level `CMakeLists.txt` | used when installed | `-DPSPRECOMP_CCACHE=OFF` |
| Path defines only on `host/main.cpp` (item 4.4) | `profiles/mhp3rd/CMakeLists.txt` | — | — |
| Job pool for generated code (rank 3) | `psprecomp_generated` pool, `JOB_POOL_COMPILE` on `Yakumo` and every overlay | one job per 4 GiB of memory, at least 1 | `-DPSPRECOMP_GENERATED_JOBS=N` |
| Explicit `-j` in `add_overlay.py` (rank 3) | `add_overlay.py -j N` | 2 | `-j N` |
| No per-overlay reconfigure, one Ninja for all overlays (rank 5) | `add_overlay.py --no-build`, `build_overlays.sh [build_dir] [jobs]` | 2 jobs | second argument |
| Regeneration keeps unchanged units (item 4.3) | `generate.sh` | — | — |

How they behave:

- **The lock.** CMake writes `<build>/ninja-locked` and makes it
  `CMAKE_MAKE_PROGRAM`; the real Ninja is kept in `PSPRECOMP_REAL_NINJA`. The
  wrapper is Appendix A with Appendix B built in and two further changes. It
  locks every invocation except
  `--version` and `-h`, including `-n` and `-t` tools, because loading a
  damaged log can truncate it and `recompact`, `restat` and `clean` write it.
  And it records the directories it holds in the environment variable
  `PSPRECOMP_BUILD_LOCKS`, so commands that Ninja itself runs, such as the
  CMake regeneration step (which calls `ninja -t recompact` and
  `ninja -t restat`) or a nested `cmake --build` of the same directory, do not
  deadlock on their own parent's lock. A reconfigure started by hand while a
  build runs waits for that build. Compile jobs inherit the locked descriptor,
  so the lock is released only when Ninja and all its jobs have exited. A
  direct `ninja` call still bypasses the lock.
- **Windows.** The lock is a no-op there, and with any generator other than
  Ninja: `flock` does not exist, and a `.bat` shim with `msvcrt.locking` is
  not written yet.
- **Deps log repair.** Before a build, inside the lock, the wrapper runs the
  validator from Appendix B. If it finds a damaged record it prints the
  offset and runs `ninja -t recompact`, which drops the bad record and
  everything after it once, instead of on every build as Ninja 1.13.2 does
  by itself. It never deletes the file and never runs `ninja -t restat`.
- **ccache.** When ccache 4.8 or later is found, the launcher also passes
  `base_dir=<source dir>`, so paths inside the checkout are hashed relative to
  it and a checkout at another path gets direct-mode hits. Older versions get
  plain `ccache`; set `base_dir` in the ccache configuration for them.
- **Job pool.** CMake 4.3 sets `JOB_POOL_COMPILE` per target, so the host
  sources of `Yakumo` share the pool with the generated units. The
  Makefile and Visual Studio generators ignore job pools.

### Ninja 1.13.2

With the lock, two Ninjas no longer write one log, so the corruption in item 1
should not happen. If it does, for example after a direct `ninja` call, the
wrapper repairs it before the next `cmake --build`. To repair a log by hand:

```bash
ninja -C out/mhp3rd -t recompact
```

The lasting fix is Ninja 1.14.0 once it is released, or Ninja from master until then
(`brew install --HEAD ninja`, or build it from source). CMake warns once per
build directory when it finds 1.13.2.

### Measured on the real tree

Ninja 1.13.2, ccache 4.14, 8 GB of memory, `-j 2`, 89 generated units plus
the host.

| Check | Result |
|---|---|
| Cold build of `Yakumo` (empty cache) | 708 s, never more than 2 compiler processes |
| Second `cmake --build` started during it | waited 697 s, then `no work to do` |
| `.ninja_deps` and `.ninja_log` deleted, rebuild from a warm cache | 1.7 s, 117 of 117 direct hits |
| `ninja -t clean`, rebuild | 1.6 s |
| All generated units touched, rebuild | 1.4 s, 90 of 90 direct hits |
| Second checkout at another path, same cache | 2.6 s, 116 of 117 direct hits (the miss is `host/main.cpp`, which carries the checkout paths) |
| `generate.sh` rerun with nothing changed | 0 units rewritten; next build `no work to do` |
| Path record checksum broken halfway through `.ninja_deps` | plain Ninja: full recompile on each of three builds; `cmake --build`: repaired, one recompile, then `no work to do` |
| Validator on the 1.5 MB log | 8 ms |
| `cmake --build` killed 6 s into a build of 8 overlays, next build started at once | next build waited 177 s for the orphaned Ninja, then `no work to do`; log valid |
| `build_overlays.sh` on 30 overlays | extraction and recompile about 22 s, then one Ninja for all 30 libraries, at most 2 compiler processes, one CMake rerun instead of 30 |
| `add_overlay.py` for one new overlay, no explicit configure | 1.8 s; the glob check reran CMake and built the new target |

---

## 1. How the build keeps its incremental state

Ninja keeps two files in the build directory. Only Ninja writes them; CMake
never touches them.

- **`.ninja_log`** is a text log with one line per finished edge: start and
  end time, the recorded mtime, the output and a hash of the command. Ninja
  uses it to detect changed command lines. An output that has no entry is
  rebuilt (`command line not found in log`), so deleting the file rebuilds
  everything. The file is line-buffered and append-only. Ninja rewrites it
  (recompaction) when it holds too many stale entries. The real file was last
  recompacted at 11:19:47, which is its birth time.
- **`.ninja_deps`** is a binary log of the header dependencies Ninja reads
  from each compiler `.d` file (`deps = gcc`, which CMake uses for every C++
  compile). It is a sequence of *path records*, each numbered implicitly by
  position and carrying the checksum `~id`, and *deps records*, which list
  node ids. Records are appended and flushed one at a time. An output that has
  no deps record is rebuilt, so losing the file rebuilds every object.

Since 1.12 (#1943), Ninja stores the **start time of the command** as the
recorded mtime, not the mtime of the output. The real `.ninja_log` shows
this: the ten overlay units started together at 07:21:56.31 carry that
timestamp, although they finished up to 74 s later. If an input changes while
its command runs, the input is newer than the recorded time, so the next build
reruns the command.

Ninja itself also writes `.ninja_lock` (an empty file it stats to read the
filesystem clock). That file is not a lock. Ninja has no protection against a
second Ninja in the same directory: upstream #2637 asks for one, is still
open, and is tagged for 2.0.

### What happens to them when Ninja is interrupted

In the synthetic project (300 sources, each compile padded to 1 s, `-j4`), each
build was interrupted after 8 s and the state checked:

| Interruption | `.ninja_deps` | Edges left for the next build |
|---|---|---|
| `kill -KILL ninja` | valid | 273 of 300: exactly the unfinished ones |
| `kill -INT ninja` (Ctrl-C) | valid | 273 of 300 |
| `kill -TERM ninja` | valid | 273 of 300 |
| one compiler job killed (memory pressure) | valid | 273 of 300 |

Killing Ninja, even with SIGKILL, never damaged either log in these tests.
Records are written and flushed one at a time, so a kill between two records
leaves a valid file. macOS does not normally kill processes under memory
pressure; it swaps. During this investigation 3 GB of the 4 GB swap was in use.

What killing *does* cause is **orphans**:

| What is killed | What keeps running |
|---|---|
| `ninja` (SIGKILL) | its 4 compile jobs, which finish and write objects nobody records |
| `cmake --build` (SIGTERM, like a timeout or a killed script) | `ninja` and all its jobs |
| a Python wrapper around `cmake --build` (like `add_overlay.py`) | `cmake --build` and `ninja` |

In the experiment, a new build started right after killing `cmake --build`
found **two Ninjas in the same directory**. That is the path from "a build
was interrupted" (item 4) to "the deps log is corrupted" (item 1).

### What happens when two Ninjas share a directory

Each Ninja reads `.ninja_deps` at startup and numbers the new paths it
discovers from that snapshot. When two Ninjas run at once, both append path
records with the same ids. The next load finds a record whose checksum does
not match its position, prints `premature end of file; recovering`, and
truncates the file there.

Reproduction: 2 × 150 sources, `ninja grp0` and `ninja grp1` started together,
then `ninja app` three times with nothing changed.

| | Ninja 1.13.2 (installed) | Ninja master (1.14.0.git, e4a1280) |
|---|---|---|
| deps log after the concurrent pair | corrupt at byte 708 of 124 032 (record 6) | same |
| next `ninja app` | warning, **300 of 300 recompiled** | warning, 300 of 300 recompiled |
| second `ninja app` | warning, **300 of 300 recompiled** | no work to do |
| third `ninja app` | warning, **300 of 300 recompiled** | no work to do |
| after `ninja -t recompact` | one more full rebuild, then no work to do | — |

The 1.13.2 column is the "checkout that rebuilt everything on every build".
The cause is upstream
[#2703](https://github.com/ninja-build/ninja/issues/2703). `DepsLog::Load`
advances the truncation offset *before* it validates a record, so recovery
keeps the bad record, and every later build hits it again and throws away
everything written after it. The fix,
[PR #2764](https://github.com/ninja-build/ninja/pull/2764), was merged on
2026-04-12 for 1.14.0. As of 2026-09-18 the latest release is still 1.13.2
(2025-11-20). CMake 4.4 contains nothing related.

Even with the fix, a concurrent pair still costs one full rebuild. The records
after the corruption point cannot be trusted, so they are dropped. The only
real fix is not to run two Ninjas at once.

### How to recognise it

A build that starts with `ninja: warning: premature end of file; recovering`
and then recompiles every generated unit is this failure. The validator in
Appendix B checks a `.ninja_deps` file read-only and reports the first bad
record. A killed CMake generate step can also leave `build.ninja.tmp*` and
`compile_commands.json.tmp*` files behind in the build directory; they are
harmless and can be deleted.

## 2. Item 2: where the load comes from

- `add_overlay.py` runs `cmake --build <dir> --target overlay_<x>` with no
  `-j`. `ninja -h` on the development machine gives `default=10`, and the
  build's `.ninja_log` shows 10–11 overlay compiles running at once. A sample
  taken during a full overlay build showed **one** Ninja with **10**
  `clang -cc1` jobs, a load average of 98 and 3.0 GB of swap in use on 8 GB. So
  one `add_overlay.py` is enough to reach load averages near 100. Two or three
  Ninjas make it worse.
- On macOS (and Windows), each overlay module links against `Yakumo`,
  which it uses as the bundle loader or import library. In a configured tree, `ninja -t inputs overlay_<x>` lists **all 89 generated
  host units and 18 host sources**. If the host is stale, whether from an edit
  or a lost deps log, the first `add_overlay.py` rebuilds it at `-j10`. At
  more than 1 GB per unit that is over 10 GB of demand on an 8 GB machine.
  `bootstrap_overlays.sh` uses `-j 3`; the other entry points use the default.
- Each host relink also relinks every overlay module on the next full build
  (implicit dependency on `bin/Yakumo`). That part is cheap: the median
  module link is 0.075 s.

## 3. Item 3: `no work to do` after an edit

The same synthetic project was run under Ninja 1.13.2, with the object held
back 3 s after the compiler read the source. That imitates a large unit that
reads its input early and writes its output late.

| Scenario | Next build recompiles? |
|---|---|
| Source edited while its own compile was running | yes |
| Header edited while a dependent compile was running | yes |
| Edit after Ninja scanned the graph, before the edge started | yes |
| Edit 0.28 s after the object was written (same second) | yes (mtimes are ns: `…041.958236606` against `…041.680060239`) |
| Edit carrying an **older** mtime (`cp -p`, `rsync -a`, `tar`) | **no: `no work to do`** |
| Another Ninja (building an overlay, which depends on the host) compiled the edit first | no, correctly |
| *For comparison:* Makefile generator, source edited during its compile | **no: the edit is silently lost** |

The edit-during-build race is real in general. It exists in Make and in
Ninja before 1.12: Ubuntu 22.04 ships 1.10.1 and 24.04 ships 1.11.1, which
matters for Linux or Steam Deck builds on distro packages. Ninja 1.12 and later detect it, so on a current
Ninja the two causes that remain are the ones in the last two rows of the table
above.

Some small measures help either way. Never copy source files between checkouts
with mtime-preserving tools. With ccache in place, `touch` is a free remedy
(section 5).

## 4. Item 4: "rebuilds from zero after an error"

A failed or interrupted build redoes only the unfinished edges (section 1). The
full rebuilds that were seen come from:

1. **Deps-log truncation after concurrent Ninjas.** An interrupted
   `cmake --build` leaves an orphaned Ninja, and the next build runs alongside
   it. On 1.13.2 this repeats on every build until the log is recompacted or
   deleted.
2. **The remedy itself.** Deleting `.ninja_deps` rebuilds every object;
   deleting `.ninja_log` does too (`command line not found in log`).
   `ninja -t recompact` keeps every record before the corruption point, so it
   is never worse than deleting.
3. **`generate.sh`** runs `rm -rf generated` before `psp_recomp`. All 89 units
   get new mtimes even when the output is byte-identical.
4. **Target-wide absolute-path defines.** `MHP3RD_DEFAULT_GAME_DIR` and
   `MHP3RD_NIDS_CSV` are set on the whole `Yakumo` target, so they are
   on every generated unit's command line. Any change to them, such as moving
   the checkout, changes 89 command lines. This also costs cache hits across
   worktrees (section 5).
5. **Real header changes.** Covered in section 7.

## 5. Mitigation: compiler cache (measured)

The generated corpus was not built, as the rules required. Instead, a proxy was
built: 89 units that each include `psprecomp/runtime.hpp` and the same system
headers as a real unit, which is about 1000 headers. Each also holds a
97 000-entry table, giving 1.9 MB of source per unit (real: 0.93 MB) and
776 680 bytes of object (real: 0.78 MB average). A cache hit costs the same no
matter how long the miss took to compile, so hashing and copying are what the
proxy has to match, and it matches or exceeds the real units on both. The proxy
also carries an absolute-path define, like `MHP3RD_DEFAULT_GAME_DIR`. All runs
used `-j2`, on a machine busy with another build.

| Run (89 units + main + link) | Time | ccache |
|---|---|---|
| A. no cache, full build | 54.4 s | — |
| B. ccache, cold cache | 52.8 s | 0 / 90 hits |
| C. **`.ninja_deps` and `.ninja_log` deleted** (items 1 and 4) | **1.2 s** | 90 / 90 |
| C at `-j8` (hits use little memory) | 0.6 s | 90 / 90 |
| D. all 89 sources rewritten, same content (`generate.sh`) | 1.4 s | 89 / 89 |
| E. `ninja -t clean` | 1.3 s | 90 / 90 |
| F. logs deleted, preprocessor mode (`CCACHE_NODIRECT`) | 11.2 s | 90 / 90 |
| G. second checkout at another path, `base_dir` set | 17.4 s | 90 / 90 |
| H. same, preprocessor mode | 17.7 s | 90 / 90 |

What this means for the real build:

- A lost deps or build log, a regenerated corpus, a clean, or the item-3
  `touch` costs about **1–2 s for the 89 host units** instead of about
  **14 min**, plus the host link, which is 0.35 s in the real log. The 1871
  overlay objects (813 MB) are about 25 s at `-j2`. That overlay figure is
  extrapolated from about 27 ms per hit and was not measured.
- The miss overhead is within noise (B against A).
- Cache size is about the size of the objects: 80 MB for the proxy's 70 MB.
  For the real host and overlays that is 0.9 GB per configuration, which fits
  the 5 GB default.
- Worktrees share the cache (`CCACHE_BASEDIR=$HOME`), but the target-wide
  absolute-path define turns direct-mode hits into preprocessor-mode hits:
  17 s instead of 1.2 s (G). Moving the two path defines onto the one source
  file that uses them would make them direct hits.
- Hits need little memory, so a warm rebuild could run at `-j8`. A cold
  rebuild still needs the job pool from rank 3.
- Windows: ccache supports MSVC, including with Ninja, but not through the
  Visual Studio/MSBuild generator. sccache is the more common choice for MSVC
  and behaves the same way for this purpose. sccache was not measured.

A cache does not stop corruption. It makes corruption cheap. It does nothing
for item 2 (memory) and nothing for Ninja failing to see an edit (item 3), so
it complements the lock and does not replace it.

Integration (applied, see "Mitigations in place"): the top-level
`CMakeLists.txt` finds ccache and sets `CMAKE_CXX_COMPILER_LAUNCHER` unless
`PSPRECOMP_CCACHE` is off or a launcher is already set.

## 6. Mitigation: no concurrent builds in one directory

**Lock via `CMAKE_MAKE_PROGRAM` (recommended).** `cmake --build` always runs
`CMAKE_MAKE_PROGRAM`, so a wrapper there covers every entry point:
`add_overlay.py`, `build_overlays.sh`, `bootstrap_overlays.sh`, `generate.sh`
and manual builds. The wrapper takes `flock` on `<build>/.build_lock`, marks
the descriptor inheritable and `exec`s Ninja, so Ninja itself holds the lock.
Measured in the synthetic project:

- two concurrent `cmake --build` calls: the second waited 2.2 s, and the deps
  log stayed valid;
- `cmake --build` killed mid-build, then a new build started: the new build
  waited **126 s** for the orphaned Ninja to finish instead of racing it, the
  deps log stayed valid, and the next build did only the remaining work.

The kernel releases a `flock` when the holder dies, so a `kill -9` never leaves
a stale lock. `ninja -t …`, `-n` and `--version` pass straight through. Only a
direct `ninja -C out/mhp3rd` bypasses the lock. A build step that ran
`cmake --build` on its own directory would deadlock; nothing in the project
does that today. On Windows the same wrapper needs `msvcrt.locking` and a
`.bat` shim, because `CMAKE_MAKE_PROGRAM` must be an executable. A lock that
only the Python process holds (`subprocess.run` with the lock fd not passed
on) is **not** enough: killing the wrapper releases the lock while its Ninja
keeps running.

**One Ninja for all overlays.** `build_overlays.sh` could run the recompile
step (`wrap_overlay.py` and `psp_recomp`) for every overlay first, then build
once with `cmake --build <dir> --target overlay_a overlay_b … -j N`, or build
the `all` target. One Ninja, one reconfigure, one parallelism limit. The
per-overlay loop can stay as the resume mechanism for the recompile step.

**Bounded parallelism.** A job pool for the generated units, for example
`set_property(GLOBAL APPEND PROPERTY JOB_POOLS aot=2)` and
`JOB_POOL_COMPILE aot` on `Yakumo` and the overlay targets, caps the
expensive compiles in every invocation, whatever `-j` the caller passed. CMake
4.3 sets this per target; CMake 4.4 adds a per-file-set `JOB_POOL_COMPILE`. Job
pools are a Ninja feature: the Makefile generator ignores them. The
alternative is to pass `-j` everywhere, which is easy to forget.

## 7. Mitigation: remove the per-overlay reconfigure

`profiles/mhp3rd/CMakeLists.txt` already globs `overlays/*/meta.txt` with
`CONFIGURE_DEPENDS`. The synthetic test confirmed that a new directory picked up
by such a glob gets its target without an explicit reconfigure. Building
`--target <new>` straight away prints `Re-checking globbed directories… Re-running
CMake…` and then builds it. `add_overlay.py` already writes `meta.txt` before
building, so its `cmake -S . -B <build>` is redundant. It costs 1.15 s per call
with 360 overlays, measured in a scratch configure of the real tree: about
7 min over 355 overlays. The glob check that replaces it costs 0.09 s. A
reconfigure also rewrites `build.ninja` while another Ninja may be running from
it, which is harmless because CMake writes through a temporary file and renames
it, but it adds churn.

## 8. Mitigation: detect and repair a damaged deps log

- **Detect:** Appendix B is a read-only validator that follows Ninja's own
  checks. It reports the offset and cause of the first record Ninja would
  reject. It takes under 0.1 s on the real 2.7 MB file. Running it before each
  build, inside the lock, turns a silent 14-minute rebuild into a message.
- **Repair on 1.13.2:** `ninja -t recompact`. Loading drops the bad record from
  memory, and recompaction rewrites the file from memory. In the reproduction
  it cost one rebuild of the records after the corruption point and was
  stable afterwards. This is also the workaround reported upstream.
- **Or upgrade:** Ninja master already does this during its normal load. It
  built from source in about a minute. `brew install --HEAD ninja` does the
  same, or wait for 1.14.0.
- **Do not use `ninja -t restat`** for this. It rewrites the recorded mtimes in
  `.ninja_log` from the outputs currently on disk. That does not repair
  `.ninja_deps`, and it can hide exactly the in-flight edits that Ninja 1.12+
  otherwise catches.
- **Do not delete the files.** Deleting is always at least as expensive as
  recompacting.

## 9. Mitigation: header coupling

Measured in the real deps log, a generated unit depends on `generated_units.hpp`,
`psprecomp/runtime.hpp`, `allegrex_context.hpp`, `guest_memory.hpp`,
`nid_registry.hpp`, and 1013 SDK headers. It does **not** depend on the host HLE
headers, so host edits rebuild only 3–8 edges.

- **Precompiled header:** those headers cost 1.45 s to compile at `-O2` and
  0.12 s through a PCH, so a PCH saves about 1.3 s of the 14.2 s median, or
  about 120 s of compile time per full rebuild (8 %). It does not reduce
  coupling: editing a precompiled header rebuilds the PCH and all 89 units.
- **Narrower header:** splitting what generated code needs (`AllegrexContext`,
  guest memory accessors, the call ABI) from the rest of `runtime.hpp` would
  stop runtime-only edits from rebuilding the corpus. Its value depends on how
  often those edits happen. Worth doing when the
  runtime is next reorganised; not urgent.
- **With ccache in preprocessor mode,** edits to those headers that leave the
  preprocessed text unchanged, such as comments, become hits.

## 10. Other build systems

The question for each one: would it avoid items 1–4, what would the migration
cost, and what would be lost?

| | Avoids 1 (deps log) | Avoids 2 (concurrency, memory) | Avoids 3 | Avoids 4 | Migration | Loses |
|---|---|---|---|---|---|---|
| **Make** (CMake generator) | Partly: dependencies are per-target files, with no shared binary log | No: concurrent makes still race on objects; `-j` default is 1; no job pools | **Worse:** the in-flight edit was lost in the test | No | None: `-G "Unix Makefiles"` | Race protection, job pools; no-op build 3.87 s vs 0.02 s, one edited module 3.87 s vs 0.08 s (360 modules, measured) |
| **Meson** | No: generates Ninja files, same `.ninja_deps` | No: same Ninja | Same as Ninja | Same as Ninja | Rewrite about 300 lines of CMake plus scripts; overlays need a generator step | CMake presets and cache; Windows support is fine |
| **xmake** | Yes: own dependency store | Takes a project file lock (see upstream issue #992), so concurrent calls serialize | Content and mtime based; not tested | Mostly | Rewrite the build, CI and scripts; learn Lua; different MSVC and SDL3/Vulkan package handling | CMake ecosystem, `WINDOWS_EXPORT_ALL_SYMBOLS` (needs a replacement) |
| **Bazel** | Yes: content-addressed action cache | Yes: one server per output base; a second command waits ("Another command is running") | Yes: content hashes | Yes: a lost state is a cache hit | Largest: BUILD files, a C++ toolchain for three OSes, psp_recomp as an action, `bundle_loader` and export-all-symbols rules written by hand | The JVM server uses memory on an 8 GB machine (my estimate: a few hundred MB to 1 GB); IDE and compile_commands support is weaker |
| **Buck2** | Yes | Yes (daemon) | Yes | Yes | As for Bazel, with less mature macOS and MSVC C++ toolchains | As for Bazel, without the JVM |

Bazel and Buck2 would avoid items 1, 2 and 4 by design. So do a 30-line lock
plus ccache, which also keep the current build, IDE support and Windows setup.
None of these alternatives fixes an observed failure that the Ninja mitigations
do not, so none is recommended. Make is ruled out: in the measurements it
reintroduces item 3 and is slower on every incremental build.

## Appendix A: locking wrapper for `CMAKE_MAKE_PROGRAM`

Tested on macOS in the synthetic project. Configure with
`cmake -B out/mhp3rd -DCMAKE_MAKE_PROGRAM=<path>/ninja-locked …`. For an
existing build directory, change the cached value and reconfigure. The lock
file is deliberately not `.ninja_lock`, which Ninja uses for its own purposes.

```python
#!/usr/bin/env python3
"""Drop-in for ninja: hold an exclusive flock on <build dir>/.build_lock for as
long as Ninja runs. The exec'd Ninja inherits the locked descriptor, so killing
cmake or a wrapper does not release it early, and a dead Ninja never leaves a
stale lock."""
import fcntl, os, shutil, sys, time

real = os.environ.get("REAL_NINJA") or shutil.which("ninja")
args = sys.argv[1:]
build_dir = "."
for i, a in enumerate(args):
    if a == "-C" and i + 1 < len(args):
        build_dir = args[i + 1]
    elif a.startswith("-C") and len(a) > 2:
        build_dir = a[2:]
if not any(a in ("--version", "-t", "-n") for a in args):
    fd = os.open(os.path.join(build_dir, ".build_lock"), os.O_CREAT | os.O_RDWR, 0o644)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.stderr.write(f"another build is running in {os.path.abspath(build_dir)}; waiting\n")
        t = time.time()
        fcntl.flock(fd, fcntl.LOCK_EX)
        sys.stderr.write(f"waited {time.time() - t:.1f} s\n")
    os.set_inheritable(fd, True)
os.execv(real, [real] + args)
```

`REAL_NINJA` must point at the real Ninja if the wrapper is itself named
`ninja` on `PATH`.

## Appendix B: read-only `.ninja_deps` validator

This follows `DepsLog::Load` in Ninja 1.13.2 (format version 4). It never
writes the file. Exit status 0 means Ninja will load the file without a
warning.

```python
#!/usr/bin/env python3
import struct, sys

def check(path):
    d = open(path, "rb").read()
    sig = b"# ninjadeps\n"
    if not d.startswith(sig) or struct.unpack_from("<i", d, len(sig))[0] != 4:
        return "bad signature or version (Ninja starts over)"
    off, nodes = len(sig) + 4, 0
    while off != len(d):
        if off + 4 > len(d):
            return f"partial record at {off}"
        size, = struct.unpack_from("<I", d, off)
        is_deps, size = size >> 31, size & 0x7FFFFFFF
        if size > (1 << 19) - 1 or off + 4 + size > len(d):
            return f"truncated record at {off}"
        body = d[off + 4: off + 4 + size]
        if is_deps:
            if size % 4:
                return f"bad deps record at {off}"
            ids = struct.unpack(f"<{size // 4}i", body)[3:]
            if any(i < 0 or i >= nodes for i in ids):
                return f"deps record with unknown node at {off}"
        else:
            if size < 4 or (~struct.unpack_from("<I", body, size - 4)[0]) & 0xFFFFFFFF != nodes:
                return f"path record id mismatch at {off} (two Ninjas wrote this log)"
            nodes += 1
        off += 4 + size
    return None

err = check(sys.argv[1])
print(err or "ok")
sys.exit(1 if err else 0)
```

## Appendix C: how the measurements were made

The synthetic projects had 2 × 150 (or 2 × 60) static-library sources, each
with its own header plus one shared header, an executable, and `MODULE` targets
linked against the executable, like the overlays. A compiler launcher could
hold each object back for `SYNTH_SLEEP` seconds after compiling it. Scenarios:
two Ninjas on disjoint targets; SIGKILL, SIGINT or SIGTERM to Ninja after 8 s;
a killed compiler job; SIGTERM to `cmake --build` and to a Python wrapper; the
edits in section 3; a locking `CMAKE_MAKE_PROGRAM`; 360 trivial modules under
Ninja and under Make; a new `CONFIGURE_DEPENDS` directory. The ccache proxy is
described in section 5. Real-tree numbers come from `out/mhp3rd/.ninja_log`,
`.ninja_deps` (read with Appendix B), and a configure of the tree into a
temporary directory, which only ran `cmake`, `ninja -t inputs` and `ninja -n`. Upstream sources: Ninja release notes
1.12.0–1.13.2, issues #2637 and #2703, PR #2764, `src/deps_log.cc` at v1.13.2,
and the CMake 4.4 release notes.
