# Predictive contacts and selective sweeps

Milestone 13 Goal 4a is a bounded comparison of two experimental collision
policies against the accepted compact scalar engine. The implementation parent
is `31457b8b5c6ed06e0b3944881109b8aadf287335`. Production defaults, public APIs,
scene data and the root build are unchanged. The rejected Goal 3 guard remains
rejected. This comparison does not complete Goal 4's blocking requirements.

**Outcome: comparison complete; both prototypes rejected for adoption.** Selective
sweeps are the stronger direction for impact behavior, but this implementation
does not provide complete protection or acceptable car behavior. No rendered
review was warranted after the headless behavior screen failed.

The user prioritizes particles and soft-body nodes against rigid obstacles,
and rigid bodies against rigid bodies, including moving/rotating obstacles.
Fast particle pairs remain a separate failing inventory. Sticking, lost bounce
or sliding, and degraded car deformation/handling are rejection criteria.

## Isolation and implementation

`tools/physics_collision_experiment/` is an independent CMake SDK consumer.
GNU-compatible link wrappers affect only its own executables; no engine archive
is edited. The private source headers must match the installed SDK. The test
engine archive is pinned separately in the evidence directory. Modes are selected
per process with `ROHR_COLLISION_EXPERIMENT=control`, `predict`, or `sweep`.
These are diagnostic settings, not engine configuration APIs.

Both prototypes snapshot starting poses, obtain the proposed movement through
ordinary force preparation/integration, then revise that proposed endpoint before
normal contact gathering. All revisions use checked physics commits. The
prediction variant also solves its active separating planes after each existing
contact/joint iteration. Complete composed substeps are the only experimental
entry point; standalone manual stages retain their existing behavior.

The broadphase builds a bounded obstacle tree and queries swept bounds. Rotation
uses an enclosing radius, rather than just the union of endpoint rectangles.
Narrow phase uses the existing convex polygon geometry even for particle–rigid
pairs. Particle–particle pairs remain on the existing circle solver. Concave
shapes and axis/transform-locked bodies are counted as unsupported by these
prototypes, with ordinary discrete response retained.

- **Prediction:** find a starting separating axis and limit closing velocity to
  the available separation divided by substep duration. Retain that plane during
  subsequent contact/joint passes. This deliberately simple prototype does not
  preserve the original impact velocity for later restitution. It can therefore
  lose bounce or respond to a plane the body would have passed beside. The
  experiment measures those defects; it does not establish that every predictive
  contact implementation must behave this way.
- **Sweep:** conservative advancement uses convex SAT separation and a bound on
  relative translational/angular travel. At a detected impact, apply an impulse
  to both responding bodies, including angular response and friction, then use
  the resulting velocities for remaining travel. Search is capped at 48
  iterations. Exhaustion is a reported warning and an unprotected case, never a
  successful cast. Initial touching/overlap uses ordinary response.

The sweep prototype processes initial candidate pairs in order, once each. It
does not schedule all impacts globally, rebuild candidates after a changed path,
or solve connected bodies at a common impact time. Repeated impacts on a moving
body are counted as secondary risks. It does not sweep solver corrections.
Prediction only retains planes found for the initial proposed motion; it does
not discover new obstacles introduced by later corrections. These limits are
central to the comparison and prohibit adopting either prototype as general CCD.
Motion is interpolated between the ordinary integrator's proposed poses; this
does not recover a curved acceleration-driven excursion within one substep.

The approach is informed by [Bepu's speculative/swept collision discussion](https://docs.bepuphysics.com/ContinuousCollisionDetection.html).
Its documented false-contact and restitution concerns motivated explicit miss,
bounce and sliding cases. [Box2D's contact-recycling discussion](https://box2d.org/posts/2026/10/contact-recycling/)
is a reference for possible later cost reduction, not an implemented optimization
or a new dependency.

## Measurement contract

- The focused matrix contains 18 declared cases, each twelve 1/240-second
  substeps. It covers translating particles/rigid bodies, thin/thick and angled
  walls, moving walls, bounce, sliding, corner misses, correction-created
  crossings, a rotating obstacle, embedding, filters, and fast particle pairs.
- Crossing checks observe every substep; geometric penetration is evaluated
  independently from published contact events. A fully escaped body with zero
  final overlap still fails. Rotation includes a swept-side check for the
  rotating-bar fixture. These are bounded fixtures, not a proof for arbitrary
  angular motion or multiple impacts.
- Bounce velocity, remaining normal travel and frictionless sliding have
  analytic expectations. Review limits are 0.1 world units for penetration/
  travel and 0.1 units/s for velocity errors. Initial embedding is a finite-state
  diagnostic, not a claim that deep overlap is resolved correctly.
- Workloads remain the normal 600-frame/ten-second car diagnostic, full
  720-frame/twelve-second 800-particle pit, and 1,440 ticks/twelve seconds of the
  intentionally overlapping 499-flies scene. Car and pit use 60 Hz/four substeps;
  flies retain 120 Hz/four substeps. Particle contacts retain two passes and
  other contacts four. No speedup is attributed to altered scenes or counts.
- The car still prints its inherited sixty-second acceptance as incomplete.
  The runner recognizes only the explicitly declared ten-second prefix as
  complete. This does not waive full-car acceptance.
- Serial runs use a 59-second interrupt/60-second kill cap. Endpoint completion
  and numerical success are separate. Numerical failures do not stop later
  cases; invalid state, fatal errors or interruption make the run incomplete.
- Clean runs exclude the optional state/deformation audit. Separate audit runs
  measure every-substep beam error, finite state, peak speed and a state hash.
  The audit's cost must not be presented as clean physics cost.

## Reproduction

From the repository root, inside `nix develop`:

```sh
cmake -S tools/physics_collision_experiment -B build/collision_experiment \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/build/example-sdk"
cmake --build build/collision_experiment --parallel 4
ctest --test-dir build/collision_experiment --output-on-failure --timeout 59
python3 tools/physics_collision_experiment/run.py \
  --build build/collision_experiment --output sandbox/collision_matrix \
  --workloads matrix --audit
python3 tools/physics_collision_experiment/run.py \
  --build build/collision_experiment --output sandbox/collision_comparison \
  --workloads car pit flies --runs 3
```

Use a new output directory for each run. A Python 3 interpreter must be available
separately if it is not in the development shell. Optional
`ROHR_EXPERIMENT_ENGINE_ARCHIVE` pins a matching installed-SDK archive. The runner
launches executables from `/tmp`, exercising staged asset resolution. A matrix
exit code of 1 records numerical failures while retaining all case endpoints.

Evidence and build commands are preserved in `sandbox/milestone_13/goal_4a/`.
The earlier `initial_matrix`, `screen` and `clean` blocks remain separate from
final verification; they must not be silently combined into one timing sample.

## Results

All 54 final Linux matrix cases reach their endpoints; ASan/UBSan reproduces the
same numerical verdicts for another 54 cases. Control and prediction each pass
4/18, but different cases. Sweeps pass 14/18:

| Case group | Control | Prediction | Sweep |
|---|---|---|---|
| Six stationary-wall bounce/travel cases | Fail | Blocks crossing, loses bounce/travel | Pass |
| Two frictionless sliding cases | Crosses wall | Pass | Pass |
| Two moving-wall impacts | Crosses wall | Blocks crossing, loses rebound | Pass |
| Two corner misses | Pass | False contacts | Pass |
| Two correction-created crossings | Fail | Fail | Fail |
| Rotating obstacle | Fail | Fail | Fail |
| Initial embedding finite-state / filtered crossing | Pass diagnostics | Pass diagnostics | Pass diagnostics |
| Fast particle pair, separate inventory | Fail | Fail | Fail |

At 1,000 units/s and restitution 0.8, the stationary-wall prediction case
rebounds at 288 rather than 800 units/s and loses 25.6 units of normal travel.
The sweep matches the expected rebound and remaining travel within the stated
review limits. This is an impact-policy result, not general collision safety.

Every-substep car audits confirm a material behavior regression:

| Car measure | Control | Prediction | Sweep |
|---|---:|---:|---:|
| Worst absolute beam-length error | 8.214239 | 15.825504 | 10.530682 |
| Worst relative beam-length error | 0.767751 | 1.523119 | 1.140521 |
| Peak collider speed | 485.921 | 1369.918 | 428.379 |

All audited states remain finite. The baseline's existing beam error remains a
failure; a lower peak speed does not establish better car handling. These data
do not isolate scheduling defects from structural weaknesses exposed by changed
contacts. They suffice to reject adoption without another poor car preview.

Three serial clean runs per workload/mode complete all 27 endpoints. Median
physics CPU seconds are:

| Workload | Control | Prediction | Sweep |
|---|---:|---:|---:|
| Car, ten simulated seconds | 2.764697 | 2.001677 | 2.761342 |
| Pit, twelve simulated seconds | 4.319738 | 4.744609 | 5.020344 |
| Flies, twelve simulated seconds | 5.186431 | 6.325430 | 7.842034 |

Prediction's lower car CPU accompanies worse behavior and is not an accepted
optimization. Sweeps have essentially unchanged total car CPU, add 16.22% pit
CPU and 51.20% flies CPU. These are net workload differences with different
trajectories, not exclusive protection costs. For example, median car integration
wall time rises from 148 to 717 ms, while gathering/solving become cheaper. Pit
integration rises from 218 to 961 ms. The sweep is not free on the car.

The tighter 4 ms average / 5 ms p95 budget remains unmet across both primary
workloads. Median per-run average/p95 physics CPU milliseconds are:

| Workload/update rate | Control | Prediction | Sweep |
|---|---:|---:|---:|
| Car / 60 Hz | 4.608 / 5.924 | 3.336 / 3.606 | 4.602 / 6.309 |
| Pit / 60 Hz | 6.000 / 7.750 | 6.590 / 8.356 | 6.973 / 9.263 |
| Flies / 120 Hz tick | 3.602 / 4.009 | 4.393 / 5.137 | 5.446 / 6.397 |

Flies tick timings are not 60 Hz frame costs. The old five-second car/six-second
pit total-CPU gates fit in this block, but accuracy and tighter budgets still
reject the candidates. No rendered, pacing, final sixty-second car, or full
milestone acceptance is claimed. Absolute timings vary; per-run samples and
hashes are in `final_clean/`, with separate state audits in `final_audit/`.

The operation counts identify concrete follow-up costs and omissions:

- Car sweeps: 96,264 casts, 6,230 responses, 632 exhausted searches, and 965
  secondary-impact risks. The exhausted searches are reported physics warnings.
- Pit sweeps: 336 casts and 190 responses, no exhausted searches. Most nearby
  candidates are already touching/overlapping and remain on discrete response.
  Candidate preparation and separation checks still cost time.
- Flies sweeps: 1,600,372 casts for 125 responses, with 507 exhausted searches.
  Merely being in a swept broadphase pair is too weak an admission test here.
- Prediction retains 50,609 car rows and makes 82,417 subsequent plane
  corrections; the car still deforms more. It makes 228,382 pit plane corrections.
- No workload exhausts the 32,768-pair storage cap. Zero unsupported-body counts
  in these scenes do not extend coverage to concavity or constrained transforms.

## Verification and next decision

Six focused checks pass in each of the three modes on Linux and with ASan/UBSan:
geometry, manual-stage/entity lifetime isolation, physics recovery, collision
response, particle collision mode, and force preparation. All six sanitized
candidate workload endpoints complete without sanitizer findings. Sweep flies
returns numerical failure because of its reported search limits, not memory
errors. An initial sandboxed sanitizer launch could not finish under ptrace;
it is retained as incomplete and was rerun outside that restriction with leak
detection enabled. Runner checks separately verify numerical failure, incomplete
execution, and sanitizer failure even after an endpoint marker was printed.

All tools and the independent installed-SDK car consumer cross-build for Windows
with MinGW. No native Windows execution was available. Linux consumers launch
from `/tmp` with staged assets. Final builds emit no C compiler warnings.
The previous engine accuracy inventory in the
[compact solver report](physics_compact_solver.md#promotion-verification) remains
unchanged; this comparison does not relabel those failures as passes.

**Recommendation:** retain the current engine and pursue a narrower sweep-based
follow-up only after user review. First, reject unreachable translating pairs
using their separating axis before casting, and measure reusable static geometry/
candidate storage. Second, feed swept impact information into a coherent contact
and connected-body solve instead of independently advancing each pair. Track
correction-created movement conservatively and treat subsequent impacts and
rotation-search failures as explicit cases requiring resolution. Preserve the
original approach speed for restitution. Test those changes separately so a
cheaper but less active or more distorted simulation cannot masquerade as a win.

These are proposed follow-up experiments, not implemented optimizations or a
promise of meeting the budget. Stop for review and commit before pursuing them.
