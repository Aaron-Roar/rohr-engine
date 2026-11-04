# Milestone 12 Goal 10 — Investigate ranked bottlenecks and solutions

**Roadmap numbering note:** this completed plan retains its historical numbering.
The later [quality/work experiment](physics_quality_experiments_plan.md) is the
new Goal 13; historical Goals 13–17 below map to current Goals 14–18. Consult
`NEXT_STEPS.md` for current execution order; evidence paths are unchanged.

**Status: complete for review.** The [investigation report](physics_bottleneck_investigation.md)
records all five causal tests and solution recommendations. The questions were followed in order,
using the completed [Goal 9 audit](physics_algorithm_audit.md) as the starting
evidence. [NEXT_STEPS.md](../NEXT_STEPS.md#12-real-time-physics-pipeline-rework)
is authoritative. Inserting this goal moves the former Goals 10–16 to 11–17;
completed goals and their evidence directories retain their numbers.

## Outcome and boundaries

Turn each ranked question into a causal finding and an evaluated solution
choice. Extend Goal 9 with focused measurements, minimal reproductions and
isolated comparisons that distinguish the competing explanations. Repeating
its cost tables or listing alternatives alone does not complete this goal.
Negative results are valid when they show why a proposed solution is ineffective
or unsafe. A speedup or production optimization is not required for completion.

Keep production behavior, public APIs, ownership, authored scenes and correction
settings unchanged. Put probes and prototypes under
`sandbox/milestone_12/goal_10/`; preserve Goal 9's sources, binaries and evidence.
Do not implement a broad scheduler, integrator or collision-system redesign
just to evaluate an idea. Use a small controlled prototype when source reasoning
and observations cannot resolve a material tradeoff. Select production changes
only after the user reviews this goal's results.

At the start, pin the current parent and check which Goal 9 evidence is reusable.
Keep the frozen reference at `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; exclude
`main`. Use 60 Hz, four configured substeps and eight solver passes as the matched
starting configuration. New runtime candidates remain at 60 Hz or below;
settings experiments must be separately identified. Preserve particle–particle
sweep protection and other collision/stability guarantees. Rejected blanket
exclusion and existing-contact-rule removal remain failure evidence.

## Ordered question checklist

1. [x] **Repeated contact discovery: what actually needs refreshing?** Trace
   correction → invalidation → collider/beam visit → candidate query → geometry
   refresh → new or retained contact. Measure changed versus visited bodies,
   correction-created contacts and initial versus solver-triggered work.
   Evaluate changed-pose worklists, incremental refits, affected-pair/beam
   refresh and conservative candidate reuse against the current design.
   Demonstrate a validity envelope and expansion/fallback behavior before
   treating cached candidates as safe. Include manual gathering, shape/filter
   changes and entity deletion/reuse. Determine whether maintenance replaces
   the supposedly removed work. Map viable changes to shared gathering
   **Goal 12**, soft consequences **Goal 14** and composition **Goal 15**.
2. [x] **Adaptive restrictions: why are steps so small, and which can be
   improved safely?** Reproduce the worst car update and identify the actual
   entities behind Goal 9's recorded pair `(350, 0)`; runtime indices alone are
   not stable identities. Record each limiting sweep branch, proxy geometry,
   separation, closing motion, acceleration/rotation bounds, discrete contact
   result, correction and actual advancement. Explain the 1,438 repeated
   restrictions: necessary collision resolution, conservative prediction,
   sweep/response mismatch, or another demonstrated cause. Separate that spike
   from the sustained 0.804 ms elastic stability cap. Evaluate selectors against
   the same proposed interval without changing the chosen duration, so removing
   one bound is not assumed to remove another. Compare tighter conservative
   bounds, targeted continuous response, local elastic integration and alternative
   spring formulations at unchanged authored stiffness/mass. Preserve accelerated,
   grazing, initially overlapping and 1,000-units/s-per-particle impacts. Do not
   clamp tiny steps or remove protection as an unexplained optimization. Map
   shared sweep work to **Goal 11**, soft numerics to **Goal 14**, and scheduling
   to **Goal 15**; an integrator redesign requires a separate user decision.
3. [x] **Response and overlap work: are later passes still useful?** Measure
   positional, normal-velocity and friction residuals, useful corrections,
   retained contact state and contact churn after each pass. Explain the dense
   particle path's missing convergence coverage and distinguish velocity changes
   from pose changes. Compare particle-specific convergence checks, solve order,
   state reuse and overlap-publication cadence; assess sleeping only with wake
   propagation through contacts, joints, beams, loads and edits. Check residual
   measurement cost against saved response and refresh work. Penetration limits
   alone cannot establish acceptable rebound, energy or support. Map composition
   to **Goal 15**, joint/soft coupling to **Goals 13–14**, and any renewed particle
   response implementation to a separately selected follow-up.
4. [x] **Unrelated bodies: how much global work is safely separable?** Measure
   dynamic dependency groups, the fraction of work outside the restricting
   group, and conservative future contact/merge boundaries in the actual car
   and pit. Shared static supports alone must not join independent dynamics;
   distinguish prescribed kinematic motion from impulse-coupled bodies.
   Compare global execution, isolated free-motion batches, local subdivision
   and local elastic scheduling. Evaluate force timing, wake/merge/split costs,
   determinism, shared-body writes, publication and entity lifetimes. Use the
   separated fast-pair and pile-perturbation controls, but do not extrapolate
   their small savings to a dense connected pile. Quantify the safely avoidable
   work before proposing a full scheduler. Map to **Goal 15**, with prerequisites
   from **Goals 11–14**.
5. [x] **Candidate growth and data access: is the bottleneck traversal,
   layout, arithmetic or dispatch?** Separate count/density effects from impact
   phase, adaptive-step count and solver repetition. Measure candidate yield,
   duplication, memory footprint and data/branch behavior. Use hardware counters
   where available; if unavailable, state that limit and use controlled layout
   or traversal comparisons without claiming measured cache misses. Compare
   retained tree topology, particle grids, compact contact records and SIMD
   batches, including setup, conversion and invalidation. Circle gathering
   already has SIMD; evaluate its remaining whole-update opportunity. For
   parallel/GPU alternatives, account for shared-entity dependencies, ordering,
   transfer, synchronization and convergence rather than assuming atomics solve
   response races. Map sweeps/gathering to **Goals 11–12** and composition to
   **Goal 15**; other implementations require selection.

The order reflects likely contribution and current evidence, not guaranteed
savings. Record pit/car differences and new findings that overturn a hypothesis.
Do not silently reorder the questions or skip one after finding a useful change.

## Evidence and decision procedure

For each question, retain a record of the current design's benefit and cost,
the leading hypothesis and a disproof test, measured results, and alternatives
including retaining the current algorithm. Complete at least one focused causal
test per question; reuse raw observations where they answer it directly. Label
untested alternatives and explain why their additional prototype cost or risk
does not yet justify building them. End each record with a recommendation,
confidence, expected effort, dependencies and concrete acceptance checks.

Compare exclusive subsystem time, total ms per simulated second, counts,
mean/p95/worst update cost and full-run physics errors. Analyze sustained work
and worst frames separately. Measure complete updates to detect costs moved
into maintenance or additional iterations. Do not sum overlapping savings;
combine mechanisms only after their individual effects and dependencies are
understood. Final performance claims require three serial AC-powered runs per
workload/version, streaming closed and no concurrent builds, plus clean builds
without diagnostic timing overhead.

The user authorized streaming-active diagnostics during this investigation.
Q1 preceded that change; Q2 onward records streaming active and cannot be used
as a clean comparison with the controlled baseline. Final acceptance conditions
remain unchanged.

Use the unchanged twelve-second 800-particle pit and authorized 600-update car
prefix for diagnostics. The prefix does not satisfy the full sixty-second car
endpoint. Do not restart user-stopped full-car or rendered trials during this
investigation. Separate scaling and minimal reproductions from acceptance
fixtures. Preserve the 0.5-unit particle-pair pit/head-on review limit and 0.1-unit
Milestone 6 extreme-fixture limits; these are review checks, not solver slack.
Keep tunneling, missed response, finite state, elapsed-time consumption, energy,
support, rebound and lifecycle checks distinct. Do not introduce a pit-wall or
car tolerance without a decision.

Follow the [measurement contract](physics_measurement_contract.md). Continue
Milestone 6 Goals 1–5 and relevant regressions; reuse prior results only with
matching source/binary/configuration provenance. Observers must preserve state
and work traces and reconcile exclusive costs with independent root timing.
Finite numerical failures finish their declared endpoint and retain full-run
worst values; fatal/nonfinite state, interruption or inability to advance is
incomplete. A promising behavioral prototype needs relevant Linux, sanitizer,
Windows cross-build and independent installed-SDK verification before being
described as verified. Rejected prototypes retain their failure evidence and
explicitly unverified dimensions.

## Completion and handoff

Deliver `docs/physics_bottleneck_investigation.md` with all five completed
question records, measured support or refutation, solution comparisons and a
ranked recommendation for user selection. Preserve reproducible probes, source
patches, configurations, binaries/hashes, raw traces and failed experiments in
the Goal 10 evidence directory. Reconcile which solutions fit **Goals 11–17**,
which would replace part of their work, and which need separate goals, without
automatically changing their scope/order. Full rendered acceptance remains
**Goal 16** and final reference/tolerance selection **Goal 17**.

Stop for review and the user's commit before implementation or another goal.
Completion does not adopt a solution or start Goal 11.
