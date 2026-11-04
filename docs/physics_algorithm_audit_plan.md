# Milestone 12 Goal 9 — Global physics algorithm audit plan

**Status: complete for review; parent pinned at `ab3c634b`.** The
[decision report](physics_algorithm_audit.md) contains the measured findings.
This is an investigation and decision report.
Do not adopt experimental engine changes or start solution goals during this
audit. The user will review the findings and select any future implementation
goals. The authoritative ordering remains in [NEXT_STEPS.md](../NEXT_STEPS.md).

The user subsequently added [Goal 10](physics_bottleneck_investigation_plan.md)
for focused investigation of each ranked question and its solutions. Forward
references here use the resulting Goals 11–17; the completed Goal 9 evidence
and original plan snapshot are preserved. Goal 10 has not started.

## Outcome and boundaries

Explain the main causes of physics cost in the unchanged car and particle pit,
why the current algorithms provide useful guarantees, and what alternatives
would cost or sacrifice. Distinguish root causes from expensive downstream
symptoms. A measured negative result is useful; a speedup is not required to
complete this investigation.

Keep production APIs, ownership, simulation behavior and runtime defaults
unchanged. Put instrumentation, prototype source copies, builds, executables,
scripts and raw evidence under `sandbox/milestone_12/goal_9/`. Experimental
behavior must remain isolated there. Commit the report and reproducible
procedure, not an automatically selected optimization. Do not add dependencies.

Compare with the immediate parent pinned when the goal starts and the frozen
`physics-pipelines-overoptimized` reference at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; exclude `main`. Goal 8 is committed
at `f6146a0a`. Its [report](physics_particle_rigid_response.md) supplies leads,
not proof of a root cause or a guaranteed optimization benefit.

## Ordered investigation checklist

1. [x] **Freeze the baseline and questions.** Record source/binary hashes,
   configurations, hardware and run conditions. Following the user's direction
   during the audit, use 60 Hz, four configured subdivisions and eight solver
   passes as the primary experimental baseline. Pursue only 60 Hz and below;
   preserve higher-rate results as historical comparisons, not future runtime
   candidates. This changes experiment scope, not production defaults. Reuse valid
   historical evidence only with its exact provenance and limitations. Keep
   the full twelve-second 800-particle pit and the existing 600-update car
   prefix as audit diagnostics. The car prefix is not the sixty-second car
   endpoint. Do not restart user-stopped full-car or rendered trials as part
   of this audit. Keep the original acceptance fixtures and their final gates.
2. [x] **Map the work and instrument its causes.** Draw the tick → configured
   subdivision → adaptive subdivision → solver iteration → refresh hierarchy.
   Attribute exclusive time and calls at each level. Separate initial contact
   discovery from solver-triggered refresh, tree preparation/query/filtering
   from geometry, and overlap-only refresh from physical response. Record
   refresh/invalidation reasons and changed versus visited bodies. For every
   adaptive decision, record proposed/returned duration, restricting pair or
   stability cause, and the final winning restriction, including ties and
   unchanged steps. Distinguish particle–particle, particle–rigid, rigid–rigid,
   joint and soft-body causes. Count actual iterations, early exits, candidates,
   unique pairs, contact visits, successful responses and cache reuse/misses.
   Validate observational instrumentation against baseline state/work traces;
   exclusive totals must reconcile with an independently measured root.
3. [x] **Measure workloads and scaling.** Rank exclusive stage costs in both
   workloads; report total ms per simulated second, mean/p95 update work,
   counts per tick/substep/iteration, and instrumentation overhead. Identify
   drop/impact and settled-pile phases separately without hiding full-run
   maxima. Run separate scaling fixtures with 200/400/800/1600 particles:
   fixed container size to expose density effects, then container area
   proportional to count to hold initial density approximately constant.
   Keep particle diameter, materials, initial placement rule and twelve-second
   duration fixed; record every derived layout. Add a settled-pile perturbation
   and a fast pair spatially separated from a slow group to expose wake and
   global-subdivision coupling. These are diagnostics, not altered acceptance
   scenes. If a run cannot finish, retain its reason and partial scope.
4. [x] **Explain each problem and compare alternatives.** Complete the problem
   records below for all five investigation areas. Connect repeated costs to
   shared causes, estimate the maximum removable portion of measured runtime,
   and avoid adding overlapping savings. Preserve findings that refute the
   initial hypothesis. Classify claims as measured, source-supported inference,
   or untested; every remaining uncertainty needs a specific validation task.
5. [x] **Run isolated experiments where needed.** Investigate a more selective
   particle–particle subdivision policy, retaining high-speed crossing protection,
   particle–rigid/rigid–rigid restrictions and other stability protections.
   The user clarified during the audit that high-speed collisions must still
   trigger substepping; the question is whether ordinary motion triggers it
   unnecessarily. The initial blanket-exclusion experiment is a rejected control,
   not the intended solution. The user also requested a diagnostic at 60 Hz
   removing all particle–particle sweep restrictions while retaining four
   configured substeps and every other adaptive limit. Compare it with the
   unchanged policy at the same 60 Hz; this is not approval to ship the policy
   or waive penetration/tunneling checks. After the initial comparison, the user
   explicitly selected keeping existing sweep protection and continuing the
   five-question investigation. Preserve the exclusion experiment as rejected
   evidence. Distinguish separated approaching pairs, existing
   contact deepening, relative acceleration and joint/soft stability restrictions.
   Define risk using proposed relative travel, size, separation and penetration
   or crossing risk rather than an unexplained absolute-speed cutoff.
   Explain exactly which duration rules change; do not conflate disabling a
   restriction with removing collision detection. Hold tick rate, configured
   subdivisions, solver passes and correction settings fixed. Then prioritize
   minimal prototypes for the largest remaining root causes when measurement
   and source analysis cannot resolve their tradeoffs. Change one mechanism
   at a time; identify dependencies before any combined experiment. Do not
   implement a broad scheduling, sleeping or collision-system redesign just
   to complete the audit. Label unbuilt alternatives and estimated benefits
   explicitly. Tick/subdivision/pass tuning, if needed for comparison, must
   be a separately labeled experiment and cannot stand in for causal analysis.
6. [x] **Deliver the decision report and stop.** Produce a ranked root-cause
   summary, one problem record per area, experiment results including failures,
   and candidate implementation goals with prerequisites and acceptance checks.
   Map candidates to existing Goals 11–17: indicate potential reuse, replacement
   or overlap without changing their order or marking anything superseded.
   Stop for user selection; creating or pursuing those goals requires a later
   instruction. The report may recommend retaining an existing algorithm.

## Questions and alternatives to cover

The user subsequently requested ordering these questions by likely contribution
to total runtime. The measured ranking is below: repeated discovery, adaptive
restrictions, unnecessary response/overlap work, unrelated work, then candidate
growth/data access. The report distinguishes car/pit priorities and confidence;
this does not reorder the six investigation steps or future implementation goals.
Additional hypotheses require explicit disproof tests, including worst-frame
causes, sweep/response consistency, per-pass convergence, dynamic coupling rather
than shared static supports, and costs moved into cache/scheduling maintenance.

| Investigation area | Required explanation | Alternatives to assess |
| --- | --- | --- |
| Repeated contact discovery | Why rigid/soft gathering repeats within solving; which corrections or invalidations require it; proportion of genuinely new contacts | Retain current refresh; retain conservative candidate sets; refresh only affected geometry/pairs; incremental broadphase updates |
| Adaptive duration restrictions | Which restrictions win and multiply world work; how configured subdivision differs from adaptive overhead | Retain current limits; more selective particle-pair risk triggers preserving high-speed protection; targeted continuous collision handling with explicit response guarantees; blanket exclusion only as a control |
| Unnecessary contact/overlap work | Cost of unchanged geometry, settled contacts, solver convergence and overlap-only refresh; why each is currently repeated | Retain current behavior; contact/manifold reuse; dirty propagation; sleeping with verified wake propagation; less frequent overlap refresh with explicit publication tradeoffs |
| Unrelated bodies repeating work | Whether a fast pair, joint or soft body reschedules unrelated bodies; how interactions reconnect groups | Retain global execution; separate execution schedules; local subdivision of connected interacting groups |
| Candidate growth and data access | Scaling with count/density, candidates per real contact, pair duplication, memory access and dispatch overhead | Retain current tree/layout; spatial partitioning changes; compact arrays; SIMD geometry batches; parallel execution only if dependency and synchronization costs are explained |

The historical 240 Hz pit has 13,091 actual versus 11,520 configured substeps over twelve
seconds, while the car prefix has 37,113 versus 9,600. Removing adaptive overhead
alone therefore cannot be assumed to solve both workloads. The pit's 100,417
rigid-gather calls and the car's 630,413 rigid-gather calls also require examining
the inner solver refresh loop. The car's 6,866,172 moving-polygon overlap-only
visits must remain separate from its 3,629 responding visits. Verify these leads
against the pinned baseline before relying on them.

For **each problem record**, include:

- Evidence: workload/configuration, endpoint, exclusive cost/share, frequency,
  scaling, source locations and links to raw results.
- Current design: what it does, why it is beneficial, and why it is costly;
  distinguish observed guarantees from inferred design intent.
- Alternatives: retaining the design plus concrete replacements, with expected
  whole-update savings and benefits as well as drawbacks. State when a proposed
  change would merely move work into another stage.
- Tradeoffs: accuracy, penetration/tunneling, energy and constraint behavior,
  complexity, memory, ownership/lifecycle, determinism, platform support,
  maintenance and API compatibility. Identify shared-entity writes and ordering
  dependencies before proposing SIMD or parallel response; do not assume
  hardware atomics make the solver safe or equivalent.
- Experiments: controlled comparisons, full error measurements, failures,
  variability, measured versus estimated benefit, and unresolved questions.
- Recommendation: pursue, retain or defer; dependencies, expected effort/risk,
  and a proposed commit-sized future goal with measurable acceptance criteria.

## Measurement, verification and completion

Follow the [measurement contract](physics_measurement_contract.md). Final timing
claims use three serial AC-powered runs per workload/version with streaming
closed and no concurrent builds. Report every trial and overhead; use builds
without extra instrumentation to check whole-update savings. Do not compare
instrumented stage totals directly with uninstrumented CPU totals, sum nested
timers, or present a headless/prefix result as rendered acceptance.

Preserve the 0.5-unit particle–particle pit/head-on review limit and provisional
0.1-unit Milestone 6 extreme-fixture penetration limits. These are measured
error checks, not solver correction slack. No new particle–wall pit limit is
implied. Retain no-tunneling/missed-response, elapsed-time consumption, finite
state, ownership and lifecycle requirements. Keep energy, rebound, support and
joint-error comparisons visible; do not silently redefine their criteria.

Run Milestone 6 Goals 1–5 and relevant regressions on the pinned baseline;
existing results can be reused only if the exact tested source/binary and
configuration match. Run focused impact, pile, thin-barrier, mass-ratio,
soft/joint and manual-stage/lifetime checks relevant to each prototype. Include
the two diameter-5 particles approaching at 1,000 units/s each across all five
head-on schedules for timestep/collision experiments. A promising prototype
requires the full relevant fixture comparison, Linux execution, sanitizers,
Windows cross-builds and independent installed-SDK consumers before being
described as verified. Untested platform or behavioral dimensions remain
explicitly unverified. A rejected prototype need not undergo unrelated platform
validation; its completed failure evidence and rejection reason must remain.

Finite numerical failures continue to the declared endpoint and report full-run
worst values. Fatal/nonfinite state, inability to advance or interruption makes
a run incomplete. A fast but failing prototype can inform the report, but is
not an acceptable solution under the retained limits. Completing the audit does
not require achieving real time or adopting any prototype.

Completion requires all six investigation steps and all five problem records,
an evidence-backed ranking with root causes separated from symptoms, explicit
experimental/validation limits, and proposed future goals ready for user
selection. Save the findings in `docs/physics_algorithm_audit.md`, with raw
logs/traces, configurations, prototype patches, executable hashes and a
reproduction procedure under the Goal 9 evidence directory. Reconcile the
roadmap/status documentation, recommend one reviewable audit commit, and stop.
The completed audit does not start any proposed implementation goal.
