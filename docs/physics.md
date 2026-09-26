# Physics

Rohr provides a standard 2D physics pipeline and exposes its stages for games
that need custom orchestration. Most games should call
`rohr_system_physics_update(dt)` or `rohr_physics_pipeline_update(dt)`.

The planned breaking changes to origin-relative geometry, center of mass, and
derived inertia are specified in the
[origin and COM contract](physics_origin_contract.md). That contract describes
the target behavior; its new APIs are not yet implemented.

## Standard pipeline

Each configured substep executes:

1. Clear substep contact and joint constraint buffers.
2. Clear force-derived acceleration from the previous substep.
3. Apply engine gravity to entities carrying `ROHR_GRAVITY`.
4. Apply spring and soft-body beam forces.
5. Integrate velocities, positions, and orientations.
6. Build/query the AABB tree and gather rigid and soft-body contacts.
7. Gather active pin and weld constraints.
8. Iterate contact and joint constraints together.

Current/previous interaction state is advanced once for the complete physics
update, not once per substep. Configure fixed substeps and solver iterations
with:

```c
rohr_physics_substeps_set(2);
rohr_physics_solver_iterations_set(8);
```

More substeps improve detection and integration for fast or light bodies but
repeat broadphase and narrow-phase work. More solver iterations improve contact
and joint convergence without increasing temporal sampling.

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

The broadphase stores active collider bounds in a dynamic AABB tree. Candidate
pairs pass mutual collision filters before narrow-phase work. Each collider has:

- a category mask describing what it is; and
- a `collides_with` mask describing what it accepts.

Both directions must accept the other category. `ROHR_COLLISION` determines
whether a detected overlap enters physical response; a filtered sensor may
report overlap without contact response.

Narrow phase uses SAT on relevant convex-piece pairs. Similar contacts are
merged while distinct external surface normals are retained. Each retained
manifold contains up to two points. The iterative solver accumulates normal and
friction impulses and applies penetration correction.

## Particles

`ROHR_PARTICLE` adds circle collision geometry without replacing the entity's
polygon hitbox:

- particle versus particle uses the configured circles;
- particle versus ordinary rigid body uses both entities' polygon hitboxes.

Create reusable standalone particles through `ParticleConfig`. The default
configuration is valid as-is; copy it and change instance state when manually
emitting multiple particles:

```c
ParticleConfig dust = rohr_physics_particle_config_default_get();
dust.radius = 3.0f;
dust.mass_value = 0.3f;

for(size_t i = 0; i < amount; i += 1) {
    ParticleConfig particle = dust;
    particle.position = positions[i];
    particle.velocity = velocities[i];
    EntityResult created = rohr_physics_particle_create(particle);
}
```

Creation derives the circular polygon hitbox from `radius` and offsets both
representations by `local_origin`. A
`rigid_vertices` value of zero selects
`ROHR_PARTICLE_RIGID_VERTICES_DEFAULT`; an explicit value changes only the
polygon approximation quality. The particle circle and initial polygon radius
therefore cannot disagree. `static_body` creates a static particle; otherwise
mass, initial motion, acceleration, and optional gravity are applied to a
dynamic particle.

Particle origin is a local offset from the rigid-body origin and rotates with
the body. Particle radius and origin have dedicated APIs:

```c
rohr_physics_particle_origin_set(entity, (Position){4.0f, 0.0f});
rohr_physics_particle_radius_set(entity, 12.0f);
```

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
apply forces; pin and weld joints are iterative constraints. In editor
constraint placement, argument order determines the moved side where a
constraint must align two anchors.

Soft bodies own particle nodes, beams, and optional generated filled surfaces.
Every beam remains a soft-body-owned constraint and owns thick-segment collision
independently of surface triangles. On creation, collision is enabled only when
both endpoint nodes enable collision, the node category and collide-with masks
are combined, and thickness starts at the smaller endpoint diameter. These are
creation defaults, not live inheritance: later node filter changes do not alter
the beam, and node growth does not grow or re-enable it.

Thickness is bounded by
`ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN` and the current smaller endpoint
diameter. Shrinking a node caps an oversized beam; if the endpoint maximum falls
below the minimum, collision is disabled while the stored thickness remains at
the engine minimum. The complete collision config setter validates before
committing, and the individual enabled, thickness, and filter setters remain
available for focused changes.

The collision primitive is a centroid-to-centroid rectangle. For each target,
the solver removes the portion inside an endpoint node only when that node's
filter allows it to handle the target. This avoids duplicate node/beam response
without preventing a differently filtered beam from reaching the endpoint.
Beam contacts belong to the beam's stable entity handle, and endpoint ordering
does not change the interpolated material response. Surface triangles provide
rendering only and do not participate in collision; physical holes are simply
open spaces in the beam topology.

## Debugging and limits

Available diagnostics include:

- AABB-tree drawing;
- contact point and normal drawing;
- per-stage timing and pair/contact counters through `PhysicsDebugStats`;
- the reusable UI physics-debug panel.

Current limitations:

- polygons must be simple; holes and self-intersections are rejected;
- collision detection is discrete, without swept collision/CCD;
- substeps are fixed rather than adaptive;
- contact reporting exposes one representative manifold even when the solver
  retains several concave surface manifolds;
- extreme speed, very small geometry, or large mass ratios may require more
  substeps and solver iterations.
