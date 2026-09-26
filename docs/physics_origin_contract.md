# Origin, center of mass, and inertia contract

This is the agreed implementation contract for the **consistent origins,
center of mass, and inertia** milestone in [the roadmap](../NEXT_STEPS.md).
The implementation includes origin-relative geometry, attachment preservation,
runtime COM state, derived inertia, COM-aware integration and constraints,
persistence, CLI commands, generated C, and editor COM controls. Runtime
transforms vertices directly from the origin without recentering. The
[editable COM example](../examples/center_of_mass/README.md) demonstrates the
contract with automatic and explicit COM, rotation, and local anchors.

## Coordinates and mutation

An entity's position is its origin in world coordinates. Geometry vertices,
explicit center of mass (COM), entity-owned joint anchors, particle centers,
and attached visual positions are offsets in the owner's local axes. For local
offset `q`, origin `O`, and orientation rotation `R`:

```text
world_point = O + R * q
world_com   = O + R * local_com
```

Shape transforms must not subtract a centroid. Translating the origin retains
every local offset, the shape, COM-to-geometry relationships, mass, and inertia.
No origin-to-centroid action or automatic origin recentering is provided.
There is no rebase operation that compensates by rewriting offsets.

`physics_position_set()` teleports the origin without deriving velocity or an
impulse. `physics_orientation_set()` changes orientation with the origin fixed;
local points, including an offset COM, rotate around that origin. Both preserve
COM linear velocity and angular velocity. Existing attachment locks still
determine which transforms attached items inherit.

World anchors remain in world coordinates. Moving a body does not move other
bodies connected by joints. A teleport can therefore introduce overlaps or
joint error that the next simulation step resolves normally. Preserving local
properties does not imply preserving interactions with the external world.

Soft-body origin edits preserve their existing group-transform behavior and
current node offsets during the edit; this does not freeze later deformation.
Particles retain their existing restriction against simulated angular motion.
Future body-local force points must use the same transform contract; this
milestone does not add force-emitter authoring or a force-at-point API.

## COM and derived inertia

COM defaults to automatic. Automatic COM is the area centroid of the active
polygon hitbox under the supported uniform-density model. Explicit COM is a
stored local offset; setting it selects explicit mode. Returning to automatic
mode discards the override. The particle collision circle is not an additional
mass contribution to its backing polygon. Pure standalone particles instead
fix COM at their circle centroid, reject explicit overrides, and retain their
existing restriction against simulated angular motion. Ordinary rigid bodies
with particle collision still support explicit COM. No compound mass distribution or
independently authored density is introduced.

| Change | Automatic COM | Explicit COM | Inertia |
| --- | --- | --- | --- |
| Translate origin or change orientation | Retain local offset | Retain local offset | Unchanged |
| Edit active geometry or select another hitbox | Recalculate centroid | Retain local offset | Recalculate |
| Edit an inactive hitbox | Unchanged | Unchanged | Unchanged until selected |
| Change mass | Unchanged | Unchanged | Recalculate |
| Set explicit COM or return to automatic | Select requested mode | Select requested mode | Recalculate |

Geometry, mass, and COM edits preserve the origin, orientation, linear velocity,
and angular velocity. They do not implicitly move the body, conserve momentum
by rewriting velocities, or recenter vertices. Automatic COM can consequently
move in world space when geometry changes. Explicit COM may lie outside the
shape; it need only be a valid finite local offset.

Inertia is derived about the effective COM. For polygon centroid `g`, effective
local COM `c`, mass `m`, and uniform polygon centroid inertia `I_g`:

```text
I = I_g + m * dot(g - c, g - c)
```

This defines the supported approximation when an explicit COM differs from
the geometric centroid; it does not infer an unknown density distribution.
There is no inertia setter or serialized inertia override. Zero mass produces
zero derived inertia. Static and kinematic-driven bodies retain their existing
response rules; stored positive mass does not make them respond to impulses.
Solvers must never divide by zero or non-finite inertia.

## Integration and solver contract

Rigid-body linear velocity and acceleration describe COM motion in world axes.
During free integration, advance world COM and orientation, then recover the
origin as `O = world_com - R * local_com`. With zero linear velocity and nonzero
angular velocity, COM stays fixed while an offset origin follows the rotation.
This differs deliberately from the direct orientation setter's fixed-origin
authoring behavior.

Contact, spring, pin, and weld calculations use lever arms measured from world
COM, while their authored anchor offsets remain relative to the origin. Torque
uses the same derived inertia throughout the pipeline. Solver rotation
corrections also recover the origin from the corrected COM and orientation.
Any applied force whose line of action passes through COM contributes no torque.

Entities without geometry may move and receive an explicit COM. In automatic
mode with no geometry, integration uses the origin as its temporary reference;
automatic-COM and inertia queries still report missing required components.
Prescribed angular velocity remains supported subject to body restrictions.
Torque and angular solver response require valid derived inertia.

Only validated geometry installs `ROHR_HIT_BOX`. A collision flag alone does
not make an entity eligible for geometry or collision processing. Collision
filters can be stored before geometry exists and take effect when valid geometry
is attached. Runtime state saves preserve those filters. Invalid
creation fails; invalid edits leave the previous hitbox, flags, and mass
properties unchanged. Removing geometry clears the hitbox flag; assigning valid
geometry restores eligibility. Generic component addition cannot install a
hitbox flag without prepared geometry; state loading derives that flag from
validated geometry rather than accepting it from a serialized mask. Reject
degenerate and non-finite polygons at
the mutation boundary rather than revalidating vertices in the pipeline.

Axis locks constrain the origin, not COM. Transform-lock local offsets remain
origin-relative. Constraint velocity calculations must convert between COM
velocity and origin/attachment point velocity, including rotation.

Existing hold, static, kinematic, axis-lock, angle-lock, and attachment policies
remain in force. The implementation must reconcile their coordinate handling
with COM integration without silently enabling previously suppressed motion.
Geometry bounds and world-space query results must reflect the current origin
and orientation after edits and solver corrections.

## Runtime C API

These declarations are available in `physics.h` with identically typed wrappers in
`rohr.h` and `src/core/rohr.c`, formed by prefixing each function with `rohr_`:

```c
ERROR_DECLARE_RESULT_TYPE(BoolResult, bool);
ERROR_DECLARE_RESULT_TYPE(MomentOfInertiaResult, float);

EngineResult physics_center_of_mass_local_position_set(
    Entity entity, Position local_offset);
PositionResult physics_center_of_mass_local_position_get(Entity entity);
PositionResult physics_center_of_mass_world_position_get(Entity entity);
EngineResult physics_center_of_mass_automatic_set(Entity entity);
BoolResult physics_center_of_mass_automatic_check(Entity entity);
MomentOfInertiaResult physics_moment_of_inertia_get(Entity entity);
```

The two position getters return the effective COM in the named coordinate
space. The automatic check returns a successful boolean for a valid entity,
or an error for an invalid entity. Its approved `BoolResult` signature retains
error reporting rather than interpreting an invalid handle as explicit mode;
this is a deliberate exception to the usual plain-boolean `_check` convention.
No COM config struct or world-space COM setter is added.

COM configuration is entity-owned and independent of hitbox-variant ownership.
New and reused entities begin in automatic mode; deletion/reset releases the
override. Setting an explicit offset or selecting automatic mode may precede
geometry creation and must not implicitly add dynamic motion or mass.
Explicit local COM can be read without geometry; automatic local COM requires
active geometry. World COM additionally requires the entity transform. Inertia
requires active geometry and a mass component, including an explicitly stored
zero mass. Missing required data returns `ERROR_ENGINE_COMPONENT_MISSING`.
Absent explicit COM state denotes automatic mode, not a missing component.

Use the existing entity-validation errors. Reject non-finite or out-of-range
offsets using the existing position-validation convention, and reject invalid
derived results rather than exposing NaN or infinity. Setters validate before
committing and leave state unchanged on failure. Getters do not allocate or
mutate authored state. Stored overrides must follow existing component-pool
ownership and allocation-failure conventions.

Existing API signatures remain intact, with clarified behavior:

| API family | Contract |
| --- | --- |
| `physics_position_set/get` | World origin position |
| `physics_orientation_set` | Set angle while holding origin fixed |
| `physics_velocity_set` and existing velocity access | World COM velocity for rigid bodies |
| Hitbox setters/getters | Origin-relative vertices, without recentering |
| `physics_shape_world_translate` | Apply rotation and origin translation directly |
| Polygon/circle inertia helpers | Geometry-centroid inertia, retaining their current signatures |
| `physics_moment_of_inertia_get` | Entity inertia about its effective COM |

The same meanings apply through the public `rohr_` wrappers. Do not add a second
origin-position API or reinterpret local joint-anchor getters as COM-relative.

## Authoring, compatibility, and acceptance

The implementation persists automatic/explicit mode and the explicit local offset through
project data, CLI, JSON, generated C, and runtime state serialization.
Project data format 3 and game-state schema 2 reject older versions without
migration. Missing COM configuration means automatic; explicit mode requires
both local coordinates. Automatic mode with an offset and other malformed COM
input return an error without replacing the previous successful description.
Automatic mode must remain automatic after a round trip; do not bake the
calculated centroid into an explicit override. Runtime and generated creation
apply authored configuration before simulation. The editor provides mode
and local-offset controls, derived inertia information, distinct origin/COM
visualization, and normal duplication and undo/redo behavior.

This is a breaking coordinate-semantics change. Rewrite affected bundled
projects, direct-C examples, and fixtures for the new contract; do not add
legacy migration, centroid compensation, or compatibility switches. Update
generated modules without overwriting developer-owned entry points. The
shared attachment redesign and new force-emitter tooling remain separate work.

Acceptance coverage, implemented with each behavior rather than deferred:

- Off-center and rotated polygons obey `O + R * q` in rendering and collision;
  origin edits retain vertex, anchor, particle-center, and attachment offsets.
- Automatic COM follows active geometry edits and hitbox switches; explicit
  COM survives them. Mass edits affect inertia without changing COM offsets.
- Free rotation keeps world COM stationary with zero COM velocity; direct
  orientation edits keep the origin stationary. Teleports preserve velocities.
- Concave polygons, clockwise/counterclockwise outlines, explicit COM outside
  the shape, and parallel-axis inertia produce consistent results.
- Contacts, springs, pin/weld constraints, and torque use the same COM/inertia;
  static, kinematic, held, locked, zero-mass, and nonrotating particle cases
  retain their response policies and avoid invalid division.
- Invalid handles, missing geometry/mass, invalid offsets, failed mutations,
  deletion, and entity-slot reuse respect error and ownership conventions.
- Direct C, public wrappers, persistence, CLI, generated runtime, and editor
  preview agree; undo/redo and duplication preserve mode and explicit offsets.
- Run focused regression tests with each goal, then integrated and relevant
  sanitizer checks, Linux/Windows builds, and installed-SDK example build/run
  verification before completing the milestone.
