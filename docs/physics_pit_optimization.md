# Baseline pit optimization

Milestone 13 Goal 2 optimizes the opt-in simpler baseline before adding collision
protection back. The [roadmap](../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline)
owns the goal order. Production defaults and public APIs remain unchanged.
**Outcome: Failed against the performance gates.** The retained four-pass
candidate improves pit CPU time by 43.3%, but its 7.664-second median misses the
six-second target; all three rendered runs also miss the strict real-time gates.
The user accepted the preview's behavior, not those performance failures.
Stop for review and explicit direction before restoring systems.

## Scope and provenance

- Immediate parent: `7e337fc2cb4f5ed93e4d2f4d7f27cee24b7fa6df`.
- Comparison control: Goal 1's simpler baseline, preserved SDK archive SHA-256
  `6c5605856e08c86e846ea1049fe0c3b4d451aa660d4192a4cbb949b5df6e2d85`.
- Historical schedule: `a8a7940a6e6d039c80aed20c6f6b2624169e1ee3`.
- Pre-rebuild checkpoint: `0b254c045b806fb13c022cd7470f063a03a130db`.
- Frozen accuracy reference: `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`.
  `main` is excluded; previous evidence stays in place.

Artifacts are under `sandbox/milestone_13/goal_2/`: parent source archive,
preserved control SDK, candidate archives/source patches, independent installed-SDK
consumers, scripts, build caches, logs and individual run records. The final
manifest pins the retained implementation and executables. Earlier experimental
archives and failed attempts remain identified rather than silently replaced.

Both workloads retain 60 physics ticks/s, four configured substeps and four
solver passes. The authored car contains 200 standalone particles and 100 nodes;
its ten-second prefix is diagnostic, not the final sixty-second acceptance run.
The unchanged pit contains 800 particles, diameter 5 and restitution 0.5, over
twelve simulated seconds. Geometry, materials, body counts and input sequences
are unchanged. Existing runtime setters still control tick duration, substeps
and solver iterations; there is no new particle-pass API or hidden iteration cap.

The user preferred retaining physics quality over sacrificing substantial
behavior to save another second near the six-second target. Four passes are
retained for review. The user found the single four-pass pit preview acceptable;
this is provisional motion feedback, not final Milestone 13 quality acceptance.
The six-second budget, rendered gates and 0.5-unit pit overlap limit are still
reported as measured criteria. The overlap limit is not solver correction slack.

## Retained implementation

Changes activate only within `ROHR_PHYSICS_SIMPLE_BASELINE=ON` complete updates
and complete-substep calls. The option remains OFF by default. Manual stages
retain their existing algorithms and explicit composition/ownership contracts.

1. Reuse validated, nonrotating particle geometry during response. Particle-pair
   correction computes the live circle without repeatedly translating every
   polygon vertex. Mixed contacts and the existing dirty flush still update
   world polygons. Invalid/extreme geometry falls back to full validation; the
   cached polygon's pose stamp is never falsely advanced.
2. Specialize point response for two nonrotating particles. Preserve restitution,
   accumulated impulses, friction, directional response masks, effective pair
   masses and sequential validated velocity commits, while omitting zero angular
   lever-arm work. Rotating circles and ordinary rigid contacts retain the
   existing response path.
3. Construct rigid constraints directly in existing owned storage. Initialize
   identity, response, contact, counts and validity flags without clearing/copying
   unused manifold payloads. Readers remain gated by those counts/flags. Existing
   allocation failure, destruction and generation checks remain in force.
4. Use a spatial hash for particle-pair discovery in beam-free complete substeps.
   Polygon interactions still use the tree. Mixed-size particles use a cell width
   at least the largest diameter and inspect neighboring cells conservatively.
   When the largest diameter exceeds four times the mean, use the existing tree
   instead. That threshold is a measured heuristic, not a claim of universal
   optimality. Sizes are reconsidered each gather; no cross-step topology cache
   or sleeping state is introduced.

Cells are not clamped to scene boundaries. Full signed cell coordinates
distinguish unrelated cells sharing one of the 4,096 hash buckets. The engine's
world-coordinate validation still applies; this does not create an infinite
numeric world. Out-of-range cell conversions use an explicit all-pairs fallback
instead of overflowing or dropping candidates. Grid scratch storage is bounded
by the existing entity capacity and does not allocate per query.

Beam-coupled scenes retain tree discovery and its original contact order. An
initial grid experiment omitted particle leaves needed by beam gathering; its
car results are rejected. Restoring those leaves fixed the beam regression, but
grid order still worsened node overlap without a worthwhile car speed benefit.
The retained tree path avoids that tradeoff. Manual gathering always builds the
complete tree. The isolated response/geometry changes preserve sampled car
trajectories; the grid deliberately changes pit contact order, so its physics
results are compared numerically rather than claimed bit-identical.

The grid can still degrade with crowded cells, adversarial hash aliases or
unfavorable size distributions. The outlier fallback addresses the measured
large-particle case, not every possible distribution. A multilevel grid or
separate oversized-particle structure would require its own cost/coverage study.

## Investigated alternatives

Initial clean screens were single runs, not final acceptance medians:

| Experiment | Pit CPU seconds / 12 simulated | Worst particle overlap | Decision |
| --- | ---: | ---: | --- |
| Goal 1 control, four passes | 13.489 | 0.303774 | Comparison |
| Two particle passes only | 9.633 | 0.490913 | Near overlap limit; retain evidence |
| One particle pass | 7.135 | 1.791743 | Reject; exceeds 0.5 by 1.291743 |
| Geometry reuse, four passes | 10.751 | 0.303774 | Retain; sampled states unchanged |
| Geometry plus specialized response, four passes | 9.622 | 0.303774 | Retain; sampled states unchanged |
| Same response changes, two passes | 6.892 | 0.490913 | Optional reduced-effort candidate |
| Add in-place construction, two passes | 6.673 | 0.490913 | Retain construction; not the pass reduction |
| Velocity response only on first pass | 8.697 | 0.976530 | Reject; exceeds 0.5 by 0.476530 |
| Tree topology reuse, bounds refreshed, two passes | 6.644 | 0.521108 | Reject; exceeds 0.5 by 0.021108 |
| Initial grid, two passes | 5.493 | 0.330803 | Exploratory pit result; original car path rejected |
| Initial grid, four passes | 7.339 | 0.266482 | Exploratory pit result; corrected and reverified below |

In those grid pit runs, two passes increased the final remaining overlap from
0.049292 to 0.133850 and final RMS speed from 0.575739 to 0.641348. Worst overlap
increased from 0.266482 to 0.330803; both remained below 0.5. Mean particle height
at the endpoint was 39.755395 versus 38.665855, showing a different pile. Floor
depth was lower with two passes (0.063147 versus 0.129438). These metrics do not
make quality a single scalar or establish equivalent motion. No reduced-pass
configuration is installed as a default in this goal.

Sleeping was screened as an upper bound, not implemented without wake-up and
support correctness. In the two-pass tree candidate, only 3.94% of particle
samples had stayed below speed 0.1 for half a second; 4.49% stayed below 0.5.
The four-pass grid settled differently: 0.40% below 0.1, but 52.59% below 0.5.
These are eligible samples, not avoided solver work or validated sleep savings.
Impacts, forces, moving support, filter/shape changes and coupled nodes would
require additional wake-up/maintenance work. No significant quality change was
introduced merely to claim those theoretical savings.

An isolated 800-particle, 120-gather sensor diagnostic tested discovery separately
from dynamics. Before the outlier fallback, one radius-250 particle among radius-2.5
particles cost 237.6 ms versus the control's 85.3 ms. The retained fallback screen
cost 98.8 ms. Equal-size and distant-cell screens cost 46.6/43.1 ms versus
75.6/77.2 ms; varied radii cost 62.5 ms versus 87.2 ms. These single-run small-scene
screens justify the fallback and expose overhead; they are not workload headroom
acceptance or a universal speedup claim.

## Verification and reproduction

Run scripts inside `nix develop`. `build.py baseline [sanitizers|windows]` builds
the selected engine and independent SDK consumers; `build.py control` uses the
preserved Goal 1 archive, and `build.py default` verifies the option-disabled
engine. `final_trials.py retained` runs the three serial clean comparisons;
`profile_trials.py` separates exclusive profiles, numerical oracles and the
pair-work audit. `rendered_one.py` checks the actual Sway client dimensions
before releasing a diagnostic-only start gate. One early window-tracking error
ended at zero simulated seconds; its incomplete record is preserved, and the
fixed attempt has a separate label. User interruption stops the visual sequence.

Each runtime execution has a one-minute wall cap, including shutdown. Final runs
use AC power, streaming last confirmed closed, and no concurrent builds/tests.
The host CPU governor was `powersave`; no governor or machine settings were
changed. Report all three measurements rather than selecting the fastest one.
Profile/oracle instrumentation is excluded from clean CPU timing. Stage tables
sum exclusive time only; pair response rows are subsets of rigid-solving time.

The expanded baseline regression covers dynamic/static/one-way/sensor pairs,
100:1 mass ratio, friction, offset circles, synchronized final hitboxes,
mixed polygon/particle discovery, changing radii, deletion/slot reuse,
far-away negative coordinates, aliased hash buckets and guarded extreme offsets.
Ownership, elapsed-time consumption and release of complete-substep policy before
callbacks/manual calls remain covered. Relevant Linux and ASan/UBSan suites
retain 20 passes and five known numerical failures. The option-disabled suite
passes all 24 tests. A six-second sanitizer drop/pile and one-second car diagnostic
complete without sanitizer findings. Windows engine/consumer cross-builds pass;
native Windows execution was not performed. The ordinary standalone car also
builds independently against the installed SDK on Linux and Windows; its short
Linux smoke is not the final sixty-second scenario.

All 228 inherited stress endpoints complete: 52 numerical passes and 176
failures, the same verdict counts as Goal 1. `comparison.json` preserves each
failure line with measured values and available limits. Existing failures include:

- Four of five original head-on schedules miss the 1,000-units/s impact, with
  rebound error 1,500 despite zero sampled overlap. The original fixture uses
  one configured substep; zero overlap does not prove collision protection.
- Thick-wall cases at 10,000 units/s still cross with no contact; recorded depth
  is about 975.5 against 0.1 (about 975.4 excess). The 1,000-units/s and initially
  embedded cases remain distinct in the retained logs.
- Pin/weld inventory spin errors remain about 665.48/614.34 against 0.1;
  current anchor errors can be small while motion quality fails.
- Correction-created beam contact is still absent in its regression. The bounded
  gather-once schedule delays rediscovery; it is not fixed by faster searching.
- Fixed-step stack/elastic fixtures retain penetration, energy and stability
  failures. Those require the later blocking/structural goals, not a silent
  tolerance change in this optimization.

The ordinary joint regression aborts on its existing numerical assertion; the
separate inventory replay reaches its endpoint and records the remaining cases.
Numerical failures are not relabeled passes. The 0.5 pit/head-on review and 0.1
stress penetration criteria are unchanged. Final quality and production-default
selection remain Milestone 13 Goal 7 work.

## Final measurements

Three serial clean runs per workload/version, with all declared diagnostic
endpoints complete:

| Workload | Goal 1 control CPU seconds | Retained candidate CPU seconds | Candidate median | Budget |
| --- | --- | --- | ---: | ---: |
| Ten-second car | 3.159, 2.883, 3.337 | 3.032, 3.105, 3.159 | 3.105 | 5.000 |
| Twelve-second pit | 13.239, 15.338, 13.511 | 7.652, 7.682, 7.664 | 7.664 | 6.000 |

The pit uses 43.3% less CPU, but still needs 21.7% less work to reach its
six-second budget. It runs at 1.57× real-time physics throughput. The car has no
convincing material speed improvement and retains all 600 sampled state hashes
and numerical measurements from Goal 1. Option-disabled car/pit runs match all
600/720 historical sampled states and report counts. Within each measured
version, trace/oracle states match and exclusive timing accounting closes exactly.

| Full-run pit quality | Control | Retained candidate |
| --- | ---: | ---: |
| Worst particle-pair overlap | 0.303774 | 0.266482 |
| Final particle-pair overlap | 0.051446 | 0.049292 |
| Worst floor-circle depth | 0.134687 | 0.129438 |
| Escape samples | 0 | 0 |
| Final RMS speed | 0.508867 | 0.575739 |
| Final mean particle height | 38.674769 | 39.755395 |

The inherited car errors remain: maximum standalone-pair overlap 5.992004,
node-pair overlap 1.289857, absolute beam-length error 9.826794 and relative error
1.049554. Matching that trajectory does not make those errors acceptable.
At the runtime's four configured substeps, the separate head-on replay completes
all five cases with three passes/two failures: normal 60 Hz detects the collision
with 0.000227 overlap, but the 30 Hz and larger-delay cases still tunnel with
rebound error 1,500. These are separate from the original one-substep stress run.

### Exclusive physics costs

Milliseconds over one full instrumented pit run, not clean CPU acceptance:

| Stage | Control | Candidate |
| --- | ---: | ---: |
| Force preparation | 90.9 | 92.9 |
| Integration | 194.3 | 201.7 |
| Contact gathering | 2769.5 | 1461.4 |
| Joint gathering | 17.8 | 18.0 |
| Rigid/particle solving, including dispatch | 10076.7 | 6008.8 |
| Joint solving | 83.0 | 87.5 |
| Finalization, including geometry flush | 1256.7 | 1418.1 |
| Reporting | 58.8 | 56.8 |
| Orchestration | 163.0 | 159.5 |

There are no subdivision sweeps or soft contacts in the pit. Within candidate
rigid solving, particle-pair response costs 3721.9 ms, particle–static response
980.1 ms, polygon response 10.5 ms and remaining solver/dispatch work 1296.3 ms.
Within finalization, instrumented dirty-hitbox flushing costs 1284.7 ms.
These are subsets, not additional costs to add to the table. Geometry-flush cost
did not disappear into a claimed response saving; it is explicitly included.
The car profile records 1043.0 ms gathering, 1027.9 ms rigid response, 231.7 ms
joint response, 119.3 ms soft response and 301.7 ms finalization; the full stage
and function breakdown is in `analysis.json`.

### What the millions of visits mean

| Particle-pair work over the full pit | Control | Candidate |
| --- | ---: | ---: |
| Distinct entity pairs seen during the run | 2,253 | 2,006 |
| First solver visits to gathered pair occurrences | 3,644,445 | 3,531,811 |
| Total response visits, four per occurrence | 14,577,780 | 14,127,244 |
| Visits with retained solved state | 10,916,999 | 10,585,596 |
| Visits changing a position | 13,727,740 | 12,702,820 |
| Visits changing a velocity | 12,214,002 | 13,521,517 |
| Visits changing neither | 339,309 | 169,171 |

Position/velocity rows overlap. Most repeated visits change state; they cannot
simply be deleted as exact no-ops. Control particle response costs about 0.539 µs
per instrumented visit (Goal 1's earlier 8033.5 ms was about 0.551 µs); the
candidate costs about 0.263 µs. The primary gain is cheaper work per visit, plus
cheaper discovery, rather than eliminating most response visits.

Both versions complete 2,880 pit substeps and 11,520 solver rounds. Reported
candidates fall from 4,275,022 to 3,971,047. The inherited `CONTACT_COUNT` counter
is not a count of all particle contacts: the scalar particle-pair branch does
not increment it before returning. Its retained values (153,047/148,745) must not
be presented as total physical contacts. The independent pair audit above supplies
the particle visit/occurrence counts. This pre-existing counter limitation is
preserved visibly rather than changing counter semantics in these comparisons.
Link wrappers count instrumented call boundaries; same-object calls can remain
inside their parent timing. In particular, joint processing also invokes an
already-empty second dirty flush; the 11,520 instrumented flush calls are not a
claim that no other empty calls occur.

### Rendered results

Actual client size is 1280×720 in each accepted measurement. All six paced runs
reach twelve simulated seconds, but none passes the strict gates:

| Version/run | FPS | p95 frame work, ms | Simulation / elapsed | End lag, ms |
| --- | ---: | ---: | ---: | ---: |
| Control 1 | 42.72 | 28.98 | 0.7120 | 4853.1 |
| Control 2 | 39.68 | 30.99 | 0.6613 | 6144.9 |
| Control 3 | 40.22 | 30.97 | 0.6703 | 5901.6 |
| Candidate 1 | 58.69 | 17.22 | 0.9782 | 267.3 |
| Candidate 2 | 58.20 | 17.98 | 0.9701 | 370.5 |
| Candidate 3 | 58.80 | 17.10 | 0.9801 | 244.2 |

Candidate p95 exceeds 16.67 ms by 0.43–1.31 ms. Simulation falls 2.0–3.0% behind
elapsed time, outside the 1% limit, with accumulated lag. The separate uncapped
run reaches 72.93 FPS, 16.30 ms p95 work and 1.216× simulation throughput; it is
not paced acceptance. It spends 7470.5 ms CPU in physics and 1574.0 ms wall time
in drawing/presentation, excluding pacing, over 720 frames: about 10.4 ms and
2.2 ms per frame respectively. Graphics is secondary to physics in this scene.
CPU and wall figures have different measurement boundaries and are not an exact
additive decomposition. No intentional slow motion or debug drawing is enabled.

## Review direction

Retain the verified reductions for review, but do not advance into restoration
as though sufficient headroom were established. The user explicitly stressed
that these isolated tests must leave room for a full game. A proposed future
budget is 4 ms average physics per 60 Hz frame with p95 below 5 ms; this is an
engineering proposal, not a universal industry standard or an approved change
to this goal's gates. Four milliseconds corresponds to 2.88 CPU seconds for this
pit, requiring about another 2.66× speedup from the retained candidate.

The next feasibility investigation should prioritize particle response and its
shared solver/geometry work: compact particle state/contact processing, prepared
invariant data, and refresh only when a consumer needs current polygons. A
reduced-pass alternative remains an explicit quality experiment. Near-zero idle
visits mean simple no-op skipping is unlikely to supply the needed reduction.
The current profile alone does not prove how much any proposal will save.

Reliable particle/node–rigid and rigid–rigid blocking must eventually fit inside
the budget. Additional collision-risk protection should be selective; making
every particle interaction trigger world-wide work would repeat the earlier
problem. Final performance must be measured with protection enabled, rather than
accepting a fast tunneling baseline. A separate graphics/frame-pacing study may
be useful, but these measurements do not justify making it the main next
optimization goal. No later roadmap goal has been started or reordered.
