# Milestone 12 Goal 11 — Shared sweep preparation and traversal

Goal 11 is complete for review against parent `43f49903`. The frozen reference
remains `physics-pipelines-overoptimized` at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; exclude `main`.
Sources, prototypes, SDKs, executables, logs, traces and reproduction commands
are preserved under `sandbox/milestone_12/goal_11/`. Goals 12–17 have not started.

## Scope and conditions

All new workload candidates use **60 Hz, four configured substeps and eight
solver passes**, retaining particle–particle sweep protection and all other
adaptive limits. Production defaults, solver correction, public APIs, scene
geometry, counts and materials are unchanged. The pit contains 800 diameter-5
particles with restitution 0.5 and reaches twelve simulated seconds. The car
retains its 200 standalone particles, 100 soft nodes, authored scene and input
sequence; only the authorized 600-update, approximately ten-second prefix is
run. The full sixty-second car remains incomplete.

These measurements use AC power, serial runs and no concurrent builds, with
**streaming active as authorized by the user**. They are headless diagnostics,
not controlled rendered acceptance. Earlier controlled measurements and the
user-stopped full car/rendered pit evidence remain intact. Timing variation is
reported rather than attributed entirely to this change.

The pit particle-pair review limit remains 0.5 units; the Milestone 6 extreme
fixture penetration limit remains 0.1. Neither is a solver input. No new hard
pit-wall or car deformation threshold is introduced. Fixed historical stress
fixtures retain their 30/60/120 Hz coverage, independently of the limit on new
runtime candidates.

## Selected implementation

Rigid sweeps prepare a compact collider list and each current swept AABB once,
then use that same bound for insertion/refitting and querying. This removes
the third alive-entity scan and duplicate bounds calculation. The initial
moving/filter eligibility scan remains. Current-pose geometry and future swept
bounds retain their separate meanings.

A dedicated rigid swept tree retains **topology only** between calls. Every
call revalidates current collider membership using generational entity handles
and prepares all current bounds before queries. Changed leaves refit ancestors;
identical bounds skip refitting. Membership changes rebuild. A sum of internal
node spans, normalized by the root span, detects deteriorating topology; growth
above 1.5 times the last-build value triggers a rebuild. This heuristic affects
cost, not collision coverage. Nonfinite bounds retain the original insertion
filtering and disable retention for the following call. The normal geometry
validation and error reporting remain in force.

Contact gathering keeps ownership of its existing tree. The swept tree uses
fixed private storage for at most `MAX_ENTITIES` leaves and `2*MAX_ENTITIES`
nodes/stack entries; a full binary tree uses at most `2N-1` nodes. Its bounded
path does not allocate or retain entity-pool pointers. Engine restart may leave
numeric topology in static storage, but no query can use it before current
membership and bounds have been prepared again.

Soft sweeps retain their linear candidate scan. They reuse each beam's prepared
corners within the same synchronous selector call, and reuse prepared rigid
geometry instead of repeating hitbox and angular-radius work for each pair.
The broad bound's angular allowance includes acceleration; the pair check's
existing instantaneous angular allowance is stored separately. An entity with
both beam and rigid components retains the original rigid fallback. Borrowed
shape pointers are guarded by validity cleared before each later invocation.

These changes add approximately 2.08 MB (1.98 MiB) of bounded static arrays at
`MAX_ENTITIES=10000`. The tree remains part of the engine's existing synchronous,
global-state architecture; this does not introduce parallel response, atomics
or a public cache API.

| Input change | Validity rule before querying |
| --- | --- |
| Pose, integration or position correction | Refresh world geometry through existing pose checks and recompute swept bounds |
| Velocity, acceleration, locks or proposed duration | Recompute current swept bounds; pair formulas read current state |
| Shape, particle radius, local origin or inertia | Existing stage-entry geometry invalidation remains; no new cross-call shape/parameter cache |
| Collision filters | Current eligibility and pair filtering execute on every call |
| Collider membership, deletion or slot reuse | Compare current generational handles and rebuild on mismatch |
| Tree quality deterioration | Rebuild when normalized internal span exceeds the recorded threshold |
| Soft-node/beam motion or geometry | Prepare again on every soft selector call; reuse only within that call |

Earliest-contact formulas, pair protection, configured steps, adaptive limits,
solver ordering and elapsed-time consumption are unchanged. Tree traversal can
visit pairs in a different order, but each pair independently reduces the same
proposed duration; it does not change entity state during the query.

## Alternatives measured

Five isolated candidates were built and run through the complete pit and car
prefix, with exact state/work/duration parity in all twelve parent/candidate
pilot workloads. None changes accuracy settings.

| Candidate | Decision and evidence |
| --- | --- |
| 1: prepared rigid list/bounds; rebuild a separate soft swept tree each call | Reject soft tree. Car soft candidate visits fall 796,983,600 → 14,955,490, but 9,962,295 insertions and 230,109,514 tree-node visits offset filtering savings. The source probe soft total rises about 5,047 → 5,525 ms. |
| 2: candidate 1 plus retained rigid topology | Retain the rigid-tree idea; it eliminates repeated insertion while refitting all current bounds. The soft-tree cost remains. |
| 3: retained rigid topology plus per-call soft geometry reuse, original soft linear scan | Selected. Removes repeated preparation while retaining simpler soft discovery. |
| 4: candidate 3 plus eager per-body motion/center/radius preparation | Reject extra cache. In the pit probe, pair work saves about 195 ms but motion preparation costs about 197 ms, before other maintenance. |
| 5: candidate 4 plus retained soft-tree topology | Reject soft tree. Its car source probe costs about 5,342 ms versus candidate 3's 4,670 ms; refits, quality checks and traversal offset fewer candidate visits. A slightly faster single ordinary profile is insufficient evidence to adopt it. |

These source probes have observer overhead and ran with streaming active.
Their timings explain mechanisms; they are not clean end-to-end speedup claims.
The final typed probes below use a common instrumentation schema.

The earlier ordinary pit profiles showed 45,258.638 → 39,713.121 ms total
wall time (12.25%), with rigid sweeps 5,644.430 → 4,358.191 ms (22.79%).
The measured rigid saving explains only about 1,286 ms of the 5,546 ms total
difference. The unexplained remainder is **not** credited to the algorithm.
Final unwrapped repetitions determine the supported whole-workload result.

## Final timing and work comparison

Three serial, alternating parent/candidate repetitions per workload use
unwrapped consumers. Values below are process physics CPU milliseconds;
p95 is the median of the three per-run p95 values. The car row covers only
the authorized prefix.

| Workload | Parent total ms | Selected total ms | Median reduction | Parent → selected p95 ms | Selected ms / simulated second |
| --- | ---: | ---: | ---: | ---: | ---: |
| pit | 45559.059 | 44846.511 | 1.56% | 111.076 → 119.032 | 3737.209 |
| car | 89756.618 | 83498.726 | 6.97% | 270.779 → 257.262 | 8349.872 |

All trial totals, in execution-pair order:

- pit, parent: 43535.510, 47603.353, 45559.059 ms.
- pit, final: 44846.511, 43634.545, 46340.445 ms.
- pit, paired reductions: -3.01%, 8.34%, -1.72%; negative means slower.
- car, parent: 89756.618, 90625.711, 84717.074 ms.
- car, final: 83498.726, 80652.216, 90137.774 ms.
- car, paired reductions: 6.97%, 11.01%, -6.40%; negative means slower.

The pit median gain is small compared with run-to-run variation; two of
three paired pit comparisons are slower and median p95 is higher. One of
three car pairs is also slower. Do not claim the earlier 12.25%
instrumented whole-pit reduction as established. Streaming can change clocks,
cache contention and scheduling. These three-run estimates are diagnostic,
not statistically established gains or rendered-FPS acceptance. Physics-only
work per simulated second still greatly exceeds the real-time budget of
1,000 ms, before rendering.

The following common-schema source probes separate shared sweep work from
pair geometry. Values are **exclusive wall milliseconds over the full pit /
car prefix**. Timers, counters and their perturbation are included. Particle
classification uses `ROHR_PARTICLE`, including soft nodes. Pair-type geometry
timers begin after eligibility filtering. A selector total includes each child
once; never add it again to the rows above.

| Rigid selector work | Pit parent | Pit selected | Car parent | Car selected |
| --- | ---: | ---: | ---: | ---: |
| Bounds preparation | 1031.349 | 521.311 | 1665.417 | 837.538 |
| Tree insertion | 845.554 | 0.238 | 1272.686 | 0.077 |
| Tree refitting | 0.000 | 299.798 | 0.000 | 339.091 |
| Tree quality check | 0.000 | 11.894 | 0.000 | 21.190 |
| Tree membership/maintenance remainder | 0.000 | 186.515 | 0.000 | 249.823 |
| Tree traversal excluding callbacks | 3660.147 | 3879.069 | 4757.120 | 4525.425 |
| Pair eligibility/filtering | 811.468 | 805.320 | 776.675 | 773.209 |
| Particle–particle geometry | 563.337 | 551.515 | 309.967 | 306.459 |
| Particle–rigid geometry | 455.607 | 451.288 | 872.288 | 894.036 |
| Rigid–rigid geometry | 0.000 | 0.000 | 92.722 | 92.836 |
| Other selector work | 1506.982 | 1260.765 | 2588.298 | 2053.123 |
| Rigid selector total | 8874.444 | 7967.712 | 12335.173 | 10092.806 |

| Soft selector work, car prefix | Parent ms | Selected ms |
| --- | ---: | ---: |
| Bounds calculation | 424.640 | 437.085 |
| Beam geometry construction | 230.173 | 122.282 |
| Beam–rigid geometry | 594.811 | 588.627 |
| Beam–beam geometry | 0.000 | 0.000 |
| Empty-target pair path | 0.000 | 0.000 |
| Pair dispatch remainder | 58.860 | 58.060 |
| Preparation, candidate scanning/filtering and remainder | 3838.004 | 3803.859 |
| Soft selector total | 5146.489 | 5009.913 |

The soft remainder includes hitbox preparation outside the bounds helper,
linear candidate filtering and outer-loop work; it is not all narrowphase
geometry. Timings of unchanged formulas may vary between runs. Source-probe
percentages are not added to clean timing gains.

In these final probes, rigid sweeps improve 10.22% in the pit
and 18.18% in the car; car soft sweeps improve 2.65%.
The selected reuse removes measured preparation work with unchanged tested
physics. Its bounded storage and maintenance costs are included. Retaining
this local improvement does not establish a consistent whole-workload gain
or resolve the larger repetition bottlenecks.

| Structural work | Pit parent → selected | Car parent → selected |
| --- | ---: | ---: |
| Rigid bounds calculations | 7,912,762 → 3,956,381 | 15,889,230 → 7,944,615 |
| Rigid entity-scan visits | 11,869,143 → 7,912,762 | 55,990,620 → 37,327,080 |
| Tree insertions | 3,956,381 → 803 | 7,944,615 → 315 |
| Tree nodes visited | 409,191,841 → 433,612,011 | 627,321,979 → 586,589,919 |
| Leaf callbacks | 15,833,891 → 15,833,891 | 20,233,553 → 20,233,553 |
| Soft beam geometry calculations | 0 → 0 | 4,035,360 → 2,017,680 |
| Soft candidate visits | 0 → 0 | 796,983,600 → 796,983,600 |

Retained rigid-tree requests, hits, refits and invalidation reasons:

| Workload | Requests / reused | Changed / unchanged leaves | Membership / quality / invalid-bound rebuild causes |
| --- | ---: | ---: | ---: |
| pit | 4,927 / 4,926 | 3,940,513 / 15,065 | 1 / 0 / 0 |
| car | 25,221 / 25,220 | 5,866,052 / 2,078,248 | 1 / 0 / 0 |

Each workload builds rigid topology once. The pit retained topology causes
more tree-node visits than rebuilding, while the car causes fewer; therefore
fewer insertions do not imply cheaper traversal. Separate regression coverage
exercises membership changes and the quality fallback. No invalid-bound
fallback occurs in these valid workloads.

Both versions execute exactly 4,927 pit steps and 25,221 car-prefix steps.
Rigid reductions remain 2,047 / 14,245, with minimum returned durations
71.0691824922 / 4.12482851652 microseconds. Car soft reductions remain 10,080,
with minimum 0.239406176335 microseconds. These sequential selector counts
must not be summed as distinct substeps.

## Behavior and verification

Independent post-substep geometry/behavior checks match the retained parent
oracle exactly (excluding validation CPU time):

| Measurement | Pit, full 12 s | Car, authorized prefix |
| --- | ---: | ---: |
| Worst standalone particle-pair depth | 0.019971246 | 0.041826790 |
| Worst soft-node pair depth | 0.000000000 | 0.015560134 |
| Final pair depth | 0.016193570 | 0.000748313 |
| Samples over 0.5 | 0 | 0 |
| Final RMS speed | 0.403878340 | 109.897861032 |
| Maximum beam absolute error | 0.000000000 | 13.419484546 |
| Maximum beam relative error | 0.000000000 | 1.472364484 |

The car beam errors remain visible; matching the parent is not a claim of
acceptable deformation or full-run behavior. Pit floor penetration remains
0.023434162 units with zero side-escape samples. No new accuracy loss or
penetration allowance is selected.

All 23 Linux and 23 ASan/UBSan regression tests pass. The new
`sweep_preparation` test covers repeated rigid and soft selector calls, changing
duration, velocity and acceleration, teleporting geometry, radius and filter
edits, membership changes, generation reuse, tree degradation/rebuilding, and
engine teardown/restart. The instrumented rearrangement diagnostic exercises
three quality-triggered rebuilds. Existing manual-stage/reporting, high-speed
particle, rotating rigid, beam and soft-body tests also pass.

All 219 timed Milestone 6 scenarios and four particle calibrations finish on
both Linux and ASan/UBSan, with exact parent state/work traces and passing
numerical criteria. The exact-source parent fixtures are preserved in Goal 8;
Goals 9–10 changed no production physics. A fresh comparison also matches all
220,994 stress-fixture updates against the frozen reference, excluding timing
and the previously documented gather-candidate-count change. The new 60 Hz
workloads compare directly with their same-configuration parent; historical
240 Hz reference workload results are not reinterpreted as a 60 Hz comparison.
No full reference car run was restarted.
The existing head-on regression retains two diameter-5 particles approaching
at 1,000 units/s each and checks the 0.5-unit bound at every completed substep.

The soft-body stress suite retains maximum penetration 0.00300585644 units
and maximum relative energy error 0.00136403434. The worst Linux
delayed update is 430.589 ms versus the historical 50 ms CPU
comparison budget. All observed violations of historical CPU limits, including sanitizer
timings, are listed in `final_analysis.json`; numerical PASS does not erase
them. Streaming-active timings are not directly comparable with earlier
controlled CPU figures. These failures do not complete Milestone 6 Goal 5.

Fresh engine and installed-SDK consumer builds pass on Linux, ASan/UBSan and
Windows cross-compilation, with no compiler warnings in the final build logs.
The independent installed-SDK joints example builds for all three variants
and runs successfully on Linux and under sanitizers from `/tmp`. Sanitized
workload checks complete four seconds of pit simulation and 134 car updates,
including the pathological car update 133. No sanitizer or leak findings occur.
Native Windows execution was not performed.

The final exact trace comparisons include all 720 pit updates and all 600 car
prefix updates, selector reductions, consumed simulation time, candidates,
contacts and diagnostic counts. Exclusive pipeline and source-probe timers
conserve their respective roots exactly. Independent geometric checks are
additional evidence; matching trace hashes alone is not a collision oracle.

## Remaining bottlenecks and review boundary

This goal reduces repeated sweep preparation; it does not reduce the number of
adaptive steps or solver passes. The worst car update still takes 1,442 steps,
including the previously identified collapsed-beam restrictions. That
sweep/response mismatch remains assigned to Goal 14. Particle/polygon geometry
separation and shared gathering/filtering remain Goal 12. Neither starts here.

The final report does not establish 60 FPS, rendered frame-work p95 at or below
16.67 ms, full-car behavior, or final tolerance acceptance. Milestone 6 Goal 5
and superseded Milestone 11 retain their existing status. Goal 16 owns combined
rendered acceptance; Goal 17 owns the final accuracy/performance selection.

Evidence entry points: `reproduction.md`, `baseline.json`, `final_analysis.json`,
`runtime_results.json`, `source/`, `logs/`, `runs/`, final SDKs/executables and
the final evidence manifest under `sandbox/milestone_12/goal_11/`.
