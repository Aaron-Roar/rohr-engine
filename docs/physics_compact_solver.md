# Compact particle contact comparison

Milestone 13 Goal 2 was reopened by the user on 2026-11-06 against parent
`926cc6f57802a814518135fed1baa2f3998637a0`. Preserve the prior Goal 2 attempts
and Goal 3's rejected static-wall guard. The user restored Goal 3's tracked
changes; its three untracked implementation/test files remain untouched.

## Approved scope

Implement and compare compact scalar contact solving, followed by SIMD response
batches, within the current C engine. Keep the unchanged 800-particle, twelve-
second pit and authored ten-second car diagnostic at 60 Hz, four configured
substeps, two particle-pair passes and four other response passes. Preserve
production defaults and public APIs. Target 4 ms average and 5 ms p95 physics
per 60 Hz frame; also report the existing two-times CPU headroom and rendered
gates. Measure the remaining budget for later wall protection.

The user permits changed trajectories and settling when errors are measured,
the existing limits remain visible, and behavior is acceptable. Begin with a
scalar comparison preserving contact order. Evaluate SIMD separately; worker
threads are a later option only if needed. Do not silently reduce collision
cadence, scene complexity, material stiffness, or response accuracy.

## Execution and verification

1. Freeze source/settings/SDK/executable provenance under
   `sandbox/milestone_13/goal_2/compact/`. Keep controls and unsuccessful
   experiments immutable. The user confirmed AC power and heavy workloads
   closed for this comparison.
2. Prepare compact particle contact state once per synchronous solve, preserving
   live sequential positions/velocities, directional response, retained impact
   targets, finite-state rejection, ownership and publication. Keep manual-stage
   edits visible and unsupported cases on the existing path. Count conversion,
   storage and dispatch costs as part of the comparison.
3. Compare a scalar implementation with the unchanged control before SIMD.
   Batch only independent response writes. Distinguish changed contact ordering
   from vector arithmetic and retain a scalar fallback.
4. Run complete diagnostic endpoints and quality observers. Record penetration,
   crossings, support, rebound, energy, settling and car deformation separately.
   Preserve known baseline failures; no sampled-overlap result proves CCD.
5. Verify relevant Linux regressions, ASan/UBSan, Windows cross-builds and
   independently installed SDK examples. Require three serial clean comparison
   runs per finalist/workload, then separate rendered/behavior review. Keep the
   one-minute cap and preserve incomplete runs. Do not attribute the existing
   presentation-pacing deficit to physics or repair graphics in this goal.
6. Record selected/rejected results, all target misses and protection headroom.
   Stop after this goal for user review and commit. Goal 3 remains a separate
   decision.

## Starting evidence

The last matched guard-disabled controls measured 4.456 ms average car physics
and 7.438 ms pit physics per 60 Hz frame. Their p95 ranges were 5.493–5.929 ms
and 10.026–10.480 ms. These are preserved Goal 3 measurements, not fresh runs.
The current contact record is 1,352 bytes, confirmed by a compiler layout probe;
this suggests a storage opportunity, not an established solver speedup.

## Retained implementation

`ROHR_PHYSICS_COMPACT_PARTICLES=ON` prepares a smaller particle response record
once per complete baseline solve. It retains contact order, live sequential
positions/velocities, directional inverse masses, accumulated impulses, friction,
impact targets and checked commits. Generic constraints still own gathering and
publication; the large generic contact record has not been removed. Offset or
rotating circles, transform locks, unsupported material/state and small contact
sets use the existing solver. Scratch storage belongs to the contact list, grows
at preparation, and is released at list destruction. Allocation failure reports
a warning and uses the existing solver. Manual staged calls keep their existing
semantics and do not retain prepared state.

The separate private `ROHR_PHYSICS_PARTICLE_PASSES=2` setting applies to all
responding nonrotating particle pairs in a complete baseline substep, including
small lists and fallback rows. Other contacts and joints retain the configured
iteration count. Zero uses the configured count. Both new options default off/
zero and require the experimental simple baseline to affect simulation. No
public API, production default, scene, material, substep count or collision
discovery cadence changes. No threads, sleeping or new wall guard are added.

## Measured candidates

Three serial clean runs per workload/version, alternating order on AC power with
heavy workloads closed, produce these medians. Averages divide complete total
physics CPU by 600 car or 720 pit frames; p95 is the median of the three run p95s.
The control uses the retained diagnostic two-pass wrapper; the candidate uses
the native private setting with that wrapper disabled. This first table compares
the retained control with the combined change, including removal of its wrapper.

| Workload | Control total CPU | Compact total CPU | Average/frame | p95/frame | CPU reduction |
| --- | ---: | ---: | ---: | ---: | ---: |
| Car, 10 s | 2.508025 s | 2.415431 s | 4.026 ms | 5.196 ms | 3.69% |
| Pit, 12 s | 5.122895 s | 4.322787 s | 6.004 ms | 7.873 ms | 15.62% |

All three pit pairs improve. Two car pairs improve and one regresses, so the
small median car benefit is not a consistent per-pair speedup. Candidate p95
ranges are 5.189–5.454 ms car and 7.635–8.018 ms pit. The earlier single-screen
27% pit improvement is exploratory; the matched three-run result above is the
comparison result. Source/library/executable identities and individual runs are
in `sandbox/milestone_13/goal_2/compact/versions/`, `parent.json`, and
`timing_summary.json`. The final Linux engine library starts with SHA-256
`f3f35ca8bc8b3953`; the pinned control starts with `70688922352ed5f8`.

Both pass the existing diagnostic CPU budgets of 5 s car and 6 s pit. **Neither
meets the new 4 ms average / 5 ms p95 target.** The pit needs another 1.442787 s
CPU reduction per twelve simulated seconds (33.38% of the retained candidate)
to reach the average target, before adding protection. The car average misses
by 0.026 ms and its p95 by 0.196 ms. These medians establish no positive budget
for a new wall system under the requested tighter target.

A second block isolates compaction: both builds use native two-pass policy,
with compact solving disabled in `native_control` and enabled in the candidate.
It also uses three serial alternating runs per workload/version. Do not pool
this later block with the first or credit removal of wrapper work to compaction.

| Workload | Native control CPU | Compact CPU | Compact average/frame | Compact p95/frame | CPU change |
| --- | ---: | ---: | ---: | ---: | ---: |
| Car, 10 s | 2.676039 s | 2.684137 s | 4.474 ms | 5.751 ms | 0.30% more |
| Pit, 12 s | 5.953452 s | 4.393571 s | 6.102 ms | 8.225 ms | 26.20% less |

All three native-control pit pairs improve. Two car pairs regress slightly;
combined with the first block's variability, **no consistent car CPU saving is
established**. The implementation helps the particle-heavy pit; car work is
dominated by other stages. Candidate p95 ranges in this block are 5.030–5.826 ms
car and 7.984–8.375 ms pit. The clean CPU gates still pass; the tighter budget
still fails, now by 0.474/0.751 ms average/p95 car and 2.102/3.225 ms pit.
These are the primary figures for attributing cost to the compact solver.
See `native_timing_summary.json`; native control library SHA-256 starts with
`f8cc2cefdea7721b`. Both native-control oracles reproduce the preserved control's
complete sampled trajectories and numerical bounds.

Two SIMD prototypes were tested and preserved, then removed from the proposed
source change:

- Four independent responses in the original order preserve all sampled pit/
  car states, but single-screen CPU is 4.226 s pit / 2.502 s car, versus the
  initial scalar's 4.139 s / 2.398 s. Packing, conflict checks, validation and
  scattered commits erase the vector arithmetic benefit in this implementation.
- Greedy coloring of independent responses changes contact order. Its pit worst
  pair depth rises from 0.330803 to 0.794605, exceeding the unchanged 0.5 limit,
  with 695 over-limit samples. Single-screen CPU is 4.293 s pit / 2.439 s car.
  Running that order with scalar arithmetic reproduces every sampled colored
  SIMD pit state and the same error. This isolates the ordering change as the
  cause of that regression; it does not establish that SIMD in general is slow.

The original scalar, ordered SIMD and final native scalar each match all 720 pit
and 600 car frame-state hashes from the control. Candidate/contact counts,
completed substeps and diagnostics also match for the final scalar. Hashes are
same-platform sampled trajectory checks, not proof of continuous protection.

## Quality and remaining cost

Full-run observers retain the control's pit worst pair depth 0.330803262, final
pair depth 0.133850183, final RMS speed 0.641348017 and floor depth 0.063147306,
with zero side-escape samples. The car retains standalone pair depth 5.522766113,
node pair depth 1.276077739 and maximum absolute beam error 8.214239180. These
car errors and the baseline's missing high-speed blocking are not repaired by
faster response. Existing rebound, energy, joint, sweep and structural failures
remain part of the inventory.

Exclusive instrumentation of the retained pit attributes 1.335 s to contact
gathering, 0.919 s to particle–static response, 0.618 s to compact response and
dispatch, 0.166 s to preparation, 0.055 s to copying results back, 0.285 s to
dirty polygon refresh and 0.135 s to interaction finalization. Nested enclosing
timings are not added to those costs. Even subtracting all compact response,
preparation and publication from this 4.136 s instrumented wall-time run leaves
approximately 3.297 s, or 4.58 ms/frame. This is a diagnostic inference, not a
clean CPU speedup estimate: further work must include gathering, mixed wall
response, geometry refresh or other costs, rather than only particle-pair math.

The rejected Goal 3 guard's historical cost is not added arithmetically to this
candidate: its query count and response behavior depend on the changed pipeline.
In particular, its tangential-travel and car-support regressions remain rejected.
Measure any replacement protection together with the selected solver. A focused
next investigation could compare an exact circle–wall response path and compact
gather/publication storage before reconsidering threads or changing solver
accuracy. This is a proposal for user direction, not authorization to begin it.

## Verification

- Production defaults: 25/25 selected Linux regressions pass. Compact baseline
  with two particle passes: 21/26 pass, with the same five failing executables
  as the matched control (20/25): head-on, sweep, joints, soft body and blocking.
  The unchanged baseline failure inventory is not a new solver regression.
  The baseline with configured particle effort also reports those same five.
- Focused mixed-mode/manual parity, slot reuse, allocation fallback,
  finite-intermediate rejection and 1,000 populated scratch ownership cycles
  pass in Release, production-default and ASan/UBSan builds. The sanitized
  selected baseline reports the same five numerical failures, with no sanitizer
  finding. Full ten-second car and twelve-second pit sanitizer diagnostics
  complete within the cap, also without findings.
- All 228 original stress endpoints/calibrations complete with the unchanged
  52 passes and 176 failures. All non-timing result fields match the preserved
  control. This suite uses original configured particle effort to isolate the
  compact solver. The selected four-substep/two-particle-pass head-on probe
  separately completes all five schedules: 60/120 Hz and one delayed schedule
  pass; 30 Hz and the other delayed schedule still tunnel.
- The continuing joint, soft-body and sweep inventory executables finish with
  identical control/candidate numerical lines. They preserve later cases after
  numerical failures that stop older assertion-based unit tests.
- Windows engine and diagnostic cross-builds pass. Independent installed-SDK
  car examples build on Linux and Windows; the Linux example launches from
  `/tmp` with staged assets. Its 60-frame startup smoke is explicitly partial
  against the example's inherited 60-second acceptance endpoint. No Windows
  runtime execution or full sixty-second car acceptance is claimed here.
- Relevant engine, diagnostic and example builds have no compiler warnings.
  Final `git diff --check` passes. The three untracked rejected guard files
  remain outside this change and untouched.

Three complete normal 1280×720 rendered runs per version/workload are retained.
The user accepted the compact car and pit behavior. One candidate car run ended
at 19/600 frames and is preserved as incomplete; the sequence stopped until the
user explicitly requested the remaining runs. Its replacement has a distinct
artifact name. No incomplete timing enters the ranges below.

| Candidate workload | FPS range | Frame-work p95 range | Simulation/elapsed range | Final lag range |
| --- | ---: | ---: | ---: | ---: |
| Car | 59.085–59.179 | 6.578–6.801 ms | 0.98474–0.98632 | 138.68–154.93 ms |
| Pit | 58.931–59.023 | 10.243–10.334 ms | 0.98218–0.98371 | 198.66–217.73 ms |

Frame work fits 16.67 ms, but sustained 60 FPS, simulation within 1% of elapsed
time, and no growing lag remain failed. Control runs also fail these gates.
The previously diagnosed presentation-pacing issue remains outside this change.
Subtracting observed frame-work p95 from 16.67 ms leaves about 9.87 ms car and
6.34 ms pit at the worst run p95. That is nominal whole-frame work space, not
an established safe wall-protection budget or proof that p95 costs add linearly.
The requested dedicated 4/5 ms physics budget has no positive measured margin.

## Outcome

**Milestone 13 Goal 2: failed.** Implementation, comparison and verification are
finished, but the 4 ms average / 5 ms p95 target and strict rendered gates remain
unmet. Retain the opt-in scalar improvement and the complete failure inventory
for user review; do not promote it to production or advance to Goal 3. The SIMD
experiments remain archived comparisons, not shipped solver modes. The user
accepted the retained behavior, not a waiver of performance or protection gates.
