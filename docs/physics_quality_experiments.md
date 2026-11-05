# Milestone 12 Goal 13 — Physics work and quality experiments

**Complete: investigation and verification, awaiting user review/commit.** All
eight isolated screens, three combined additions and two finalists are recorded.
This is an experiment record, not an adopted runtime configuration. The
[approved plan](physics_quality_experiments_plan.md) defines the single review
boundary. Production physics, public APIs, defaults and review limits are unchanged.

**Subsequent roadmap update:** the user inserted
[cheaper particle/node collision work](physics_particle_collision_plan.md) as
Goal 14. Implementation has not started. Historical future-goal references in
this report retain their completion-checkpoint numbers: Goals 14–18 now map to
15–19. This does not alter the experiments or select their proposed follow-up.

The balanced candidate is **60 Hz, four configured substeps, four solver passes**.
Its three-run medians are 24.465 seconds of pit physics work per 12 simulated
seconds and 54.354 seconds per ten-second car prefix. The 30 Hz/four/four
alternative improves pit cost to 18.160 seconds, but car cost is still 53.256
seconds and update spikes grow. Neither establishes real-time playback. See the
[final comparison](#final-comparison-and-recommendation) for variability, numerical
tradeoffs and the remaining budget gap.

## Provenance and interpretation

The parent is `db1d52cb6e4019f3ce3901e933b3c7570e20dc1d`. Its engine matches
Goal 12's verified `81e76ff5` implementation. The frozen reference remains
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; `main` is excluded. Historical
reference measurements at different settings are not isolated optimization
comparisons. Production source, defaults, correction settings and APIs are unchanged.

Sources, independent installed-SDK executables, configurations and raw evidence
are preserved in `sandbox/milestone_12/goal_13/`. `baseline.json` records archive
and SDK provenance; `analysis.json` collects workload records without replacing
their raw logs. `runs/*/conditions.json` identifies each configuration and scope.

The baseline is 60 Hz, four configured substeps and eight solver passes. The
unchanged 800-particle pit runs for 12 simulated seconds; the unchanged car uses
the authorized 10-second prefix. These are headless diagnostics, not rendered
FPS acceptance or full-car bounds. AC was connected, streaming was closed by
user confirmation, and no builds ran alongside measurement trials.

During testing the user imposed a one-minute wall-time limit per test execution.
The runner reserves the final second for an interrupt and terminates a remaining
process at 60 seconds. A capped run is incomplete and its clean repeat is skipped.
Original simulated targets remain unchanged. Completed evidence from before that
instruction remains valid at its recorded duration. Partial trace costs and errors
are never compared as though they covered a complete workload.

Three clean baseline trials measured pit physics CPU at 34.971, 35.466 and 36.606
seconds, and car-prefix physics CPU at 65.267, 72.849 and 67.764 seconds. Medians
are **35.466 seconds per 12 simulated seconds** and **67.764 seconds per 10**.
Thus average physics work alone is 2.96 and 6.78 times elapsed simulation time.
The older Goal 12 medians were lower; these new measurements do not establish
the cause of that timing difference. Baseline state/work traces match Goal 12
exactly for both complete authorized workloads. Timing attribution is exclusive;
observer and nested response instrumentation are kept separate from clean timing.
Candidate/contact counters describe repeated work visits, not unique physical
pairs. Adaptive reduction counters count each selector shortening its input;
several selectors can restrict the same attempt, so their counts must not be
summed as distinct subdivisions. Completed integration steps are reported separately.

**Stress-fixture “pass” below means numerical acceptance, not the older CPU
budgets.** The fixtures retain full-run p95 and delayed-update costs. Applying
their unchanged performance predicates offline is recorded separately in
`fixture_cpu_comparisons.json`: 60 Hz p95 at most 2 ms and delayed work at most
50 ms, with the original drop-fixture rate condition retained. No criterion is
redefined. The baseline four-substep batch has four CPU-comparison failures;
four global passes retain two. The 30 Hz finalist's additional delayed fixtures
also retain two CPU-comparison failures, with delayed work up to 340.046 ms.
These failures remain visible despite successful numerical verdicts and do not
complete Milestone 6 or the rendered gates.

## 1. Configured substeps

At 60 Hz/eight passes, lowering configured substeps retains every adaptive selector.
The actual adaptive work can increase when changed trajectories create harder
interactions. This experiment is complete, with the user's interrupted candidate
and skipped repeat explicitly retained.

| Workload | Configured steps | Clean physics CPU, seconds | Actual steps | Maximum particle overlap | Scope |
|---|---:|---:|---:|---:|---|
| Pit | 4 | 35.466 median | 4,927 | 0.019971 | Full 12 seconds |
| Pit | 2 | 23.822 | 3,701 | 0.159280 | Full 12 seconds |
| Pit | 1 | 23.227 | 3,193 | 0.208370 | Full 12 seconds |
| Car | 4 | 67.764 median | 25,221 | 0.041827 | Full authorized 10-second prefix |
| Car | 2 | 76.664 | 28,949 | 0.036003 | Full authorized 10-second prefix |
| Car | 1 | Not run | 168,494 partial | 0.006104 partial | Interrupted at 7.45/10 seconds |

The reduced pit settings remove about 33–35% of baseline median cost in single
screening trials, with higher overlap. Both remain below the 0.5 particle-pair
review limit. Floor overlap is 0.023434 / 0.068877 / 0.157461 at four/two/one
substeps; no new pit-wall acceptance threshold is invented. No side escapes
occurred. Floor/side checks in the inherited pit executable sample after complete
ticks; they are not substep-resolved or continuous penetration bounds. The
particle-pair oracle samples every completed substep. The two-substep car costs
about 13% more in its single clean trial.
Its maximum beam absolute/relative errors are 13.836899 / 1.570460 versus the
baseline 13.419485 / 1.472364.

The one-substep car was stopped by user direction after 869.43 wall seconds,
before the new wall-time cap was introduced. It was finite and advancing, but
completed only 447 of 600 updates. Observed beam error reached 52.105854 absolute
and 6.438006 relative; these are partial bounds. The original jump-force lifetime
limitation remains present. Neither causality nor full-run quality is inferred
from this interrupted run. `substeps_interruption.json` records the decision.

All 224 stress/head-on scenarios reach their declared endpoints at each of the
three settings. The four- and two-substep settings each fail one stack-behavior
criterion; one substep passes this fixture batch. No penetration criterion is
relabeled and successful extreme fixtures do not establish acceptable car motion.
All 13 Linux regressions pass, including live public setting changes, returning
to baseline settings, elapsed-time consumption and persistent-force ownership.
The independent SDK consumers compile without warnings. Final prototype
platform/sanitizer results are recorded below.

**Screening decision:** retain reduced substeps as pit-specific candidates for
later combinations; retain four for the car. No common default is selected.

## 2. Particle-only solver passes

The isolated prototype selects nonrotating particle pairs, including eligible
soft nodes. Rotating bodies carrying particle data and mixed particle/polygon
contacts retain the ordinary solver schedule. Four selected rounds run at
2/4/6/8; two run at 4/8. Ordinary particle correction uses the remaining selected
rounds as its denominator; existing soft solving retains full correction each
round. Slop and correction caps are unchanged. Exact early-exit detection cannot
cancel selected future response rounds merely because an earlier round skipped
them. Manual stage invocation retains its original behavior.

| Particle-pair passes | Pit clean CPU, seconds | Actual steps | Maximum particle overlap | Floor overlap | Car result |
|---:|---:|---:|---:|---:|---|
| 8 | 35.466 median | 4,927 | 0.019971 | 0.023434 | Complete baseline prefix |
| 4 | 29.295 | 4,794 | 0.065548 | 0.042436 | Capped at 8.983/10 seconds |
| 2 | 19.762 | 4,401 | 0.234337 | 0.126423 | Capped at 8.917/10 seconds |

Both pit candidates complete 12 simulated seconds with no side escapes. Their
single clean trials use about 17% and 44% less work than the baseline median.
Even the faster candidate averages 1.65 seconds of physics CPU per simulated
second; its p95 update work is 60.390 ms. This is not real-time acceptance.
Final pit RMS particle speed rises to 1.002651 with two passes, versus 0.529912
with four; penetration alone does not capture the settling tradeoff.

The four-pass diagnostic visits 40,149,099 eligible particle responses and skips
20,071,816; two passes visit 40,497,780 and skip 30,364,633. Dispatch and list
traversal still occur for skipped responses. Actual solver rounds are 38,352 and
35,208. These counters explain why impulse reduction is not proportional to total
cost. Clean timings exclude the diagnostic counters and nested pair timers.

Both car candidates remain incomplete under the new wall cap; their clean repeats
are skipped. Partial observed particle overlap is 0.092254 and 0.178075, respectively.
Those values are not full-prefix bounds. The candidate with its behavior disabled
matches the full parent pit and first five seconds of the parent car in exact
state/work traces. Root timing and source-work accounting checks pass.

Both settings finish the stress/head-on fixtures. Four particle passes retain
the baseline stack failure. Two also fail the delayed 60 Hz soft-body stress case:
penetration reaches **0.125231**, above **0.1**. Its full 30-second scenario finishes.
The two-pass prototype passes the 13 Linux lifecycle, ownership and regression
checks. Compilation is warning-free; final platform/sanitizer results are recorded
below. The four inherited thin-barrier rebound microcases run with the explicitly
configured pass override; their legacy `iterations=` labels denote authored requests,
not a separate effective budget. All 224 timed/head-on cases plus these four state
microcases are retained in the raw logs.

**Screening decision:** retain four/two passes as pit-only combination candidates.
Do not recommend two passes for soft-node contacts under the existing stress limit.
No complete-car performance or numerical recommendation follows from capped runs.

## 3. Global solver passes

This comparison restores the original particle policy and uses the existing
global iteration setter at 60 Hz/four configured substeps. Every candidate
finishes the full pit and authorized car prefix within the wall cap.

| Workload | Passes | Clean CPU, seconds | Actual steps | Maximum particle overlap | p95 update CPU, ms |
|---|---:|---:|---:|---:|---:|
| Pit | 8 | 35.466 median | 4,927 | 0.019971 | 98.280 median |
| Pit | 4 | 25.558 | 4,778 | 0.101639 | 89.421 |
| Pit | 2 | 17.070 | 4,368 | 0.156216 | 60.794 |
| Car | 8 | 67.764 median | 25,221 | 0.041827 | 201.652 median |
| Car | 4 | 55.738 | 25,099 | 0.098565 | 175.050 |
| Car | 2 | 42.296 | 25,822 | 0.189272 | 139.911 |

These are single candidate screening trials against the three-run parent median.
The two-pass pit still needs about 1.42 times elapsed simulation time in physics
CPU alone, and the car needs 4.23 times. Both remain far above the p95 frame-work
gate. Pit floor overlap is 0.045247/0.111816 at four/two passes; final RMS particle
speed is 0.559138/1.264194. Car maximum absolute/relative beam errors are
13.962458/1.132047 at four passes and 11.482813/1.173021 at two. These changed
trajectories do not establish better car motion merely from a lower peak error.

Four passes pass all 224 timed/head-on scenarios plus four rebound microcases at
the configured test settings. Two passes finish them all but fail one stack case,
four pin-impact cases and three soft-body cases. Pin-impact penetration reaches
0.423089 (swept 0.423631); soft-body penetration reaches 0.133799. Both exceed
the unchanged 0.1 stress criterion. The stack exceeds post-impact motion limits.

At two passes, diagnostic car exclusive costs still include 18.358 seconds of
gathering and 10.890 seconds of sweeps, versus 8.176 seconds of rigid response.
The corresponding pit costs are 5.836, 4.093 and 7.858 seconds. These instrumented
wall-time partitions conserve their enclosing total and are not added to clean
CPU timings. Fewer response rounds leave substantial discovery and adaptive work.

**Screening decision:** retain four passes for the general combination track.
Two passes are a pit-specific candidate and a failed general numerical candidate;
do not waive joint or soft-body criteria to recommend them globally.

## 4. Physics tick rate

Four configured substeps and eight global passes remain fixed. Caller scheduling,
dt and the existing car input times move together: the pit executes 720/360/240
updates and the car requests 600/300/200 at 60/30/20 Hz. The complete-update
integration still consumes the full requested simulated time when a run finishes.

| Pit tick rate | Clean CPU, seconds | Actual steps | Maximum particle overlap | p95 update CPU, ms |
|---:|---:|---:|---:|---:|
| 60 Hz | 35.466 median | 4,927 | 0.019971 | 98.280 median |
| 30 Hz | 26.296 | 3,701 | 0.159280 | 172.795 |
| 20 Hz | 24.601 | 3,515 | 0.172917 | 264.850 |

Both candidates finish the unchanged 12-second pit without side escapes and
remain below the 0.5 particle-pair review threshold. Tick-sampled floor overlap
is 0.068764/0.100840 at 30/20 Hz. The car reaches only 8.233/6.750 seconds before
the wall cap, so neither gets a clean repeat or a complete-prefix recommendation.
The capped runs are finite with valid completed-update/work accounting.

Fewer ticks do not proportionally reduce actual integration steps: adaptive
selectors still constrain each requested interval. Lower-rate updates also
contain larger work bursts. A 30 Hz interval permits 33.33 ms of simulation time,
but does not change the rendered 60 FPS frame-work target of 16.67 ms. These
results do not establish rendered performance or endorse intentional slow motion.

The existing 30/60 Hz stress results remain applicable to this unchanged
four-substep/eight-pass engine configuration. Additional normal/delayed 20 Hz
schedules pass 124 timed/head-on scenarios and four rebound state checks, keeping
geometry, materials, endpoints and review limits unchanged. The 1,000-units/s
head-on test detects response without crossing; its largest observed overlap is
0.000500202. The sandbox drop helper initially rejected 20 Hz before setup because
its authored guard admitted only 30/60/120; that incomplete setup attempt is
preserved. Extending this sandbox guard permits all ten 20 Hz drop cases to finish
and pass. The production example is unchanged.

**Screening decision:** retain 30/20 Hz only as pit combination options. Neither
isolated rate reduction is a supported car improvement. Prefer the higher update
rate when measured cost and quality are otherwise comparable; no default changes.

## 5. Contact rediscovery cadence

Screening and focused stress verification are complete. Rigid and soft
discovery were varied separately and together. Every reduced schedule retains
current response geometry, initial discovery, a final full discovery, one extra
full-correction reconciliation round, and another discovery for publication.
All of that work is timed. Standalone/manual stages retain their original schedule.

| Reduced discovery | Schedule | Pit clean CPU, seconds | Pit maximum particle overlap | Car simulated seconds before cap |
|---|---|---:|---:|---:|
| Rigid | Every second pass | 36.715 | 0.011602 | 8.050 |
| Rigid | Initial/final | 32.629 | 0.027740 | 6.933 |
| Soft | Every second pass | 31.808 | 0.019971 | 6.733 |
| Soft | Initial/final | 34.675 | 0.019971 | 8.200 |
| Both | Every second pass | 36.389 | 0.011602 | 7.650 |
| Both | Initial/final | 34.893 | 0.027740 | 8.150 |

Every pit reaches its full 12-second endpoint; every car is incomplete and skips
its clean repeat. Soft-only settings remove no work from this pit: exact state
and step counts match the parent. Their timing variation cannot be claimed as an
optimization. Both/rigid settings likewise produce identical pit trajectories
at matching cadence. Single-trial differences do not establish a reliable benefit.
The car's soft initial/final schedule reaches 0.775450 particle overlap before
its cap; the both initial/final schedule reaches 0.443120. These are partial
observations, not full-prefix bounds or newly accepted car tolerances.

The pit's every-second and initial/final rigid schedules skip 22,700 and 39,424
refresh requests but add 4,912 and 4,928 reconciliation rounds. Final discovery
leaves 4,681 and 5,889 newly gathered response candidates unvisited during that
substep. These counts identify candidate-list entries, not independently confirmed
penetrating pairs. The final publication distinguishes overlap from completed
response while retaining eligibility for a subsequent manual solve. A dedicated
correction chain exposes one actual overlapping pair without completed response
after the reduced solve; manual solving responds to it. Query/detail consistency,
contact exit and deleted-slot reuse pass. Ordinary particle scheduling counters
exclude reconciliation visits; exclusive response timers include them.

Disabled full-pit/five-second-car control traces match the parent exactly. All
14 Linux regressions pass with disabled, both-every-second and both-initial/final
settings. Each of the six schedules finishes all 224 timed/head-on scenarios and
four rebound state checks. Each retains one baseline stack-behavior failure;
none adds a penetration failure. Compilation is warning-free. Final platform
verification is recorded below.

**Screening decision:** retain the original gathering cadence for combinations.
The reduced schedules add reconciliation and delayed response without establishing
a reliable complete-workload saving. Initial/final-only soft gathering also raises
the observed partial car particle overlap substantially.

## 6. Adaptive collision restrictions

Screening is complete. The prototype multiplies only supported existing-overlap travel
allowances by two or four: half the smaller radius for particle pairs, one
quarter of the projected feature thickness for mixed/rigid pairs, and 0.005 of
the feature for soft sweeps. First-crossing/grazing expressions, the delayed-step
cap, solver correction and the 0.5/0.1 review limits stay unchanged. Concave,
locked and rotating rigid cases retain the original restrictions. Beams retain
their original angular-motion bound; relaxing its existing-contact allowance is
tested separately from the floor/endpoint certificate.

| Allowance changed | Multiplier | Pit clean CPU, seconds | Car seconds before cap | State/work comparison |
|---|---:|---:|---:|---|
| Particle–particle | 2 | 32.278 | 7.783 | Exact parent parity over completed scope |
| Particle–particle | 4 | 32.208 | 7.383 | Exact parent parity over completed scope |
| Particle–rigid | 2 | 33.009 | 6.983 | Exact parent parity over completed scope |
| Particle–rigid | 4 | 32.235 | 7.033 | Exact parent parity over completed scope |
| Rigid–rigid | 2 | 36.757 | 7.883 | Exact parent parity over completed scope |
| Rigid–rigid | 4 | 36.118 | 8.083 | Exact parent parity over completed scope |
| Soft beam | 2 | 37.285 | 7.317 | Pit exact; car prefix takes 5.35% fewer steps |
| Soft beam | 4 | 32.849 | 7.183 | Pit exact; car prefix takes 3.96% fewer steps |

Every pit reaches 12 seconds and still takes 4,927 actual steps. The PP allowance
is consulted 3,522,915 times and the mixed allowance 209,576 times, but changing
either does not shorten the actual work schedule. The pit has no supported RR
allowance visits. The car does exercise RR allowances, yet its completed prefixes
also match the parent exactly. None of these timings establishes a work-removal
speedup; they include ordinary run variability and experimental dispatch overhead.
The six car runs remain incomplete and skip their clean repeats.

Each of these six settings passes 15 Linux regressions, including 16 focused
selector cases for accelerated motion, initial overlap, first-crossing equality
and angular velocity/acceleration fallback. Each finishes all 228 stress/state
cases, retaining only the baseline stack-behavior failure. The dedicated
1,000-units/s-per-particle head-on checks remain included. Disabled full-pit and
five-second-car control traces match the parent exactly. Both beam settings also
finish all 228 stress/state cases with only the baseline stack failure and pass
15 regressions. Their partial car pair-overlap maxima are 0.040994/0.037506;
absolute beam-error maxima are 15.406116/13.809660. These are partial observations,
not full-car bounds. Both car runs hit the cap and skip clean repeats. Platform
verification and the final recommendation remain part of the overall goal.

The user requested stricter evidence of collision risk before subdivision. The
additional isolated prototype compares separation-only certificates with
certificates that also bound existing-contact closing travel. It uses extrema of
signed quadratic motion, preserving original travel allowances, uncertain-case
fallbacks and first-crossing protection. This is distinct from increasing overlap
allowances and does not tighten or loosen the penetration review limits.

Separation-only screening is complete: pit clean CPU is 36.045 seconds, with
exact parent state/work parity. The car reaches 8.533 seconds before the cap,
taking 22,297 steps versus 22,243 at the matching parent prefix (+0.24%); its
clean repeat is skipped. All 16 regressions and 228 stress/state endpoints
finish, retaining only the baseline stack verdict. The car accepts 3,416 PP
separation certificates and 20 mixed-pair certificates, but that local rejection
does not establish an overall saving.

Adding existing-contact certificates produces exact pit state/work parity,
with 33.244 seconds clean CPU. Its car run reaches 8.267 seconds before the cap,
taking 21,913 steps versus 21,859 for the parent (+0.25%). It matches the
separation-only variant's state/work over that common prefix. The extra 585 PP
and 400,012 mixed-contact certificates remove no actual subdivisions there.
All 16 regressions and 228 stress/state endpoints finish with only the baseline
stack verdict. Both stricter-trigger settings retain partial car pair-overlap
maxima of 0.030499 and absolute/relative beam errors of 13.384972/1.466712.
Neither demonstrates a useful saving; retain the original adaptive policy.

The supported paths are accelerated nonrotating particle pairs and fixed
orientation mixed pairs. The custom joint-motion integrator excludes particles,
so those supported moving bodies retain the quadratic position model used by
the proof. Ordinary rotatable rigid bodies retain their original selector. A
focused returning-path RR selector case returns the full proposed duration in
both versions: parity there does **not** verify continuous collision protection
for that path. The unchanged dedicated head-on and stress checks remain separate.

The separate beam/floor prototype accepts only a static rectangular top face,
away from finite edges, whose two simulated endpoint polygons cover the beam's
downward support and respond to that floor. Every check uses current accelerated
motion; locked, driven, small-radius and uncertain geometry fall back. Its 12
focused geometry/lifetime cases and all 16 Linux regressions pass. All 228
stress/state cases finish with only the baseline stack failure. Disabled full-pit
and five-second-car traces match the parent.

It is rejected from combination testing: the pit has no beam work to remove and
retains exact state/work parity (31.330 seconds clean CPU is timing variation).
The car reaches only 5.25 seconds before the cap, taking 18,651 steps versus
16,811 at the same parent prefix, a 10.94% increase. Of 714,904 certificate
attempts, 107,461 succeed, but the changed response timing substantially changes
deformation: partial absolute/relative beam-error maxima reach 50.941603/5.324252.
The clean repeat is skipped. A geometric support certificate alone does not
establish equivalent coupled response or a stable car trajectory.

## 7. Elastic integration and collision cadence

Screening is complete. The prototype retains every elastic integration step, force
preparation and frame update. Only an elastic-limited interval can defer
collision/constraint response. Current-state joint, rigid and soft sweep checks
must permit the proposed two- or four-step response horizon; collision limits,
configured-step boundaries and remainders force response. Extra proof queries
are included in work and timing, and manual stages retain their original behavior.

The disabled full-pit/five-second-car controls match the parent exactly. All 16
disabled and four-step precheck regressions pass. In the focused free-spring
case, four-step cadence skips 12 response passes while retaining all 16 elastic
steps and the same measured internal energy error. Focused high-speed and
accelerated-wall cases retain depths 0.000100/0.000020. Those small cases do not
establish the workload or stress-fixture tradeoff.

Two-step screening finishes the pit with exact state/work parity and 32.072
seconds clean CPU: it has no elastic work to batch, so the timing difference is
not an optimization result. The car hits the cap at 8.15 seconds. It executes
22,799 steps versus 21,691 for the matching parent prefix (+5.11%), defers 1,880
responses, and adds 3,497 checks of each rigid/soft sweep selector. Partial maximum
pair overlap is 0.035108, and absolute/relative beam errors are 14.112974/1.416972.
Its clean repeat is skipped.

All 228 stress/state endpoints finish. Besides the baseline stack failure, three
30 Hz soft-body cases exceed the 0.1 penetration criterion: kinds 2/3 with a ramp
reach 0.245548/0.141765, and kind 4 without a ramp reaches 0.267437. The two-step
candidate is not numerically eligible.

Four-step cadence also preserves pit state/work (36.174 seconds clean CPU).
The car reaches 7.85 seconds before the cap and skips its clean repeat. It takes
22,090 steps versus 21,244 for the parent (+3.98%), defers 2,163 responses and
adds 3,740 rigid/soft proof checks. Partial pair overlap reaches 0.037216 and
absolute/relative beam errors reach 16.115728/1.432117. All 16 regressions and
228 stress/state endpoints finish. It retains the three two-step penetration
failures and adds two delayed kind-1 cases at 0.232558/0.223661. The baseline
stack verdict remains visible. Neither schedule qualifies for combinations;
retain response at every elastic integration step.

## 8. Convergence and sleeping

Early-exit and sleeping screening are complete. The early-exit
prototype sums absolute position and velocity-travel changes at validated commits,
so changes cannot cancel within a round. This global sum is more conservative
than a maximum per body. It normalizes by the smallest supported particle/support
feature and independently checks penetration above existing slop and normal speed
against retained rebound targets. Two qualifying rounds and a full-correction
confirmation must fit within the original pass budget. Newly gathered contacts
reset qualification. Unsupported soft, joint, rotating, driven and locked cases
retain their original solve; manual stages remain unchanged.

Both normalized thresholds, 1e-4 and 1e-3, complete the pit with all 39,416 rounds
and **zero early exits**. State hashes and contact counts match the parent; extra
assessment gathers add 25/4,758 candidate visits. Clean CPU is 36.006/34.840 seconds,
which does not establish an optimization when no rounds are removed. Both car
runs fall back throughout and match their parent prefixes exactly, reaching only
7.20/7.617 seconds before the cap; clean repeats are skipped. Both settings pass
16 regressions and finish 228 stress/state endpoints with only the baseline stack
failure. The focused nearly settled chain does exercise early exit and the
correction-created-contact check. Disabled full-pit/five-second-car controls match.
Retain original convergence behavior for combinations.

The sleeping prototype is limited to standalone particle groups and static
axis-aligned rectangle supports. It uses bounded exact geometry/identity and
neighbor snapshots, retaining sweeps/discovery and the first current response
per contact. Soft bodies, joints, driven/locked bodies, explicit loads and
unsupported geometry remain awake. Quiet-time/speed presets remain 0.01 units/s
for 0.5 seconds and 0.05 for 1 second, with support and residual checks. Small
velocity removal on sleep entry is recorded as dissipation. Ten focused wake
cases cover impact, force, teleport, support deletion, filter/shape edits, slot
reuse, gravity, substeps and manual integration/full-substep calls. They pass,
as do 1,000 create/sleep/delete cycles and ten engine-restart cases. The first
manual-stage fixture incorrectly expected net motion while pressing a neighbor;
that failed attempt is preserved, and a separate build tests a teleported body
with free space. Engine sources are identical between those builds.

Neither preset sleeps any of the 800 pit particles: zero entries, integration
skips, response skips and removed energy. Both retain 4,927 steps, matching
state hashes and contact counts, while 720 assessment gathers add 62,371
candidate visits. Clean costs are 33.002/33.491 seconds; no work is removed, so
those timing differences do not establish an optimization. The cars fall back
throughout, matching parent prefixes exactly at 7.233/7.30 seconds before the
cap. Clean repeats are skipped. Both presets pass 16 regressions and all 228
stress/state endpoints finish, with only the baseline stack failure. Disabled
full-pit/five-second-car controls match exactly. Retain original awake behavior.

## Bounded combination search

All eight isolated screens preceded combination selection. Three combined
additions were tested using the user's shortlist of at most three levers; no
Cartesian sweep or new-factor experiment was added. Failed additions were not
combined further. Capped car runs never established a car saving.

The predeclared shortlist is global passes (four), configured substeps (two),
and tick rate (30/20 Hz). Four passes is the only numerically eligible isolated
setting with completed gains in both workloads. The other two are explicitly
pit-specific alternatives, tested as additions to that control. One configured
substep's pathological car is not repeated. Four particle-only rounds rank
below these pit savings and remove no further rounds at a global budget of four;
two particle rounds fail stress penetration. No reduced discovery, adaptive,
elastic, early-exit or sleep policy enters combinations.

At four passes, adding two configured substeps completes both workloads:
17.857 seconds clean pit CPU and 49.054 seconds car CPU. Pit/car pair maxima
are 0.154165/0.102973, with 3,529/24,589 actual steps. The stack fixture adds
a 30 Hz penetration failure at 0.127244, above 0.1, and a delayed-update motion
failure. All 228 stress/state endpoints finish. This is failed numerical
evidence, not an accepted general configuration or a basis for further additions.

Adding 30 Hz while retaining four configured substeps completes both workloads
at 19.342/52.960 seconds clean CPU. All 228 stress/state endpoints pass. In both
workloads, its end-of-update state hashes exactly match every second update of
the 60 Hz/two-substep combination; paired work counts match too. Both configure
120 steps per simulated second before adaptation. This measured equivalence
applies to the authored input schedules, not arbitrary game inputs or publication
latency. Lower tick rate and fewer configured substeps are overlapping levers;
their isolated savings cannot be added. The two-substep stack failure occurs at
30 Hz, where that fixture has a larger configured interval than 30 Hz/four steps.
The 20 Hz addition also completes both workloads, at 15.615/53.789 seconds
clean CPU, with pit/car pair maxima 0.278795/0.086573 and 3,187/24,748 steps.
The 228 original stress/state endpoints pass, but the additional 128 endpoints
at 20 Hz expose two stack failures: residual motion in the normal schedule and
0.186233 penetration in the delayed schedule. All endpoints finish. This is
failed numerical evidence and is excluded from the final shortlist. The 30 Hz
finalist additionally passes all 128 normal/delayed validation endpoints
numerically before final repetitions; its separate CPU failures remain recorded.

The car's adaptive work explains why lowering nominal tick rate helps less than
the pit. At four passes, the car takes 25,099 actual steps at 60 Hz, 24,589 at
30 Hz and 24,748 at 20 Hz, despite configured minima of 2,400/1,200/800. Most
work survives the lower outer tick rate. In the completed 60 Hz/four-pass trace,
rigid and soft gathering run 225,878/125,495 times; the solve executes 100,396
rounds. Exclusive instrumented costs are 20.998 seconds gathering, 12.955 rigid
response and 9.713 sweeps, versus 2.247 joint solving and 0.871 soft solving.
These are attribution costs, not the clean CPU measurements. Optimizing only
the latter two solves cannot remove the observed whole-car budget gap; reducing
global repetition or its discovery/response cost remains necessary. Future
algorithm gains are hypotheses, not a promise of real-time performance.

## Final comparison and recommendation

Each finalist finishes three serial clean trials for each workload on AC power,
with streaming closed and no concurrent builds. Fresh pit controls alternate
placement around candidate batches. The three historical car controls completed
before the one-minute cap; their costs already exceed that cap, so they are
reused rather than rerun. Car savings against that older control are therefore
not contemporaneously interleaved measurements. Exact work counts and timing
variability remain separate evidence. `final_comparison.json` retains every value.

| Rate / configured steps / passes | Pit CPU median, seconds (range), full 12 s | Car CPU median, seconds (range), 10 s prefix |
|---|---:|---:|
| 60 / 4 / 8 control | 33.409 (32.175–35.467), fresh | 67.764 (65.267–72.849), historical |
| 60 / 4 / 4 | 24.465 (24.199–27.183) | 54.354 (53.174–55.724) |
| 30 / 4 / 4 | 18.160 (17.483–19.578) | 53.256 (51.517–53.758) |

Four passes reduce median pit cost by 26.8% and car cost by 19.8% against those
respective controls. Adding 30 Hz reduces pit cost another 25.8%, but car cost
only 2.0%, with overlapping car timing ranges. Its larger per-update bursts are
a real tradeoff; fewer outer updates do not imply smoother rendered frames.

| Configuration | Pit median p95 / worst update CPU, ms | Car median p95 / worst update CPU, ms | Physics-only work/budget ratio, pit / car |
|---|---:|---:|---:|
| 60 / 4 / 4 | 86.517 / 261.877 | 173.217 / 939.983 | 2.04× / 5.44× |
| 30 / 4 / 4 | 145.131 / 375.914 | 322.763 / 2,425.524 | 1.51× / 5.33× |

“Worst” is the maximum across all three full requested runs. These are physics
update CPU measurements, not rendered frame timings or FPS. Even the lower-rate
option needs about **34% less pit work and 81% less car work** merely to fit elapsed
simulation time, before rendering and other systems. The 1280×720 sustained
60 FPS/p95-frame-work/backlog gates remain unverified and unmet by these diagnostics.

| Configuration | Pit maximum pair / floor overlap | Pit final RMS speed | Car maximum pair overlap | Car maximum absolute / relative beam error |
|---|---:|---:|---:|---:|
| 60 / 4 / 8 | 0.019971 / 0.023434 | 0.403878 | 0.041827 | 13.419485 / 1.472364 |
| 60 / 4 / 4 | 0.101639 / 0.045247 | 0.559138 | 0.098565 | 13.962458 / 1.132047 |
| 30 / 4 / 4 | 0.154165 / 0.125510 | 1.026129 | 0.102973 | 13.844010 / 1.257465 |

These quality observations cover the complete requested 12/10-second traces;
pair overlap is sampled after every completed substep and floor overlap after
complete ticks. No side escapes were observed. They are not continuous-path
bounds. The lower-rate pile retains more residual motion. Head-on, impact,
mass-ratio, joint, spring, soft-contact, energy and rebound fixtures retain their
recorded numerical verdicts; neither smaller beam error nor a passing penetration
check proves better overall motion. No new wall, deformation or trajectory
tolerance is invented.

Additional matched five-second car attribution runs measure soft-node centroid
distances from the parent of 0.812659 units at 60/four/four and 0.570644 at
30/four/four. These are endpoint differences, not maximum path errors. The
original jump-force lifetime limitation is preserved throughout. Full 60-second
car behavior and motion/window-switch review remain Goal 17 work.

**Recommendation:** use 60/four/four as the balanced configuration to carry
forward for user selection. Consider 30/four/four as a pit-focused game option
when its additional overlap, residual motion and update latency are acceptable.
Do not adopt the failed two-pass, two-substep combined, 20 Hz combined or elastic
cadence candidates by relabeling their failures. Retain existing discovery,
adaptive, convergence and awake policies: their tested alternatives establish
no useful combined gain. No production setting changes in this goal. The engine's
one-substep/eight-pass defaults remain distinct from the authored benchmark.

## Exclusive attribution and remaining bottlenecks

The source instrumentation passes exact state/work comparisons against the
matching ordinary traces: full 12-second pits and matching five-second car
prefixes for the parent and both finalists. Every source partition conserves its
enclosing time. Car attribution deliberately uses five seconds to fit the
instrumented diagnostic under the wall cap; it does not replace the completed
ten-second clean comparison or establish full-car bounds.

At 60/four/four, the source partitions expose the following costs. All numbers
are instrumented milliseconds; they are not estimates of removable clean CPU.

| Source boundary | Pit, 12 simulated seconds | Car, 5 simulated seconds |
|---|---:|---:|
| Rigid gathering | 11,002.2 | 9,071.2 |
| Soft gathering | 9.1 | 4,196.9 |
| Rigid sweeps | 6,378.1 | 5,149.6 |
| Soft sweeps | 0.1 | 2,719.7 |

Within rigid gathering, circle–circle overlap geometry costs only 59.1/16.3 ms
(pit/car), while query traversal alone costs 4,719.5/2,954.1 ms. Mixed geometry
costs 192.3/785.9 ms. Within rigid sweeps, tree traversal costs 3,048.7/2,142.4 ms,
versus particle-pair geometry at 443.6/140.5 ms. These nested numbers subdivide
the table; **do not add them again**. Geometry classification follows particle
components, while response accounting separately identifies nonrotating standalone
and node-involving pairs.

Hitbox-helper work outside those sweep/gather roots costs another 6,909.4/3,070.7
ms, already included in the full pipeline's response/integration/finalization
and other stages. It is not an extra cost to add to whole-update totals. The
instrumentation records 54.5/40.8 million such helper requests and 30.4/14.6
million cache-miss refresh paths; some car requests reject entities without a
hitbox before either cache counter. Rigid sweeps themselves record zero pose-cache
hits in these runs. This supports the existing Goal 16 investigation of deferred
geometry and cache scope, without claiming every refresh can safely be removed.

The practical conclusion is to target repeated discovery, geometry maintenance
and global stepping as well as response convergence. Faster circle overlap math
alone cannot remove the measured gap. Joint and soft solving still deserve their
planned analysis, but their direct solve cost is a smaller part of the car total.
The ranked future register below separates measured exposure from untested savings.

## Platform and safety verification

All 24 configured ASan/UBSan regression batches pass (391 test invocations,
not 391 distinct tests), across the particle-budget, cadence, allowance,
beam/floor, stricter-trigger, elastic, early-exit, sleeping and unchanged-engine
configuration SDK variants. Their independent Windows cross-builds also pass;
native Windows execution is not claimed. Linux workload and fixture runs use
independent installed-SDK builds launched outside their source directory.

Supplementary sanitized examples use explicit four-second pit and one-second
car safety prefixes, excluded from performance recommendations. Seven pit
prefixes and all nine car prefixes finish. Cadence and elastic pit prefixes stop
at 3.983/4 seconds under the wall cap and remain incomplete, with no reported
sanitizer finding before interruption. They are not retried. Focused tests for
those variants pass. Numerical stress failures described above remain failures;
sanitizer success does not change their numerical verdicts.

The attribution instrumentation also passes its ASan/UBSan accounting checks
and Windows cross-build. Separate independent Linux/Windows builds cover the
30 Hz normal/delayed validation; all 128 additional endpoints pass. Native
Windows execution remains untested.

## Existing game controls

| Lever | Current control | Scope and verified boundary |
|---|---|---|
| Configured substeps | `rohr_physics_substeps_set/get` | Global positive integer; change between complete updates. Engine default is one; this benchmark authors four. |
| Global passes | `rohr_physics_solver_iterations_set/get` | Global positive integer; change between complete updates. Engine default is eight. |
| Tick duration | `rohr_physics_dt_per_tick_set/get`, `rohr_physics_engine_time_per_tick_use`, explicit-delta updates | Duration per update; the caller separately controls scheduling frequency. |
| Particle-only passes | No shipped separate control | Sandbox experiment only; other constraints retain their configured rounds. |
| Discovery cadence, adaptive allowances, elastic refresh, convergence, sleeping | No shipped controls for these prototypes | Setup-only experiments; proposals and invalidation requirements are listed below, without adoption. |

`runtime_overrides` changes 60 Hz/four substeps/eight passes to 30 Hz/one/two and
back between complete updates. It verifies three simulated seconds, 510 actual
steps, the expected velocities under a persistent caller-owned force, explicit
force removal, returning to engine-clock dt, and defaults after engine restart.
This establishes the tested existing-control combination, not arbitrary mutation
inside a solve or thread safety. Prototype environment options are setup controls;
their existence does not establish a runtime-safe public API.

The additional `runtime_selected_controls` check changes between the shortlisted
settings (60 Hz/two or four substeps/four passes and 30/20 Hz/four/four), then
restores 60 Hz/four/eight. It verifies five simulated seconds and 800 integration
steps under a persistent caller-owned force, explicit force removal and defaults
after restart. This tests live API transitions between complete updates; it does
not make the numerically failed 20 Hz or two-substep settings acceptable.

The missing controls below are proposals for a separately selected implementation,
not exported APIs. Keep the existing behavior as their default. None has enough
workload evidence here to justify adding a public control now.

| Potential control | Units, tested range and scope | Change boundary and required handling |
|---|---|---|
| Particle response budget | Global integer rounds, 2/4/up to the total budget; default follows total rounds. Nonrotating pairs only, including eligible soft nodes. | Setup only in this prototype. A future between-update setter must recompute round placement/correction fractions; never exceed the total budget or reuse a pending round schedule. |
| Rigid/soft rediscovery cadence | Separate global enum: every round, every second round, initial/final; default every round. | Setup only. Finish pending reconciliation/publication before changing; invalidate gathered eligibility and preserve manual gathering. |
| Supported contact-motion allowance | Global dimensionless multiplier per PP/PR/RR/soft class: 1/2/4, default 1. First crossings retain protection. | Setup only. Invalidate cached selector bounds on change; unsupported geometry keeps the existing fallback. This is unrelated to penetration review thresholds. |
| Conservative adaptive rejection | Global enum: existing policy, separation proof, separation plus bounded closing; default existing policy. | Setup only. Motion/shape/filter changes invalidate any retained proof. These prototypes recompute proofs and establish no useful work reduction. |
| Elastic response cadence | Global integer maximum 1/2/4 elastic steps, default 1; actual refresh may happen sooner. | Setup only. Flush an accumulated interval before changing; never defer force/integration or lose the remainder. Reduced settings failed stress penetration. |
| Solver early exit | Global normalized correction/residual thresholds 1e-4/1e-3; default disabled; minimum two qualifying rounds plus confirmation within the original budget. | Setup only. Reset qualification and rebound targets after settings/contact changes; unsupported constraints retain their original solve. |
| Particle-group sleeping | Global speed in units/s (0.01/0.05), quiet time in seconds (0.5/1), normalized residual 1e-4; default disabled. | Setup only. A future live change must wake groups and clear support/neighbor/identity snapshots; changes to loads, poses, topology, geometry, filters and timing already require wake validation. |

The beam/floor certificate is an internal algorithm experiment, not a proposed
game-facing quality slider. Its coupled car behavior rules out adoption here.
No prototype promises safe mutation during a solve or across threads. Shipped
substep/pass setters remain positive-integer controls; tested values are a
recommendation envelope, not an invented new API limit. Tick rate remains owned
by the caller's scheduler together with dt; changing dt alone changes simulation
speed. No automatic quality controller or editor UI is added.

## Ranked discovery for a future goal

These are observations and source-supported hypotheses, not measured additional
savings. The detailed evidence, risks, API implications, dependencies, effort and
verification needs are in `future_factors.json`; estimates overlap and must not
be summed.

1. **Retain support impulses across substeps.** Improve convergence so fewer
   rounds can preserve pile/support behavior. Contact identity, feature matching,
   changing dt and stale impulses make energy/lifecycle checks essential.
2. **Separate velocity, position, friction and restitution budgets.** Determine
   which terms still need work when fewer whole passes fail impacts or soft
   contacts. Changed normals and correction scheduling are the main risks.
3. **Alternative elastic integration or local scheduling.** Target car-wide
   repetition. Reduced configured steps and deferred response do not solve its
   stability/coupling problem; an alternative method needs its own numerical study.
4. **Compact particle state and defer unused derived geometry.** Target remaining
   per-visit cost, with per-consumer invalidation and manual/lifetime safety.
   Deferred geometry already fits Goal 16.
5. **Contact ordering or particle-group block response.** Potentially improve
   convergence per round; measure assembly cost and creation-order sensitivity.
6. **Bound configured step duration, rather than only fixing a count.** The
   measured 60 Hz/two-step and 30 Hz/four-step equivalence suggests a future
   maximum-interval policy could retain a 60 Hz outer update while adding steps
   during delays. The two-step stack failures make the delayed-update check
   essential. This is an untested scheduling hypothesis, not a new setting or
   permission to truncate elapsed time; it could fit Goal 16 if separately selected.
7. **Scale-aware convergence thresholds.** The tested early-exit prototype uses
   a global sum of absolute committed changes, which becomes stricter as group
   size grows. Zero qualifying pit rounds do not rule out per-body or per-island
   accumulation. A future comparison must retain noncancelling accumulation,
   physical residual checks, new-contact handling and full-correction confirmation;
   no additional norm experiment is run here.
8. **Cheaper execution of nonlimiting adaptive checks.** Preserve the decisions.
   Looser allowances and the stricter risk prototypes do not establish useful
   workload subdivision savings.
9. **Broader sleeping.** Defer: no pit particles qualify at either tested preset.
   Broader supported body types alone would not fix the measured lack of quiet
   state; first justify it with settling or a different supported workload.
10. **Accelerated returning paths for ordinary rotatable rigid bodies.** A separate
    correctness investigation before broader rigid shortcuts. The measured selector
    returns the full interval in the focused returning-path scene; an end-to-end
    continuous-path failure has not been established. This is not a saving claim.

Recommended future goal, **proposal only**: investigate convergence per solve
using retained support state, then separate response-term budgets, then ordering
only if residual evidence warrants it. Keep isolated controls before bounded
combinations; measure assembly/cache maintenance and complete-update cost; verify
impacts, friction, mass ratios, energy, changing dt and entity lifetime. Compare
the unchanged pit/car with the selected Goal 13 settings and existing 0.5/0.1
review criteria. Keep API/default adoption separate. This proposal is not added
to the milestone and does not start or reorder Goals 14–18.
