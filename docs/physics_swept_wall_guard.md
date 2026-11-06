# Minimal swept wall protection

This is the preserved implementation plan for Milestone 13 Goal 3. That attempt
**failed behavior review**: the user rejected sticking/lost bounce and sliding,
and unstable soft-body car behavior, then restored its tracked implementation.
It is not enabled in the current engine. Evidence remains in
`sandbox/milestone_13/goal_3/`. The later
[Goal 4a comparison](physics_collision_experiments.md) evaluates alternatives.
The [roadmap](../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline)
owns current priority and the measurement contract. The scope and ordered plan
below are historical, not authorization to resume the rejected guard.

## Fixed scope and starting configuration

- Use the retained experimental baseline at 60 Hz, four configured substeps,
  two particle–particle response passes and four particle–rigid/rigid–rigid
  passes. Include nonrotating soft-body nodes in the cheaper particle-pair policy.
  Keep sleeping disabled and production defaults unchanged. Preserve public APIs;
  the existing private experiment mechanism can supply the two-pass setting.
- Sweep actual circular collision geometry against eligible static solid rigid
  walls. A center-point ray is insufficient. Respect collision filters, sensors,
  directional locks, ownership and generation-safe entity access.
- Query conservative swept bounds and guard the earliest impact along a proposed
  displacement. Stop on the safe side and remove the inward velocity component.
  Ordinary contact response still runs; do not claim accurate impact restitution
  after this deliberately dissipative guard.
- Advance the full configured simulation interval. The cheap policy may hold a
  blocked body at its safe position for the remainder of that substep instead of
  following its remaining bounce/slide path. Record lost travel, energy changes
  and sticking as intentional tradeoffs, not dropped simulation time or backlog.
- Cover integration and subsequent solver-created displacement of eligible
  particles/nodes. Later position corrections must not bypass the guard. Check
  how this composes with beams, joints and transform locks rather than silently
  overriding their ownership or enabling global rediscovery.
- Do not restore world-wide adaptive subdivision, particle–particle sweeps,
  repeated global contact gathering or a full time-of-impact event solver.
  Moving/rotating rigid obstacles, polygon–polygon swept response and continuous
  beam-edge protection remain Goal 4 or later work. Static walls may be placed
  at any fixed orientation. Explicitly document supported geometry and exclusions;
  unsupported cases are not protected merely because the guard exists.

## Ordered implementation and measurement plan

1. **Freeze the comparison.** Pin the exact implementation parent, source patch,
   settings, SDKs and executable hashes under `sandbox/milestone_13/goal_3/`.
   Goal 2's retained patch is committed at `2a6a116f`; record any subsequent
   documentation commit as part of the actual parent. Preserve Goal 2's artifacts.
   Build matched guard-disabled/enabled candidates
   with the same two-pass policy; do not attribute its existing savings to the
   guard. The reviewed two-pass median is 2.609 CPU seconds per ten-second car
   and 5.533 per twelve-second pit. The latter leaves about 0.65 ms per 60 Hz
   frame before the 2× average physics target, not a measured guard budget.
2. **Audit and explain the insertion points.** Identify proposed integration
   movement, every relevant position-correction commit, geometry consumers,
   broadphase query availability and manual-stage boundaries. Reuse suitable
   existing circle/wall geometry helpers without invoking global duration
   selection. Explain the implementation before editing physics code. Design
   conservative candidate bounds; any early rejection must account for the
   entire proposed displacement, radius and wall geometry, not a guessed speed
   threshold. Account for query/cache upkeep and capacity fallbacks explicitly.
3. **Implement the bounded guard.** Restrict candidate queries to relevant static
   walls and clip to the earliest valid hit with a documented numerical margin.
   Apply the same protection to correction-created movement. Define tangent,
   zero-motion, initially touching and initially embedded handling; permit safe
   separation rather than trapping an embedded body. Multiple-wall and corner
   handling must remain safe if unused travel is held. Do not add hidden
   allocation or unbounded impact loops. Preserve composed/manual and public/
   direct behavior at the documented stage boundaries, current geometry for
   consumers, accurate contact publication and truthful reports.
4. **Verify the protected cases and inventory the rest.** Test particles and
   nodes against thin/thick static walls at fixed orientations, normal/oblique/
   grazing approaches, both directions, corners and multiple obstacles. Include
   1,000 and 10,000 units/s, delayed updates, variable radii, large coordinates,
   initial touching/embedding, filtered/sensor walls, wall edit/deletion/reuse,
   and correction-created crossings. Use crossing/side tests as well as full-run
   maximum penetration; an object emerging with zero overlap is still a failure.
   Retain 0.1 stress penetration checks and 0.5 particle-pair review, without
   changing solver slack. Keep the head-on, rigid, joint and soft-body failures
   visible with values, limits and causes. Run relevant Linux regressions,
   ASan/UBSan, Windows cross-builds and independent installed-SDK examples.
5. **Measure the addition on both workloads.** Keep scene geometry, counts,
   materials and input sequences unchanged. Run three serial matched clean
   trials per workload/version, plus three normal rendered trials at 1280×720
   per workload/version, on AC with streaming closed and no concurrent builds.
   Use the declared ten-second car diagnostic and twelve-second pit endpoints,
   with the existing one-minute wall cap. Preserve incomplete runs. Separately
   profile exclusive query/cast/response/correction-guard costs and counts of
   candidates, casts, hits and clamped movements. Report total physics cost,
   average/p95 frame work, rendering, pacing, FPS, simulated/elapsed ratio and
   backlog. Separate the existing pacing deficit from newly added physics work;
   graphics changes are outside this goal. Report uncapped diagnostics separately.
6. **Review protection versus cost.** Open one car/pit preview at a time and ask
   about sticking, lost bounce/slide travel, node deformation and settling.
   Compare guard-enabled/disabled error inventories and operations. Completion
   requires verified in-scope protection, mandatory safety/compatibility checks
   and complete declared measurement endpoints. Unresolved in-scope tunneling is
   a failed implementation, not an accepted quality tradeoff. Report headroom and
   rendered gates separately and preserve every miss; neither this staged goal
   nor visual acceptance establishes final milestone acceptance. If cost is too
   high, identify the measured cause and alternatives without expanding scope or
   silently disabling protection. Stop for review and the user's commit.

## Planning-time evidence and limits (historical)

The user accepted both 60 Hz/two-pass previews. Their latest single rendered runs
had car/pit average physics CPU of 4.29/7.38 ms, frame-work p95 of 6.83/12.61 ms,
and 59.11/58.60 FPS. They still missed the simulation-time drift gate. These are
guard-disabled observations, not repeat acceptance or evidence of wall safety.
At the time this plan was approved, the guard had not been implemented or timed.
Its later implementation and measurements do not override the rejection above.
Full sixty-second car acceptance and final physics-quality selection remain Goal 8.
