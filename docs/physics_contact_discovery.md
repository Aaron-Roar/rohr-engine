# Shared contact discovery

Milestone 12 Goal 12 uses committed Goal 11, `506b696af1d25d487a885a41737cb1a074c9a71f`,
as its immediate parent. The frozen reference remains
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; `main` is excluded.

**Status: complete; committed at `81e76ff5`.** The
[Goal 13 quality/work investigation](physics_quality_experiments.md) is also
complete, without production adoption. The subsequent
[particle/node collision plan](physics_particle_collision_plan.md) is the new
Goal 14; the original remaining Goals 13–17 are now 15–19.
Goal 12's selected combination
reduces measured discovery work while retaining all tested trajectories and
collision work. Final numerical tolerances have not changed.

## Selected implementation

- Prepare query sources while building/refitting the tree, preserving collider
  iteration order instead of scanning every collider a second time.
- During complete synchronous substeps, collect actual pose commits in a
  separate ordered bitmap. Refit only notified colliders, still comparing exact
  final poses to reject changes that cancel out. Velocity-only commits do not
  dirty this consumer. Manual stages retain their pose scans, including direct
  component-array edits made while membership is frozen.
- Summarize eligible order keys in tree branches. Skip a branch only when every
  leaf would already fail the existing self/duplicate-order checks. Preserve
  leaf visitation order, four-lane particle packing, narrowphase arithmetic,
  retained contacts, sensor overlaps, append-failure behavior and publication.
- For an unchanged beam, traverse only branches containing a rigid whose pose
  changed since the soft gatherer's own previous observation. Changed beams
  still query the complete current tree. A moved rigid can therefore enter an
  unchanged beam and create a contact.
- Reuse each unchanged beam's query bounds. The candidate callback constructs
  current, pair-specific geometry, including endpoint cutouts, when it needs it.
  Rigid bounds read live vertices directly instead of copying a fixed-capacity
  `Shape` into `math_aabb_create`.

Public APIs, ownership, response equations, solver slop, materials, authored
scenes, sweep protection and runtime settings are unchanged. The extra caches
use about 0.4 MiB of bounded static storage; there is no new heap allocation.
The tree still rebuilds at the start of each substep and retains its original
topology during solving.

| Data | Validity and refresh boundary |
|---|---|
| Collider membership and iteration ranks | Frozen complete substep; refreshed by the initial full build. Manual gathering rebuilds/scans as before. |
| Pending pose work | This gatherer owns its bitmap. Successful pose commits mark it; gathering consumes it. Hitbox flushing cannot clear it. Enable/disable and full builds reset it. |
| World hitboxes and particle circles | Existing exact-pose cache and validation remain; local geometry is immutable inside a coherent call. External shape/radius/origin edits are observed by the next independent call. |
| Rigid subtree order keys | Recomputed for the current tree and changed-source order before querying. Unchanged targets remain eligible during refits. |
| Soft subtree changes | Compared against the soft gatherer's own previous poses, independent of rigid gathering and hitbox refresh. |
| Beam query bounds | Exact endpoint poses, radius and existing per-solve cache lifetime. Independent stages compute fresh bounds; pair geometry remains current. |
| Velocity, mass, filters and contact state | No new retained approximation. Existing live checks and public-call invalidation remain authoritative. |

## Measurements and attribution

All new workload diagnostics use **60 Hz, four configured substeps, eight solver
passes**. The unchanged pit completes 12 seconds/720 updates. The car completes
the authorized 600-update prefix, approximately 10 seconds; this is not a full
60-second car result. AC power remained connected, streaming was active with
user authorization, and builds never ran concurrently with measurements.

| Workload/version | Median total physics CPU ms | CPU ms / simulated second | Median p95 CPU ms | Worst update CPU ms |
| --- | --- | --- | --- | --- |
| pit parent | 43,456.006 | 3,621.334 | 128.393 | 434.451 |
| pit final | 41,048.789 | 3,420.732 | 95.482 | 500.030 |
| car parent | 91,339.979 | 9,133.997 | 280.047 | 4,074.413 |
| car final | 83,796.750 | 8,379.675 | 271.923 | 4,281.470 |

Three serial unwrapped runs per version/workload supply these CPU figures.
Trial 2 reverses parent/final order. They are streaming-active headless
diagnostics, not controlled rendered acceptance. The p95 figures are physics
CPU samples, not rendered frame-work p95. No window trials were restarted.
No debug drawing or intentional slow motion is included in these headless runs.

Pit median CPU reduction: **5.54%**. Paired reductions:
8.70%, 10.74%, 5.54%.

Car median CPU reduction: **8.26%**. Paired reductions:
-2.49%, 8.26%, -9.38%.

Car: **2 of three pairs are slower**. The lower median does not
establish a consistent gain. These trials cannot separate load/frequency
variation from a real regression; do not attribute the slower pairs solely to
streaming or claim that the workload cannot regress.

Worst observed update CPU also increases: pit 434.451 → 500.030 ms, car 4074.413 → 4281.470 ms. No worst-update latency improvement is established.

Exploratory single-run wall timings, **whole update / all gathering ms**.
These instrumented pilots select candidates; they do not replace repeated clean
measurements. Every completed pilot has exact parent state/work parity.

| Variant | Pit, full 12 s | Car, 600 updates |
| --- | --- | --- |
| Committed Goal 11 | 39,143.7 / 12,965.8 | 78,030.6 / 33,970.9 |
| Prepared query list | 38,757.4 / 12,751.1 | 88,262.1 / 38,132.4 |
| Query list + precise pose work | 40,365.2 / 13,205.2 | 75,388.7 / 32,443.5 |
| Candidate 2 + duplicate subtree pruning | 38,535.5 / 10,934.4 | 81,695.3 / 34,589.8 |
| Candidate 2 + soft changed-subtree pruning | 41,838.7 / 13,677.9 | 78,306.4 / 33,003.2 |
| Candidate 4 + pointer-based rigid bounds | 40,002.5 / 12,597.3 | not run separately |
| Candidate 4 + reused beam query bounds | not run separately | 75,658.5 / 30,276.1 |
| Selected: combined 3, 5 and 6 | 35,620.7 / 9,756.1 | 75,474.1 / 29,283.7 |

The source probes use nested exclusive timers, with rigid gathering, soft
gathering and hitbox requests outside gathering as separate roots. Each root's
exclusive terms sum exactly to its inclusive duration. Their overhead and
streaming variability make them attribution tools, not headline benchmarks.
Do not add their costs to the pipeline wrapper costs: these are two views of
the same execution. Soft pair handling includes filtering and shape/argument
preparation; its manifold-body and append/update costs are separately timed.

| Exclusive rigid work (ms) | Pit parent | Pit selected | Car parent | Car selected |
| --- | --- | --- | --- | --- |
| Gather orchestration | 538.797 | 458.119 | 1,211.218 | 879.660 |
| Build/refit setup + pose list | 1,405.358 | 1,481.286 | 2,406.023 | 2,554.292 |
| Tree insertion + refit | 1,277.345 | 1,440.689 | 1,838.171 | 2,218.922 |
| Current bounds | 1,220.370 | 686.224 | 1,642.208 | 956.949 |
| Hitbox validation/refresh | 1,093.886 | 1,263.661 | 1,528.117 | 1,837.438 |
| Tree traversal | 10,907.353 | 8,675.051 | 11,863.301 | 11,479.221 |
| Pruning metadata | 0.000 | 251.887 | 0.000 | 1,296.928 |
| Pair filtering/retained lookup | 2,424.627 | 1,367.083 | 3,809.746 | 3,485.036 |
| Particle packing/reporting | 625.369 | 695.979 | 579.014 | 678.807 |
| Circle geometry | 60.279 | 67.872 | 38.101 | 45.091 |
| Mixed geometry | 204.201 | 235.937 | 2,698.966 | 3,231.341 |
| Contact publication | 764.816 | 868.771 | 827.407 | 985.271 |
| Rigid gather total | 20,522.400 | 17,492.560 | 28,442.271 | 29,648.955 |

| Exclusive soft work (ms) | Car parent | Car selected |
| --- | --- | --- |
| Gather orchestration | 21.753 | 23.714 |
| Beam scans/query preparation | 2,722.095 | 1,684.671 |
| Initial beam shape/bounds | 2,033.862 | 404.451 |
| Tree traversal + unchanged-leaf rejection | 7,245.591 | 6,291.095 |
| Changed-subtree metadata | 0.000 | 585.164 |
| Pair filtering/shape/argument preparation | 2,360.542 | 2,834.742 |
| Manifold routine body | 2,194.168 | 2,657.667 |
| Contact append/update | 87.495 | 105.325 |
| Beam-pair gathering | 1,292.462 | 1,539.772 |
| Soft gather total | 17,957.968 | 16,126.602 |

The instrumented car rigid-gather total increases despite reduced work; the
unchanged outside-gather hitbox work also takes about 20% longer in the selected
probe run. Do not convert these individual timer differences into a predicted
whole-update saving. The clean paired trials above include all maintenance cost.

| Work count | Parent | Selected | Reduction |
| --- | --- | --- | --- |
| pit Refit pose visits | 28,182,891 | 9,234,465 | 67.23% |
| pit Query-source visits | 32,139,272 | 13,177,559 | 59.00% |
| pit Rigid tree node visits | 1,427,090,917 | 1,158,529,499 | 18.82% |
| pit Rigid leaf callbacks | 65,123,325 | 27,405,671 | 57.92% |
| car Refit pose visits | 127,017,450 | 23,164,857 | 81.76% |
| car Query-source visits | 134,962,065 | 19,661,980 | 85.43% |
| car Rigid tree node visits | 1,695,010,226 | 1,386,379,574 | 18.21% |
| car Rigid leaf callbacks | 59,901,913 | 28,452,218 | 52.50% |
| car Soft tree node visits | 1,230,036,790 | 873,061,624 | 29.02% |
| car Soft leaf callbacks | 44,501,566 | 15,974,390 | 64.10% |
| car beam query shapes | 17,889,120 | 4,270,685 | 76.13% |

Changed poses, query sources, queued particle pairs, narrowphase calls, contact
publication calls, initial tree builds/refits and all state/work traces remain
equal. The reductions are avoided discovery work, not omitted collision tests.
Hitbox recomputation counts also remain equal. The source probe records cache
hits and invalidation causes separately; position and orientation causes may
overlap. Requests returning before cache inspection are non-live/non-collider
requests, not extra transformed shapes.

## Other investigated options

**Particle centers versus polygon geometry.** Circle packets already borrow
centers/radii prepared with their tree leaves. Another SIMD arithmetic rewrite
has little budget: the parent circle-gather arithmetic is about 60 ms for the
pit and 38 ms for the car prefix in this source probe. Queue preparation,
filtering and publication are separate costs and remain visible above.

The parent performs 3,928,470 particle polygon refreshes inside pit gathering
and 43,289,833 outside it; the corresponding car counts are 6,114,726 and
47,215,658. The outside-gather hitbox root also includes validation and cache-hit
requests, so its whole duration is not a removable polygon-transform budget.
Particle-pair response currently prepares polygons even when its immediate
geometry query uses circles. However mixed response, rigid/soft sweeps, soft
contacts and final/public state also consume world polygons. Particle polygons
can extend outside their circle; replacing their broadphase bounds with circle
bounds without a containment proof would lose coverage.

This goal retains that shared eager preparation while removing avoidable bound
copies and beam shapes. A separate circle-validity/derived-polygon validity
design remains a cross-consumer opportunity under Goal 17's already planned
deferred-geometry work. Its required checks are invalid/nonfinite local geometry,
radius/origin edits, current mixed-contact geometry and callback/public-boundary
publication. No measured whole-engine benefit for that larger separation is
claimed here; this is not a finding that it would be unprofitable.

**Conservative envelopes.** The [Goal 10 containment model](physics_bottleneck_investigation.md#1-repeated-discovery)
already proves the necessary expansion fallback and generation invalidation.
Its 0.1/0.5/1-unit margins needed 83/6/5 rebuilds but 68,239/174,244/176,800
candidate visits, plus 51,200 containment checks. Those are model counts, not
engine timings. Larger envelopes add false candidates; retained candidate order
must also preserve sequential solving or undergo a behavioral comparison.
Retain exact bounds for this implementation. Envelope width is unrelated to
the 0.5 penetration-review threshold.

**Tree reuse/grid and reverse beam indexes.** Existing swept-tree reuse remains.
Retaining gathering topology across substeps or replacing it with a particle
grid would alter traversal/contact order and still need mixed/soft coverage.
The prior snapshot experiments do not establish a whole-workload gain for that
replacement. The selected subtree summaries avoid much repeated traversal
without changing order. They still scan beams to compare endpoints; a reverse
node-to-beam index remains Goal 16's planned investigation, with topology and
generation maintenance included. No savings from unimplemented alternatives
are added to the selected result.

## Verification and remaining limits

- Fresh Linux and ASan/UBSan builds each pass all 23 regressions, 219 declared
  stress-fixture endpoints and four particle restitution calibration endpoints.
  The complete stress traces match Goal 11; 220,994 historical fixture updates
  also match the frozen reference after excluding wall time and the already
  documented historical gathering-candidate-count difference.
- The focused gathering tests cover notification consumption after hitbox
  flushing, corrections that cancel, velocity-only changes, word-boundary and
  handle-order cases, stationary beams approached by moved rigids, moved beams,
  manual thickness/origin edits, deletion/reuse and existing allocation failure.
  Separate Linux and sanitizer checks each complete 1,000 create/use/destroy
  cycles. No sanitizer or leak finding occurs.
- Windows cross-builds and independent installed-SDK joints examples build
  without compiler warnings. Linux and sanitized SDK examples run from `/tmp`.
  Native Windows execution was not performed. Sanitized workload checks finish
  four pit seconds and 134 car updates, including pathological update 133.
- Full-pit geometric review: worst particle-pair overlap
  **0.019971246**, below the retained 0.5 limit;
  final overlap 0.016193570, final RMS speed
  0.403878340. The full oracle matches Goal 11.
- Car-prefix geometric review: worst standalone-particle overlap
  0.041826790; worst node-pair overlap
  0.015560134; maximum beam absolute/relative
  deformation 13.419484546 /
  1.472364484. All remain equal to Goal 11;
  no new wall-contact or deformation threshold is invented.
- All existing 0.1 stress-fixture penetration checks pass. Soft-fixture maximum
  depth is 0.00300585644 and maximum energy error is
  0.00136403434. Numerical success does not erase CPU failures:
  3 Linux and 14 sanitizer cases exceed
  one or both historical CPU comparison criteria. Worst delayed CPU is
  383.838 ms on Linux and 1891.698 ms
  under sanitizers, against the 50 ms comparison budget. Every case reaches
  its declared endpoint and full-run maxima remain in `final_analysis.json`.


The report does not establish sustained 60 FPS, rendered p95 at or below
16.67 ms, full-car behavior, or final tolerance acceptance. Milestone 6 Goal 5
and superseded Milestone 11 keep their existing status. Goal 13's eight-lever
investigation is complete; it recommends measured settings for user selection
and retains failed/incomplete evidence, without changing production defaults.
Goals 16–17 still own implementation of
the beam sweep/response mismatch and broader consumer/orchestration work;
Goal 18 owns combined rendered acceptance, and Goal 19 owns reference tradeoff
selection. Goal 13 supplies experimental evidence without adopting a solution.

Evidence is under `sandbox/milestone_12/goal_12/`: `reproduction.md`,
`baseline.json`, `pilot_analysis.json`, `probe_analysis.json`,
`final_analysis.json`, `runtime_results.json`, `archive_verification.json`,
`provenance_verification.json`, source snapshots, logs, traces, SDKs,
executables and the final SHA-256 inventory. The final Linux archive matches
the selected candidate byte-for-byte: 93 parent object members are unchanged
and only the four selected implementation objects differ.
