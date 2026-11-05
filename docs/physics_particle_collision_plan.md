# Milestone 12 Goal 14 — Cheaper particle and node collision work

**Status: in progress; implementation authorized.** The user selected this as
the next goal after the completed [Goal 13 experiments](physics_quality_experiments.md).
Former Goals 14–18 become 15–19. Completed goals and evidence retain their
numbers. [NEXT_STEPS.md](../NEXT_STEPS.md#12-real-time-physics-pipeline-rework)
remains authoritative.

Reduce repeated searches, geometry maintenance and response work for standalone
particles and soft-body nodes while retaining the current force/structural model.
The user's priorities are reliable particle–rigid blocking, especially preventing
obvious passage through thick bodies, and accurate rigid–rigid interactions.
Particle bounce, friction, settling and contact precision may be approximated in
measured candidates. Applying the cheap policy to nodes is explicitly allowed;
node deformation and support changes must be measured alongside particle errors.

The user subsequently instructed pursuing this goal and explicitly permits
numerical test failures during rework. Record each failed criterion, measured
value, threshold, excess and supported explanation as work proceeds; distinguish
established causes from hypotheses. Do not change a failing verdict or treat
invalid state, safety failures or capped runs as completed numerical evidence.

The original insertion was documentation only. Elastic stepping that repeats world-wide
work will be discussed separately; it is not an additional implementation task in
this goal. That separate read-only analysis need not await a new goal, but tracked
code changes still require an explicitly approved goal.

## Boundaries and quality policy

- Cover standalone–standalone, standalone–node and node–node contacts, plus
  standalone/node contacts with static and moving rigid bodies. Preserve existing
  collision filters. Explicitly classify ordinary rotatable rigid bodies that
  also have particle properties; a component tag alone must not silently move
  them out of the rigid-quality policy.
- Retain spring/beam forces, structural constraints, authored masses, geometry,
  materials and body counts. No replacement car chassis, elastic integrator or
  elastic scheduling redesign. Preserve beam collision and self-collision
  eligibility; node blocking alone does not establish beam/seam coverage.
- Keep rigid–rigid detection protection and solver quality independently intact.
  Cheaper particle contacts can change mixed-scene impulses and trajectories;
  distinguish that coupling from degradation of the rigid-only algorithm.
- Retain particle–rigid path/crossing protection, including moving rigid bodies.
  Detecting a hit is insufficient: response must also prevent the remaining
  movement from carrying a particle through the body. Thin barriers remain
  regression coverage even though thick-body passage is the user's main concern.
- Keep the 0.5-unit particle-pair pit/head-on and 0.1-unit extreme-fixture review
  thresholds. They are review criteria, not solver slack. Keep numerical failures
  visible and present any proposed acceptance change for explicit user selection.
  No generic particle–wall pit tolerance is invented. High-speed particle–particle
  protection may be weakened only as an identified experiment; retain the
  1,000-units/s head-on test and do not relabel missed collisions as passes.
- Preserve APIs, force ownership, entity lifetime, standalone-stage composition,
  truthful interaction publication and all elapsed simulation time. Runtime
  candidates stay at 60 Hz or below. Record controls available through existing
  APIs separately from internal experimental settings; new APIs need a decision.

## Ordered implementation and investigation checklist

1. [ ] **Pin the parent and establish matched controls.** Preserve sources,
   executables and configurations under `sandbox/milestone_12/goal_14/`. Compare
   against the immediate parent and frozen `6f283268` reference, excluding `main`.
   Use Goal 13's 60 Hz/four-substep/four-pass balanced candidate as the initial
   matched configuration, recording its distinction from production defaults.
   Separate particle/node pair types, shared discovery, geometry, sweeps and
   response costs. Count searches, candidate reuse, refreshes, solver rounds and
   actual subdivisions; measure clean complete-update CPU separately.
2. [ ] **Reuse conservative particle/node candidates.** Search a movement envelope
   and reuse its candidate set while valid. Account for motion of both endpoints,
   acceleration, solver/joint/soft corrections, changes in requested duration,
   geometry, radius, filters and entity deletion/reuse. Refresh or fall back before
   leaving the envelope. Use current geometry for contact checks; discover pairs
   created by corrections and bodies entering a cached region. Measure envelope
   maintenance and candidate growth, not only skipped queries. Investigate
   circle-only preparation for consumers that do not require polygon geometry.
3. [ ] **Preserve particle–rigid blocking with less repeated work.** Reuse valid
   candidates for path checks and test local impact handling where feasible.
   Cover impacts partway through an interval, remaining-time consumption, moving
   and rotating rigid bodies, multiple hits and particles/nodes initially inside
   a collider. Retain the existing protected fallback for unsupported cases.
   Do not implement local elastic integration or a new world scheduler here;
   record that dependency if local impact handling requires one.
4. [ ] **Separate particle response budgets.** Evaluate fewer particle–particle
   and particle–rigid response passes, including node contacts, while retaining
   rigid–rigid response quality and structural constraint work. Keep positional
   blocking distinguishable from bounce/friction refinement. Measure overlap,
   energy, rebound, friction, pile settling, mass ratios, deformation and support.
   Preserve sequential shared-entity effects; no unproven parallel response.
5. [ ] **Reduce particle–particle discovery/response cadence.** Test bounded reuse
   and reduced cadence across all standalone/node combinations, preserving
   filtering, current-geometry checks and explicit invalidation. Isolate any
   reduced continuous-protection candidate from the protected control and report
   head-on/crossing failures. Do not weaken particle–rigid or rigid–rigid protection
   as a side effect, or delete self-collision coverage silently.
   The user explicitly selected an additional isolated case: disable
   particle–particle sweeps and their adaptive subdivisions, including node
   pairs, while retaining four configured substeps and particle–rigid/rigid–rigid
   protection. This does not remove elastic stability restrictions or adopt
   the discrete candidate without measured review.
6. [ ] **Combine useful changes and verify the result.** Compare isolated changes
   before the combined candidate. Require repeatable complete-workload savings
   for a selected optimization; retain measured no-change findings when a rewrite
   does not justify its maintenance. Report both benefits and regressions rather
   than promising a particular speedup. Provide one reviewable implementation,
   settings recommendation, evidence report and commit boundary; do not advance
   to Goal 15 automatically.

## Verification and performance reporting

Use the unchanged full 12-second pit and declared ten-second car prefix for
matched diagnostics. Each test execution is capped at one minute of wall time,
including shutdown. Preserve capped runs as incomplete without changing their
simulated endpoint or repeating the same known-slow candidate. A shorter focused
test must have its own declared endpoint and cannot replace workload acceptance.
Continue to declared endpoints after numerical failures unless state becomes
invalid, time cannot advance, a fatal error occurs or the wall cap interrupts.

Run the relevant Milestone 6 Goals 1–5 and particle regressions. Add focused
coverage for candidate-envelope invalidation, correction-created contacts,
generation-safe deletion/reuse, manual/full-pipeline composition and thick-body
crossing by both standalone particles and nodes. Include high-speed, oblique,
moving-body and delayed-update cases; inspect crossing/contact response as well
as sampled overlap, since endpoint overlap alone can miss pass-through.
Check node support, beam/seam penetration and rigid-only control scenes.
Require relevant ASan/UBSan checks, Linux execution, Windows cross-builds and
independent installed-SDK examples; distinguish native execution from cross-builds.

Final clean comparisons use three serial runs per workload/version on AC power,
with streaming closed and no concurrent builds. Report exclusive instrumented
costs separately from clean CPU measurements, with full-run maxima and p95 spikes.
The requested **2× physics-throughput direction** means at most six CPU seconds
for the 12-second pit and five for the ten-second car prefix, before rendering.
These are diagnostic targets, not rendered FPS or a promise this goal alone will
meet them. Track the remaining gap explicitly. The unchanged rendered contract,
full 60-second car and motion/window-switch acceptance belong to Goal 18;
reference tradeoff and final tolerance selection belong to Goal 19.

This goal does not adopt the separate global elastic-stepping redesign. If that
remains the dominant cost, quantify it and report the limitation for the user's
separate discussion and decision.

## Historical car diagnostic checkpoint

At the user's request, compare the pre-roadmap engine `a8a7940a` (the parent of
`27e49645`) with current parent `0b254c04`. The user confirmed the historical
playable car matches the remembered behavior. This additional comparison does
not replace the frozen `6f283268` reference or introduce a comparison with `main`.
Goal 14 remains in progress; no historical physics implementation is adopted.

Each version uses its own example and independent installed Release SDK, at
60 Hz, four configured substeps and four solver passes. Both ten-second scripts
idle for two seconds and drive right for eight, without jumping, debug drawing
or intentional slow motion. Three serial windowless clean runs per version
completed on AC power with no concurrent builds; the user's streaming-closed
confirmation was retained. Median physics CPU is **7.178 seconds historical
versus 48.456 seconds current**, or **11.964 versus 80.760 ms per update**.
Median per-run physics CPU p95 is 16.947 versus 153.275 ms.

One separate 1280×720 rendered diagnostic per version also completed: historical
elapsed time is 10.049 seconds with 17.127 ms p95 measured frame work; current is
49.373 seconds with 156.756 ms p95. Pacing is excluded from frame work. The old
version is approximately real-time in this run but exceeds the 16.67 ms p95
criterion. These single ten-second runs do not satisfy final rendered acceptance.
All accepted executions finished within the one-minute cap.

Separate exclusive profiles reveal **2,400 versus 25,099 executed substeps**,
**2,400 versus 225,878 rigid gathers**, and **2,400 versus 125,495 soft gathers**.
Historical discovery runs once per step; current response also refreshes contacts
inside solver rounds. Gathering totals 1.982 versus 22.127 seconds, current
adaptive sweep/step selection costs 10.235 seconds, and rigid response totals
3.641 versus 12.183 seconds. Exclusive sums reproduce each profiled root exactly;
these instrumented wall times are separate from clean CPU timings. The counts
include early exits and do not mean that every gather rebuilds a tree.

The versions have different soft boundary/beam models, protection and resulting
trajectories. This establishes a large whole-version cost difference, not an
isolated causal particle benchmark or equivalent accuracy. The strongest lead is
the frequency of global steps and contact rediscovery. The historical schedule
may inform a subsequent measured experiment; reverting protection or changing
elastic scheduling is not an outcome of this diagnostic.

Evidence and runnable copies are under
`sandbox/milestone_12/goal_14/history_compare/`, including
`comparison_report.txt`, `summary.json`, per-frame CSVs, exclusive profiles,
source snapshots and hashes. Initial harness timer/torque-lifetime mistakes and
interrupted trials remain explicitly excluded in `measurement_corrections.json`.
Within each version, accepted clean/profile/rendered runs preserve the sampled
chassis trajectory and repeat reported work counts. Linux installed-SDK builds
and execution pass; this diagnostic does not requalify penetration, energy,
sanitizers or Windows support, and does not complete the goal's verification.
