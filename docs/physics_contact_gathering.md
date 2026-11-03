# Milestone 12 Goal 4 — Contact gathering

Goal 4 is complete for review and commit. Goal 5 has not started.
The frozen reference remains
`physics-pipelines-overoptimized` at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; the immediate parent is Goal 3 commit
`61564039`. `main` is excluded. Evidence, installed SDKs, executables, scripts,
logs and traces are under ignored `sandbox/milestone_12/goal_4/`.

The selected changes preserve 240 Hz, four configured substeps, eight solver
iterations, every integration pass, collision margins, scenes and materials.
Public APIs, ownership and tolerance criteria are unchanged. There is no new
allocation or SIMD instruction-set requirement.

## Analysis and selected changes

| Area | Finding and decision |
| --- | --- |
| Broadphase construction/update | Retain the balanced AABB tree, pose-based refits and traversal order. Refit already avoids rebuilding topology; particle bounds include the explicit circle as well as polygon geometry. No broadphase replacement is selected. |
| Candidate filtering | Previously, when both endpoints moved, their two refit queries could test the same negative pair twice. Record each changed collider's iteration order and skip the second visit. The car removes 2,706,677 tests; the pit removes 9,592. |
| Rigid narrowphase | Evaluate the same two float products and sum directly in the SAT projection loop, eliminating an external dot-product call for every vertex. Preserve ordered minimum/maximum comparisons, axes, containment depth, decomposition seams and contact selection. |
| Particle contacts | Reuse the circle center/radius already computed for broadphase bounds. Batch four candidate pairs with SSE when available, with exact scalar fallback and tails. Preserve polygon geometry for mixed pairs, sensor publication and original contact order. |
| Soft boundaries | Retain pose caches and endpoint cutouts. A sensor overlap may later become a response contact, so refresh and response-shape checks remain necessary. The shared SAT projection cleanup applies here too; no separate soft-gather speedup is established by the workload pair. |
| Beam pairs | Retain shared-endpoint filtering, AABB rejection, topology checks, response cutouts and accumulated contact data. This stage costs about 1.85 seconds of the stage-1 car's 110.64 seconds; its existing pair lookup and larger contact layout remain candidates for later solver work. |

The duplicate-visit rule is local to one gather. Bounds and poses remain fixed
while its callbacks run. A symmetric AABB query from the earlier changed body
has therefore already visited the pair. Use collider iteration order rather
than numeric handle order: deletion and slot reuse can make those orders differ.
The first visit and constraint append order remain unchanged. Unchanged targets
are still tested when their partner moves. Subsequent gathers test previous
misses again after motion. Only changed entries read the order array, which adds
40,000 bytes of bounded static storage and no clearing pass or retained entity
references. Existing canonical handles and positive-pair lookup remain intact.

Manual gathering retains its existing append semantics; callers begin a new
substep to clear prior constraints. Manual stages rebuild after arbitrary
public edits. No cache lifetime is extended across those calls.

The circle snapshot adds 120,000 bytes of bounded static storage alongside the
40,000-byte query-order array. It shares broadphase build/refit invalidation;
radius, origin and lifecycle edits cannot extend its validity through a manual
gather. Invalidated world shapes use the original scalar path. The four-pair
queue lives on the gathering stack and spans adjacent tree queries; mixed
geometry flushes it before the polygon pair, and the gather flushes its tail.
The unique-pair traversal rule prevents duplicate queued contacts even before
they enter the positive-pair lookup.

SIMD runs on one thread and reads geometry only. Entities may occur in several
lanes without a race. Overlaps are published in their original order and solver
responses remain sequential; no atomics are needed. On allocation failure,
publication stops at the failed pair, retains earlier contacts and records the
same fatal source/target. All four completed geometric checks still count as
actual work. A deterministic second-append failure test verifies this boundary.

## Particle SIMD decision

The four-lane helper preserves the scalar subtract, multiply/add, square root,
division and threshold sequence. It uses no approximate reciprocal, horizontal
reduction or fused operation. SSE is enabled only with `__SSE2__` and without
`__FMA__` or `__FAST_MATH__`; other builds use the scalar implementation, as does
the diagnostic `ROHR_PARTICLE_BATCH_SCALAR` build. This adds no platform
requirement or public API. Scalar tails handle one to three pairs. Sixteen
thousand three hundred eighty-four random and threshold pair checks match
scalar overlap flags, depths and normals bit-for-bit.

Three warmed, serial pairs of isolated four-pair kernel runs give:

| Four candidate pairs | Scalar ns/batch | SIMD ns/batch | Reduction |
| --- | ---: | ---: | ---: |
| All separated | 14.076 | 4.671 | 66.8% |
| All overlapping | 19.871 | 7.401 | 62.8% |
| Mixed | 15.771 | 6.380 | 59.5% |

Checksums match and saved assembly contains packed multiply/add, square root
and division. The unchanged 800-particle pit executes 3,715,588 full packets
(14,862,352 pairs) and 37,386 scalar tail pairs: 99.75% of its circle-pair checks
use full packets. The car prefix executes 1,474,853 packets and 289,991 tail
pairs, giving 95.31% coverage. These are gathering checks; swept checks and
solver response are not vectorized by this change.

Six full pit trials compare the same cache, batching and reporting machinery
with only the particle kernel switched. Run order is scalar/SIMD, SIMD/scalar,
scalar/SIMD. Three-run medians are:

| Mode | Physics wall ms | Rigid gathering ms | Update p95 ms |
| --- | ---: | ---: | ---: |
| Scalar batches | 79353.668 | 16689.538 | 168.694 |
| SIMD batches | 82091.630 | 17163.145 | 168.299 |

The SIMD median total is 3.45% higher, although two of three temporal pairs are
faster. Unchanged solver costs also drift. To reduce this timing confound, two
further full runs alternate scalar/SIMD each update and reverse that assignment
on the second run. Each of the 720 physical updates is then represented once in
each mode, in the same executable. Hooks and logging lie outside physics timing.
Across matched updates, scalar totals are 80627.860 ms physics / 16861.479 ms
rigid gathering; SIMD totals are 80606.058 / 16845.061 ms. The reductions are
only 0.027% / 0.097%, within noise. Every trial has exact state/contact/work
trace parity. Neither experiment establishes a whole-pit SIMD speedup.

Retain the small four-pair SIMD kernel for its measured local benefit and
portable fallback. Its arithmetic is too small a share of gathering to produce
a large workload gain. Reusing geometry and batching timer/report work account
for the larger observed gathering reduction; these experiments do not isolate
their individual shares. Further response batching/data-layout work belongs to
Goal 6 and requires separate accuracy and ordering checks.

## Polygon SIMD decision

SIMD was evaluated for polygon projections with GCC 15.2 `-O3`, without
fast-math or a native-CPU target. The diagnostic compares the external scalar
dot helper, direct scalar loop, an automatically vectorized temporary array,
and explicit four-lane SSE packing. All variants retain scalar, ordered extrema
reduction. Compiler diagnostics confirm vectorized dot-product loops.

Three serial repetitions, each preceded by warm-up, give these median
nanoseconds per projection. All tested outputs match bit-for-bit; checksums also
match. These are isolated kernels, not rendered measurements.

| Vertices | External helper | Direct scalar | Automatic SIMD | Explicit SIMD |
| --- | ---: | ---: | ---: | ---: |
| 3 | 3.658 | 2.499 | 4.096 | 4.370 |
| 4 | 4.566 | 2.969 | 3.857 | 3.936 |
| 8 | 7.737 | 5.229 | 5.929 | 5.738 |
| 16 | 25.541 | 9.525 | 10.623 | 9.696 |
| 100 | 98.160 | 81.476 | 78.742 | 88.906 |

Retain direct scalar projection. Packing and temporary storage lose on the
common small shapes. The automatic variant's roughly 3.4% isolated gain at
100 vertices does not justify adding a second execution path here. No claim is
made that SIMD is generally unsuitable for physics. Particle batching above is
a separate selected path with measured packing, filtering and tail costs.

Three paired, unwrapped installed-SDK benchmarks confirm the selected change
also improves complete polygon overlap, not just the projection microkernel:

| Square versus polygon vertices | Parent ns/query | Goal 4 ns/query | Reduction |
| --- | ---: | ---: | ---: |
| 4 | 295.726 | 270.658 | 8.5% |
| 8 | 504.851 | 397.220 | 21.3% |
| 16 | 1057.509 | 766.187 | 27.5% |
| 100 | 21809.414 | 11983.873 | 45.1% |

The duplicate-heavy gathering benchmark uses 128 particles with overlapping
AABBs but disjoint circles. Across 2,000 refits, candidate tests fall from
256,000 to 128,000, contacts remain zero, and median gather cost falls from
65.717 to 56.131 microseconds (14.6%). These synthetic shapes and placements are
diagnostic workloads; the authored car and pit fixtures remain untouched.

## Workload comparison

Fresh parent and stage-1 runs execute serially without concurrent builds.
Both report AC power offline, so these are battery/headless diagnostics with
one run per workload and version, not controlled final acceptance. Their
wrappers and observers add overhead. Timings do not establish a repeatable
whole-workload speedup; in particular the pit does not improve in this initial
pair. Stage 1 includes duplicate suppression and scalar SAT cleanup, before
circle caching and particle batching. Its binaries and sources are preserved
under `stage_1/` alongside these historical measurements.

| Measurement | Parent car, 10 s | Stage-1 car, 10 s | Parent pit, 12 s | Stage-1 pit, 12 s |
| --- | ---: | ---: | ---: | ---: |
| Physics wall ms | 112438.965 | 110643.245 | 85419.380 | 86320.667 |
| Contact gathering, exclusive ms | 46778.236 | 45464.936 | 19851.311 | 19937.838 |
| Rigid gathering, exclusive ms | 26129.541 | 24554.654 | 19746.721 | 19829.706 |
| Physics-update p95 ms | 379.912 | 383.328 | 176.470 | 177.519 |
| Subdivisions | 37113 | 37113 | 13091 | 13091 |
| Solver iterations | 296904 | 296904 | 104728 | 104728 |
| Candidates | 18340652 | 15633975 | 15508376 | 15498784 |
| Contacts | 1659222 | 1659222 | 746387 | 746387 |

The car has 14.8% fewer candidates, 6.0% less rigid-gather time, 2.8% less total
contact-gather time and 1.6% less total physics time in this pair. Its p95 is
slightly higher. The pit removes only 0.062% of candidates and total physics time
is 1.1% higher. Retain the measured kernel improvements without claiming the
real-time gates have been met. Stage-exclusive sums exactly reproduce root
physics time; candidate/contact increments reproduce returned report totals.

The final SIMD car prefix takes 109815.836 ms physics, 23749.221 ms rigid
gathering and 368.776 ms update p95, with the same stage-1 work counts. Against
the fresh parent, those single-run differences are 2.3%, 9.1% and 2.9% lower.
Final SIMD pit medians above are 3.9% lower total and 13.1% lower rigid gathering
than the single parent sample; the scalar-batched medians are lower still.
These unequal sample counts and battery/thermal variation preclude treating the
differences as controlled whole-workload gains. No real-time gate is met here.

The car run is a ten-second prefix, not completion of its declared 60-second
scenario. Its benchmark force-lifetime limitation remains recorded in the
[Goal 3 report](physics_integration_subdivision.md#clock-ownership-follow-up).
No previously interrupted rendered or full-reference trial is restarted.

## Verification and remaining limits

- Fifteen Linux SIMD, fifteen scalar-fallback and fifteen ASan/UBSan regressions
  pass, with leak detection and halt-on-error enabled for sanitizers. The new
  regression covers negative pairs, both/one
  moved endpoint, unchanged gathers, new sensor contacts, canonical handles,
  slot reuse, manual-stage invalidation, full batches/tails, scalar bit parity,
  radius/origin edits without pose changes and failed contact publication.
  Existing tests cover particle
  sweeps, beam seams, masks, recovery, lifecycle and report composition.
- Independent installed-SDK car, pit and fixture consumers build on Linux,
  sanitizers and Windows without compiler warnings. Linux execution occurs
  outside the source directory. Car/pit sanitizer smoke runs pass. Windows
  cross-builds pass; native Windows execution was not performed.
- All 219 timed scenarios plus four calibrations finish at their declared
  endpoints. Numerical records match the frozen reference after excluding CPU
  timings. Per-update state, contacts, subdivisions, observed duration,
  diagnostics and finite-state checks match exactly for those fixtures and the
  600-update car prefix and 720-update pit. Candidate counts intentionally
  decrease and are compared separately.
- No new missed contacts, tunneling or penetration increase is observed in
  these fixtures. The full pit retains depth 0.002340078 and no escape. Soft-body
  worst depth remains 0.00300585644 and energy error 0.00136403434. Trace hashes
  supplement these numerical checks; they are not an exhaustive geometry proof.
- The soft-body delayed-update CPU comparison still fails: 368.877 ms against
  50 ms. Its scenario finishes and retains full-run measurements. Rendered
  real-time acceptance, full car coverage and user motion/window-switch review
  remain pending Goal 10. Final tolerance decisions remain Goal 11.

The audit verifies all 1,457 frozen-reference artifact hashes and all 788 Goal 3
artifact hashes unchanged. Release Linux/Windows archives differ from the frozen
reference only in the five intended implementation objects across Goals 2–4.
Sanitizer archives also contain changed non-executable sections in dependents of
the private header; executable section changes remain those same five objects.
Scalar/SIMD comparison SDKs differ only in `particle.c.o`.

`build.py`, `measure.py`, `measure_parent.py`, `verify.py` and `benchmark.py`
preserve the initial procedure. `build_particle_experiment.py`,
`particle_verify.py`, `particle_kernel.py` and `interleaved.py` reproduce the
particle experiments; `final_verify.py` verifies the final failure-injection
regression on both kernel variants and sanitizers. Run directories are immutable
evidence; choose new output directories for repeats. `summary.json`,
`kernel_summary.json`, `particle_summary.json` and `interleaved_summary.json`
retain comparisons. `audit.json` verifies frozen evidence and archive members;
`manifest.json` hashes preserved artifacts, and `source/` includes new untracked
test/document files as well as the tracked diff. Earlier evidence remains intact.
