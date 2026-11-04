# Milestone 12 Goal 10 — Ranked bottleneck investigation

**Roadmap numbering note:** this report retains the goal numbers used when its
experiments completed. The later [quality/work experiment](physics_quality_experiments_plan.md)
is the new Goal 13; historical Goals 13–17 below map to current Goals 14–18.
Results, recommendations and archived evidence are otherwise preserved.

**Complete for review.** Production physics is unchanged. Parent `f9abe365e83a2e73568cb389a29adf2ef5a4439d`;
frozen reference `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; `main` is excluded.
Follow the [ordered plan](physics_bottleneck_investigation_plan.md).
Evidence is retained under `sandbox/milestone_12/goal_10/`.

## Common measurement and behavior context

Baseline production is identical to Goal 8 across engine, headers, examples,
tests and measurement tools. Before reuse, 1,841 Goal 9 artifacts, 999 Goal 8
artifacts and 1,457 frozen-reference artifacts were rehashed successfully.
Goal 9's verified binaries, corrected run selection and exact configuration
remain authoritative for reusable results; its unfiltered first car oracle is
explicitly excluded from physical penetration claims.

Every new main workload observer uses 60 Hz/four configured substeps/eight
passes, unchanged scenes/materials and existing PP protection. Four observers
each finish the full 12-second pit and authorized 600-update car prefix, with
exact parent state/work hashes and identical production counters. The car prefix
is 10.000000522 seconds; its original full-60-second completion flag remains
false. No stopped full-car or rendered trials were restarted.

The controlled clean Goal 9 medians remain 34,465.826 ms physics CPU for the pit
and 68,498.759 for the car prefix: 2,872.2 / 6,849.9 ms per simulated second,
47.869 / 114.165 ms per update, p95 93.306 / 200.970 ms. Maximum single-update
CPU across its three serial trials is 286.627 / 2,933.600 ms. This is already
2.87× / 6.85× the mean real-time physics budget before rendering. Goal 10 adopts
no optimization, so it establishes no improved rendered FPS or acceptance.

Representative **exclusive** Goal 9 source-observer costs, milliseconds over
the whole workload (not clean CPU totals):

| Subsystem | Pit, 12 seconds | Car prefix, 10 seconds |
|---|---:|---:|
| Particle–particle response | 13,662.04 | 9,415.43 |
| Soft gathering | 8.72 | 13,137.16 |
| Solve-iteration remainder | 4,936.17 | 12,311.40 |
| Broadphase queries | 8,427.27 | 9,471.04 |
| Broadphase build/refit | 2,504.55 | 3,945.32 |
| Pair filtering | 2,618.10 | 3,807.68 |
| Shared sweep preparation | 1,505.75 | 2,769.79 |
| Shared sweep queries | 2,615.20 | 3,928.18 |
| Overlap-only response visits | 0 | 3,022.22 |

These selected leaves are disjoint within that partition; they omit other
leaves and must not be added to inclusive phase totals or wrapper measurements.
The remainder includes joint/soft/lock work not separately timed by this fine
partition. Full source partitions and root conservation are retained in
`workload_verification.json` for the new observers.

After Q1, the user authorized continuing diagnostics while streaming from the
same PC. Q2 onward records streaming active and does not pass the runner's
`--controlled` confirmation. AC is recorded and builds do not overlap trials.
New instrumented update wall times follow; they include observer work and are
**not matched speedup comparisons** against clean CPU. Q2 adds duplicate
selector calls; Q3 adds residual/state scans; Q4 adds graph and bound tests.

| Observer/workload | Total ms | ms/simulated second | Mean update ms | p95 ms | Worst ms |
| q1_pit | 41152.31 | 3429.36 | 57.16 | 116.65 | 363.98 |
| q1_car | 78709.94 | 7870.99 | 131.18 | 233.71 | 3369.20 |
| q2_pit | 47124.79 | 3927.07 | 65.45 | 138.10 | 436.49 |
| q2_car | 92318.39 | 9231.84 | 153.86 | 278.82 | 4133.40 |
| q3_pit | 51728.24 | 4310.69 | 71.84 | 154.04 | 481.04 |
| q3_car | 95462.98 | 9546.30 | 159.10 | 286.51 | 4065.89 |
| q4_pit | 49722.79 | 4143.57 | 69.06 | 138.27 | 437.48 |
| q4_car | 80689.50 | 8068.95 | 134.48 | 247.93 | 3267.97 |

Unchanged-behavior error evidence is reused with exact trace provenance. Full pit
PP depth is 0.019971246, final depth 0.016193570, floor-circle depth 0.023434162,
with zero side escapes and zero samples above the 0.5 PP review limit. Final/
peak particle RMS speed is 0.403878340 / 24.214038849. The corrected car-prefix
oracle records maximum standalone PP depth 0.041826790, node-involving depth
0.015560134, beam absolute error 13.419484546, relative error 1.472364484, and
final/peak particle RMS speed 109.897861032 / 211.762253841. Car values bound the
prefix only. No hard car deformation or pit-wall threshold is invented.
Retain all numerical results, including the historical soft-body delayed CPU
failure (367.545 ms against 50 ms). The 0.5 PP and 0.1 extreme-fixture review
limits remain unchanged and are not solver correction inputs.

The worst 5% of updates account for about 15% of observed root time in both
scenes. Fixing the car's striking tail alone cannot remove its sustained cost.
Do not add the individual opportunity estimates: fewer steps/passes also reduce
the gathering, response and traversal work counted by other questions.

## 1. Repeated discovery

The current design retains contacts and impact targets through a substep,
refits existing broadphase leaves and queries moved sources. It detects contacts
created by corrections and preserves manual gathering after topology/shape edits.
Its cost is repeatedly discovering which poses changed, plus broad soft refresh.

**Causal probe:** a shadow set collects existing hitbox-dirty notifications.
Every normal refit compares that set with the engine's exact pose comparison;
it never changes execution. Full pit and authorized car prefix retain exact
Goal 9 state/work hashes. Every observed changed collider was notified.

| Counter | Pit, 12 seconds | Car, 600 updates |
|---|---:|---:|
| Refit visits | 28,182,891 | 127,017,450 |
| Changed poses | 9,221,178 | 11,717,365 |
| Notified but unchanged | 9,195,705 | 26,819,054 |
| Changed but unnotified | 0 | 0 |
| Visits avoidable using existing notifications | 34.65% | 69.66% |

These are visit fractions, not predicted CPU savings. Notifications also occur
outside refits; a worklist still needs deduplication, ordering, clearing and
pose comparison. Velocity/contact bookkeeping currently creates many redundant
notifications. Existing source pose-commit tracking is a better future place
to distinguish pose changes from velocity-only changes. The direct-array pose
edits in `tests/contact_gathering.c` deliberately bypass notifications: a
notification-only replacement without a manual-stage fallback would break that
contract even though both workloads show zero misses.

Of 18,159,120 eligible car beam visits, 4,035,360 are uncached, 235,325 have changed
endpoints, 13,618,435 have unchanged endpoints but a changed rigid somewhere,
and 270,000 skip because neither changed. The large unchanged-beam count is
caused by the global `rigid_changed` flag, not beam deformation. However a changed
rigid can enter an unchanged beam, so skipping those queries outright is unsafe.
The existing callback skips unchanged pairs only after traversal.

**Candidate-envelope control:** `envelope_probe.py` demonstrates AABB pair reuse
while every current bound remains inside its stored envelope. Leaving it forces
rediscovery before reuse; shape/filter/lifetime edits force invalidation and
handles include generations. An adversarial correction creates an initially
absent pair and demonstrates the required fallback. Across 400 frames/128 bodies,
0.1/0.5/1-unit envelopes require 83/6/5 rebuilds and 68,239/174,244/176,800 candidate
visits, with zero missed eligible overlaps. All three still perform 51,200
containment checks. This is a correctness/maintenance model, not an engine
prototype or timing claim; larger envelopes trade rebuilding for false candidates.
Envelope margins are unrelated to permitted penetration. The production engine
already has shape-edit, deletion/reuse and manual-gather tests; their unchanged
source/SDK verification is reusable, but does not verify a future cache.

**Decision:** retain current response and contact discovery until a spatially
limited refresh is implemented and verified. Prioritize a pose worklist with
stable ordering/manual fallback and reverse node-to-beam dependencies in Goal 12;
query changed rigid bounds against unchanged beams rather than all beams against
the whole tree. Conservative per-solve candidate envelopes are a second option,
requiring containment/expansion and structural revisions. Confidence is high in
the measured cause, moderate in net performance benefit. Effort is medium;
Goal 14 must validate correction-created soft contacts and Goal 15 owns shared
invalidation. Acceptance requires exact traces where ordering is preserved,
full endpoint error comparisons where it changes, edit/lifecycle/manual tests,
and clean complete-update timing including maintenance. No speedup is established.
Each cache consumer needs its own revision/consumption boundary: flushing a
rendering or hitbox dirty flag must not consume another stage's pending refresh.

## 2. Adaptive restrictions

**Causal finding:** the worst car update is reproduced exactly, including 1,442
steps and 1,438 final soft-sweep restrictions. Pair `(350,0)` is generational
handle `(65887,65537)`: `Car_rear_tire_beam_155` against the first static terrain
rectangle, centered (-60,-225), extending x=-300..180 and y=-235..-215.
The beam joins rear tire nodes 47/48 (indices 106/107, handles 65643/65644),
with thickness 2, stiffness 1477.5 and masses 0.400000006 each.

The beam compresses to a minimum length 0.0001863, median 0.04509 during this
update. Its rotation allowance is radius times **full relative endpoint speed
/ length**, including axial compression. That allowance reaches 573,868 units/s,
median 2,680.75; instantaneous rotation alone has median 11.38 but also has a
large near-collapse peak. Replacing the full bound with instantaneous transverse
speed is therefore not proven conservative. Acceleration and possible endpoint
crossing must remain bounded over the entire proposed interval.

The sweep uses a full rectangle. Discrete response uses endpoint cutouts where
node collisions own the response. Of 993 retained records for this pair, 978 are
nonresponding and only 15 respond. Those 15 have 120 solver visits but only 15
currently overlap. Maximum final recorded geometric depth is 0.0003052. Across
1,442 steps, both nodes advance at integration; only 44 of their 2,884 combined
post-integration pose samples change during solving. The static terrain does
not move. Thus this is not a float-pose stall or 1,438 effective beam impacts.
It combines an expensive near-collapse bound with sweep/response mismatch.

The existing side-plane extension declines 1,437 times because its initial
margin is exhausted. Its plane test includes distant vertices of the long
terrain rectangle; the ordinary projection/local-motion fallback consequently
remains active. `audit_q2.log` preserves all branch/depth/closing/acceleration,
identity, pose and contact observations for the update. The misleading
`response_depth` copied into a nonresponding temporary is not used as a physical
penetration measurement.

**Same-interval selectors:** observational calls evaluate all four selectors on
the original proposal, then normal selection/integration runs unchanged. Both
workloads retain the exact parent state/work trace. The extra work is separately
classified as `probe` in the source timing partition; wrapper call totals include
these observational calls and are not production call counts.

| Selector | Pit proposals shortened | Car proposals shortened |
|---|---:|---:|
| Joint | 0 | 0 |
| Elastic | 0 | 20,759 |
| Rigid sweeps | 2,047 | 16,903 |
| Soft sweeps | 0 | 16,910 |

Car same-interval minimum ownership is elastic 6,209, rigid 6,637, soft 9,975,
unchanged 2,400. This differs from the sequential winner histogram because a
selector's result can depend on its input interval. Removing the soft selector
on these frozen worst-update states leaves a median other-bound interval
0.0003069 seconds, 95.61 times the actual duration. That is an opportunity
indicator, **not a resimulated step-count or speedup prediction**.

**Focused alternative control:** for a plane, a thick segment is contained in
the convex hull of equal-or-larger endpoint disks; its support cannot extend
past both disks' supports. All 1,442 recorded states have conservative swept
x extents within this floor's span over the remaining other-bound interval.
A mathematical control checks 4,000 accelerated/high-speed/overlap/grazing-capable
samples with zero support-bound violations and retains a counterexample for a
small obstacle between separated endpoints. A plane-equivalence certificate
could avoid duplicate beam restriction while retaining endpoint protection.
This is a geometric control, not verified engine CCD: response masks, radius
changes, finite edges, correction-driven changes, initial overlaps and future
acceleration invalidation need explicit implementation tests. General corner or
beam-pair cases must fall back. Targeted continuous response is another option,
but it must resolve the same geometry/ownership mismatch before advancing time.

**Elastic cap is separate:** the 0.0008037056-second cap is speed independent,
from accumulated stiffness/mass. Local elastic integration would preserve small
spring steps while avoiding repeated unrelated work; whether bodies are safely
separable is question 4. A linear oscillator control retains the observed
single-beam stiffness/mass and separately tests the network's rate bound.
At that network bound, Verlet at the cap has at most 1% energy error, versus
26.88% at one configured 1/240-second step. Implicit Euler damps away essentially
all energy over ten seconds; implicit midpoint preserves energy but changes
phase. These are mathematical mode comparisons, not the nonlinear car's error
bounds or accepted integrators. Stability alone cannot justify loosening the
cap. A full alternate spring formulation is deliberately unimplemented because
it changes numerical behavior and requires a separate decision.

**Decision:** high confidence that tighter, response-aware beam bounds address
this tail's cause. Start with restricted plane/endpoint equivalence and bounded
near-collapse geometry, retaining conservative fallbacks. Medium implementation
effort in Goal 14, shared traversal in Goal 11, scheduling in Goal 15. Do not
remove PP protection, clamp tiny steps, or replace the integrator here. Acceptance
requires the actual worst update, finite-edge/corner, accelerating, grazing,
initial-overlap and 1,000-units/s-per-particle regressions, plus full car/pit
behavior and clean serial timing. The unchanged-source prior regression results
remain valid; these geometric/numerical alternatives are not platform-verified
behavioral candidates.

## 3. Response and overlap passes

**Causal probe:** snapshots before/after every pass record pose/velocity changes,
new constraints, retained particle state and projected impulse residuals. Separate
per-visit counters record whether PP/PR/RR/overlap calls change either endpoint.
Both workload state/work hashes exactly match the parent. Residuals use current
circle geometry and retained normal/rebound/friction impulses; they measure
remaining projected impulse updates in velocity units, not new acceptance limits.
They cover currently overlapping retained PP pairs, not missed pairs or a proof
of whole-system convergence. Changes to normal geometry can affect these values.

| Measurement | Pit | Car |
|---|---:|---:|
| Pass-1 PP visits | 4,768,739 | 3,779,279 |
| Pass-8 PP visits | 4,830,657 | 3,781,328 |
| Pass-8 PP visits changing position | 4,584,597 | 2,032,102 |
| Pass-8 PP visits changing velocity | 4,292,889 | 3,764,135 |
| Mean summed normal residual, pass 1 → 8 | 29.93 → 26.67 | 13.90 → 11.37 |
| Worst normal residual, pass 8 | 20.16 | 37.86 |
| Worst PP depth, pass 8 | 0.0199712 | 0.0418268 |
| New records discovered in pass 8 | 21,489 | 328 |
| Full residual/body scan CPU, all passes | 4,513.06 ms | 5,329.14 ms |

The numerical residuals are not monotonic in a coupled sequential solve. Pit
position fraction increases from 1/8 to 1 across passes; later corrections can
activate more contacts and make earlier ones imperfect again. Car's soft path
uses full position fraction but joints, locks and soft response remain coupled.
The existing small polygon block solver cannot supply convergence for large
particle sets: its limits are 32 pairs/bodies and 96 rows, and particle endpoints
use the iterative path. That missing flag does not mean later particle work is
unnecessary.

**Disproof of a cheap naive exit:** 684 pit steps have an unchanged pass after
pass 1, but 64 change again in a later pass. Skipping all remaining passes there
would remove 4,049 pass invocations but only 2,116 responding-pair visits out of
tens of millions; much of this opportunity is empty/free-flight work. Car has
zero exactly unchanged passes. A correct convergence check must account for
future correction fractions, new-contact discovery, joints, locks and soft
coupling. Do not adopt an unchanged-pass shortcut from this probe.

The residual scans alone cost seconds, excluding per-visit bookkeeping and CSV
output. These streaming-active diagnostic timings are not clean overhead or
speedup comparisons, but demonstrate why adding a second all-contact scan is
not free. Prefer collecting candidate convergence information during the
existing solve, then verify a full-correction condition before early exit.

**Ordering/pass control:** an unchanged-SDK ten-particle chain, diameter 2,
initial adjacent overlap 0.02, opposing endpoint speeds 20, restitution 0.5,
60 Hz/four substeps, runs every case to one second. Initial kinetic energy is
400. With 1/2/4/8 passes, maximum sampled penetration is
0.0809846/0.0429058/0.0327387/0.0295744 and final energy is approximately
0/2.8454/51.0722/167.5589. Every depth is below 0.1, yet rebound differs greatly.
Reversing entity creation order mirrors the asymmetric velocity distribution;
8 passes are not asserted to be an exact multi-impact solution. Changing order
is a numerical change requiring behavior comparison, not a free optimization.

**State reuse:** the engine already retains targets and impulses within each
substep. A fixed-contact, velocity-only ten-mass support model compares carrying
impulses across steps. Eight cold passes retain a 0.03961 units/s closing
residual; one warm pass reaches a last-five-second worst of 1.63e-8. This is
specific evidence for testing persistent state, not proof for the car/pit:
the model omits geometry, contact churn, impacts and cache maintenance. A real
cache must match generation/feature/normal, scale or reject impulses after dt
changes, separate restitution targets from support impulses, and invalidate
shape/filter/teleport/deletion changes. Blindly retaining impact targets can
inject energy or preserve an obsolete contact.

**Overlap publication:** overlap-only visits make no endpoint pose/velocity
changes. Their repeated polygon geometry belongs to interaction refresh, not
impulse convergence. A final-pose refresh could avoid intermediate work, but
must preserve documented event/contact publication and explicitly called
standalone stages. Goal 15 owns that composition decision.

**Sleeping:** the existing low-speed counts and disturbed-pile control do not
establish a sleeping policy. Wake must propagate through responding contacts,
joints, beams and node/frame relationships, with new loads, kinematics, edits,
teleports and merges invalidating sleep. Avoid a global sleeping implementation
until those dependencies and wake costs are measured. The actual pile still
changes velocity in nearly every later response visit.

**Decision:** retain eight passes for this investigation. Prioritize reducing
discovery/overlap overhead without changing the solve; consider persistent
particle support state as a separately selected medium-to-large follow-up.
Convergence monitoring should be collected inside response, not added as a
full scan. Confidence is high that blanket pass cuts/exact-zero exits are
inadequate; confidence in warm-start benefit for the actual scenes remains
moderate. Goal 15 owns overlap cadence and convergence composition; Goals 13–14
own joint/soft participation. Acceptance needs rebound, support, momentum,
energy, churn/lifecycle, perturbation/wake and full endpoint checks, plus clean
complete-update timings. No new solver or sleeping behavior is adopted.

## 4. Global work and dependency groups

**Causal probe:** build dynamic groups from previous retained responding contacts,
current joints, beam endpoints, soft-frame/node ownership and transform drivers.
Static supports and prescribed kinematics do not join independent dynamics.
Generation-tagged previous edges are validated again. At the first decision in
each update, and every 32nd decision during car update 133, add possible future
contacts using outward-rounded motion bounds over the original proposed interval.
Beam bounds include endpoint motion/radius; rigid bounds include translation,
prepared acceleration and COM-relative angular travel. This observational graph
does not alter physics. Both full workload traces match the parent exactly.

| Sample | Existing-contact/dependency groups | Largest group | Groups after possible future merges | Bodies outside limiting groups after merge bounds |
|---|---:|---:|---:|---:|
| Pit update 1 | 800 | 1 | 800 | No adaptive restriction |
| Pit update 202, worst impact | 194 | 594 | 4 | 3 / 800 |
| Pit update 360 | 1 | 800 | 1 | 0 / 800 |
| Pit update 720 | 1 | 800 | 1 | 0 / 800 |
| Car update 1 | 201 | 106 | 201 | 200 / 306 |
| Car update 133 | 21 | 106 | 21 | 200 / 306 |
| Car update 360 | 27 | 117 | 17 | 159 / 306 |
| Car update 600 | 34 | 116 | 21 | 179 / 306 |

The pit's impact limiter initially touches a tiny group: 798 bodies are outside
it in the previous-contact graph. Possible contacts instead connect 797 bodies,
leaving only three outside the limiting group. This disproves treating current
islands as safely independent through a longer step. In the settled pile, there
is no useful independent dynamic island. Car's median outside fraction among
sampled restricted decisions is 65.36%, with range 42.48–98.04%. The car has 306
simulated entities (including derived soft-body frames), 54 prescribed movable
entities and 50 transform locks. These fractions count entities, **not measured
avoidable CPU or ordinary integration calls**.

The future bounds cover prepared-force free motion, not new impulses, changing
spring forces or transform-lock propagation. They deliberately overconnect some
sensor/filter cases and retain previous contact edges. A safe local scheduler
must expand/refit envelopes after corrections, merge before interaction, and
materialize driven poses from their drivers. The observer establishes an
opportunity ceiling; it establishes **zero whole-update work as safely removable
without those mechanisms**. A broad scheduler prototype is not justified here,
especially because it cannot solve the connected pit's sustained bottleneck.

**Controls:** the exact-source separated fast-pair control from Goal 9 requires
121 versus 120 global steps in half a second, causing 200 extra integrations of
remote slow particles. It proves coupling, not large scene savings. The existing
800-particle perturbation shares its first 600 updates exactly with the parent;
its final two seconds still require 481 steps/3,848 iterations. This is a
wake/perturbation input to future tests, not evidence of a working sleep policy.

A new constant-gravity free-body control follows the recorded actual duration
sequences. Switching to only four configured steps per tick changes its endpoint
by -0.02373 units for the pit's twelve-second sequence and -0.17529 for the car's
ten-second sequence. Semi-implicit position integration depends on the sum of
squared step durations. An algebraic batch including that sum matches the split
double-precision calculation within 1.2e-11 units, but does not reproduce float
rounding, collisions or force changes. Consuming all elapsed time alone does not
preserve trajectory or establish acceptable accuracy.

**Alternatives:** retain global execution for shared-body contact groups; batch
provably free motion while preserving the selected integration rule; subcycle
elastic connected groups with conservative merge boundaries; or redesign the
integrator. The last option needs a separate decision. Batching must respect
external force consumption, spring/beam force preparation, transform/kinematic
sampling and observer-visible substep state. If every observer must see every
body at each substep, deferred materialization can erase the proposed savings.
Parallel groups still require stable ordering, independent writes and ordered
publication; merging/splitting/waking and generation reuse need explicit costs.

**Decision:** lower priority than discovery and beam-bound fixes for the combined
gate. Local elastic scheduling may help the car, but not the dense connected pit.
Confidence is high in that distinction; net gains remain unverified. Effort is
large and belongs to Goal 15 after Goals 11–14 establish bound/cache ownership.
Acceptance needs future-contact merge tests, independent/static/kinematic cases,
force timing, callbacks, locks, lifecycle, deterministic ordering and full-run
accuracy/elapsed-time checks. Do not convert the outside-body percentages into
a speedup forecast or adopt local stepping in this investigation.

## 5. Candidate growth, traversal, layout and SIMD

**Phase-controlled analysis:** reprocess Goal 9's selected, validated scaling
traces by early (updates 1–180), impact (181–300) and late (601–720) phases.
Normalize callbacks by actual substeps and body count, rather than attributing
all total-runtime growth to more particles. In the fixed container, late-phase
callbacks/step/body rise from 5.91 at 200 particles to 15.72/39.63/49.52 at
400/800/1,600. At 800 particles, only 7.6% of late callbacks reach a narrowphase
check; narrow checks are 1.25 times per-step unique tested candidates. Density-
scaled late values are 17.78/28.60/39.63/43.49 callbacks/step/body. This shows
phase, packing and repetition as well as count effects; it is not a measured
asymptotic complexity law. The corrected density layouts and original failed
layout attempts remain identified by Goal 9's run selection.

**Controlled traversal comparison:** capture actual particle circles at selected
car/pit updates. Compare building the existing tree from that snapshot, retaining
its initial-scene topology and refitting all leaves, and a sorted uniform grid
whose cell width is the largest particle diameter. Every method must match the
complete brute-force AABB pair set, without duplicates. The grid uses a 3×3
neighbor search and exact AABB filtering; this proof applies to these circle
bounds/cell sizes, not arbitrary swept, large or nonparticle shapes.

Median build/refit plus query milliseconds per replay, three serial trials:

| Snapshot | Rebuild tree | Retain initial topology | Grid |
|---|---:|---:|---:|
| Pit airborne, update 1 | 0.3192 | 0.2352 | 0.3077 |
| Pit impact, update 202 | 0.4475 | 0.3018 | 0.3258 |
| Pit settled, update 720 | 0.5416 | 0.3534 | 0.3384 |
| Car tail, update 133 | 0.1157 | 0.0778 | 0.0859 |
| Car later, update 600 | 0.1725 | 0.0973 | 0.0914 |

These are streaming-active **snapshot microbenchmarks**, not complete-update
performance estimates. Build/refit/sorting is included; the retained tree's
initial build is outside repeated replay. Each replay refits every leaf, even
though a real dirty worklist may need fewer. The grid excludes walls, polygons,
beams, filters, lifecycle edits and changing swept envelopes. Reusing the initial
topology happens to improve these snapshots; a persistent tree still needs a
quality metric and rebuild fallback as motion changes its quality. Existing
within-solve refits already retain topology; the new opportunity is across
substeps and sweep preparation, not reimplementing an existing optimization.

Hardware counters are available through `perf_event_open` despite no `perf`
executable on PATH. Retained settled-pit topology visits 47,674 tree nodes per
query batch versus 65,548 after rebuilding. Both produce 1,876 canonical AABB
pairs and 4,552 leaf callbacks; the grid visits 7,554 cell entries. Over 1,000
replays, median branch misses are approximately 28.89M/19.29M/31.32M for
rebuild/retain/grid. Grid's lower instruction work can offset additional search
branches. These counters describe the microbenchmark, not cache/branch events
inside the full physics update. Raw enabled/running times are retained for
multiplexing; one car trial has a scaling factor up to 1.270, so unscaled counts
there are not direct event-rate comparisons. The first counter version reset
values but reported cumulative enabled times; it is preserved and superseded by
`data_v2_*` for hardware-accounting interpretation.

**Geometry arithmetic:** settled pit has 1,770 circle overlaps among 1,876 AABB
pairs (94.35%); the impact snapshot has 1,034/1,482 (69.77%). Four-wide SIMD and
scalar results agree for every captured candidate, including normals/depth.
Packing is included in the batch timing. Settled-pit scalar/batch medians are
0.00993/0.00522 ms per candidate batch; impact is 0.00660/0.00351 ms. Car's 300
circles include 200 standalone particles and 100 soft nodes; scene counts are
unchanged. Production circle gathering already uses this SIMD path. Goal 9's
exclusive circle-gather cost is only 52.47 ms for the entire pit run and 34.84 ms
for the car prefix, so more gathering arithmetic optimization cannot recover
seconds. Response-side geometry must remain synchronized with preceding shared-
body corrections; batching it before solving can use stale poses.

**Layout/dispatch control:** `SystemContactConstraint` is 1,352 bytes because its
union accommodates polygon manifolds and soft state. Peak retained capacity is
2,768,896 bytes in the pit and 692,224 in the car. A deliberately incomplete
32-byte particle summary tests identical field-sum work with compact stride.
For the settled snapshot, 2,536,352 versus 60,032 bytes yields median reads of
0.00223 versus 0.00105 ms per scan. Generic allocation/fill is about 0.719 ms;
conversion of selected fields is about 0.00873 ms in the recorded third trial.
Over 20,000 scans, median hardware cache misses are 7,842 versus 453. This mixes
stride, tag dispatch and instruction-layout effects; it does not isolate cache
latency or benchmark the solver. The toy record omits generation handles,
publication, complete retained impact state and lifecycle bookkeeping, so its
42.25× footprint ratio is not a promised production reduction. Compact particle
state is worth a real design, with conversion/maintenance and complete-update
cost measured rather than assuming every saved byte translates to saved time.

**Alternatives and decision:** prioritize topology reuse with quality-triggered
rebuilds in Goals 11–12, behind the changed-pose/affected-beam invalidation work.
A particle grid is a useful comparative candidate, but should not replace all
collision discovery; mixed shapes need a compatible route and candidates must
remain complete after envelope expansion and edits. Confidence is high in the
snapshot traversal/layout findings, moderate in whole-scene benefit. Effort is
medium for retained topology, medium-to-large for hybrid grids and complete
compact particle state. Compact response data and persistent support state need
separately selected follow-ups; neither is silently added to the roadmap.

Parallel response requires disjoint write sets or an explicitly different solve
schedule. Coloring preserves independence within a batch but adds graph work and
barriers; Jacobi accumulation changes convergence and iteration needs. Atomics
alone cannot preserve the current read/solve/write ordering. GPU execution also
needs persistent buffers, deterministic ownership, transfers, synchronization,
interaction publication and fallback for mixed constraints. Those costs are
unmeasured here; a GPU implementation is not justified merely by particle count.
Accept a future SIMD/parallel/GPU candidate only with equal full-run physical
checks and complete-update measurements including preparation/transfer costs.
The present evidence favors fixing repeated CPU work first.

## Verification and final recommendations

All five questions have focused causal evidence and evaluated alternatives.
The recommendation below ranks implementation opportunities, not additive
savings or an automatic change to the approved goal order.

| Priority | Proposed work | Evidence and tradeoff | Confidence / effort | Roadmap fit |
|---|---|---|---|---|
| 1 | Refresh affected poses, pairs and beams | Repeated scans and the global rigid-change flag account for substantial avoidable visits; maintenance and manual-stage fallback must be included | High cause confidence, moderate net-benefit confidence / medium | Goal 12, soft validation 14, cache ownership 15 |
| 2 | Make beam sweep bounds match response ownership, with conservative collapse/plane handling | Explains the 1,438-restriction car spike; does not remove the sustained elastic cap or help the particle-only pit directly | High cause confidence, candidate safety unverified / medium | Goal 14, shared traversal 11 |
| 3 | Retain useful tree topology across steps, rebuild when quality declines | Exact snapshot candidates with lower traversal cost; topology can degrade, and snapshot gains are not full-update gains | High microbenchmark confidence, moderate scene benefit / medium | Goals 11–12; compare a hybrid particle grid here |
| 4 | Design compact particle state and persistent support impulses | Layout and fixed-contact controls are promising; complete lifetime/impact state and changing contacts remain untested | Moderate / medium to large | Separate user-selected follow-up; not silently reopening Goals 7–8 |
| 5 | Reduce redundant overlap publication and collect convergence information during solving | Overlap visits do not move bodies; naive pass exits fail, and full residual rescans cost seconds | High evidence confidence, publication compatibility unverified / medium | Goal 15 with joint/soft participation in 13–14 |
| 6 | Batch free motion or integrate elastic groups locally | Car has possible separation, but future contacts connect almost the entire pit; safe merge/materialization work is still required | High workload distinction, low net-gain confidence / large | Goal 15 after Goals 11–14; integrator redesign needs a separate decision |

Keep eight passes and existing sweep protection until a behaviorally verified
replacement is selected. Additional circle SIMD has little remaining gathering
budget to recover. GPU/parallel response and global sleeping remain unmeasured
design alternatives, not selected next steps. **Goal 11 remains next in the
approved roadmap**, where retained sweep topology, prepared bounds and shared
traversal can be evaluated without prematurely changing numerical behavior.
Goals 13–14 retain their joint/soft response scope; this report does not complete
their implementation or testing. Goal 16 still owns full rendered real-time
acceptance and Goal 17 the reference comparison and user-approved tolerances.

Verification and preserved evidence:

- All eight new main workload traces match the parent's state/work hashes and
  production counters, with exact source-exclusive timing conservation and
  recorded executable hashes. Four full pits and four authorized car prefixes
  finish; no car prefix is reported as full-car completion. See
  [workload verification](../sandbox/milestone_12/goal_10/workload_verification.json).
- Four isolated installed SDKs build on Linux with ASan/UBSan. Each passes all
  22 regressions (88 test executions), eight one-second chain controls, a
  four-second pit and a car diagnostic. Q2's car reaches update 134, exercising
  the pathological update; the other three run six-update smokes. The final
  snapshot/data probe also passes under sanitizers. Leak detection and fatal
  sanitizer reporting were enabled; no sanitizer errors or compiler warnings
  were found. See [sanitizer results](../sandbox/milestone_12/goal_10/sanitizer_results.json).
- Fifteen selected v2 data trials (three serial repetitions of five snapshots)
  finish with brute-force candidate parity, scalar/SIMD geometry parity and
  layout checksums. Counter enabled/running deltas distinguish multiplexing;
  the earlier counter-accounting version remains preserved and superseded.
- Exact-source Goal 8 Linux and sanitizer regressions, 219 timed plus four
  calibration stress endpoints per platform, Windows cross-builds and
  independent installed-SDK joint examples remain reusable. These were not
  rerun as new production validation. There is no native Windows execution or
  new Windows-verified optimization; the observational helpers are Linux
  diagnostics. Existing numerical/CPU failures remain visible.
- Production source, headers, scenes, tests and measurement tools are unchanged
  from the pinned parent and Goal 8. Frozen-reference and Goal 8/9 artifacts
  are rehashed without modification. No rendered trials or final acceptance
  claims are added. Q2 onward is explicitly streaming-active diagnostics.

The [reproduction notes](../sandbox/milestone_12/goal_10/reproduction.md) identify
final source copies, build inputs, observer boundaries and selected run versions.
The [final verification](../sandbox/milestone_12/goal_10/final_verification.json)
and [SHA-256 inventory](../sandbox/milestone_12/goal_10/manifest.json) preserve
sources, patches, configurations, SDK archives, executables, raw traces and
analysis. The sandbox evidence is local/ignored and must be retained alongside
this review; committing the report alone does not transfer its executables.
No production physics change, new tolerance or implementation ordering is adopted.
