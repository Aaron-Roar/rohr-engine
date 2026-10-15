# Soft-body collision penetration diagnosis

Status: diagnosis and the separately approved contact-face correction are
complete; the isolated game copy is ready for user visual review.
The original defect was investigated against `8e2c791` (`feat(physics): implement local beam force and
torque loads`) on Linux. This work precedes Unified Forces and Torques Goal 4.

## Original finding

The following describes the pre-fix behavior; verified resolution is recorded
below.

The reported penetration of `rigid_body_8` in `/tmp/rohr-projects/test` is caused
by incomplete contact faces when a polygon has redundant collinear vertices.
This is a shared polygon contact-generation defect, not a beam-load defect or
an absent collision filter. It affects both soft nodes and beam boundaries.

The triangle has five authored vertices. Two vertices subdivide otherwise
straight sides; they do not change the triangle's outline. Overlap detection
accepts this geometry, but contact generation selects an individual edge by
normal alignment. Equal-facing edges on the same supporting line are not
combined into the full face. The selected edge can be on the other half of the
triangle from the colliding object. Clipping against that short segment then
returns no contact points despite a genuine overlap.

- [`contact_face_best_get` and `contact_face_incident_get`](../src/physics/collision/contact_manifold.c)
  select a single segment by normal alignment. `contact_manifold_convex_get`
  clips against its endpoints; `contact_manifold_set_polygon_get` drops empty
  candidates. Rotation/rounding can change which tied segment wins.
- The [rigid-pair solver](../src/physics/rigid_body/rigid_body_step.c) uses polygon
  manifolds for a node against a rigid polygon. With no manifolds, its response
  loop applies neither impulse nor positional separation. The overlap may
  still be published while contact is false.
- The [beam solver](../src/physics/soft_body/soft_body_solver.c) obtains its
  boundary overlap through the same manifold generator. With no manifold, it
  treats the pair as non-overlapping and does not gather a contact constraint.

Increasing solver iterations cannot repair a missing manifold. Continuous
collision detection is not required to reproduce this defect: the minimal
fixture starts already overlapping and moves only 1/60 world unit per step.

## Source and executable checks

The saved project and generated C agree on a 1/60-second tick (stored as
`0.01666666753590107`), one substep, eight solver iterations, and gravity
`(0, -9.8)`. Nodes have mass 10 and radius 10. All six beams have stiffness 1,
damping 0, thickness 2, and enabled collision. Node, beam, and triangle filters
use category 1 / collides-with 1. The triangle has mass 69, gravity enabled,
zero initial velocity, and explicit local COM `(5.43377686, 17.2075500)`.

The generated triangle origin is `(-26.9645386, -108.438736)`, with these local
vertices, in order:

```text
(  0.0000000,  67.6931839)
( 78.1653595,  67.6931839)
( 39.0826797,   0.0000000)
(  0.0000000, -67.6931839)
(-78.1653595,  67.6931839)
```

The generated entry point calls `rohr_physics_update` with the latest engine
tick delta. The project executable and generated source were built/generated
within two seconds of each other on October 15, 2026. A fresh CLI generation
in an isolated project copy produced byte-identical `project_objects.c`.
The original executable ran headlessly for 12 seconds and shut down cleanly.
That launch verifies startup/lifetime, not visual collision correctness.

A deterministic harness linked against the game's original engine archive and
against the current installed SDK produced identical 3,600 node-state records
for 900 ticks. Thus the observed failure is present in both builds and is not
explained by stale generation or the Goal 3 source changes.

Source fingerprints (SHA-256):

```text
objects/project.rohr.json
1a47e65482a81030f0bdd41b8d52fd4a00203f7be2c1a30a367798a26540a3ad
src/generated/project_objects.c
6be8dfb80878570302d772e4290be1b90652cb38ed2edec1e1d9c551563088b6
build/Test
f3ffb7344b32893585be2696baf2616d817874d80a7d5890df8380d337cccc94
build/lib/librohr_engine.a
78c0227a64a7bb05723a13c0f793e6c28ad8be13814abcbd18811fa2910ea351
```

The diagnosis did not edit the supplied project, executable, or production
engine sources. The subsequent correction changes only contact-face extraction
in production; the supplied project and original binaries remain untouched.

## Measurements

The harness creates the original generated scene and calls
`rohr_physics_update(1)` once per recorded tick. It measures node-centre distance
to the triangle boundary independently using point-to-segment distances and
point-in-polygon classification. Negative distance means the centre is inside.
It also captures stored overlaps, contacts, impulses, current manifolds,
triangle pose, beam-contact counts, and diagnostic counts.

Ticks below are zero-based completed updates. At the first observed node
failure, tick 291 (about 4.87 seconds), node 2 has:

| Measurement | Value |
| --- | --- |
| Position | `(-58.8096733, -31.7667427)` |
| Velocity | `(-1.88768744, -48.9426727)` |
| Centre distance to triangle boundary | `9.29987126` (node radius is 10) |
| Published overlap | true; depth `0.655509949` |
| Overlap normal, node toward triangle | `(-0.0943632647, -0.995537817)` |
| Contact / points / current manifolds | false / 0 / 0 |
| Contact impulse | `(0, 0)` |
| Triangle origin | `(-20.586401, -112.727928)` |
| Triangle clockwise orientation | `5.41467285` degrees |
| Triangle/beam contacts and report diagnostics | 0 and 0 |

Node 2's centre first enters the triangle at tick 367 (about 6.13 seconds).
At tick 375 its centre is 5.88824 units inside; published overlap depth is
15.80416. These are substantial penetrations, not the small difference between
an eight-sided node hitbox and an ideal radius-10 circle.

Controlled 900-tick runs (15 simulated seconds), changing only the stated
runtime settings or geometry:

| Variant | First node-centre entry (tick) | Minimum signed centre distance | Overlap records without contact |
| --- | ---: | ---: | ---: |
| Original, 1 substep / 8 iterations | 367 | -5.88824 | 93 |
| Original, 2 substeps / 8 iterations | 302 | -11.94263 | 41 |
| Original, 4 substeps / 8 iterations | 302 | -5.12282 | 371 |
| Original, 8 substeps / 8 iterations | none in interval | 3.53806 | 260 |
| Original, 1 substep / 32 iterations | 302 | -9.89238 | 53 |
| Equivalent three-vertex triangle | none in interval | 9.65238 | 0 |
| Original triangle, beam collision disabled | 382 | -39.94527 | 182 |
| Three-vertex triangle, beam collision disabled | none in interval | 9.23276 | 0 |

Counts are post-update node observations, not unique contacts. Different
substeps change the trajectory and rotation, so they need not monotonically
reduce these counts. Eight substeps mask centre entry during this interval but
still leave missing contacts and substantial radius penetration. No physics
report diagnostics occurred in the baseline reproduction.

A further beam-only control disables node response. The five-vertex triangle
allows node centres inside at tick 303; the three-vertex version has no centre
entry in the same interval. Node overlap-without-contact counts in that control
are intentional sensors and are not used as failure evidence.

## Minimal deterministic reproduction

No graphics, initial high speed, springs, other bodies, or catch-up ticks are
needed. Use the five-vertex triangle above at world origin with orientation
zero, static and collision-enabled. Use category/mask 1 for every participant.
Disable gravity. Keep eight solver iterations and `dt = 1/60`.

**Node case:** create a soft node at `(-40, 75)`, mass 10, radius 10, velocity
`(0, -1)`. Disable beam collision. The node's collision outline is the engine's
normal eight-vertex circle. The SAT overlap is true, depth 2.6931839 before
stepping, but the manifold count is zero in both argument orders.

After one step the node is at `y = 74.9833298`, still has `vy = -1`, and reports
overlap depth 2.70985413 with no contact or diagnostics. The next two steps
continue downward without response.

**Beam case:** create nodes at `(-60, 68)` and `(-20, 68)`, mass 10 each,
velocity `(0, -1)` each. Disable their collision response. Connect them with a
zero-stiffness, zero-damping beam, collision enabled, thickness 2. Its boundary
already overlaps the triangle's top face. After one step the first endpoint
is at `y = 67.9833298`, still has `vy = -1`, and no beam contact is reported.

**Geometry-equivalent control:** replace the triangle with the three vertices
`(78.1653595, 67.6931839)`, `(0, -67.6931839)`,
`(-78.1653595, 67.6931839)` without changing other settings. The node now settles
at `y = 77.6831818`, `vy = 0` after the first step. The beam endpoints settle at
`y = 68.6931839`, `vy = 0`. Both report a contact on the initial step.

Moving the node to `x = +40` makes the original five-vertex triangle produce a
manifold too. Rotating the entire geometry by 30 degrees makes the left contact
work in the measured case; at 60 degrees it fails again. The equivalent
three-vertex triangle works for both sides at 0, 30, and 60 degrees, in either
argument order. This isolates support-face selection rather than filtering,
mass, timestep, or contact iteration count.

## Approved resolution goal (implemented and verified)

**Restore complete polygon contact faces across redundant collinear vertices.**

1. Add failing geometric and runtime regressions using the fixtures above,
   including both argument orders and node/beam isolation. Test cyclic vertex
   order, reversed winding, redundant points on either participant, and the
   polygon's closing edge. Retain the full-scene case as integration evidence.
2. Make reference and incident face selection operate on complete contiguous
   collinear boundary runs of each convex collision piece. Expand both ends of
   the selected edge, including wraparound, while retaining a consistent normal
   and tangent. Bound traversal by the piece's vertex count. Use double
   intermediates; treat only co-directed edges as candidates. Bound all accepted
   vertices' deviation from the original support line by the existing contact
   plane tolerance (`0.001f`) and require normalized edge directions to be
   parallel within `8 * FLT_EPSILON`. Do not merge across a genuine corner or
   a nonadjacent parallel face. Validate these private tolerances with rotation,
   translation, and near-collinear counterexamples before accepting the fix.
3. Keep this correction inside contact-face extraction. Preserve authored
   vertices, vertex IDs, hitbox shape, COM/inertia, rendering, persistence,
   decomposition topology, public APIs, and the solver's response policy.
   Do not replace the triangle asset, change project substeps, introduce CCD,
   or apply a blanket SAT fallback that can create invalid concave contacts.
4. Verify existing node, beam, convex, concave, friction, restitution, fixed-body,
   and contact-order behavior. Include a thin genuine corner that must not be
   merged and collision-disabled/filter-excluded controls. A supported overlap
   on either half of the minimal straight face must yield finite correctly
   oriented contacts; separation must oppose penetration and stop inward motion.
5. Replay the unchanged generated scene for at least 900 fixed ticks, including
   the original 1-substep/8-iteration setting. Require no node-centre entry into
   `rigid_body_8`, no empty manifolds for the reproduced supported face contacts,
   no nonfinite motion, and no diagnostic failures. Judge circle clearance using
   the actual polygon hitboxes and existing separation slop, not exact analytic
   circle tangency. Verify beam-only and node-only fixtures separately.
6. Run the full native suite and focused ASan/UBSan suites with leak detection;
   compile Linux and Windows; run installed-SDK consumers and affected standalone
   examples on Linux. Build/run an isolated copy of the supplied project and ask
   for user visual confirmation after the fix. Label Windows compilation
   separately from runtime verification. Stop at that goal's review boundary.

Expected regression suites include `contact_manifold`, `concave_collision`,
`beam_contacts`, `soft_body`, `particle_collision_mode`,
`rigid_contact_stability`, and applicable report/recovery tests. No implementation
should be declared complete merely because the equivalent three-vertex control
passes; the unchanged five-vertex geometry must pass.

## Resolution and verification

`contact_face_best_get` and `contact_face_incident_get` now expand the selected
edge to the outer endpoints of its contiguous collinear face. Expansion walks
both directions, including the closing vertex, and is bounded by the piece's
vertex count. It uses double intermediates, accepts only co-directed edges
within `8 * FLT_EPSILON` normalized cross-product magnitude, and checks each
added vertex against the original supporting line with `0.001f` tolerance.
Comparisons stay anchored to the original edge so small bends cannot accumulate.
The temporary face retains the selected normal. There is no allocation,
geometry rewrite, decomposition change, public API change, or solver-policy
change; authored vertices, IDs, mass/inertia, and persistence are preserved.

Before the production edit, the new geometric regression failed with zero
manifolds on the five-vertex triangle, and the runtime regression failed its
contact assertion. Both pass after the correction. The shipped tests cover:

- Both triangle halves with node and beam outlines, both argument orders,
  rotations of 0/30/60/90 degrees, translation, cyclic ordering, and reversed
  winding. Contacts match the equivalent three-vertex outline.
- Subdivided reference and incident faces on either/both participants, including
  wraparound. A `MAX_VERTICIES` polygon tests long runs in every cyclic ordering
  and both windings against independently calculated points, normal, and depth.
- An accepted near-collinear edge, a shallow corner outside the angular limit,
  an edge within the angular limit but outside the distance limit, and a genuine
  corner. These assert the expected clipping endpoints.
- 72 isolated node/beam runtime configurations across both halves, three
  rotations, enabled response, disabled response, and excluded filters. They
  check finite response, oriented contact queries, stopped inward velocity, and
  separation using the actual eight-vertex node projection and existing slop.
  Existing suites retain friction, restitution, fixed-endpoint, contact-order,
  and concave-contact coverage.

The unchanged generated scene was replayed for 900 fixed ticks at its original
one substep/eight iterations. Additional node-only and beam-only controls were
also replayed for 900 ticks each:

| Fixed-engine variant | Node-centre entries | Minimum signed centre distance | Node overlaps without contact | Current node overlaps without manifolds |
| --- | ---: | ---: | ---: | ---: |
| Original scene | 0 | 9.65241213 | 0 | 0 |
| Beam collision disabled | 0 | 9.23277594 | 0 | 0 |
| Node response disabled | 0 | 0.971050485 | 49 (intentional sensors) | 0 |

Every replay produced 3,600 finite node records and zero report diagnostics.
The beam-only replay also checked every beam boundary against the triangle;
no current beam overlap lacked a manifold.
The original scene had beam/triangle contacts on 219 ticks. Its maximum final
polygon overlap depth was `0.0100097656`, consistent with the existing `0.01`
separation slop and float rounding. The node-only control's maximum final depth
was `0.0180778503` under the unchanged iterative solver. Beam-only nodes are
sensors, so their radius clearance is not an acceptance condition. One original
scene record retains a published contact after final geometry has separated;
its absent final manifold is expected, not a missed supported overlap.

The minimal five-vertex node and beam fixtures now match the three-vertex
control: after one step the node has `y = 77.6831818`, `vy = 0`, and the beam
endpoints have `y = 68.6931839`, `vy = 0`. Both report contact. No source geometry
or scene settings were replaced to obtain these results.

Verification completed:

- All 84 native tests passed.
- Ten focused ASan/UBSan suites passed with leak detection: `contact_manifold`,
  `beam_contacts`, `concave_collision`, `soft_body`, `particle_collision_mode`,
  `rigid_contact_stability`, `physics_recovery`, `physics_reports`,
  `collision_sensor`, and `center_of_mass`.
- Linux and Windows builds passed without compiler warnings. The only warning
  in the command logs is Nix's expected dirty-worktree notice.
- Five installed-SDK consumers, all 12 standalone examples, and a freshly
  generated fixture built for both platforms. Linux consumers ran, and examples
  and the generated fixture passed bounded launches from outside their source
  directories. Windows verification is compilation only.
- An isolated copy of the actual supplied game built for Linux and Windows;
  its Linux executable launched headlessly for 12 seconds and shut down cleanly.
  Its project data, generated objects, and entry point match the original.
  All four original SHA-256 fingerprints above remain unchanged.

For visual review, run
`/tmp/rohr_collision_diagnosis/project_copy/build_linux/Test` with the normal
video driver. Headless execution does not constitute visual confirmation.
Unified Forces and Torques Goal 4 remains pending.

## Evidence and limits

Temporary artifacts are under `/tmp/rohr_collision_diagnosis`: `probe.c`,
`minimal.c`, `CMakeLists.txt`, `minimal.log`, `s*_i*_m*.csv`,
`original_library.csv`, fixed-engine `fixed_s1_i8_m*.csv`,
`fixed_minimal.log`, and the isolated `project_copy`. They are diagnostic
artifacts, not shipped tests, and may be removed by temporary-directory cleanup.
The fixture and measurements above preserve the essential reproduction.

Build the harness through `nix develop -c cmake`; `probe.c` compiles the supplied
`src/generated/project_objects.c` against the installed SDK and internal contact
headers. The command-line arguments are substeps, solver iterations, tick count,
and mode bits: 1 = three-vertex control, 2 = disable beams, 4 = disable node
response. Example fixed-engine invocation:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software \
  /tmp/rohr_collision_diagnosis/fixed_build/collision_diagnosis 1 8 900 0
```

Diagnostic builds completed without compiler warnings. Ten controlled scene
runs, the original-library replay, and minimal node/beam cases completed without
fatal errors. The correction addresses the reproduced missing-face defect.
These finite fixtures and replay intervals do not prove freedom from other high-speed
tunneling or soft-body stability issues. User visual review remains the next
handoff step; no further physics milestone has been started.
