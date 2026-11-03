# Integration and subdivision rework

Milestone 12 Goal 3 uses the frozen `physics-pipelines-overoptimized` reference
at `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; `main` is excluded. The
[measurement contract](physics_measurement_contract.md) remains authoritative.
Executables, commands, logs and comparison scripts are preserved separately in
`sandbox/milestone_12/goal_3/`. Goal 2's committed implementation is also retained
as the immediate parent for timing attribution.

## Integration and subdivision findings

A tick is not a completed integration subdivision. Each `physics_update(ticks)`
executes every requested fixed tick. Each tick is divided into the configured
substeps. For each configured part, adaptive selection may shorten the next
complete world pass; the remainder is then processed until exhausted. Nothing
limits the number of passes or drops elapsed time. A nonpositive/unrepresentable
advance reports a fatal error rather than claiming completion. Explicit
`physics_pipeline_substep(dt)` and standalone integration retain their caller's
duration; they do not invoke adaptive selection.

The complete substep prepares force acceleration and external loads before
selecting a duration. Selection runs joint limits, soft elastic limits, rigid
sweeps, then soft sweeps. A small duration from any interaction applies to the
whole world: membership and hitbox caches, force preparation, integration,
contact/joint gathering, all solver iterations and finalization run again.
Caches do not survive arbitrary public edits between substeps. Local time steps
would need to account for shared endpoints, new contacts, equal-time collisions
and interaction publication; simply integrating only the limiting pair would
change those contracts.

| Work | Existing behavior | Goal 3 decision |
| --- | --- | --- |
| Rigid integration | Semi-implicit velocity then position; angular acceleration, angular limits, orientation and locks retain their order and checked commits | Retain: 2.3% of frozen car time and 1.0% of pit time covers all integration work, so merging passes offers limited benefit and changes failure/report ordering |
| Joint integration | Isolated eligible pin/weld assemblies use constrained motion; center-attached isolated springs use an implicit midpoint solve; connected/locked bodies retain fallback integration | Retain endpoint generation checks, force ownership, atomic pair updates and manual-stage exclusion |
| Joint duration | At most 1/120 s with valid joints, ten degrees of angular travel, and 0.1/spring frequency | Retain and measure reductions; do not remove high-spin protection |
| Soft integration | Elastic nodes use half-acceleration position and a second spring-force evaluation for velocity; other nodes retain ordinary integration | Retain: the second evaluation uses the new positions, so removing it changes energy behavior |
| Soft elastic duration | 0.2 divided by the square root of the maximum accumulated node stiffness/mass rate | Retain: the car already needs smaller steps than its nominal tick even without contact |
| Rigid sweeps | Relative circle/quadratic translating bounds, polygon axes and a 1/60 s collision-update ceiling | Retain geometric and accelerating turnaround safeguards |
| Soft sweeps | Swept bounds, local beam intervals, 0.005-feature closing margin and a conservative side-plane bound | Reduce representation/projection work; retain every duration formula and margin |

The frozen ten-second car segment used 37,113 complete subdivisions versus a
configured minimum of 9,600. The twelve-second pit used 13,091 versus 11,520.
The pit therefore has much less extra adaptive work: gathering/solving cost
remains the larger target for later goals.

Goal 3 instrumentation counts strict positive duration reductions separately for
each selector and records the smallest returned duration and mean returned/input
fraction among reductions. These are sequential selector events, not unique
collisions or additive counts of final limiting steps. The immediate-parent car
segment reports 18,747 rigid-sweep reductions (minimum 2.776536 microseconds),
11,066 soft-sweep reductions (minimum 0.328336 microseconds), and 13,077 elastic
reductions (minimum 803.705588 microseconds). Joint limits do not shorten this
segment. No timer or diagnostic is added to the engine/public API.

## Selected implementation

Soft sweeps previously embedded a complete `Shape` in each temporary record and
copied that record for local beam intervals. This copied storage for 100 vertices
and concave decomposition even though beam geometry has four corners and the
sweep uses only the convex vertex bounds. The private record now keeps its four
beam corners by value and borrows rigid geometry from the current world-hitbox
record. The borrow ends with the synchronous sweep call; it is never retained
across integration, topology edits, destruction or slot reuse. No allocation,
persistent geometry cache, public structure or ownership change is introduced.

Geometry projection is separated from motion projection. Each axis projects
geometry once; current and duration-expanded velocity intervals use that same
geometry. Endpoint arithmetic, iteration order, axis selection, float rounding,
side-plane bisection, collision margins and adaptive duration formulas remain
unchanged. Standalone sweep calls still refresh hitboxes from the current pose.

## Verification and configuration comparisons

Goal 3 is complete for review. No real-time acceptance or final tolerance change
is claimed. Fourteen independent installed-SDK regressions pass on Linux Release
and fourteen pass under ASan/UBSan with leak detection. The beam-motion test now
also samples three-vertex and maximum-size (100-vertex) rigid geometry, reversed
beam endpoints and accelerated/rotated paths. Sanitized car and pit smokes pass.
Linux, sanitizer and Windows consumers build without compiler warnings; native
Windows execution was not performed.

All 219 timed Milestone 6 scenarios plus four calibrations finish. Every numerical
record and per-update state/work/duration trace matches the frozen reference.
The default ten-second car segment (600 frames) and complete twelve-second pit
(720 frames) also match exactly, including all 37,113 and 13,091 subdivisions.
Completed-time observers, report counts, candidate/contact attribution and
exclusive timing conservation agree. No fatal/nonfinite state or report loss
occurs in these completed runs. Car segments remain incomplete against the full
60-second workload; they are not full-car worst-case bounds.

Soft-body worst penetration remains 0.00300585644 and worst energy error remains
0.00136403434. Its longest delayed update is **372.958 ms versus the 50 ms
comparison budget**, still a failure; maximum scenario p95 is 1.047 ms. The default
pit retains floor penetration 0.002340078 and zero escapes. No existing numerical
tolerance or CPU failure is reclassified.

### Cost of the selected change

The immediate-parent and current car runs use the same Goal 3 instrumentation
and unchanged settings. Both report AC disconnected, so these are diagnostic
battery measurements, not the contract's controlled final AC-power trials.
There were no concurrent builds. Historical frozen-reference timings retain
their original conditions and must not be treated as a controlled timing pair
with these runs.

| Ten-second car diagnostic | Immediate parent (ms) | Current (ms) |
| --- | ---: | ---: |
| Soft sweep selection | 5,895.947 | 4,739.727 |
| Complete physics | 110,682.450 | 108,906.831 |
| Per-update physics wall p95 | 375.352 | 374.752 |

The sweep stage falls 19.6% in this pair; complete physics is only 1.6% lower,
which is a single-run observation subject to timing variability. The pit has no
soft beams and is not expected to benefit from this code change. Its current
complete physics time is 85,064.003 ms and wall p95 is 167.507 ms; this does not
establish a pit speedup or rendered acceptance.

An unwrapped sweep microbenchmark isolates the changed work: eight beams,
32 particles, fixed poses/motion, 64 warmups and 2,000 calls per case. Three serial
frozen-reference/current pairs produce identical duration sums. Median of the
three per-call means:

| Rigid vertices | Frozen reference (microseconds) | Current (microseconds) | Reduction |
| --- | ---: | ---: | ---: |
| 16 | 164.394 | 129.193 | 21.4% |
| 100 | 1,897.148 | 1,195.458 | 37.0% |

These are kernel diagnostics, not replacement workloads. They support keeping
the smaller temporary representation and single geometry projection without
claiming an equivalent whole-frame gain.

### Rejected setting alternatives

The completed alternatives use eight solver iterations throughout. Both frozen
and current engines finish each declared diagnostic segment, with identical
per-update state/work/duration traces at each matching configuration. Their
trajectories differ from the default configuration. Current-engine results:

| Workload | Tick Hz / substeps | Subdivisions | Physics wall (ms) | Wall p95 (ms) | Floor penetration |
| --- | --- | ---: | ---: | ---: | ---: |
| Car, first 10 s | 240 / 4, retained | 37,113 | 108,906.831 | 374.752 | Not independently measured |
| Car, first 10 s | 240 / 1, rejected | 29,292 | 88,719.696 | 306.608 | Not independently measured |
| Car, first 10 s | 120 / 1, rejected | 23,928 | 75,368.656 | 226.761 | Not independently measured |
| Pit, full 12 s | 240 / 4, retained | 13,091 | 85,064.003 | 167.507 | 0.002340078 |
| Pit, full 12 s | 240 / 1, rejected | 4,927 | 43,906.581 | 131.755 | 0.023434162 |
| Pit, full 12 s | 120 / 1, rejected | 3,701 | 31,412.732 | 121.485 | 0.068877459 |

No pit escape occurs in these completed alternatives, but floor penetration
increases about tenfold and 29-fold. Their finite-state/escape verdicts do not
mean unchanged accuracy. These alternatives do not satisfy the rendered contract
and are not selected.

A further frozen-reference car exploration at 60 Hz / one substep was abandoned
following the user's decision to retain the original integration settings. It
became much slower near 9.4 simulated seconds, including a completed frame with
2,184 subdivisions and 13.080 seconds of physics work. The runner forwarded an
interrupt, but the child did not return from its current physics update and was
then forcibly terminated. The retained trace ends at 565 frames / 9.416667 s;
wall time was 345.303 s. **This is incomplete**, with no final stage-accounting
record and no full-run worst-case bound. The paired current-car trial and the
60 Hz pit trials were not run. `interruption.json` records the decision and
shutdown; the original script remains `verify_initial.py`.

`summary.json` records all completed comparisons and microbenchmark repetitions;
`audit.json` verifies the 1,457 frozen artifact hashes and unchanged archive
membership. Against the frozen reference, only the previously committed
joint-solver object and this goal's beam-motion object differ on all three build
variants. No public header changed. After the user's review and commit, Goal 4
is the next ordered work; further integration accuracy/performance tradeoffs
remain unselected.

### Experimental configuration mechanics

Settings experiments keep scene geometry, counts, materials and
solver iterations unchanged. They use independent diagnostic consumers with an
explicit wrapper that records the effective tick rate and substeps. The wrapper
also adjusts ticks per frame to preserve the original simulated frame duration;
changing tick duration alone would otherwise alter simulation speed. The
fixture's historical hard-coded settings line is superseded by the explicit
`experiment_configuration` record in these experimental logs only.

The user explicitly prefers retaining the existing integration passes and
accuracy safeguards over a small speed improvement from fewer passes. Lower-rate
and lower-substep runs remain experiments only: the shipped defaults and both
workload settings stay at 240 Hz, four substeps and eight solver iterations.
There is no tolerance relaxation or acceptance decision in this goal.

## Clock ownership follow-up

Physics integration does not advance the engine clock or expire lifetime-owned
sources. Existing shared-source regressions verify this separation, and Goal 3
preserves it. Code inspection found that the car's benchmark branch also omits
`engine_update`, while an accepted jump creates a tick-lifetime force through
`physics_force_apply`. Consequently, if that jump is accepted, its source will
not expire through physics updates alone. This is an existing benchmark-path
concern, not a changed integration behavior or a measured explanation for the
baseline slowdown. The unchanged benchmark remains useful for paired engine
comparisons, but its jump/lifecycle handling needs review before Goal 14's final
motion and playback acceptance. Preserve the existing traces when reconciling
that fixture; do not silently replace its historical evidence.
