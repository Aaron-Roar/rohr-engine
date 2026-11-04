# Milestone 12 Goal 13 — Experiment with physics work and quality

**Status: planned; not started.** This is one goal with eight ordered experimental
sub-goals, followed by a combined comparison and one review/commit boundary.
It is the next unfinished priority. Former Goals 13–17 move to 14–18;
completed goals and their evidence directories keep their numbers.

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
- Keep experimental implementations isolated and explicitly configured. Numerical
  prototypes within these levers are in scope; public API redesign, new dependencies
  and production adoption are not. Preserve manual-stage behavior, ownership,
  lifecycle safety and truthful reporting. Read applicable subsystem instructions
  before implementation. Store sources, executables and evidence under
  `sandbox/milestone_12/goal_13/`, with separate sub-goal and candidate directories.

## Ordered experimental sub-goals

1. [ ] **Configured substeps: 4 / 2 / 1.** Hold 60 Hz and eight passes fixed.
   Retain all existing adaptive selectors for this isolated comparison. Count
   actual subdivisions and their causes: adaptive work may replace removed
   configured steps. Measure penetration, impacts, spring/beam behavior and cost.
2. [ ] **Particle–particle passes: 8 / 4 / 2.** Hold other constraints at eight
   passes and retain four configured substeps at 60 Hz. Define particle eligibility
   explicitly, including soft-node participation versus ordinary rotating bodies
   carrying particle data. Record pass placement and any correction-fraction
   scheduling needed to avoid accidentally reducing the intended total correction.
   Keep correction slop/caps unchanged. Measure skipped-pass dispatch, contact
   traversal and geometry overhead as well as actual response calls; do not claim
   proportional savings from skipped impulses alone. Review pile compression,
   settling, rebound, friction/support, momentum and car coupling.
3. [ ] **Global solver passes: 8 / 4 / 2.** Restore the baseline particle policy
   and vary the overall pass count independently at 60 Hz/four substeps. Include
   joints, locks and soft contacts in work/error accounting. Review mass ratios,
   stacks, pin/weld error, deformation and energy in addition to penetration.
4. [ ] **Physics tick rate: 60 / 30 / 20 Hz.** First isolate tick rate with four
   configured substeps/eight passes; then reuse promising settings in the combined
   comparison. Keep all elapsed simulation time and existing force ownership.
   Report work per simulated second, actual step durations and input/trajectory
   error. A 30 Hz tick has a 33.33 ms physics interval but rendered 60 FPS still has
   a 16.67 ms frame budget; interpolation does not repair collision accuracy.
5. [ ] **Contact rediscovery cadence.** Compare gathering every pass, every second
   pass, and only at the initial/final solve boundaries. Define final-boundary
   handling of newly discovered contacts and include any required response work
   in timing. Existing contacts still use current geometry; do not freeze hitboxes.
   Separate rigid and soft cadence where required to attribute effects. Test
   correction-created contacts, missed/transient overlaps, penetration, tunneling
   and callback/publication semantics. Unsafe schedules remain failed candidates.
6. [ ] **Adaptive collision restrictions.** Prototype more selective restrictions
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
7. [ ] **Soft-body integration versus collision cadence.** Keep the existing small
   elastic integration steps and compare collision/constraint refresh every one,
   two and four such steps, with explicit handling of remainders and conservative
   collision-triggered fallback. Measure actual calls avoided and fallback cost;
   include spring force timing, node/frame/lock coupling, rigid impulses, seam
   penetration, deformation and energy. The pit is a control for unintended global
   effects. This tests cadence separation, not an unapproved replacement integrator.
8. [ ] **Convergence and sleeping.** Run distinct early-exit and sleeping prototypes
   before combining them. Collect residual/correction evidence within existing
   solves where possible; compare explicit thresholds and minimum/maximum pass
   counts, including their measurement overhead. Account for changing correction
   fractions, new contacts, joint/soft coupling and nonmonotonic convergence; an
   unchanged pass alone is insufficient. Separately compare explicit sleep-speed,
   residual and quiet-time thresholds, with waking through contacts, joints,
   beams and frames after impacts, loads, driven motion, edits or teleports.
   Include disturbed piles and entity deletion/reuse. Record jitter, drift,
   delayed reactions, missed wakes and net work saved, including wake maintenance.

## Combined comparison and verification

After all eight isolated experiments, combine promising candidates incrementally.
Retain a control for each addition and explain dependencies. Avoid a full Cartesian
product; test combinations supported by the isolated findings. Count actual work
and measure complete-update cost: savings overlap and cannot be added or multiplied.
If every candidate for a lever is worse or numerically unacceptable, retain the
baseline for that lever and document the completed experiments.

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
verification alongside proposed default and game-specific configurations.

Stop once the single Goal 13 report and verification are complete for user review
and commit. Do not adopt a prototype, change production defaults, revise final
tolerances or start Goal 14 automatically. Goals 14–16 retain their joint, soft-body
and orchestration implementation scope; incorporate useful experiment evidence
when separately pursued. Goal 17 owns combined rendered acceptance and Goal 18
owns final reference tradeoff selection and explicit tolerance acceptance.
