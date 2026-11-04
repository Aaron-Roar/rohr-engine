# Milestone 12 Goal 6 — Rigid-contact solving

Goal 6 is complete and committed at `479d50a2`. The requested 120 Hz and 60 Hz trials
are preserved as experiments; production defaults remain unchanged. The
subsequent priority insertion and retained penetration limits are recorded
below. The frozen reference is
`physics-pipelines-overoptimized` at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; the immediate parent is Goal 5 commit
`6aba1548`. Exclude `main`. Scripts, SDKs, binaries, logs and traces are preserved
under ignored `sandbox/milestone_12/goal_6/`.

The subsequent [Goal 7 particle-response report](physics_particle_response.md)
replaces the misinterpreted solver-slack experiments as the optimization
baseline. It uses original correction settings, preserves the review limits,
and records the selected rework and repeated unwrapped measurements.

## Analysis and selected rework

The parent pit executes 113,088,580 rigid-contact solve calls. Its diagnostic
probe classifies 108,337,588 as circle pairs and 4,750,992 as mixed/polygon pairs.
Only 8,208 circle checks miss. Of the circle solves, 100,188,706 change linear
velocity and 18,019,538 change position. Repeated responses therefore cannot
simply be skipped; shared entities require the existing sequential ordering.

| Area | Finding and decision |
| --- | --- |
| Particle geometry | Hitbox validation already calculates the current circle center/radius. Reuse that exact result for scalar overlap and contact-point construction, and reuse its radius for penetration slop. |
| Contact offsets | Lazily cache world COM at each validated pose. Rebound and normal-response calculations reuse it while velocities and impulses remain live. Pose changes force refresh. |
| Particle response | Preserve effective mass, rotational response for ordinary bodies carrying particle properties, friction, restitution targets, impulse accumulation and position correction. No SIMD response batching or atomics are introduced. |
| Polygon response | The same COM reuse helps the iterative fallback. Preserve manifold ordering, point matching, retained impulses and pose-checked manifold caches. |
| Block solving | Preserve bounded reciprocal polygon sets, grouping, extended-precision QR, double Cholesky/QR fallbacks, geometric correction weights and convergence tests. Its top-level exclusive cost is tiny in these particle-heavy workloads; extreme mass-ratio/stack behavior remains covered by the existing fixtures. |
| Contact layout | A constraint occupies 1,352 bytes, including 1,348 bytes of rigid state and 92-byte contact records. A separate compact particle arena would require new append/rollback and mixed-order indirection. Retain ownership and list layout; reuse small per-body geometric values instead. |
| Iterations | Measure four/eight/twelve on the unchanged full pit. Retain eight: fewer iterations increase penetration, while twelve costs more. No convergence threshold or accepted tolerance is changed. |

Three bounded arrays add 210,000 bytes of static storage: circle geometry,
world centers and center-valid flags. They share the existing hitbox cache's
exact position/orientation checks and reset boundary: one synchronous full
substep or one manual solve invocation. Arbitrary radius, origin, COM and shape
edits between public calls cannot reuse the previous invocation's data. No
allocation, owned resource or public API is added. Invalid geometry keeps the
existing rejection path; no stale cached value is used after failed validation.

All integration passes, 240 Hz, four configured substeps, scene geometry, body
counts, materials and response order remain unchanged. The small SIMD circle
gathering path from Goal 4 remains separate from sequential response.

## Retained-contact lifetime

A focused installed-SDK reproduction on the parent confirms that a saved rigid
contact could operate on a replacement body after deletion and slot reuse.
Its stored entity handles were not checked against the cached indices before
iterative response or interaction publication. This could carry an old body's
contact state into another entity lifetime.

Validate both identities before response and finalization, using the existing
membership lookup during complete substeps and ordinary handle resolution for
manual calls. Stale contacts do not modify bodies, mutate retained contact data
or publish a replacement interaction. The block path already validates response
handles before applying its body changes; its iterative fallback uses the new
guard. This corrects stale manual usage without changing ordinary full-update
trajectories. No extra references or ownership transfers are introduced.

The regression covers deletion/reuse both with and without a membership snapshot,
and checks response and finalization separately. Other focused cases compare
cached/fresh geometry responses, current radius/origin/position edits and COM
edits without pose changes, including ordinary rotating bodies carrying particle
properties. Shared-body impulse order and retained rebound targets are compared
bit-for-bit across sixteen passes.

## Measurements and verification

All 219 timed fixture scenarios plus four calibrations finish. Final numerical
records and per-update state/work traces match the parent exactly. They match
the frozen reference under the existing comparison rules: exclude timing, and
exclude the candidate counts intentionally reduced by Goal 4. The car is a
600-update, ten-second prefix of its declared sixty-second scenario; the full
pit reaches twelve seconds. These are headless diagnostics, not rendered
acceptance, and no interrupted full-car baseline or rendered trial was restarted.

Seventeen Linux and seventeen ASan/UBSan regressions pass, including the new
cache and lifetime cases. Sanitized car/pit smokes pass. Engine and independent
installed-SDK consumers compile without compiler warnings for Linux, sanitizers
and Windows. The standalone joints example builds independently on all three,
and passes bounded Linux/sanitizer execution and teardown from a `/tmp` launch.
Windows execution remains unverified; this goal cross-builds it.

The unchanged soft-body numerical maxima are penetration 0.00300585644 and
relative energy error 0.00136403434. Its full-run delayed-update CPU maximum is
364.860 ms, still failing the existing 50 ms comparison criterion. No failure is
converted into an accepted tolerance. All fixture endpoints are reached with
valid accounting and no incomplete updates; completing this rework goal does
not complete Milestone 6's soft-body goal or the real-time gates.

### Iteration experiment

The geometry-reuse build before the lifetime guard is preserved separately under
`stage_1/`. It exercises the unchanged 800-particle fixture to twelve seconds,
changing only the configured solver iterations. All other settings and materials
remain unchanged. Individual diagnostic results are:

| Iterations | Physics CPU time (ms) | p95 update CPU (ms) | Worst floor penetration |
| --- | ---: | ---: | ---: |
| 4 | 59892.973 | 110.002 | 0.004838467 |
| 8 | 63865.696 | 127.447 | 0.002340078 |
| 12 | 73140.877 | 146.943 | 0.001905441 |

All three finish without escapes or invalid state. Four iterations approximately
double penetration; twelve reduce it by about 19% at increased cost. These are
individual runs, not a precise speed comparison. Retain eight to preserve the
reference trajectory and the user's accuracy preference. Final guarded-build
parity verifies that the lifetime correction does not alter this fixed-topology
fixture. No reduced integration passes, sleeping policy or response approximation
is selected.

### Timing contract and evidence

Final paired kernels and full-pit trials run serially on AC power, with no
concurrent builds. Three runs per version alternate parent/current order; report
medians separately from exclusive instrumented stage accounting. Stream closure
uses the user's existing confirmation; no streaming-state detector is added.
The initial final car profile overlapped an accidental duplicate regression
invocation and is excluded from timing claims. Its trace is preserved; an
isolated repeat supplies the reported car measurement after the pit batch.

The final guarded build's isolated kernel uses three coupled particles, 1,000
warmup rounds and 300,000 measured rounds of three sequential pair solves. Both
modes preserve identical parent/current final state hashes across all three runs.
These synthetic results isolate geometry reuse; they do not predict whole-scene
throughput.

| Kernel mode | Parent median ns/solve | Revised median ns/solve | Reduction |
| --- | ---: | ---: | ---: |
| Fixed geometry, changing velocities | 331.586 | 217.105 | 34.5% |
| Positions reset and corrected each round | 687.600 | 609.130 | 11.4% |

Three serial unwrapped pit runs per version produce these medians. Each reaches
twelve simulated seconds, with floor penetration 0.002340078, no escapes,
13,091 completed subdivisions, 15,498,784 candidates and 746,387 contacts.

| Full pit measure | Goal 5 parent | Revised | Change |
| --- | ---: | ---: | ---: |
| Physics CPU time | 75106.812 ms | 64148.778 ms | 14.6% lower |
| p95 physics update CPU | 157.413 ms | 127.415 ms | 19.1% lower |
| Active elapsed time | 75.387 s | 64.397 s | 14.6% lower |

The headless diagnostic remains far slower than real time. These CPU p95 values
are not rendered frame-work p95 or evidence of sustained 60 FPS.

Single instrumented runs against the preserved Goal 5 profiles show:

| Workload/stage, exclusive ms | Parent | Revised |
| --- | ---: | ---: |
| Car ten-second prefix: all physics | 111121.879 | 103596.066 |
| Car: contact wrapper, excluding nested work | 5290.014 | 5021.383 |
| Car: block solver, excluding nested work | 10.472 | 10.698 |
| Car: iterative pair solving | 27182.363 | 22593.878 |
| Pit twelve seconds: all physics | 82615.939 | 71332.028 |
| Pit: contact wrapper, excluding nested work | 3054.926 | 3058.143 |
| Pit: block solver, excluding nested work | 39.289 | 40.236 |
| Pit: iterative pair solving | 43802.616 | 32521.145 |

Every measured update's exclusive stage costs sum to its root physics time;
nested gathering/joint work is not counted again as rigid solving. Pair solve
calls remain 59,199,857 for the car and 113,088,580 for the pit. Full physics time
is 6.8% lower for this single car diagnostic and 13.7% lower for the profiled pit;
these single-run changes are supporting diagnostics, not repeated car acceptance.

### Particle-only quality experiments

At the user's request, additional sandbox experiments compare eight, four and
two particle-only response passes while preserving eight passes for other rigid
pairs. Selected particle passes are evenly distributed through the eight rounds,
and their position-correction fractions account for the smaller pass count.
Only nonrotating particle pairs qualify. The comparison also tests
0.5- and 1.0-unit particle-only correction tolerances; other contacts retain their
existing tolerances. Production settings remain unchanged pending review.

An independent sorted-axis oracle checks all possible circle pairs after every
completed substep, outside the solver's candidate list. Its measured CPU cost is
subtracted from the experimental physics CPU timer and reported separately;
experimental wall/stage times include it and do not establish exclusive stage
cost or rendered acceptance.
The fixture retains all original geometry, materials and body counts. Record
actual worst overlap, including impacts, rather than treating a configured
correction tolerance as a hard upper bound. Each configuration below is one complete twelve-second diagnostic on AC, run
serially with the same experimental instrumentation. The eight-pass control
reproduces the production subdivision/candidate/contact counts and floor depth.
All five finish without escapes or nonfinite state. Their existing fixture
`PASS` verdict covers its original floor/escape checks; it does not assert an
accepted particle-overlap tolerance or visual quality.

| Particle passes / allowance | Physics CPU ms | Change from control | Worst particle overlap | Worst floor overlap | Final RMS speed |
| --- | ---: | ---: | ---: | ---: | ---: |
| 8 / existing | 63564.910 | control | 0.002638 | 0.002340 | 0.1194 |
| 4 / existing | 67035.027 | 5.5% higher | 0.005054 | 0.004664 | 0.1753 |
| 2 / existing | 57319.989 | 9.8% lower | 0.014594 | 0.012118 | 0.3733 |
| 2 / 0.5 | 35088.489 | 44.8% lower | 0.501142 | 0.001958 | 0.3887 |
| 2 / 1.0 | 33730.787 | 46.9% lower | 1.000047 | 0.000687 | 0.3871 |

These are exploratory single runs with an instrumented control, separate from
the three-run production benchmark. The oracle costs 0.60–0.74 CPU seconds over
each full run and checks 11,625–13,091 completed substeps. Calls skipped by the
prototype still incur list traversal and wrapper dispatch; no claim is made
about an eventual production implementation's exact speed.

Two passes alone worsen floor penetration even though wall contacts retain all
eight passes: changing particle impulses changes the pile's load on the walls.
Four passes halve particle response calls but show no useful total-time saving
in this sample; their measured CPU time increases by 5.5%. The combined slack/pass reduction saves substantially more:
with 0.5 allowance, completed subdivisions drop to 11,625, candidate work to
13,861,363 and executed particle solves to 24,029,850, from the control's
108,337,588. Integration and gathering still cost about 23.7 seconds inclusive,
so even this candidate remains slower than real time.

The final pile height decreases from 84.4705 to 80.1201 units with 0.5 allowance
(about 5%); final mean height decreases from 40.7539 to 37.4602. Residual RMS
speed increases by about 3.3 times. The 1.0 allowance saves only another 3.9%
CPU time relative to 0.5, with final height 80.1068 and mean height 37.3351.
Both allowances are exceeded slightly during the run, so neither demonstrates
a strict hard maximum equal to its configured value.

The user initially considered this candidate, then clarified that "0.5 slack"
meant the maximum penetration before review flags a violation, not solver
correction slack. The agent misinterpreted that request. Retain these runs as
historical experiments, not approved runtime configurations. Production settings
were never changed; the original eight-pass trajectory remains the verified
change. The two-particle-pass idea may still be evaluated with original solver
correction settings. Rebaseline subsequent optimizations accordingly. Final
overall tolerance acceptance remains the user's Goal 17 decision (originally
Goal 11); the provisional review thresholds below are explicitly approved now.

The first version of the experimental oracle incorrectly read the dynamically
sized component-mask array up to `MAX_ENTITIES` before checking liveness. ASan
caught the bounds error. Those initial five trials are withdrawn and preserved
under `particle_tradeoffs/invalid_oracle/`; they are not accepted evidence.
The corrected oracle enumerates live entities before accessing components.
The corrected helper passes ASan/UBSan through 3.1 seconds and 402,332 executed
particle responses, and the experimental consumer cross-builds for Windows.
All five full trials were repeated; the table above uses only corrected runs.
The production engine and its already completed regressions are unaffected.


### Subdivision tradeoff and remaining costs

At the user's request, compare two and one configured substeps with the selected
particle candidate. Keep the 240 Hz tick rate, eight main solver rounds, two
particle response rounds and 0.5 particle-only allowance. Four ticks are consumed
per nominal 60 Hz update: four configured subdivisions mean sixteen nominal
substeps per update; two mean eight; one means four. Adaptive sweeps remain
active and may add steps. An observer independently sums completed durations.

| Configured subdivisions | Physics CPU ms over 12 s | p95 CPU ms/update | Active elapsed s | Worst particle overlap | Worst floor penetration | Final RMS speed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 35088.489 | 62.370 | 35.831 | 0.501142 | 0.001958 | 0.3887 |
| 2 | 23461.283 | 47.256 | 23.936 | 0.525371 | 0.022869 | 0.6649 |
| 1 | 14272.171 | 28.382 | 14.514 | 0.580482 | 0.086014 | 0.8373 |

Both reduced-subdivision trials finish without escapes or invalid state and
consume all twelve seconds: 5,874 completed substeps at two, and 3,024 at one.
The headless throughput equivalents are roughly 20, 30 and 50 updates/s,
respectively. These are single diagnostics, not rendered FPS. One subdivision
still exceeds the 16.67 ms p95 physics budget before rendering and exceeds the
proposed 0.5-unit overlap maximum. It also reduces final pile height to 71.4548
and increases residual motion. No global subdivision reduction is selected;
the authored car's stiffness and collision behavior require separate coverage.
Power is recorded for the preceding profiled run in this serial batch, but the
two standalone subdivision logs do not independently capture AC metadata.

Exclusive profiles distinguish response from discovery and swept checks:

| Stage, exclusive mean ms per update | Four subdivisions | One subdivision |
| --- | ---: | ---: |
| Contact gathering | 16.932 | 6.206 |
| Swept collision checks | 14.584 | 3.947 |
| Rigid-contact solving | 13.877 | 8.024 |
| Other stages | 4.573 | 1.687 |

The profiles use the existing exclusive measurement contract and disable the
independent overlap oracle. State/work counts agree with the corresponding
oracle trials. The first four-subdivision profile omitted a block-fallback
caller when composing experimental dispatch and is explicitly invalid under
`runs/particle_candidate_profile/`; use `particle_candidate_profile_fixed`.
The corrected composition filters every original caller and measures actual
response calls separately from dispatch. Collision-related work remains about
91% of total physics cost. With one subdivision, response becomes the largest
individual stage, but gathering and sweeps together still cost more. Joint work
is small in this pit; it must still be evaluated in the car and joint fixtures.

### Additional response-copy experiment

Following the user's request to stay with the remaining bottleneck, test
constructing contact `PhysicsChange` records using only their owned fields.
Use the same validation/commit path, response ordering, equations and candidate
settings. Three paired warmed kernels preserve identical state hashes. The
fixed-geometry median changes from 217.593 to 228.042 ns/solve (4.8% slower);
the moving-geometry median changes from 632.297 to 621.974 (1.6% faster).
The full one-subdivision candidate preserves all numerical and work counts.
Its apparent roughly 5% reduction also occurs in untouched integration and
gathering stages, so it does not establish a benefit from this change. Retain
the existing production record construction; preserve the unselected prototype
and measurements under `particle_tradeoffs/` and `logs/owned_*`.

### Two and one ticks per nominal frame

The user then requested two ticks per nominal 60 Hz update, followed by one.
These consumers use 120 Hz and 60 Hz respectively, one configured subdivision,
eight main solver rounds, two particle-only response rounds and 0.5-unit
particle allowance. They change both the tick duration and ticks consumed per
update, preserving the twelve-second endpoint and all authored fixture data.
Adaptive subdivision remains enabled. Each trial runs serially on AC power,
after its build completes; the independent duration observer confirms exactly
twelve seconds of completed simulation.

| Tick rate / ticks per update | Physics CPU over 12 s | p95 CPU ms/update | Active elapsed s | Headless updates/s | Worst particle overlap | Worst floor penetration | Final RMS speed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 240 Hz / 4 | 14272.171 ms | 28.382 | 14.514 | 49.6 | 0.580482 | 0.086014 | 0.8373 |
| 120 Hz / 2 | 8582.456 ms | 16.546 | 8.747 | 82.3 | 1.041389 | 0.350550 | 1.4933 |
| 60 Hz / 1 | 5034.291 ms | 14.406 | 5.126 | 140.5 | 1.617722 | 0.982507 | 2.2170 |

The 240 Hz row repeats the preceding one-subdivision diagnostic. The two new
trials complete 1,545 and 862 adaptive substeps respectively, with no escapes,
nonfinite state or physics diagnostics. Both exceed the proposed 0.5- and
1.0-unit particle-overlap limits. At 60 Hz the worst overlap is about 32% of
particle diameter, final overlap remains 1.458772 units, and final pile height
falls to 69.132 units. The fixture's original `PASS` verdict is not acceptance
of these particle errors or the substantially increased floor penetration.

The headless rates include oracle wall cost; physics CPU excludes its measured
cost (97.734 ms at 120 Hz and 52.024 ms at 60 Hz). These are individual trials,
not rendered FPS or the required three-run acceptance. Even though both p95
physics times are below 16.67 ms, maximum physics work is 24.812 and 24.900 ms,
and rendering is unmeasured. The car and other scenarios have not been verified
at these settings. Keep tick-rate selection open alongside visual review and
the user's final tolerance decision. Sources, binaries and serial runners are
under `particle_tradeoffs/` and `build/particle_tradeoffs/`; logs are
`logs/particle_ticks_2.log` and `logs/particle_ticks_1.log`. The machine-readable
comparison is `particle_tick_summary.json`.

### Collision breakdown and follow-up priorities

After Goal 6, the user requested exclusive timings by pair type and then four
new optimization goals before the remaining milestone work. The following are
milliseconds over twelve simulated seconds in the unchanged 800-particle pit,
with one configured subdivision, eight main passes, two particle passes and
0.5-unit particle-only slack. Each column is one serial AC-powered diagnostic.

| Stage / pair type | 240 Hz | 120 Hz | 60 Hz |
| --- | ---: | ---: | ---: |
| Response: particle–particle | 3379 | 2271 | 1409 |
| Response: particle–rigid | 1903 | 1155 | 699 |
| Response: rigid–rigid | 14 | 8 | 4 |
| Response: shared overhead | 536 | 309 | 183 |
| Sweeps: particle–particle | 286 | 162 | 95 |
| Sweeps: particle–rigid | 223 | 122 | 72 |
| Sweeps: rigid–rigid | 0 | 0 | 0 |
| Sweeps: shared preparation/traversal | 2664 | 1405 | 830 |
| Gathering: particle–particle | 684 | 398 | 236 |
| Gathering: particle–rigid | 163 | 89 | 52 |
| Gathering: rigid–rigid | 5 | 3 | 1 |
| Gathering: shared broadphase/filtering | 3849 | 2185 | 1336 |

Particle means a nonrotating body carrying particle properties. Gathering pair
buckets include narrowphase and publication; particle batches remain ordered.
Shared gathering includes tree construction/refit, traversal, duplicate checks,
filtering and queueing. Shared sweeps include scans, bounds/tree preparation,
queries and eligibility filtering. Shared response includes dispatch, skipped
passes and block-solver overhead. Nested costs are counted only once. Static
rigid pairs can still incur gathering/response checks without moving bodies;
no eligible rigid–rigid swept pair is tested in this pit.

All three runs reach the endpoint and exactly match the preceding per-update
state/work traces. Timing instrumentation adds overhead; these single profiles
identify priorities and do not establish rendered acceptance or a speedup.
The independent penetration oracle is disabled in these profiles; preceding
quality failures still apply. Scripts, source snapshots, binaries, traces,
accounting checks and hashes are preserved under
`sandbox/milestone_12/goal_6/collision_breakdown/`; `summary.json` and
`summary.csv` contain the unrounded results.

The four inserted collision goals are now 7, particle–particle response;
8, particle–rigid response; 11, shared sweep preparation/traversal; and 12,
shared gathering broadphase/filtering. Goal 9 audits global algorithms and
adaptive stepping; the new Goal 10 investigates its ranked questions and
potential solutions before more local optimization.
The original remaining goals are now 13–17. The
1,000 ms concern is a prioritization of total work over the run, not a new
per-frame threshold. No new optimization goal starts with this roadmap update.

The user clarified that 0.5 is a particle–particle penetration-review threshold
for the diameter-5 pit, not a requested solver correction setting. Flag only
values above it; even the earlier 0.501142 result exceeds it. All Milestone 6
extreme-fixture penetration checks now provisionally use 0.1 units, including
particle–rigid contacts in those fixtures. Stack and joint collision checks
already used 0.1; drop, thin-barrier and soft-body checks previously used
0.0001, 0.001 and 0.02 respectively. Historical measurements and criteria remain
preserved. No separate particle–wall pit threshold has been selected.

The displayed breakdowns used the misinterpreted solver-slack prototype. They
remain evidence about that prototype and must not be represented as the cost
of an approved configuration. New goals re-establish matched baselines with
the original correction settings. Report full-run maxima even below review
thresholds, preserve unrelated support/tunneling/energy/joint-error checks, and
do not round violations into a pass. Production solver settings remain unchanged;
final overall tolerance acceptance remains Goal 17.

### Preserved evidence

`summary.json` verifies complete fixtures, exact parent/reference comparisons
and the isolated car repeat. `experiment_summary.json` records the three paired
production pit runs and kernels; `particle_tradeoff_summary.json` records the
corrected quality experiments. `audit.json` verifies all 1,457 frozen-reference
and 591 Goal 5 hashes and confirms that only `rigid_body_step` changes executable
code in the installed engine archives. `manifest.json` records the final source,
SDK, executable and evidence hashes. Invalid experimental attempts remain
explicitly labeled; they are excluded from accepted comparison results.
