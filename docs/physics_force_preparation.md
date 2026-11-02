# Force preparation rework

Milestone 12 Goal 2 analyzes force preparation against the frozen
`physics-pipelines-overoptimized` reference at `6f283268`. The
[measurement contract](physics_measurement_contract.md) remains authoritative.
Evidence, executables, compiler reports and scripts are under
`sandbox/milestone_12/goal_2/`. No comparison uses `main`.

## Findings and selected change

In Goal 1, exclusive force preparation took 1,521.551 ms of 113,398.913 ms
(1.34%) in the first ten simulated car seconds, and 514.149 ms of 85,885.779 ms
(0.60%) in the full twelve-second pit profile. These are diagnostic timings;
the car segment is not the full car workload. Force preparation alone cannot
resolve the real-time failures, but a small local improvement is worthwhile.

| Work | Frozen car cost (ms) | Frozen pit cost (ms) | Decision |
| --- | ---: | ---: | --- |
| Clear force/torque acceleration | 44.648 | 18.044 | Retain the live-entity pass |
| Gravity | 81.672 | 160.757 | Retain eligibility checks and atomic contribution validation |
| External force/torque sources | 211.817 | 68.426 | Retain source order, live reference resolution and integration ownership |
| Soft-body frame updates | 315.530 | 69.244 | Retain validation and motion-dependent refresh boundaries |
| Spring preparation | 306.303 | 117.708 | Bound the scan and reuse computed COM offsets |
| Beam preparation | 367.633 | 5.706 | Retain force arithmetic and atomic endpoint commits |
| Force-pipeline remainder | 193.948 | 74.264 | Retain reporting/composition boundaries |

Spring preparation previously scanned all 10,000 possible entity indices on every
substep, including particle-only scenes. It now scans allocated joint-table
capacity, rejects non-joint masks before handle/liveness checks, and retains
ascending index order. Bounds checks preserve safety if table capacities differ.
Filtering uses component membership rather than pool usage: a manually enabled
empty joint component must still report its unavailable endpoints. Nothing is
cached across public calls.

Each spring previously resolved its two endpoint COMs six times: once per endpoint
for point velocity, then separately for each coordinate of each torque arm. The
same already-computed offsets now supply torque calculation. Poses do not change
between those uses. Two overwritten origin-relative offset calculations are also
removed. The force formula, floating-point accumulation order, damping, units,
endpoint validation and pairwise commit remain unchanged.

## Retained behavior and alternatives

- **Clearing and gravity:** clearing touches force-derived acceleration only;
  authored linear/angular acceleration persists. Gravity remains a separate,
  additive public stage. Fusing those stages would complicate manual composition
  for a small measured cost; clearing entire storage would touch inactive slots.
- **External loads:** the integration stage applies force sources, then torque
  sources, after spring/beam preparation and before adaptive duration selection.
  Zero-duration manual integration still evaluates loads. Validation can repeat
  during description/geometry resolution, especially for soft-body COM references;
  this is retained so missing/stale references, absorbed shares, overflow and
  source-local failure retain their existing outcomes. Ownership follows the
  source entity's declared owner, never its target or geometric references.
- **Springs:** retain world-anchor resolution, current point velocities and
  equal-and-opposite force calculation. Shared endpoints make contribution order
  observable through floating-point accumulation. No persistent resolved-anchor
  cache is introduced.
- **Beams:** the existing scan already uses allocated beam capacity and used
  slots. Keep its short-beam check, endpoint validation, spring/damping expression
  and atomic two-node commit. The second elastic-force evaluation during complete
  soft-body integration is motion-dependent work, not a duplicate to delete here.
- **Soft-body frames:** a complete substep calls frame refresh before spring/beam
  forces, before integration, after integration and after constraint solving.
  Some calls discover no movement, but each validates owned nodes and can report
  invalid state. The standalone stages must also observe edits between calls.
  Motion refresh/invalidation changes would need to preserve those semantics;
  they are not justified by this stage's measured share in Goal 2.

## SIMD assessment

The Linux Release build uses GCC 15.2 with `-O3`, without `-ffast-math` or a
machine-specific instruction requirement. Compiler vectorization reports and
assembly are preserved in `vectorization/`. GCC already combines some x/y
arithmetic into 8-byte vectors and some spring calculations into 16-byte vectors.
It does not vectorize the outer clearing/source loops across entities: their
control flow and calls prevent it. This is compiler-generated basic-block SIMD,
not evidence of batching multiple bodies.

Explicit SIMD is not selected for force preparation: the measured share is small,
and collecting/scattering eligible bodies plus preserving ordered shared-endpoint
updates would add work and complexity. No public data layout, CPU requirement or
floating-point tolerance changes in this goal. GCC's optimization behavior is
explained in its [optimization documentation](https://gcc.gnu.org/onlinedocs/gcc-15.1.0/gcc/Optimize-Options.html).

Particle distance/overlap tests and polygon projection batches remain candidates
for Goal 4; contact-response batching and data layout belong to Goal 6. Measure
packing/scattering cost, candidate density, small-batch overhead and scalar fallback.
Contacts sharing a body require ordered or otherwise verified state updates.
Those potential collision changes are not implemented by Goal 2.

## Verification and measured results

The final regression set passes 14/14 on Linux Release and 14/14 under
ASan/UBSan with leak detection. Installed-SDK car/pit consumers and tests build
without compiler warnings for Linux, Linux sanitizers and Windows. Native Windows
execution is not claimed. The new standalone contract regression also passes
against the frozen SDK: sparse high-index joints, manual additive forces,
external-load timing, persistent authored acceleration, gravity, off-center
torque, component toggles, deletion/reuse and source ownership retain their
behavior. Existing shared-source, frame, joint and recovery regressions are
included. Sanitizer car/pit smokes pass.

A separate unwrapped diagnostic uses 800 ordinary entities and 0, 1 or 64 springs,
20,000 timed preparation calls after 1,000 warmups, with acceleration clearing
outside the timed interval. Endpoint poses and spring configuration remain fixed;
it is a force microbenchmark, not a replacement car/pit workload. Three serial
reference/current pairs use the same Release consumer. Median-of-run-medians:

| Springs | Frozen reference (ns/call) | Current (ns/call) | Reduction |
| --- | ---: | ---: | ---: |
| 0 | 8,292 | 392 | 95.3% |
| 1 | 8,511 | 573 | 93.3% |
| 64 | 21,242 | 10,011 | 52.9% |

Final force/torque values match exactly in all six version/repetition cases per
spring count. The large local percentage is not an equivalent whole-frame gain.
Raw repetitions, commands and binaries remain under `logs/`, `benchmark/` and
`build/benchmark_*`. Preliminary data without the `final_` prefix used an earlier
pool-usage filter; it is exploratory and does not verify the shipped mask filter.

All 219 timed Milestone 6 scenarios plus four particle calibrations finish.
Every numerical record and per-update state/work/duration trace matches the
frozen reference. The car's first 600 frames (ten simulated seconds) and the
pit's full 720 frames (twelve seconds) also match exactly. Timing conservation,
source-attributed candidate/contact counts and observed subdivisions agree;
there are no lost reports, fatal errors or nonfinite states in these runs.

| Profiled diagnostic | Frozen reference (ms) | Current (ms) |
| --- | ---: | ---: |
| Car spring preparation | 306.303 | 20.923 |
| Car total force preparation | 1,521.551 | 1,257.789 |
| Car outer physics time | 113,398.913 | 114,808.530 |
| Pit spring preparation | 117.708 | 6.448 |
| Pit total force preparation | 514.149 | 424.639 |
| Pit outer physics time | 85,885.779 | 89,400.688 |

These single profiled workload comparisons use unchanged scenes, materials,
body counts, 240 Hz ticks, four configured substeps and eight iterations.
The local reduction does not establish an end-to-end throughput gain: whole-run
times increased amid run variability. The repeated isolated benchmark supports
retaining this small cleanup; no broader force rewrite is selected.

The soft-body worst depth remains 0.00300585644 and energy error remains
0.00136403434. The final delayed-update maximum is **397.578 ms versus 50 ms**,
still a failed CPU comparison (Goal 1 measured 376.743 ms). Maximum scenario p95
across rates is 1.084 ms. The full pit retains depth 0.002340078 and zero escapes;
its profiled physics CPU p95 is 181.267 ms. No tolerance is changed or failure
reclassified as a pass.

`summary.json` retains the comparisons, per-function timing/call/work records,
repetitions and artifact hashes. `audit.json` verifies all 1,457 frozen artifact
hashes and confirms the only changed engine archive member on Linux, sanitizers
and Windows is the joint-solver object. The only changed engine source is
`src/physics/joints/joint_solver.c`; public headers remain unchanged.

**Goal 2 is complete for review.** Further force optimization stops here, as
requested by the user. Integration/subdivision is the next ordered goal, after
review and the user's commit. Goal 1's user-directed incomplete full-car
and rendered evidence remains intact; this goal does not restart those trials or
claim rendered real-time acceptance.
