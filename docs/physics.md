# Physics

Rohr provides a standard 2D physics pipeline and exposes its stages for games
that need custom orchestration. Most games should call
`rohr_system_physics_update(dt)` or `rohr_physics_pipeline_update(dt)`.
Both return an owned `Report`; check `report.header.status` for failure. See
[Physics reports](reports.md#physics-reports) for the report tree, custom pipeline
prerequisites, metrics, diagnostics, and migration from `EngineResult`.

The [origin and COM contract](physics_origin_contract.md) specifies this
milestone's behavior. Origin-relative geometry, automatic/explicit COM,
derived inertia, COM integration, persistence, generated APIs, and editor controls
are implemented. The [center-of-mass example](../examples/center_of_mass/README.md)
provides an editable project and generated runtime demonstration.

## Physics creation and ownership

Physics creation uses value descriptions with `_add` APIs and matching direct
and `rohr_` entry points. Inline compound literals and reusable variables are
both supported. `owner` optionally assigns lifetime ownership; omitting it
creates an independent entity. Ownership does not follow endpoints, force
targets, or reference entities, and `rohr_entity_owner_set` preserves connections.

`ParticleDescription` retains particle configuration fields and defaults, with
an optional owner. Use `rohr_physics_particle_description_default_get()` for a
valid unit particle. Joints use one `JointDescription` for creation, complete
replacement, and retrieval:

```c
JointDescription spring = {
    .owner = assembly,
    .type = JOINT_SPRING,
    .anchor_a = anchor_a,
    .anchor_b = anchor_b,
    .rest_length = 100.0f,
    .rest_length_explicit = true,
    .stiffness = 20.0f,
    .damping = 2.0f,
};
EntityResult created = rohr_physics_joint_add(spring);
/* Check created before using its value. */
EngineResult changed = rohr_physics_joint_description_set(joint, spring);
JointDescriptionResult current = rohr_physics_joint_description_get(joint);
```

Only anchor handles connect joints. Anchors are borrowed; adding or replacing a
joint never allocates anchors or makes their bodies lifetime owners. The full
setter also replaces `owner`; use the getter before editing an existing joint
when its other fields and owner should remain unchanged. Validation, capacity,
and ownership failures leave both configuration and owner untouched.

Without `rest_length_explicit` / `rest_angle_explicit`, add/set measures the
current world-anchor distance / relative body angle once. The getter returns
resolved values with both flags true, so get-modify-set preserves the rest pose
after the bodies move. Explicit flags allow zero rest length or angle. Angles
are clockwise degrees. Rest length, stiffness, and damping must be finite and
nonnegative; angles must be finite. An invalid anchor sentinel disconnects that
endpoint and leaves the joint inactive; stale nonzero handles are errors.
Automatic values resolve to zero while disconnected and are measured again on
an automatic replacement after reconnecting.

The positional pin/weld/spring setters and public raw component setter have been
removed. New descriptions replace the former body-plus-local-offset constructor.
Create anchors explicitly for manual wiring, or spawn a whole authored object to
create and wire all its internal anchors and joints automatically. The solver
uses the existing anchor-based path. Saved connected joints without anchor
references are rejected with an explanatory error and load rollback; no anchors
are synthesized. Anchor-based and intentionally disconnected saves remain valid.

`rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body,
.local_offset = {1, 0}})` returns a resource ID attached to `body`, not an entity
with a separate lifetime owner. `SoftBodyNodeAnchorPinDescription` supplies
`owner`, `node`, and an existing `anchor` to the node-to-anchor pin add helper.
Its owner controls the resulting joint only. Delete the joint and remove the
returned node anchor separately; removing an anchor disconnects referencing
joints without deleting them. Deleting its attached entity releases the anchor.

`ForceDescription` and `TorqueDescription` also carry `owner`, independently of
`target`. Full description setters atomically replace configuration and owner;
field setters preserve ownership. Description getters and entity snapshots
report the current generic owner even after transfers, including when a source
has both force and torque components. Failed creation or replacement leaves
existing ownership and configuration unchanged.

One-shot force/torque `_apply` requires `owner == ENTITY_INVALID`. It retains the
existing temporary-source tick lifetime; use `_add` for persistent owned loads.
There are no compatibility wrappers for replaced constructors or `ParticleConfig`.
Regenerate existing projects and migrate handwritten calls when updating the SDK.

## Angles and angular motion

Angles are clockwise degrees, angular velocity is degrees/second, and angular
acceleration is degrees/second squared. Zero heading is world up (+Y); 90 degrees
points right. Zero orientation preserves the geometry as authored. Values such
as -450 and 810 remain unwrapped, including angle-lock bounds and saved state.

For example, `rohr_physics_orientation_set(body, 90)` turns local up toward
world right. `rohr_physics_orientation_radians_set(body, PI_F / 2)` does the same
through the explicit radians API. Structs and component pools always use
degrees; radians variants apply only to the named scalar arguments/results.

Torque, angular impulse, and inertia keep their physical units. Positive torque
rotates clockwise; angular acceleration is `degrees(torque / inertia)`, and an
angular impulse adds `degrees(impulse / inertia)` to angular velocity.
See the [complete angle contract](angle_convention_contract.md) for conversion
rules and the breaking project/state format versions.

## Soft-body angular velocity

The ordinary angular-velocity setter/getter also accepts soft-body entities,
including the explicit radians variants. Nodes remain the source of truth.
The getter measures clockwise angular momentum divided by point-mass inertia
about the current mass-weighted COM. This is an aggregate angular rate, not a
unique orientation for a deforming shape.

Setting a rate adjusts each node velocity by the difference between requested
and current aggregate rotation. COM translation and residual deformation motion
are preserved. Setting zero removes aggregate rotation without stopping other
motion. The edit happens once; subsequent simulation can change the rate, and
beam stretch and tension develop through the existing spring solver. Each node
retains its static/dynamic/held state. Empty bodies, coincident nodes with zero
inertia, invalid masses/motion, and unrepresentable results fail atomically.

The soft-body container's stored orientation is not integrated independently,
including stale angular-velocity state from old saves. Explicit orientation
edits still rotate the node group around the authored origin.

Editor-generated inherited initial velocities use clockwise degrees/second and
COM-relative offsets. Explicit per-node motion overrides remain authoritative,
so a mix of inherited and overridden nodes can yield a different aggregate
rate than the authored inherited value. Regenerate existing projects to apply
this correction; project JSON does not change.

## Origin-relative geometry

Hitbox vertices are local offsets from the entity origin. World geometry is
`origin + rotated local vertex`, without implicit centroid subtraction. Moving
the origin preserves local vertices, particle-center offsets, and attached
anchor/visual offsets. Existing attachment inheritance settings still apply.
Direct orientation edits rotate geometry around the origin. Position edits do
not derive impulses or change stored linear/angular velocities.

`rohr_physics_global_hit_box_get()` uses the current transform immediately,
including before the next physics tick, so hitbox drawing reflects edits.
Overlap/contact history still updates through the physics pipeline. Soft-body
origin edits move the node group while retaining its current local offsets.
Particles retain their restriction against simulated angular motion.

This breaks the former implicit polygon recentering behavior. Rewrite authored
vertices or placement explicitly where an existing project relied on it; there
is no legacy compensation or origin-recentering operation. Simulated rigid-body
rotation and solver lever arms use COM; direct orientation edits still hold the
origin fixed.

## Standard pipeline

Each completed substep executes:

1. Clear substep contact and joint constraint buffers.
2. Clear force-derived acceleration from the previous substep.
3. Apply engine gravity to entities carrying `ROHR_GRAVITY`.
4. Apply spring and soft-body beam forces.
5. Apply explicit loads, select collision-aware duration, and integrate velocities,
   positions, and orientations.
6. Build/query the AABB tree and gather rigid and soft-body contacts.
7. Gather active pin and weld constraints.
8. Iterate contact and joint constraints together, refreshing rigid candidates
   as position corrections introduce additional contacts.

Current/previous interaction state is advanced once for the complete physics
update, not once per substep. Configure minimum substeps and solver iterations
with:

```c
rohr_physics_substeps_set(2);
rohr_physics_solver_iterations_set(8);
```

More substeps improve detection and integration for fast or light bodies but
repeat broadphase and narrow-phase work. More solver iterations improve contact
and joint convergence without increasing temporal sampling.

Full updates consume all requested time. Moving rigid collider pairs use swept
translational projections to shorten steps near impact and bound closing travel
along the contact normal while touching. Tangential sliding and separation do
not trigger thickness-sized steps. Collision steps are at most 1/60 second.
Active joints additionally bound steps to 1/120 second, ten degrees at the current
angular speed, and one tenth of the spring's inverse natural frequency. These bounds
consume all elapsed time without limiting speed. Reports count actual subdivisions. The explicit
`physics_pipeline_substep(dt)` entry point still executes exactly one caller-sized
step, for manual pipeline composition.

Rigid penetration slop is the smaller of 0.01 units and 0.0001 times the smaller
collider thickness along the contact normal. Reciprocal polygon contact
collections of up to 32 pairs can use simultaneous support solves, with at most
32 bodies and 96 normal/friction rows per block. Contacts sharing a movable body
are grouped together; sharing only a static floor does not couple independent
groups. Small or extreme-weight sets use extended-precision, mass-weighted QR.
Larger sets with moderate weights use sparse double-precision Cholesky, with QR
fallback when necessary. Active constraints reuse their factorization, and
repeated active sets terminate the attempt instead of cycling.

Sticking friction is decided independently at each contact within its Coulomb
bound. Contacts with tangential slip use bounded sliding friction and may stick
once the slip stops. Sliding friction uses each solve's new normal impulse,
preserving earlier coupled impulses. Geometric penetration correction uses equal
translational weights and shape-dependent rotational weights, independent of
stored mass. It uses signed depths at individual contact points and repairs the
connected set on each pass;
later passes gather contacts created by those corrections. Position and velocity
convergence are checked separately, with positional roundoff tolerance capped by
the contact slop. Geometry is reused while both poses remain unchanged, and the
cache is reset for each solver invocation. Public edits between manual solves
therefore invalidate it, including shape edits that preserve the pose. A
contact-only solve can finish early after a preceding pass has applied transform
locks; joint systems retain their configured iterations. This avoids moving a
light support many body widths to repair a small overlap under a heavy load.
Stored masses, impulse response, body modes, and freedom to move are unchanged. Larger,
degenerate, directional, particle, and soft-boundary sets retain the iterative
solver. These are bounded solver paths, not a guarantee for every extreme scene.
Iterative rigid contacts retain each point's restitution velocity target for the
current substep. Later iterations may adjust its accumulated impulse without
undoing the intended rebound. Targets are discarded with the substep contacts;
they do not add persistent state or change public contact-report fields.

The [Milestone 6 fixtures](../NEXT_STEPS.md#6-physics-robustness) include thin-wall
checks for a 1 × 1 square and radius-0.5 standalone particle at up to 10,000
units/s. The particle uses its 32-vertex collision polygon against the flat wall.
Run `sandbox/milestone_6/goal_2/bin/linux/thin_barriers_test`; `--performance`
also enforces the Release CPU limits on the reference laptop on AC. Every
completed substep is checked, including updates delayed by 100 ms and 1 s.

The twenty-box stack fixture runs for 60 seconds before a sideways strike, then
observes natural toppling through 90 seconds. Run
`sandbox/milestone_6/goal_3/bin/linux/persistent_stack_test`; `--performance`
also checks the same Release CPU limits. Every completed substep checks collision
depth, finite state, the fixed floor, energy, and elapsed time. Settling checks
enforce the user-approved pre-strike drift/speed limits and residual speeds during
seconds 85–90, as recorded in the roadmap. The solver does not freeze bodies to
meet those limits.

## Center of mass and inertia

Entity position remains the world origin. Rigid-body linear velocity and
acceleration describe center-of-mass (COM) motion. Free rotation keeps world COM
fixed when its linear velocity is zero; an offset origin moves around it.
Direct orientation setters instead hold the origin fixed.

Automatic COM is the active polygon's uniform-density centroid. Set an explicit
origin-relative offset with `rohr_physics_center_of_mass_local_position_set()`,
or discard the override with `rohr_physics_center_of_mass_automatic_set()`.
Local/world COM getters and `rohr_physics_moment_of_inertia_get()` report derived
properties without allocating. Inertia includes the parallel-axis contribution
of an explicit COM; manual inertia is unsupported.

Geometry and mass edits preserve origin, orientation, and velocities. Invalid
geometry edits fail atomically. Entities without geometry may move and retain
explicit COM, but cannot collide or receive angular response requiring inertia.
Automatic motion temporarily uses the origin when geometry is absent; automatic
COM and inertia queries still report missing components.

Axis locks constrain the origin. Contacts, torque, and joint lever arms use COM.
See the [origin contract](physics_origin_contract.md) for formulas and API error
semantics. Project JSON, CLI, generated C, and runtime state preserve COM mode
and explicit offsets. Standalone particles use their circle centroid and reject
explicit COM overrides. The editor exposes mode and local-offset controls,
derived inertia, and an explicit COM drag handle; see [editor authoring](editor.md#center-of-mass-authoring-and-persistence).

## Body modes

- Static bodies do not move in response to simulation and need no mass.
- Dynamic bodies with positive mass are simulated and respond to impulses.
- Dynamic bodies without positive mass are kinematic-driven: application
  velocity/acceleration may move them, but contacts cannot push them.
- `ROHR_HOLD`, axis locks, angle locks, and transform locks further constrain
  motion.

Gravity is opt-in per entity. `rohr_physics_gravity_set()` changes the global
acceleration; `rohr_physics_gravity_enable()` adds the entity to that stage.

## Polygon collision

Hitboxes are simple polygons with 3 to 50 vertices. `rohr_physics_hitbox_set()`
validates the outline and caches an ear-clipped convex decomposition when the
polygon is concave. Applications still provide and retrieve one polygon; the
convex pieces are internal.

An entity may own multiple ordered hitbox variants while only one is active.
Variants have stable `HitboxId` values so bindings survive reordering. The
index APIs are convenient for direct list access; the ID APIs are intended for
persistent references.

Animation binding is a separate physics component. Graphics animations and
frames contain stable IDs but no hitbox references. The binding stage observes
the current frame immediately before the normal physics pipeline and selects
the mapped hitbox:

```c
rohr_physics_hitbox_animation_binding_set(entity, animation_id, frame_id,
    hitbox_id);
```

Several frames may map to one hitbox. One frame maps to at most one hitbox for
an entity. Removing a hitbox removes bindings that target it.

The broadphase stores active collider bounds in a dynamic AABB tree. Narrow
phase records geometric overlaps independently of response masks. Each collider
has a category mask describing what it is and a `responds_to` mask describing
which categories it responds to.

For each overlapping pair, the solver evaluates the masks independently:

| A responds to B | B responds to A | Physical response |
|---|---|---|
| Yes | Yes | Both participate with their usual effective mass/inertia. |
| Yes | No | Only A receives contact correction and impulses. |
| No | Yes | Only B receives contact correction and impulses. |
| No | No | Overlap remains queryable; no contact response. |

A nonresponding side has zero inverse mass/inertia **for this contact only**.
Its velocity still contributes to relative motion, and both bodies' materials
still determine friction and restitution. Static, kinematic, held, and
nonrotating restrictions continue to apply. A dynamic body can respond to one
category and act as a moving obstacle to another. Joints, beams, and other
constraints can still transmit motion indirectly; masks govern contact response.
`ROHR_COLLISION` must be enabled on both bodies for physical response. Hitboxes
without it remain geometric sensors. Enabled beam boundaries similarly retain
overlap without response; disabled beams have no collision geometry.

Use `.responds_to` in `CollisionFilterDescription`, or the focused
`rohr_physics_collision_response_set`, `_all_set`, and `_none_set` functions.
`rohr_physics_collision_response_to_check(a, b)` checks A's mask against B's
category; `rohr_physics_collision_response_between_check(a, b)` checks whether
either direction permits response. Neither checks geometry, enabled state, or
motion type. Contact/overlap queries describe the physics update's interactions;
a one-sided contact is queryable in both argument orders, with reversed normals.

The old `collides_with`, `collision_with_*`, and `collision_between_check` C
names are removed so callers must review the semantic change explicitly. In
particular, response-none does not prevent another body from responding. To
exclude a pair, configure neither side to respond, or use nonoverlapping category
assignments and response masks. Turning collision off remains a sensor operation.
Editor/CLI controls are named **Responds To** / `responds-to`; editor JSON and
runtime descriptions use `responds_to`.

Project format 12 and game-state format 10 identify these semantics. Older
formats fail with a migration-required diagnostic without modifying files or
replacing loaded state. Do not just bump their version or rename their masks:
C11 will supply behavior-preserving category/mask migration.

Narrow phase uses SAT on relevant convex-piece pairs. Similar contacts are
merged while distinct external surface normals are retained. Each retained
manifold contains up to two points. The iterative solver accumulates normal and
friction impulses and applies penetration correction.

## Particles

`ROHR_PARTICLE` adds circle collision geometry without replacing the entity's
polygon hitbox:

- particle versus particle uses the configured circles;
- particle versus ordinary rigid body uses both entities' polygon hitboxes.

Create reusable standalone particles through `ParticleDescription`. The default
configuration is valid as-is; copy it and change instance state when manually
emitting multiple particles:

```c
ParticleDescription dust = rohr_physics_particle_description_default_get();
dust.radius = 3.0f;
dust.mass_value = 0.3f;

for(size_t i = 0; i < amount; i += 1) {
    ParticleDescription particle = dust;
    particle.position = positions[i];
    particle.velocity = velocities[i];
    EntityResult created = rohr_physics_particle_add(particle);
}
```

Creation derives the circular polygon hitbox from `radius` and offsets both
representations by `local_origin`. A
`rigid_vertices` value of zero selects
`ROHR_PARTICLE_RIGID_VERTICES_DEFAULT`; an explicit value changes only the
polygon approximation quality. Creation and full configuration replacement keep
circle and active polygon geometry synchronized. `static_body` creates a static
particle; mass, velocity, acceleration, and gravity settings are retained even
while static, ready for a later dynamic transition. `kinematic_driven` stores
explicit prescribed-motion intent independently of mass. Getters report that
intent even while Static and never infer it from zero mass. Full replacement
sets or clears intent; an automatically kinematic zero-mass particle resumes
simulation once mass becomes positive, whereas explicit intent survives mass
changes. Kinematic particles move with prescribed motion and ignore gravity
and contact impulses. Response masks remain independently editable.

`rohr_physics_particle_description_get()` returns current configurable values,
including the owner and active polygon quality. Modify its result and pass it to
`rohr_physics_particle_description_set()` to edit selected fields. Full replacement
is atomic: invalid input or allocation/ownership failure preserves the original
particle. Omitted fields become zero/false, with the documented zero-vertex
exception above. These full accessors accept standalone particles only; hybrid
rigid bodies and soft-body nodes retain their own APIs. Names, parenting,
connections, lifetimes, hold state, and visual resources are preserved.

Particle origin is a local offset from the rigid-body origin and rotates with
the body. Particle radius and origin have dedicated APIs:

```c
rohr_physics_particle_origin_set(entity, (Position){4.0f, 0.0f});
rohr_physics_particle_radius_set(entity, 12.0f);
```

For standalone particles, these focused setters also rebuild the active polygon,
retaining its ID, inactive variants, and animation bindings. For hybrid rigid
bodies and soft-body nodes, their existing geometry behavior remains unchanged.

If no explicit radius exists, it is derived from the hitbox centroid and
farthest vertex. Broadphase bounds include both the polygon and an explicit,
possibly offset particle circle. Draw them independently with
`rohr_graphics_hit_box_draw()`, `rohr_graphics_particle_draw()`, or their
all-entity variants.

Particles do not integrate angular motion. Soft-body nodes are particle
entities and also participate in the soft-body boundary-contact path.

## Contacts and overlaps

An overlap is geometric intersection. A contact is an overlap that enters
physical collision response. Query families are symmetrical:

```c
rohr_physics_overlap_check(first, second);
rohr_physics_overlap_get(first, second);
rohr_physics_contact_check(first, second);
rohr_physics_contact_get(first, second);
```

Entered, stayed, and exited queries describe transitions between the current
and previous complete physics updates. Returned normals always point from the
first requested entity toward the second.

`ContactInfo` reports the representative pair normal/depth and up to two
manifold points with relative velocity, accumulated normal impulse, and
accumulated friction impulse. Internally, concave pairs may retain more than one
surface manifold for solving.

## Joints and soft bodies

Joint anchors store local offsets on one entity or world positions. Springs
apply forces; pin and weld joints use coupled anchor constraints (including the
angular row for welds). Complete physics substeps also integrate independent
joint pairs in constrained coordinates: a pin with an immovable support uses
angular momentum about its anchor, a free welded pair uses composite mass and
inertia, and a free center-attached spring pair uses implicit midpoint forces.
These paths require ordinary rigid bodies with one joint per endpoint and no
motion locks or angular speed limits. Connected networks, noncentral springs,
and unsupported configurations retain ordinary integration and iterative solving.
Contacts still run through the shared contact pipeline. No artificial damping,
sleeping, or settling rule is added. Standalone `physics_pipeline_integrate`
retains its prepared-load semantics and does not perform these joint operations.

Rotation calculations reduce complete turns before converting to radians while
preserving stored, unbounded degree values. Restitution applies at low speeds
without a one-unit/second cutoff; iterative manifold points capture their
initial rebound targets before any point applies an impulse.

The three `pin_stability`, `weld_stability`, and `spring_stability` tests cover
isolated high-energy motion and floor impacts, with joint error, independent
energy/momentum, collision, and elapsed-time checks. The spring test also checks
an independent numerical reference and an analytic axial oscillator. Executables
and local preview evidence are under `sandbox/milestone_6/goal_4/`.

In editor
constraint placement, argument order determines the moved side where a
constraint must align two anchors.

Soft bodies own particle nodes, beams, and optional visual node-loop areas.
The current [Milestone 6 Goal 5 work](../NEXT_STEPS.md#6-physics-robustness)
adds enabled beam-to-beam contacts, including same-owner contacts. Shared-endpoint
beam pairs are excluded; other pairs obey directional collision filters and
distribute response across both pairs of nodes. Node contacts retain ownership
of matching endpoint regions. Beam collision remains disabled by default.

Complete physics substeps use a second spring-force evaluation for elastic
soft-body nodes, with stiffness and collision-motion limits on adaptive steps.
Zero-stiffness nodes and standalone manual integration keep their existing
prepared-force integration behavior. Contact-only convergence does not freeze
bodies or damp residual motion. These changes do not alter public descriptions
or persistence formats. Goal 5 currently **fails** its extreme-load penetration
and CPU requirements; the roadmap records the measured failures. Its visual
acceptance is also unfinished. These changes must not be treated as completion
of the soft-body robustness milestone.
Each beam has independent lifetime ownership and optional thick-segment collision,
separate from visual areas. **New beams have collision off.** Thickness and filters
are explicit; creation, endpoint edits, and node-radius changes never derive,
clamp, enable, or disable beam collision settings from the nodes. Editor-created
beams follow the same opt-in rule. Existing saved settings remain explicit.

`SoftBodyBeamCollisionDescription` groups enabled state, width, and filter.
Enabled collision requires width at least
`ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN`; disabled collision permits zero.
Setters reject invalid values without changing the beam. The collision default
getter supplies a fixed minimum width and node-category filter, with collision
off. Callers can enable it explicitly or supply a complete enabled description.
Focused enable/disable/thickness setters remain available.

The collision primitive is a centroid-to-centroid rectangle. For each target,
the solver removes the portion inside an endpoint node only when that node's
directional responses match the beam's responses for that target. These cutouts
apply only during contact solving; geometric overlap uses the whole beam. This
avoids duplicate node/beam response
without preventing a differently filtered beam from reaching the endpoint.
Beam contacts belong to the beam's stable entity handle, and endpoint ordering
does not change material response. Areas provide
rendering only and do not participate in collision; physical holes are simply
open spaces in the beam topology.

Beams own independent `friction` and `restitution`, initially 0 and 0.25.
Use `rohr_physics_friction_set(beam, value)` and
`rohr_physics_restitution_set(beam, value)`, and read the stored values through
`rohr_physics_soft_body_beam_get`. Beam setters reject nonfinite values, negative
friction, and restitution outside [0, 1] without changing state or enabling
collision. Endpoint materials no longer determine beam response; node contacts
still use node materials. Contact restitution is the lower of the two contacting
materials, so both the beam and particle need nonzero restitution for bounce.
Friction combines as the geometric mean. Endpoint interpolation still determines
contact velocity and impulse distribution, independently of materials.

Beam contacts participate in the shared iterative contact solve alongside rigid
contacts and joints. Each impact retains its initial relative rebound target
throughout the substep, accumulating normal and friction impulses as supporting
contacts change node velocities. Restitution is not reapplied on each iteration.
A heavy particle can continue forward while pushing a light beam away; bounce
specifies relative separation speed, not an unconditional direction reversal.
Separation uses the distance needed to leave the contact, including when a thin
beam's projection is fully inside the other collider's projection. This does not
provide continuous collision detection between simulation steps.

### Derived soft-body mass and COM

Node masses are authoritative. `rohr_physics_mass_get(body)` sums them, while
`rohr_physics_center_of_mass_local_position_get` and its world counterpart
calculate their mass-weighted center using current node positions. All nodes
contribute, including hidden, collision-disabled, and static nodes. Empty bodies
have total mass zero and unavailable COM; zero-total-mass bodies also lack COM.

`rohr_physics_mass_set(body, total)` proportionally scales positive node masses.
`rohr_physics_center_of_mass_local_position_set(body, offset)` redistributes them
at fixed total mass without moving nodes or changing velocities. The offset is
in the body's origin-relative, rotated coordinate system. Redistribution
minimizes relative entropy between the new and previous normalized masses,
retaining custom preferences and preserving ratios between coincident nodes.
No separate aggregate override or mode is stored. Automatic COM checks return
true, and enabling automatic COM is a successful no-op. Later deformation,
node edits, and topology changes naturally update the derived values.

Both edits require a nonempty body with finite positive node masses. COM must
lie in the relative interior of the node centers' convex hull: inside a polygon,
inside the segment for collinear nodes, or at the shared point for coincident
nodes. Floating-point feasibility and output checks use span-relative numerical
tolerances (up to two parts per million for COM and one for total mass).
A bounded, allocation-free solve validates all resulting positive float masses
before committing. Unreachable targets, nonfinite values, overflow, underflow,
and nonconvergence return an error without changing any node. Total mass must
also fit the public float `Mass` type. Existing node records carry the resulting
masses through runtime persistence; no extra aggregate fields are needed.

The editor exposes the same controls through the soft-body Physics accordion,
a selectable/draggable COM marker, and CLI properties. Edits store only node
masses, preserving runtime behavior in generated projects. See
[soft-body authoring](editor.md#soft-bodies).

### Creating soft-body entities

Soft-body creation takes typed descriptions by value, either reusable variables
or inline compound literals. Use `rohr_physics_soft_body_add`, `node_add`,
`node_local_add`, `beam_add`, and `area_add` (each element name has the
`rohr_physics_soft_body_` prefix). Matching direct APIs omit `rohr_`.
These replace the positional constructors; there are no compatibility wrappers.

```c
EntityResult created_body = rohr_physics_soft_body_add(
    (SoftBodyDescription){.position = {10, 20}, .orientation = 90});
if(rohr_error_check(created_body)) return;
Entity body = created_body.result.value;
SoftBodyNodeDescription node = {
    .owner = body, .position = {2, 0}, .mass = 1, .radius = 2
};
EntityResult local = rohr_physics_soft_body_node_local_add(node);
// Check local before using its handle. The description is unchanged and reusable.
```

`SoftBodyDescription` carries owner, world position, and orientation.
`SoftBodyNodeDescription` carries owner, position, positive mass, and positive
radius. `SoftBodyBeamDescription` carries owner, node endpoints, stiffness, and
damping. `SoftBodyAreaDescription` carries owner and copied geometry.
An omitted owner means independent lifetime; any live owner is accepted by the
world-space constructors. Only immediate soft-body ownership establishes
membership in its physics lists. The local-node constructor requires a soft-body
owner and applies its origin position and rotation exactly once.

Descriptions do not change ownership or connections until creation succeeds.
Failure rolls back the new entity and its resources. Explicit zero values remain
zero: zero node mass/radius is invalid; zero beam stiffness/damping is valid.
Use `physics_soft_body_node_description_default_get()` for standard node material
and collision settings. Omitted fields in a literal become zero/false. Nodes
expose current velocity, acceleration, mass, material, filter, static/gravity/collision
flags, and color override through the same description used for creation. Static
nodes retain their configured motion and mass. Creation requires positive mass;
full replacement also accepts a current zero mass configured through focused
setters, retaining the existing kinematic behavior. Node circle-radius edits preserve
the independently editable polygon and never change attached beam settings.
Complete beams measure rest length once unless `rest_length_explicit` is true;
getters return the resolved length marked explicit. Disconnected endpoints measure
zero. Collision settings are always explicit. Incomplete beams and areas remain
supported. Area descriptions include geometry, color override, and visibility;
use the default getter for a visible area.

All five families have typed `_description_get` and atomic `_description_set`
APIs with matching `rohr_` wrappers. Full setters replace configuration, not patches;
use get-modify-set or focused setters to retain other fields. World positions are
canonical for node full accessors; local-node creation and local-position setters
remain explicit. Container transform descriptions use clockwise degrees and move
existing nodes without rebuilding children. Unchanged descriptions preserve
tracking and solver state. Areas retain sibling order; reordering stays a focused
operation. Allocation, invalid input, stale handles, or ownership failures leave
the previous state intact.

Anchor descriptions can reattach an existing handle to another live entity.
All referencing joints follow that handle atomically, keeping ownership and rest
values. The supplied offset is local to the new entity. Anchor capacity is checked
before commit. Authored anchor lookup and save/load resolve the current attachment.


### Runtime visual areas

An area owns a copied outer node loop and up to 16 copied hole loops. Each loop
closes implicitly, contains 3–64 distinct live nodes that may have different owners, and
needs no connecting beams. Different loops and areas may share nodes.

The fill is every bounded region of the current outer boundary, minus the union
of every bounded hole region. Traversal direction and repeated enclosure never
erase a region. An hourglass fills its two lobes and leaves its sides empty.
Holes subtract only from their owning area: partial overlap creates a notch,
outside holes remove nothing, and complete subtraction is a valid invisible
result. Another area can remain visible through a hole or cover it by layer order.

```c
SoftBodyAreaGeometry geometry = {
    .outer = {.nodes = {a, b, c, d}, .node_count = 4},
    .holes = {{.nodes = {e, f, g, h}, .node_count = 4}},
    .hole_count = 1
};
EntityResult created = rohr_physics_soft_body_area_add(
    (SoftBodyAreaDescription){.owner = body, .geometry = geometry});
if(!rohr_error_check(created)) {
    Entity area = created.result.value;
    rohr_graphics_soft_body_area_color_set(area, (Color){40,120,220,255});
    rohr_graphics_layer_entity_set(area, 3);
    // Replace edited geometry and check the result before assuming success:
    EngineResult edited = rohr_physics_soft_body_area_geometry_set(area, geometry);
}
```

Creation and geometry replacement accept self-crossing loops when each loop
encloses nonzero space. Invalid changes preserve the last successful description.
A hole count above `SOFT_BODY_MAX_AREA_HOLES` (16) returns
`ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED` before accessing hole entries. A loop with
no enclosed space cannot be authored, but subsequent node motion may collapse
any loop without deleting its description; its fill disappears and can return.

Rendering builds a planar arrangement, merges coincident boundary segments,
and uses the unbounded face to identify each loop's filled silhouette. Horizontal
slabs then triangulate the outer silhouette minus hole silhouettes without
overlapping triangle interiors. Temporary intersections are visual geometry,
not entities, beams, or physical connections. Areas alpha-blend once per covered
point. Coordinate-relative double-precision tolerances handle touching geometry;
features below numerical resolution can collapse.

Generated triangles are cached against all referenced node positions and geometry
edits, shared across unchanged draws/cameras, and released with the area. Scratch
storage is explicitly engine-owned and reused until engine shutdown. Moving
complex crossings cost more than simple loops; the geometry regression reports
ordinary-window and 17-loop tangled-case rebuild timings.

Areas start visible and inherit the draw call's surface color. Color clear restores
that fallback. Higher entity layers draw on top; later entries in a body's area
list draw on top within a layer. Unassigned area layers inherit the body's active
draw layer. Geometry replacement preserves style, entity identity, and ordering.

The getter returns owner, geometry, visibility, and color override by value.
Changing a snapshot does not mutate the area. Reordering uses a zero-based
destination index and preserves other areas' relative order. Entity deletion
removes an area, as does deleting its lifetime owner. Deleting a referenced node
clears that slot and leaves the area alive with incomplete geometry. Deleting
beams does not delete areas. Limits remain 64 nodes
per body, 64 nodes per loop, 16 holes per area, and 128 areas per body.

This is a breaking replacement of the single-loop creation argument,
`area_nodes_set`, authored reference positions, and fixed triangulation. Use
`SoftBodyAreaGeometry` and `area_geometry_set`; no compatibility wrappers remain.
The editor persistence, generation, and authoring goals follow this runtime work.
Visual fill holes do not change collision: physical openings still follow node
and beam collision geometry.

## Debugging and limits

Available diagnostics include:

- AABB-tree drawing;
- contact point and normal drawing;
- per-stage timing and pair/contact counters through `PhysicsDebugStats`;
- the reusable UI physics-debug panel.

Current limitations:

- collision polygons must be simple; collision holes and self-intersections are rejected;
- swept timing uses translational polygon projections; arbitrary fast rotation,
  concave sweep accuracy, and general circle/circle or beam continuous collision
  are not certified by the flat-wall fixtures;
- contact reporting exposes one representative manifold even when the solver
  retains several concave surface manifolds;
- extreme speed, very small geometry, or large mass ratios may require more
  substeps and solver iterations.

## Configuration setter convention

Use the same value description for creation and complete configuration
replacement, with a matching getter. Retain focused setters for single fields;
those setters preserve the other fields and lifetime ownership. Solver state
and derived caches are not part of an authored description.

The joint migration establishes this convention without changing other APIs.
A follow-up audit identified the multi-field transform-lock setters as
description candidates. Camera attachment configuration is another candidate.
Existing collision, beam, viewport-item, and UI configuration setters already accept value structs; any
naming alignment should be considered separately. Simple scalar/vector setters
and timing-aware camera moves do not need descriptions merely to reduce their
argument count.
