# Physics measurement contract

This is the measurement contract for [Milestone 12](../NEXT_STEPS.md#12-real-time-physics-pipeline-rework).
Goal 1 establishes the baseline; later goals retain the same workloads and report
format. Numerical comparison failures remain failures even when they are not
intermediate completion gates. Engine APIs, force ownership, and simulation
settings are unchanged by the diagnostic consumers.

## Reference and provenance

The only reference is `physics-pipelines-overoptimized` at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`. The rework branch is
`physics-pipelines`; do not compare with `main`. A branch name alone is not a
reference pin: always record the full commit, source hashes, build configuration,
compiler, installed SDK archive hash, consumer executable hash and command.

Local preserved artifacts are under `sandbox/milestone_12/goal_1/reference/`:

- `source.tar` and `source/`: tracked source from the pinned commit.
- `sdk_linux/`, `sdk_sanitizers/`, `sdk_windows/`: copies of the reference SDKs.
  Their engine archive hashes match the previous reference evidence; 34 recorded
  source hashes also match the pinned commit.
- `original_bin/`: the existing Linux car, pit and soft-body stability
  executables, with assets. New diagnostic consumers do not replace them.
- `evidence/`: preserved Milestone 11 logs and evidence manifests. Historical
  directions inside those files are superseded by Milestone 12.
- `manifest.json`: SHA-256 inventory and the full reference commit. Revalidate
  it before using or relocating the reference. Never rebuild into these SDKs.

The installed dependencies, compiler paths and linker commands are recorded in
each consumer's CMake cache/build files. New measurements belong under
`sandbox/milestone_12/goal_<n>/`, with separate directories per version, mode and
repetition. Keep the reference and previous measurements intact.

## Frozen workloads and settings

| Workload | Endpoint and controls | Reference settings |
| --- | --- | --- |
| Authored car | 3,600 frames / 60 simulated seconds; idle to 2 s, drive right to 30 s, then left; grounded jump requests at 5 and 35 s | Authored scene, terrain, materials, 200 particles; existing float-valued 240 Hz tick, four configured substeps, eight solver iterations |
| Particle pit | 720 frames / 12 simulated seconds, unchanged drop-and-pile sequence | 800 particles, diameter 5, 16 rigid vertices, mass 1, friction 0.5, restitution 0.5; gravity 9.8; 240 Hz, four configured substeps, eight iterations |

Both consumers submit four separate physics ticks per displayed frame through
`physics_update(4)`. Configured subdivisions are a minimum; adaptive subdivisions
may add work. Record actual completed subdivisions and observed duration, rather
than inferring them from requested ticks. The car's float tick produces a small
representational difference from exactly 60 seconds; preserve and report it.

The pit uses a staggered 40-column, 20-row grid at 5.5-unit spacing and 30-unit
initial bottom clearance in the existing 230-unit-wide pit. Neither workload may
change geometry, body counts or materials to improve performance. Later goals
may tune ticks, subdivisions and iterations, but every tested configuration must
be explicit. Shorter runs and smaller particle counts are diagnostics only.

## Exclusive stage accounting

[`tools/physics_measurement`](../tools/physics_measurement/CMakeLists.txt) builds
independent consumers against an installed SDK. GNU-compatible link wrappers
observe cross-object calls, forward the original arguments exactly once, and
return the original results. They do not replace the engine archive, disable
stages, change solver iterations, or introduce a public profiling API.

Every timed call pushes a bounded stack frame. On return, it records inclusive
nanoseconds and subtracts the complete inclusive durations of its immediate
timed children. The remaining exclusive nanoseconds are charged once to that
call. Sum **exclusive** costs across functions to obtain stage costs. Never add
inclusive parent and child costs. The sum of exclusive costs must equal the
outer physics interval exactly; stack overflow, underflow or invalid clocks
invalidate accounting. A deterministic test covers nested/recursive calls,
conservation, invalid timestamps and stack bounds.

| Stage | Included measured work |
| --- | --- |
| `force_preparation` | Acceleration clearing, gravity, external force/torque evaluation, spring and beam loads, soft-body frame updates |
| `integration` | Integration entry points, rigid integration remainder, joint and elastic-node motion |
| `subdivision_sweeps` | Rigid/soft swept-duration queries and joint/soft step limits |
| `contact_gathering` | Initial gathering and later rigid, soft-boundary and beam-pair refreshes |
| `joint_gathering` | Joint assembly and its pipeline entry |
| `rigid_solving` | Contact-pass remainder, block solving and individual rigid responses |
| `joint_solving` | Joint response and transform locks |
| `soft_solving` | Soft-boundary and beam-pair response |
| `finalization` | Contact finalization and observed dirty-hitbox flushing |
| `reporting` | Observed report-finalization and report-merge calls |
| `orchestration` | Outer update and solver-loop work left after subtracting timed children |
| `observer_validation` | Existing fixture substep-observer work, separated from engine stages |

The exact call boundaries are in
[`profile_functions.inc`](../tools/physics_measurement/profile_functions.inc).
Same-object and static calls cannot be intercepted by `--wrap`; their cost stays
in the enclosing timed function. Thus the table is a measured partition, not a
claim of exhaustive helper-level instrumentation. For example, same-object
reporting helpers remain in the caller's remainder. When a later goal needs finer
attribution, extend the partition, verify conservation and trajectory parity,
and record the changed instrumentation. Do not reinterpret old stage totals.

Call counts mean actual completed intercepted invocations. The contact-pass
callback count measures executed solver iterations, including early convergence;
it is not configured iterations multiplied by subdivisions. Candidate/contact
counts are the existing reports' work occurrences, aggregated once from the
returned root report. The counter-add wrapper also attributes candidate/contact
increments to the deepest timed call at their source; summing these exclusive
work counts must reproduce the returned root totals. They include
rediscovery/refresh work and are **not unique pair counts**. Report loss or
saturated counters invalidate work accounting. Attempted/completed subdivisions
come from that same report.
The existing substep observer independently accumulates completed duration and
count while preserving any fixture observer and its caller-owned context.
These observer counts describe complete updates; a standalone substep has no
update-level completion counter or observer callback in the frozen engine.

`ROHR_MEASUREMENT_PROFILE=0` forwards the same wrappers without stage timers.
Both modes capture a per-update hash of live entity handles, masks and scalar
position/velocity/acceleration/orientation state outside the physics timer. They
also check finite values and record counts and observed duration. Compare every
trace row except `wall_ns` to prove profiling preserves the observed trajectory
and work. This is a same-platform checksum, not a penetration or tunneling oracle.

Timer overhead, wrapper dispatch and the observer cost perturb timings. State
validation and CSV writing occur outside the recorded outer physics interval,
but inside the consumer's broader frame work. Report profiling overhead against
the timer-disabled consumer. Instrumented results explain costs; they do not
qualify for rendered-FPS acceptance. Preserve unwrapped reference executables for
performance comparisons. The separately built `car_playback` and `pit_playback`
targets link the same installed SDK without diagnostic wrappers, observers,
state scans or per-update CSV logging; use those for clean rendered measurements.

## Repeatable commands

Enter `nix develop`. Configure this standalone CMake project with `Rohr_DIR`
pointing at the selected installed SDK and `CMAKE_PREFIX_PATH` containing its SDL
dependencies. Set `ROHR_MEASUREMENT_SOURCE_DIR` to source matching that SDK's
internal headers. Linux Release, Linux ASan/UBSan and MinGW builds use the same
consumer sources. The local `goal_1/build_measurement.py` retains the exact
configuration used for this baseline; its logs record all build diagnostics.

Examples after building the Linux consumers:

```sh
python3 tools/physics_measurement/run.py \
  sandbox/milestone_12/goal_1/bin/linux/car_measurement \
  sandbox/milestone_12/goal_1/runs/car_profile \
  --profile --arguments --benchmark=headless

python3 tools/physics_measurement/run.py \
  sandbox/milestone_12/goal_1/bin/linux/pit_measurement \
  sandbox/milestone_12/goal_1/runs/pit_profile \
  --profile --arguments --headless
```

Use a **new output directory** for every invocation; the runner refuses to
overwrite evidence. Omit `--profile` for the timer-disabled comparison. The runner
executes serially from a temporary directory outside the source tree, forwards
terminal interrupts to the active process, and retains exit codes even when a
numerical or performance verdict fails. It never silently substitutes a shorter
endpoint. `--repetitions=3` performs three serial runs. `--controlled` records an
operator confirmation that AC power, streaming closure and build isolation are
in place; it does not independently prove those conditions.

For normal rendered runs, use `--rendered` on the runner and
`--arguments --benchmark=rendered` on the corresponding unwrapped playback
executable. The pit starts immediately
and exits after the endpoint; Escape/window close interrupts it. Add `--uncapped`
only for a separate throughput run. The default pit remains paused and
interactive. Car `--debug` and `--slow-motion` remain separate diagnostics.

## Comparison report and completion

`results.json` uses `schema_version: 1`. Each record retains the command, binary
hash, mode, operator confirmation, AC observation, exit code, elapsed time,
endpoint records, numerical pass/fail counts, original summary lines, timing
partition and trajectory/work hash. `run_<n>.csv` records each update's wall
nanoseconds, state hash, finite/fatal flags, attempted/completed subdivisions,
candidate/contact occurrences, retained diagnostics, observed duration and
observer count, and report-loss flag. Keep raw logs alongside summaries; summaries never replace
worst-case fixture records.

For each later version, report:

1. Source/SDK/consumer provenance, exact workload and runtime configuration.
2. Declared and reached endpoint, observed simulation duration, incomplete
   reason, numerical verdict and worst measurements across the entire run.
3. Per-function calls and inclusive/exclusive nanoseconds; stage totals from
   exclusive values only; subdivisions, actual iterations, candidates/contacts.
4. Three serial normal rendered trials per workload/version: frame-work p95
   excluding pacing, displayed FPS, elapsed/simulated ratio, and final/maximum
   schedule lag. Report every trial, not just the best or their average.
5. Separate uncapped throughput/headroom, instrumentation overhead, debug and
   slow-motion results. Identify headless or short results as diagnostic.
6. Milestone 6 goals 1–5 full-run error comparisons, relevant regressions,
   sanitizer results, Linux execution, Windows cross-builds, installed-SDK
   examples and visual review status.

Both final workloads require normal 1280×720 playback, sustained 60 FPS, p95
frame work ≤16.67 ms excluding pacing, simulated time within 1% of elapsed time
and no accumulating backlog. Observe the lag series/endpoint and retain all
trials; a single process's `realtime_pass` flag is insufficient to establish
these combined gates. Measurements must use AC power, streaming closed and no
concurrent builds. Headless throughput is never rendered-FPS acceptance.

Finite numerical failures continue to the declared endpoint. Invalid/nonfinite
state, fatal errors, inability to advance time and interruption are incomplete,
even if a process prints an endpoint. Check trace validity and observer/report
counts alongside endpoint status. Keep original tolerances and their failures
visible; only Goal 11 may adopt user-approved final tolerances.

## Goal 1 evidence status

Goal 1 is complete for review: reference preservation, diagnostic accounting
and fixture verification are established, with the user-directed incomplete car
baseline retained as an explicit exception. The machine-readable inventory is
`sandbox/milestone_12/goal_1/baseline_summary.json`; raw logs, per-update traces,
commands and endpoint records are in its `runs/` directory. All measurements
below use the frozen engine SDK with the unchanged settings above. No physics
implementation, public API, material or geometry changed.

The user explicitly stopped the full car diagnostic after 925 displayed-frame
updates: **15.416667471 of 60 simulated seconds**, taking 1,872.008 seconds of
application wall time. It remains **incomplete**, with partial physics CPU p95
7,349.162 ms and maximum 41,047.311 ms. Those are not full-run bounds. This run
started before the controlled-condition confirmation and included two debugger
inspections; it is diagnostic evidence only. Its executable and source snapshot
are retained in `initial_consumer_bin/` and `initial_consumer_source/`. The
60-second car baseline and final three-run rendered comparison remain outstanding;
this explicit user-directed stop does not redefine their endpoints.

The subsequent 600-frame car profile covers only its first 10 simulated seconds.
Every state/work/duration trace matches the corresponding prefix of the stopped
run. The pit timer-disabled/profiled pair both finish 720 frames / 12 simulated
seconds with identical state/work/duration traces. Exclusive timing conservation,
observer subdivision counts and source-attributed candidate/contact totals agree
exactly, with no lost reports or nonfinite states.

| Exclusive stage | Car: first 10 simulated seconds (ms) | Pit: full 12 simulated seconds (ms) |
| --- | ---: | ---: |
| Force preparation | 1,521.551 | 514.149 |
| Integration | 2,598.612 | 880.919 |
| Subdivision sweeps | 17,873.893 | 12,685.445 |
| Contact gathering | 46,652.143 | 19,893.047 |
| Joint gathering | 224.547 | 78.523 |
| Rigid solving | 32,740.719 | 47,368.204 |
| Joint solving | 6,549.363 | 730.248 |
| Soft solving | 2,072.236 | 0 |
| Finalization | 977.366 | 2,706.514 |
| Reporting | 727.277 | 295.071 |
| Orchestration | 1,461.207 | 733.659 |
| Fixture observer validation | 0 | 0 |
| Total | 113,398.913 | 85,885.779 |

Per-function call counts remain in the JSON and raw logs. The car segment executes
37,113 subdivisions, 296,904 solver iterations, 18,340,652 candidate occurrences
and 1,659,222 contact occurrences. The full pit executes 13,091 subdivisions,
104,728 iterations, 15,508,376 candidates and 746,387 contacts. The pit's outer
physics time increases from 81.489 to 85.886 seconds with timers (5.4% observed
overhead in this serial pair, including ordinary run variability). The stopped
car and later profile are not a controlled overhead pair.

Both full pit diagnostics report floor-circle penetration 0.002340078 and zero
side escapes. Timer-disabled physics CPU p95 is 163.218 ms; profiled p95 is
174.836 ms. Headless results do not establish rendered performance.

All **219 timed scenarios plus four state-based particle calibrations** finish:
20 extreme mass drop, 120 thin barrier, four persistent stack, eight pin, eight
weld, 11 spring and 48 soft-body timed scenarios. Timer-disabled/profiled runs
match numerical records and every update's state/work/duration trace. All original
numerical checks pass; CPU comparisons remain separate. Selected full-run worst
errors across the original rates/configurations are below; retain the raw
per-scenario records for the complete comparison.

| Fixture | Selected worst measurements |
| --- | --- |
| Extreme mass drop | Energy ratio 0.999999636; horizontal correction 0.000244141; settling support gap 0.00000223331 |
| Thin barriers | Depth 0.0000100024; energy error 0.0000000858057 |
| Persistent stack | Depth 0.00108020604 |
| Pin | Anchor error 0.0000197122; depth 0.00135425663; swept depth 0.00197294993 |
| Weld | Anchor error 0.00440671286; angle error 0; depth 0.0010425307 |
| Spring | Depth 0.00313801879; energy error 0.0000613170; reference position/velocity deviations 5.13510711 / 31.62185 |
| Soft body | Depth 0.00300585644; energy error 0.00136403434 |

The soft-body delayed-update maximum is **376.743 ms versus the original 50 ms
budget**, still a failed comparison. Its maximum scenario p95 across all update
rates is 1.073 ms. Numerical passes do not erase CPU-budget failures or complete
Milestone 6.

Independent installed-SDK consumers compile without warnings on Linux Release,
Linux ASan/UBSan and Windows cross-builds. All ten relevant tests pass on Linux
and under ASan/UBSan with leak detection: eight existing regressions, nested
exclusive-clock accounting and endpoint/failure reporting. Sanitizer car and pit
smokes also pass. Native Windows execution is not performed. Motion/window-switch
acceptance, debug-drawing cost and intentional slow-motion measurements remain
separate future verification, not inferred from these results.

Two batches of three serial unwrapped 1280×720 rendered pit trials were launched
after the user confirmed AC power and streaming closure, with no concurrent builds
or benchmarks. All six windows closed before their endpoints; the user confirmed
closing the retry windows, then explicitly directed that further trials stop and
the existing information be used. Raw `window_closed` records remain under
`runs/pit_rendered/` and `runs/pit_rendered_complete/`. The latter directory was
named before execution and **does not denote completed runs**.

These are **incomplete runs with observed performance below the real-time target**.
Partial frame-work p95 ranges from 29.505 to 209.506 ms against 16.67 ms, and partial
FPS ranges from 12.826 to 34.703 against 60. These values are not full-run bounds;
no successful rendered trial or complete rendered numerical assessment is claimed.

| Batch / trial | Simulated seconds of 12 | Active wall seconds | Partial frame-work p95 (ms) | Partial FPS |
| --- | ---: | ---: | ---: | ---: |
| Initial 1 | 4.400 | 20.583 | 197.720 | 12.826 |
| Initial 2 | 3.917 | 16.847 | 209.506 | 13.949 |
| Initial 3 | 1.950 | 3.371 | 29.505 | 34.703 |
| Retry 1 | 3.100 | 7.408 | 96.090 | 25.106 |
| Retry 2 | 3.133 | 7.874 | 99.635 | 23.877 |
| Retry 3 | 3.283 | 9.891 | 169.411 | 19.916 |

Per the user's stop instruction, no further rendered trials or uncapped run were
launched. Uninterrupted three-run rendered baselines, separate uncapped throughput
and full-car errors remain unmeasured in this goal. Preserving interrupted evidence
does not waive the combined real-time gates or constitute visual acceptance. No
tuning or accuracy tradeoff is selected by Goal 1.
