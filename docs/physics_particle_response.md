# Milestone 12 Goal 7 — Particle–particle response

Goal 7 is complete and committed at `13ac660e`. Goal 8 follows in the
[particle–rigid response report](physics_particle_rigid_response.md).
The immediate parent is `932e6e21`; its engine archive
is unchanged from Goal 6 (`479d50a2`). The frozen reference remains
`physics-pipelines-overoptimized` at `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`.
Exclude `main`. Sources, executables, SDKs, traces and measurements are preserved
under ignored `sandbox/milestone_12/goal_7/`.

## Review policy and configuration

The particle–particle pit review threshold is 0.5 units of measured overlap.
Milestone 6 extreme-fixture penetration checks use the provisional 0.1-unit
threshold. Neither value is a solver correction parameter. Original absolute
and relative correction slop, correction caps, materials and geometry remain
unchanged. The earlier Goal 6 experiments with solver slop 0.5 or 1.0 remain
historical evidence of the agent's misinterpretation; they are not the baseline
or an approved runtime configuration.

The production configuration remains 240 Hz, four configured subdivisions and
eight solver passes. A separate diagnostic wrapper evaluates two particle
passes while mixed and polygon contacts retain eight. It executes particle
response on passes four and eight, with position fractions one half and one,
and records executed and skipped dispatch separately. This experiment is not
installed into the engine. All runs retain the authored 800 diameter-5 particles,
restitution 0.5, other materials, initial positions and twelve-second endpoint.

## Selected implementation

The parent's normal and friction impulse paths use general body-change records.
Those records copy position, orientation, acceleration and force fields in
addition to the velocities these operations own. The selected implementation uses compact velocity records and a dedicated
internal pair commit for circle–circle response. It validates both linear and
angular velocities before writing either body and preserves convergence motion
tracking. Rotating rigid bodies carrying particle properties still receive the
same angular arithmetic. Circle–polygon contacts retain the generic commit path.
Successful compact commits no longer resolve source and target entity handles
that are used only in rejection diagnostics. Rejected commits resolve the same
current identities and retain the same report stage, source and target.

Normal and friction commits remain separate: a rejected friction contribution
cannot undo a successful normal impulse. Identity checks, geometry validation,
response masks, mass weighting, retained impulses and position correction stay
in their existing order. A retained single-point rebound target is reused
without first calculating a replacement that would immediately be discarded.
No allocation, public API or persistent contact ownership changes are added.

Pairs sharing an entity still solve sequentially. The coupled three-particle
regression checks those effects, including a 40:1 mass ratio and a rotating
circle body. SIMD response batching would require proving independence without
changing this order; atomics alone would not preserve it. This goal retains the
existing contact list and scalar pair ordering. Gathering SIMD remains separate.

The first prototype introduced a dedicated single-circle assembly branch. Its
kernel and full-pit pilot showed no benefit, so that branch was removed. The
compact-commit prototype is preserved separately from the final SDK build.
The final implementation is selected on repeated exclusive response and total
physics savings, including independent unwrapped playback.
The earlier candidate with eager diagnostic handle lookup and its superseded
timing batch are retained under `stage_1/`. Its interrupted parent trial reached
236 of 720 updates and remains explicitly incomplete; those partial bounds are
not used for selection.

## Verification contract

Focused tests cover retained impact targets after another constraint changes
velocity, atomic rejection on either side of the pair, invalid angular state on
nonrotating particles, unchanged-state convergence, and fields outside impulse
ownership. Existing cases cover geometry edits, manual pipeline composition,
generation safety, friction, restitution, rotating circle bodies and masks.

The head-on fixture uses two diameter-5 particles moving at 1,000 units/s each,
restitution 0.5 and no gravity. It measures overlap after every completed
substep, rejects pass-through and missed response, and runs five normal/delayed
schedules to 2.5 seconds. Its overlap limit remains 0.5.

All numerical scenarios run to their declared endpoint despite tolerance
failures; fatal errors, invalid state or inability to advance time remain
incomplete. Headless pit and ten-second car-prefix diagnostics cannot establish
rendered FPS acceptance or complete the car's sixty-second endpoint. Milestone
11 remains superseded, and the existing Milestone 6 CPU comparison remains
visible independently of penetration acceptance.

## Controlled profile comparison

Three serial runs per version use AC power, the existing streaming-closed
confirmation and no concurrent builds. Each pit run completes 720 updates and
twelve simulated seconds, independently observed through the substep callback.
State/work hashes agree between versions at each configuration. The original
correction settings are used throughout.

| Configuration at 240 Hz | Parent physics CPU, median ms | Candidate physics CPU, median ms | Reduction | Parent particle response, exclusive ms | Candidate particle response, exclusive ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Four subdivisions, eight particle passes | 74112.569 | 71912.280 | 2.97% | 27768.335 | 25698.493 |
| One subdivision, two particle passes | 24456.939 | 24147.123 | 1.27% | 5708.597 | 5401.572 |

The particle-response reductions are 7.45% and 5.38%, respectively. Median
per-update p95 CPU times are 148.818 → 145.103 ms and 77.480 → 75.761 ms.
These instrumented diagnostics include wrapper overhead and establish no
rendered-FPS acceptance. Unwrapped playback is measured separately.

The following exclusive elapsed times are medians over the same twelve
simulated seconds in the candidate. Gathering and sweep totals include their
geometry work; they do not isolate shared preparation/traversal. Subsequent
goals must remeasure that split with original correction settings.

| Work | Four subdivisions / eight passes, ms | One subdivision / two passes, ms |
| --- | ---: | ---: |
| Executed particle–particle response | 25698.493 | 5401.572 |
| Executed particle–rigid response | 3124.241 | 2746.051 |
| Executed rigid–rigid response | 55.691 | 21.594 |
| Experimental dispatch for executed pairs | 3202.895 | 341.759 |
| Experimental dispatch for skipped particle passes | 0 | 884.205 |
| Rigid contact gathering, including geometry | 16334.364 | 6949.399 |
| Rigid sweeps, including geometry | 11916.429 | 4248.942 |

Dispatch timings belong to the diagnostic pass filter, not a new production
pass-selection system. Nested timers subtract child work, and each run's
exclusive sum equals its measured root duration. The profile-only dispatch
footer has zero placeholders for overlap statistics; those are not measurements.
Only the separate independent-oracle runs supply penetration and pile metrics.

## Unwrapped playback and configuration review

Three serial unwrapped pit runs per version retain 240 Hz, four subdivisions
and eight passes. Parent CPU totals are 61380.498, 62596.818 and 62097.934 ms;
candidate totals are 58902.988, 59175.668 and 58797.027 ms. The median decreases
from 62097.934 to 58902.988 ms, a **5.15% reduction**. Every paired candidate
run is faster. Median per-update p95 CPU decreases from 125.359 to 117.249 ms.
All runs reach twelve seconds without escapes or diagnostics. These are
headless CPU measurements; the 16.67 ms rendered frame-work gate remains open.

The independent oracle samples all particle pairs after every completed
substep. Floor depth is sampled once per nominal update, as in the existing
fixture. The following single-run configuration probes use the candidate,
original solver correction settings and eight main passes for other contacts.
CPU excludes the separately measured overlap oracle but includes dispatch
overhead; these figures are not directly interchangeable with unwrapped totals.

| Hz | Subdivisions | Particle passes | CPU ms | p95 CPU ms | Maximum particle overlap | Sampled floor depth | Particle review |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 240 | 4 | 8 | 63871.030 | 128.026 | 0.002638297 | 0.002340078 | Pass |
| 240 | 4 | 2 | 56963.762 | 104.664 | 0.014594020 | 0.012118340 | Pass |
| 240 | 1 | 2 | 21528.289 | 67.968 | 0.189830119 | 0.103580713 | Pass |
| 120 | 1 | 2 | 15379.514 | 64.147 | 0.556208960 | 0.264598608 | Fail |
| 60 | 1 | 2 | 13194.415 | 63.653 | 0.803068857 | 0.420157433 | Fail |

Every oracle run independently observes twelve simulated seconds and agrees
with the completed-substep count. No new hard particle–wall pit threshold is
implied. The 120/60 Hz variants are rejected under the retained 0.5-unit particle
review limit. Lower cost alone does not accept their penetration.

Two particle passes at 240 Hz remain an evaluation candidate, not a production
setting. With four subdivisions, final RMS speed rises from 0.119396 to
0.373309 and pile height falls from 84.470505 to 83.731316. With one subdivision,
final RMS speed is 1.035724 and pile height 78.904846. The lower pile and greater
residual motion remain visible tradeoffs even where maximum overlap passes.
No probe meets the rendered real-time contract.

The head-on fixture passes all five schedules with both eight and two particle
passes on parent and candidate, with identical matching-configuration traces.
Worst overlap is 0.000500202179, there is no pass-through, and rebound error is
zero. The measured 0.5-unit limit is preserved.

Initial original-correction baseline probes, earlier prototypes and the failed
AC preflight are retained separately. Initial baseline timings were recorded
on battery and are exploratory only; final comparisons above use AC. The frozen
reference's incomplete full-car and interrupted rendered-pit evidence has not
been replaced or relabelled.

## Final verification and evidence

All 219 Milestone 6 timed scenarios and four rebound calibrations finish on
Linux and ASan/UBSan. All five head-on schedules also pass on both platforms,
for 224 timed scenarios per platform. Eighteen Linux and eighteen sanitizer
regressions pass, including the new atomic-commit and retained-rebound cases.
There are no sanitizer findings or compiler warnings in the final builds.
An initial test build used a nonexistent velocity accessor; that failed log is
preserved in `stage_1/logs/build_linux_attempt_1.log`, and the corrected test is
included in all final builds.

Every fixture state/work trace matches the immediate parent exactly. They also
match the frozen reference after excluding timing and the candidate counts
intentionally reduced by Goal 4. The default full pit matches that reference
over all 720 updates. The five-schedule head-on traces match parent/candidate
at both evaluated pass counts. All final profiled and oracle pit probes
independently verify complete elapsed simulation time, with no invalid state,
fatal errors or missing updates.

The car diagnostic is a single 600-update prefix: 10.000000521541 simulated
seconds, matching the existing float tick duration and both parent/reference
traces exactly. It consumes 103948.219 ms of physics CPU, with p95 352.330 ms.
This is neither a completed sixty-second scenario nor a repeated car performance
comparison; no car speedup is claimed. The existing jump-force clock finding
remains assigned to Goal 16. No stopped full-car or rendered-window trial was
restarted.

Linux engine execution, sanitized car/pit smokes, Windows cross-builds and
independent installed-SDK consumers pass. The standalone joints example builds
for Linux, sanitizers and Windows, and runs with teardown on Linux and sanitizers
from `/tmp`. Native Windows execution is not claimed.

The soft-body stress fixture retains maximum penetration 0.00300585644 and
relative energy error 0.00136403434. Its delayed-update CPU maximum is 373.020 ms,
still failing the historical 50 ms comparison. Completing Goal 7 does not
complete the Milestone 6 soft-body goal or the combined real-time gates.

Three paired response kernels preserve exact state hashes. Their fixed-pose
median improves from 214.267 to 204.051 ns per solve; the moving-pose median
changes from 630.962 to 634.463 ns and establishes no improvement. Selection
rests on the repeated whole-pit results, not a claim that every microbenchmark
gets faster.

`summary.json` records endpoint, clock, trajectory, threshold and timing audits.
`source_manifest.json` pins the tested implementation. `build.py`,
`final_prepare.py`, `verify.py`, `audit_fixtures.py`, `measure.py` and
`summarize.py` retain build/run/check procedures; `runs/`, `logs/`, `bin/`,
`build/final_parent/`, `build/final_current/` and the three SDKs retain evidence.
The final manifest hashes the preserved artifacts. Existing output directories
are not reused for measurements. The initial driver was corrected to forward
interrupts without reentering a subprocess wait from its signal handler.

Goal 8 distinguishes rotating circle–circle pairs from polygon contacts in its
profiler, but its measured car prefix has no rotating circle pairs. The response
timers measure visits to the solver, including overlap-only refresh where
applicable, rather than only successful physical responses. See the next report
for the refined classification. Preserve the measured particle limits and
original correction settings in subsequent comparisons.
