# Milestone 12 Goal 5 — Joint gathering

Goal 5 is complete for review and commit. Goal 6 has not started. The frozen reference remains
`physics-pipelines-overoptimized` at
`6f283268aa4bf038c4ed78271baeb9d5986bd7d9`; the immediate parent is Goal 4 commit
`bb737c6b`. Exclude `main`. Evidence is preserved under ignored
`sandbox/milestone_12/goal_5/`.

## Analysis and decision

| Work | Existing boundary and decision |
| --- | --- |
| Membership | Each gather clears the list, scans occupied joint-pool slots in index order, checks live joint membership, and appends pin/weld identities. Springs remain force-stage work. Retain the scan and ordering. |
| Anchor resolution | Anchors carry generation-checked IDs and borrowed body handles. Resolve current world positions during solving, after contact and earlier joint corrections. Gathering does not calculate anchor geometry. |
| Constraint assembly | Gathering selects identities only. Jacobians, COM offsets, effective masses and weld angular rows are assembled during solving from current motion and body properties. Moving this assembly into gathering would make it stale. |
| Validation | Description setters validate topology/configuration. An incomplete pin/weld remains gatherable; solving reports unavailable endpoints/anchors and retains its definition. Preserve these timing and recovery rules. |
| Reusable data | The pool owns descriptions and anchors; the list retains allocated capacity. These already separate topology storage from changing motion. A new persistent cache would add invalidation for type/anchor edits, component changes, deletions and slot reuse without a useful gathering-time saving. |

Goal 4's preserved measurements show the actual collection helper costing
41.976 ms across 37,113 car-prefix calls (0.038% of 109815.836 ms physics), and
5.662 ms across 13,091 pit calls (0.0069% of 82400.017 ms physics). The public
wrapper adds 180.513 / 71.374 ms exclusive time respectively, separate from
reporting descendants. Pin/weld fixture helper totals are 6.259 / 6.786 ms over
70,433 / 75,251 calls. Anchor resolution inside solving is not charged to these
gathering measurements. Broad reporting/orchestration work remains Goal 15;
solver assembly and reuse remain Goal 13 (renumbered after the collision
optimization goals, global algorithm audit and focused investigation were inserted).

Retain the gathering algorithm. Even eliminating its entire measured cost
would not materially improve car/pit throughput. No SIMD or topology cache is
introduced. Keep 240 Hz, four configured substeps, eight iterations, scene
geometry, materials, public APIs and numerical criteria unchanged.

## Lifetime correction

The previous list stored raw entity indices. A focused installed-SDK test
reproduces this sequence on the parent: gather a pin, delete it, create a weld
in the same slot, then solve without gathering. The replacement incorrectly
moves the endpoints. The retained list has accidentally selected a different
entity lifetime.

Store the generation-checked joint handle instead, and resolve it before each
solve. Both handle and index are 32 bits, so list size, allocation growth and
ownership are unchanged. A deleted joint is skipped; its replacement participates
after a new gather. A surviving joint still uses its current type, anchor binding,
offsets, poses and body properties. Removing an anchor or endpoint retains the
existing warning/skip behavior; reattaching a surviving joint remains visible
without a new gather. Each gather replaces membership; it does not append to
the previous gather.

This is an intentional correction to stale manual-stage membership, not a
solver algorithm or normal full-update behavior change. Lists borrow handles
and do not own joints, anchors or bodies. No cached pointers survive edits.

## Verification

Sixteen Linux and sixteen ASan/UBSan regressions pass, with sanitizer leak
detection and halt-on-error enabled. The new focused regression covers joint
slot reuse, current anchor/body/offset/pose edits, type changes, anchor removal
and replacement, endpoint deletion, repeated gathering and incomplete definitions.
The failing parent reproduction is preserved in `logs/parent_reproduction.log`.

Independent installed-SDK measurement/playback examples and fixture consumers
build on Linux, sanitizers and Windows without compiler warnings. Linux tests
and car/pit sanitizer smoke checks execute outside their source directories.
Windows cross-builds pass; native Windows execution was not performed.
The standalone joints example also builds independently against all three SDKs.
Its Linux and sanitizer smoke runs complete 12 frames and 22 physics ticks, then
quit and tear down normally from a `/tmp` launch. An external wrapper supplies
the bounded quit event and a 10 ms frame delay; the example source is unchanged.
This checks startup, execution and cleanup, not interactive or FPS acceptance.

All 219 timed fixture scenarios plus four calibrations reach their declared
endpoints. The soft-body delayed-update CPU comparison remains failed at
382.453 ms versus 50 ms. The ten-second car prefix is not a completed 60-second
car scenario. Its existing jump-force lifetime limitation and incomplete full
reference/rendered coverage remain as recorded in
[Goal 3](physics_integration_subdivision.md#clock-ownership-follow-up) and
[Goal 1](physics_measurement_contract.md). No interrupted visual trials are
restarted, and no tolerance is changed.

Current runs report AC power connected; the preserved parent workload samples
report battery power. These serial headless runs have no concurrent builds but
are diagnostic, not controlled performance acceptance. One sample per workload
and version gives:

| Measurement | Parent car, 10 s | Goal 5 car, 10 s | Parent pit, 12 s | Goal 5 pit, 12 s |
| --- | ---: | ---: | ---: | ---: |
| Physics wall ms | 109815.836 | 111121.879 | 82400.017 | 82615.939 |
| Physics-update p95 ms | 368.776 | 379.289 | 168.299 | 167.551 |
| Joint collection helper, exclusive ms | 41.976 | 42.318 | 5.662 | 5.729 |
| Joint gather wrapper, exclusive ms | 180.513 | 186.104 | 71.374 | 71.069 |
| Gather calls | 37113 | 37113 | 13091 | 13091 |
| Subdivisions | 37113 | 37113 | 13091 | 13091 |
| Solver iterations | 296904 | 296904 | 104728 | 104728 |
| Candidates | 15633975 | 15633975 | 15498784 | 15498784 |
| Contacts | 1659222 | 1659222 | 746387 | 746387 |

No throughput improvement is claimed. Joint collection remains 0.038% of car
physics time and 0.0069% of pit time; retaining the simple scan remains justified.
The full pit finishes at 12 seconds. All per-update state, contact, candidate,
subdivision, duration and diagnostic records match the parent exactly after
excluding wall time. The frozen-reference comparison also matches, excluding
Goal 4's intentionally reduced candidate counts. Fixture numerical records
match both versions after excluding CPU timings. Exclusive stage sums equal
physics wall time, and attributed work equals report counters. These checks
cover the measured scenarios, not every possible motion or topology.

`build.py`, `verify.py`, `example_verify.py` and `measure.py` reproduce builds and checks using new
run directories; `summarize.py` records comparisons in `summary.json`.
`audit.json` verifies all 1,457 frozen-reference hashes and 874 Goal 4 hashes
unchanged. Against the parent, only `joint_solver` has changed executable
sections on Linux, sanitizers and Windows. The sanitizer/Windows debug builds
also have changed non-executable sections in private-header dependents.
`source/`, `changes.patch` and `manifest.json` preserve the reviewable source and
artifact hashes. Rendered acceptance remains Goal 16; final tolerance decisions
remain Goal 17.
