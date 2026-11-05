# Milestone 12 Goal 13 — Experiment with physics work and quality

**Status: complete; awaiting user review/commit.** The user authorized execution
of this detailed plan. All eight screens, bounded combinations and verification
are recorded in the [experiment report](physics_quality_experiments.md).
Parent: `db1d52cb`; frozen reference and production behavior remain unchanged.
The [experiment record](physics_quality_experiments.md) contains completed
checkpoints and explicitly incomplete runs.
This is one goal with eight ordered experimental
sub-goals, followed by a combined comparison and one review/commit boundary.
At this plan's completion, former Goals 13–17 had moved to 14–18.
The user subsequently inserted [cheaper particle/node collision work](physics_particle_collision_plan.md)
as the new Goal 14; implementation has not started. Future-goal references in
this completed plan retain their historical numbers: Goals 14–18 now map to
15–19. Completed goals and their evidence directories keep their numbers.

Find how much repeated physics work can be removed and measure the resulting
accuracy and behavior tradeoffs in the unchanged car and 800-particle pit.
Numerical differences are expected in experiments; no candidate is adopted and
no tolerance is changed automatically. Completing this investigation does not
require reaching real time, but does require testing every lever and reporting
its result, including unsuccessful candidates. Do not substitute another paper
analysis for executable experiments.

## Baseline and scope

- Pin the starting commit, sources, binaries and configuration when starting.
  Compare each isolated lever with that same parent at matching workload duration;
  preserve `physics-pipelines-overoptimized` at
  `6f283268aa4bf038c4ed78271baeb9d5986bd7d9` as the frozen reference. Exclude `main`.
- Start from the current diagnostic baseline: 60 Hz, four configured substeps,
  eight solver passes, original correction settings and existing sweep protection.
  The no-streaming Goal 12 diagnostics are reusable evidence only after provenance
  checks: pit median 30.172 seconds of physics work for 12 simulated seconds;
  car median 61.808 seconds for the authorized 10-second prefix. These are
  headless costs, not rendered acceptance or full-car bounds.
- Keep geometry, body counts, materials, masses, initial conditions and input
  timelines unchanged. Use the full 12-second pit and the previously authorized
  10-second car prefix. At lower tick rates, use equivalent simulated durations
  and sample the existing inputs by simulation time, not a fixed 600 updates.
  Preserve and report input/force-lifetime limitations; do not silently fix the
  authored fixture during comparison. Full-car/rendered acceptance remains Goal 17.
- **User-directed run cap:** limit each test execution to 60 seconds of wall
  time, including shutdown. Reserve the final second for an interrupt, then
  terminate at the deadline if needed. Keep the 10/12-second simulated targets;
  capped runs are incomplete, with partial evidence preserved and the same slow
  candidate's clean repeat skipped. This also applies to stress-fixture executables;
  their declared scenario endpoints remain unchanged. Earlier completed runs retain their original
  provenance. This supersedes completing every workload endpoint when the cap
  intervenes; it does not turn partial error measurements into full-run bounds.
- Keep experimental implementations isolated and explicitly configured. Numerical
  prototypes within these levers are in scope; public API redesign, new dependencies
  and production adoption are not. Preserve manual-stage behavior, ownership,
  lifecycle safety and truthful reporting. Read applicable subsystem instructions
  before implementation. Store sources, executables and evidence under
  `sandbox/milestone_12/goal_13/`, with separate sub-goal and candidate directories.

## Ordered experimental sub-goals

1. [x] **Configured substeps: 4 / 2 / 1.** Hold 60 Hz and eight passes fixed.
   Retain all existing adaptive selectors for this isolated comparison. Count
   actual subdivisions and their causes: adaptive work may replace removed
   configured steps. Measure penetration, impacts, spring/beam behavior and cost.
   Screening is complete: both reduced settings improve pit cost; two substeps
   regress car cost and one becomes pathologically slow. The user stopped the
   latter at 7.45/10 seconds and skipped its clean repeat. All 224 stress/head-on
   scenarios finish at each setting; the four- and two-substep stack behavior
   failures remain visible. Evidence: `sandbox/milestone_12/goal_13/runs/`.
2. [x] **Particle–particle passes: 8 / 4 / 2.** Hold other constraints at eight
   passes and retain four configured substeps at 60 Hz. Define particle eligibility
   explicitly, including soft-node participation versus ordinary rotating bodies
   carrying particle data. Record pass placement and any correction-fraction
   scheduling needed to avoid accidentally reducing the intended total correction.
   Keep correction slop/caps unchanged. Measure skipped-pass dispatch, contact
   traversal and geometry overhead as well as actual response calls; do not claim
   proportional savings from skipped impulses alone. Review pile compression,
   settling, rebound, friction/support, momentum and car coupling.
   Screening is complete: pit clean costs are 29.295/19.762 seconds at four/two
   particle passes, with maximum overlap 0.065548/0.234337. Both car candidates
   hit the wall cap and remain incomplete. Two passes fail one soft-body stress
   penetration check at 0.125231; four do not add this failure. Disabled full-pit
   and five-second car control traces match the baseline exactly. Final platform
   and sanitizer verification is recorded in the completed experiment report.
3. [x] **Global solver passes: 8 / 4 / 2.** Restore the baseline particle policy
   and vary the overall pass count independently at 60 Hz/four substeps. Include
   joints, locks and soft contacts in work/error accounting. Review mass ratios,
   stacks, pin/weld error, deformation and energy in addition to penetration.
   Both workloads finish at four/two passes. Clean pit costs are 25.558/17.070
   seconds and car-prefix costs 55.738/42.296. Four passes pass all configured
   stress/head-on scenarios and rebound checks. Two retain a stack failure and
   introduce pin-impact penetration up to 0.423089 and soft-body penetration up
   to 0.133799, above 0.1; retain those as failed numerical candidates.
4. [x] **Physics tick rate: 60 / 30 / 20 Hz.** First isolate tick rate with four
   configured substeps/eight passes; then reuse promising settings in the combined
   comparison. Keep all elapsed simulation time and existing force ownership.
   Report work per simulated second, actual step durations and input/trajectory
   error. A 30 Hz tick has a 33.33 ms physics interval but rendered 60 FPS still has
   a 16.67 ms frame budget; interpolation does not repair collision accuracy.
   Both lower-rate pits finish; clean costs are 26.296/24.601 seconds at 30/20 Hz,
   with maximum particle overlap 0.159280/0.172917. Both lower-rate cars hit the
   wall cap and remain incomplete. The additional 20 Hz normal/delayed fixtures
   pass 124 timed/head-on scenarios and four rebound state checks. The inherited
   drop-fixture rate guard was extended only in its sandbox copy; its rejected
   setup attempt is preserved separately.
5. [x] **Contact rediscovery cadence.** Compare gathering every pass, every second
   pass, and only at the initial/final solve boundaries. Define final-boundary
   handling of newly discovered contacts and include any required response work
   in timing. Existing contacts still use current geometry; do not freeze hitboxes.
   Separate rigid and soft cadence where required to attribute effects. Test
   correction-created contacts, missed/transient overlaps, penetration, tunneling
   and callback/publication semantics. Unsafe schedules remain failed candidates.
   All six schedules finish the pit and stress fixtures, retaining one baseline
   stack-behavior failure each. All car trials hit the wall cap. Reconciliation
   and delayed response remain measurable costs; no reliable speed benefit is
   established. Retain the original cadence for combinations. Disabled control
   traces and 14 Linux regressions pass; final platform checks are recorded in the report.
6. [x] **Adaptive collision restrictions.** Prototype more selective restrictions
   using relative closing motion, relevant feature size and current contact state.
   Predeclare numeric trigger/margin candidates and fallback rules; record each
   selector's effective durations and downstream work. Isolate particle–particle,
   particle–rigid, rigid–rigid and soft-beam changes. Keep high-speed detection and
   the 1,000-units/s-per-particle head-on test in every applicable review; also cover
   thin barriers, rotating/grazing/accelerating motion and initial overlaps.
   Test the known beam sweep/response mismatch where relevant, retaining uncertain
   geometry fallbacks. Do not truncate time or silently cap the subdivision loop.
   A changed collision margin is an experimental setting, not the penetration
   review threshold. Production retains existing sweep protection throughout.
   The user additionally requests stricter **triggers**, meaning stronger
   conservative evidence of collision risk before subdivision, not tighter
   penetration tolerances. Separately test signed accelerated-motion bounds
   that prove separation or bound additional closing at an existing contact.
   Compare separation-only rejection with existing-contact rejection; preserve
   first-crossing/grazing protection and fall back for unsupported geometry,
   rotation, locks, nonfinite bounds or uncertain motion. Report proof overhead,
   accepted/rejected checks and actual subdivisions removed.
   Screening is complete: eight allowance settings, the separate beam/floor
   certificate and both stricter-trigger settings. None establishes a useful
   both-workload saving. All stress endpoints finish with only the baseline
   stack failure; car runs hit the wall cap. The floor candidate increases car
   work/deformation, and stricter triggers leave pit work unchanged and slightly
   increase matched car-prefix work. Retain the original policy for combinations;
   platform and sanitizer verification is recorded in the completed report.
7. [x] **Soft-body integration versus collision cadence.** Keep the existing small
   elastic integration steps and compare collision/constraint refresh every one,
   two and four such steps, with explicit handling of remainders and conservative
   collision-triggered fallback. Measure actual calls avoided and fallback cost;
   include spring force timing, node/frame/lock coupling, rigid impulses, seam
   penetration, deformation and energy. The pit is a control for unintended global
   effects. This tests cadence separation, not an unapproved replacement integrator.
   Screening is complete. Both schedules preserve pit state/work and cap the car,
   with more actual car steps despite skipped responses. Extra proof sweeps are
   included. Two/four-step cadence adds three/five soft-body penetration failures,
   up to 0.267437 against 0.1. Retain original response cadence for combinations.
   All stress endpoints finish and 16 regressions pass at each schedule;
   final platform/sanitizer verification is recorded in the completed report.
8. [x] **Convergence and sleeping.** Run distinct early-exit and sleeping prototypes
   before combining them. Collect residual/correction evidence within existing
   solves where possible; compare explicit thresholds and minimum/maximum pass
   counts, including their measurement overhead. Account for changing correction
   fractions, new contacts, joint/soft coupling and nonmonotonic convergence; an
   unchanged pass alone is insufficient. Separately compare explicit sleep-speed,
   residual and quiet-time thresholds, with waking through contacts, joints,
   beams and frames after impacts, loads, driven motion, edits or teleports.
   Include disturbed piles and entity deletion/reuse. Record jitter, drift,
   delayed reactions, missed wakes and net work saved, including wake maintenance.
   Screening is complete: neither early-exit threshold removes a pit round and
   neither sleep preset puts a pit particle to sleep. Both add assessment work;
   the unsupported car retains exact parent-prefix behavior and hits the cap.
   All four settings finish stress endpoints with the baseline stack failure.
   Focused wake/edit/manual/lifecycle cases pass. Retain original policies;
   final sanitizer/platform verification is recorded in the completed report.

## Combined comparison and verification

Use the user-selected staged search, not an exhaustive parameter sweep. For each
sub-goal: record the hypothesis/configuration, build an isolated candidate, verify
its disabled baseline, run focused safety checks, then run the full pit and
authorized car prefix with numerical/work observation and one clean screening
trial. Report the checkpoint and continue without a separate sub-goal approval.
All eight sub-goals remain required; an unsuccessful experiment is a valid result.

After all eight isolated experiments, shortlist at most three levers for
combination testing, as directed by the user. Prioritize measured savings in both
workloads; rank using the smaller of the two relative savings so a large pit gain
cannot conceal a car regression. Separate numerical eligibility from speed.
If fewer than three levers show both-workload savings, include at most the
strongest workload-specific alternatives, explicitly labeled as such; incomplete
car runs do not establish a car saving. The top three are an upper bound, not a
quota. Leave combinations of other levers for the user's later decision.
Combine the shortlisted candidates incrementally.
Retain a control for each addition and explain dependencies. Avoid a full Cartesian
product; test combinations supported by the isolated findings. Count actual work
and measure complete-update cost: savings overlap and cannot be added or multiplied.
If every candidate for a lever is worse or numerically unacceptable, retain the
baseline for that lever and document the completed experiments.

Retain at most two candidates per lever: the fastest numerically eligible setting
and the least aggressive setting removing useful work. Build a preserved-behavior
track and a speed track, adding shortlisted levers in sub-goal order with a control for each
addition. Limit the combined search to sixteen configurations. Revert additions
that fail existing criteria or lose their benefit. Select at most three distinct
finalists; measure each and the baseline in three serial clean trials per workload,
alternating comparison order. Failing fast candidates remain diagnostic evidence.

### Approved detailed experimental controls

- Particle-only passes retain the established nonrotating-pair eligibility,
  including eligible soft nodes; report standalone and node-involving work
  separately. Distribute selected rounds evenly through all rounds, including
  the last. Preserve intended total correction using the established experimental
  fractions, not new slop or correction caps.
- Rate comparisons use 720/360/240 pit updates and 600/300/200 car-prefix updates
  at 60/30/20 Hz. Coordinate caller scheduling, dt and input time; do not change
  simulation speed by changing dt alone. Do not add rendering interpolation.
- Rediscovery candidates vary rigid and soft cadence independently before both.
  Reduced cadence includes a final discovery, one reconciliation response round,
  and a final observation/publication refresh. Count the extra work and contacts
  created by reconciliation that remain unresolved until the next substep.
- Adaptive candidates compare baseline/2x/4x allowances only for supported
  existing-contact motion restrictions, keeping first-contact/crossing protection.
  Separately prototype the known floor/endpoint beam equivalence, falling back
  for edges, corners, uncertain geometry and unsupported motion. Isolate pair
  types; do not invent absent restriction paths, disable all PP sweeps, truncate
  time or cap the subdivision loop.
- Elastic cadence batching is limited to elastic-shortened intervals where
  existing collision/joint bounds permit the accumulated interval. Refresh at
  collision restrictions, configured-step boundaries and remainders; re-evaluate
  bounds as forces and poses change. Preserve force/node/frame timing and count
  scheduling/fallback overhead.
- Early-exit prototypes use minimum two/maximum eight passes and normalized
  correction/residual thresholds 1e-4 and 1e-3. Normalize correction and residual
  travel over dt by the relevant feature size, including angular surface motion.
  Require two consecutive qualifying rounds, no new contacts and a full-correction
  confirmation round. Unsupported constraints or normalization fall back to the
  existing solve. Count observation and confirmation overhead.
- Sleeping starts with nonrotating particle contact groups. Groups connected to
  soft bodies, joints, driven bodies or unsupported force behavior stay awake.
  Compare speed/quiet-time presets 0.01 units/s for 0.5 s and 0.05 units/s for 1 s,
  requiring stable support/contact state and qualifying residuals. Wake before
  response to impacts and after loads, edits, teleports, support/lifetime changes.
  Keep wake discovery active; broader sleeping belongs to the future-factor list.

These are experimental search values, not new acceptance thresholds. Preserve
separate clean timing, exclusive attribution and numerical observation modes;
first verify instrumentation-only state/work parity against the baseline.

## Discovery for a future goal

Maintain a ranked discovery register from existing traces, measurements and source
inspection during the eight experiments. The user selected a ranked future plan,
not additional causal probes: do not add dedicated experiments or implementations
for new factors in Goal 13.

For each factor record the symptom/evidence, mechanism, potentially avoidable work,
affected workloads, numerical/behavioral risks, API implications, dependencies,
effort and verification needs. State whether it fits an existing future goal or
needs a new one. Label confidence as measured observation, source-supported
hypothesis or untested idea. Rank by measured exposure, credible removable work,
both-workload relevance, risk and effort; avoid overlapping saving estimates.

Possible leads include cross-step warm starting, solver ordering, separate
position/velocity budgets, friction/restitution cadence, local scheduling,
alternative elastic solvers, compact particle data and broader sleeping. These
are discovery prompts, not promised additions. Deliver one recommended future
experiment-goal proposal without automatically adding or starting it.

## Game configuration and runtime overrides

Recommended defaults must remain starting points that games can override, rather
than a single fixed quality policy. For each lever, record whether it already has
a public control, is controlled by the caller's update schedule, or would need a
new API. Separate experimental settings from shipped engine defaults and authored
scene overrides: the current four-substep benchmark is not the engine's default
of one substep. The existing solver default is eight iterations.

Existing controls include `rohr_physics_substeps_set/get`,
`rohr_physics_solver_iterations_set/get`, `rohr_physics_dt_per_tick_set/get`,
`rohr_physics_engine_time_per_tick_use`, and explicit-delta update entry points.
Changing simulated duration per tick alone does not change how often the caller
runs physics; tick-rate experiments must coordinate scheduling and dt to preserve
real-time simulation speed. Do not use these setters as an implicit slow-motion
optimization or silently change the timing of other systems.

For proposed new controls, recommend units/ranges, defaults, scope and supported
combinations. Specify whether changing a value is supported only during setup or
between complete updates, and what retained impulses, contacts, sleep state or
other caches must be invalidated or rescaled. Do not imply safe mutation midway
through solving or thread safety. Test live changes for any override proposed as
runtime-safe, including returning to the baseline and preserving elapsed time,
force ownership and wake behavior. Recommend only coherent, verified combinations.

The report must distinguish a possible general-purpose default from optional
game-specific quality/performance choices. This goal inventories existing APIs
and proposes missing controls; public API additions/redesign and adoption of new
defaults require separate user selection. It does not implement an automatic
quality controller or an editor/settings UI.

## Measurement record

Each candidate record includes:

- Exact configuration, code revision, input schedule, declared duration, completion
  status and reason. Numerical failures accumulate through the endpoint. Invalid
  state, fatal errors, inability to advance or interruption produce incomplete
  results, never full-run bounds.
- Total physics work and milliseconds per simulated second; mean, p95 and maximum
  update work; actual configured/adaptive steps, selector restrictions, solver
  passes, response visits, discovery calls, candidates, geometry refreshes and
  contacts. Split pair types and shared costs with exclusive timers that conserve
  the enclosing total. Keep instrumented attribution separate from clean timing.
- Full-run maximum penetration and tunneling/missed-response evidence, rebound,
  momentum/energy, support/friction, pile height/settling, joint error, beam
  deformation and car trajectory. Compare current parent and available frozen
  evidence without presenting mismatched settings as an isolated algorithm gain.

Retain **0.5 units** for pit particle-pair and dedicated head-on penetration review,
and **0.1 units** for Milestone 6 stress-fixture penetration. Neither is solver
slop. Keep existing other criteria visible and do not invent a new pit-wall or
car-deformation acceptance threshold. Numerical failure does not invalidate a
completed experiment, but cannot become an accepted configuration by relabeling.
The historical 60 Hz/one-substep/two-particle-pass pit result (0.803 overlap)
remains a failure, despite its near-real-time average cost.

Use serial clean runs on AC power with streaming closed and no concurrent builds.
Exploratory screening may use single diagnostic runs; repeat baseline and finalists
three times per workload/version before making performance recommendations. Record
conditions and variability. At lower rates, do not equate physics throughput with
rendered FPS. The final 1280×720 rendered gates remain unchanged in Goal 17.

Run Milestone 6 Goals 1–5 and relevant regressions to their declared endpoints.
Apply relevant ASan/UBSan, Linux execution, Windows cross-build and independent
installed-SDK example checks to executable prototype changes. Record numerical
failures separately from safety/platform failures; unsafe or unverified candidates
cannot be recommended. Do not rerun unchanged checks without a relevant reason.

## Deliverable and review boundary

Produce one report covering all eight sub-goals and tested combinations, with a
table of measured cost, work removed, error/behavior changes, remaining budget gap,
verification and recommended retain/reject/follow-up decisions. Distinguish measured
results from predicted savings, prefix bounds from full scenarios, and acceptable
review criteria from pending user choices. Recommend alternatives that provide
different measured balances rather than assuming minimum penetration is the only
quality objective. Include the per-lever API/override matrix and runtime-change
verification alongside proposed default and game-specific configurations, the
ranked future-factor register and one recommended follow-up goal proposal.

Stop once the single Goal 13 report and verification are complete for user review
and commit. Do not adopt a prototype, change production defaults, revise final
tolerances or start Goal 14 automatically. Goals 14–16 retain their joint, soft-body
and orchestration implementation scope; incorporate useful experiment evidence
when separately pursued. Goal 17 owns combined rendered acceptance and Goal 18
owns final reference tradeoff selection and explicit tolerance acceptance.
