# Physics field access

Read or change individual runtime physics values without an `EntityInfo`
snapshot. Both `rohr_physics_*` and direct `physics_*` entry points are available.
Reads copy values, allocate nothing, and acquire no ownership.

```c
ScalarResult x = rohr_physics_velocity_x_get(box_id);
if(rohr_error_check(x)) goto done;
float current_x = x.result.value;

EngineResult changed = rohr_physics_velocity_x_set(box_id, current_x + 10.0f);
if(rohr_error_check(changed)) goto done;
```

`ScalarResult` is defined in the shared `error.h`. Its value is `float` (`Vec1D`
is already a typedef of `float`). Vector reads use `VelocityResult`,
`AccelerationResult`, `PositionResult`, or `ForceResult`, with the corresponding
`Vec2D` alias as their value. References, enums, counts and complete
configurations retain typed results. Existing named result types remain valid.
Global gravity X/Y getters return `float` directly, matching the existing
non-errorable global gravity getter. Boolean conditions use `_check` and return
false for invalid IDs or missing components.

## Force and torque

Force operations take the **force source ID**, not its target body's ID. A
body may have several independent force sources. These operations edit or read
one source; they do not sum all forces acting on a target.

```c
EntityResult created = rohr_physics_force_add((ForceDescription){
    .target = box_id,
    .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = box_id}
});
if(rohr_error_check(created)) goto done;
Entity force_id = created.result.value;

EngineResult changed = rohr_physics_force_set(force_id, (Force){.x = 3, .y = 4});
if(rohr_error_check(changed)) goto done;
changed = rohr_physics_force_x_set(force_id, 8); /* Preserves Y = 4. */
if(rohr_error_check(changed)) goto done;

ForceResult force = rohr_physics_force_get(force_id);
ScalarResult force_x = rohr_physics_force_x_get(force_id);
if(rohr_error_check(force) || rohr_error_check(force_x)) goto done;
```

The caller owns a standalone source's lifetime; delete it with
`rohr_entity_delete(force_id)`, or supply an owner at creation for grouped cleanup.
The source's lifetime/expiry is preserved by field setters.

`force_set/get` and `force_x/y_set/get` use the force's **configured orientation
reference**. World-referenced forces use world X/Y. Body-, beam-, and
between-locations references provide a moving frame; the stored components
continue to follow it. Zero orientation offset points along the reference's
positive Y direction; positive X is clockwise from that direction.

Storage remains angle plus signed magnitude. Vector/axis setters recompute those
two fields, preserve the existing magnitude sign, and preserve the angle when
the resulting vector is zero. The untouched component is preserved within
floating-point polar-conversion precision. Getters likewise reconstruct the
vector; there is no new vector cache or serialized field. Nonfinite values and
unrepresentable magnitudes fail without changing the source. Reference geometry
need not resolve to edit/read stored components: a collapsed beam or coincident
aiming locations remain valid configurations under the existing contract.

Torque is scalar. `torque_set/get` are equivalent to the existing
`torque_magnitude_set/get`; target, ownership, and lifetime are preserved.

## Force fields with matching setters and getters

Each family below has both direct and facade spellings. Prefix names with
`rohr_physics_` (or `physics_`) and append `_set` / `_get`.

| Family | Value |
|---|---|
| `force`, `force_x`, `force_y` | Complete reference-frame vector or one component |
| `force_location_offset`, `force_location_offset_x`, `force_location_offset_y` | Application-point offset in the position reference's coordinate space |
| `force_location_reference` | Complete tagged `ForcePositionReference` |
| `force_location_reference_entity` | Referenced entity in COM, origin or beam mode |
| `force_location_reference_beam_anchor`, `force_location_reference_beam_fraction` | Individual beam-reference fields |
| `force_orientation_offset`, `force_orientation_offset_radians` | Angle offset, preserving reference and magnitude |
| `force_orientation_reference` | Complete tagged `ForceOrientationReference` |
| `force_orientation_reference_entity` | Referenced entity in body or beam mode |
| `force_orientation_between_from`, `force_orientation_between_to` | One complete aiming `ForceLocation` |
| Each between-location family plus `_offset`, `_offset_x`, `_offset_y`, `_reference`, `_reference_entity`, `_reference_beam_anchor`, `_reference_beam_fraction` | One field of the selected aiming location |
| `force_target`, `force_magnitude`, `torque_target`, `torque_magnitude`, `torque` | Existing focused source properties, plus scalar torque shorthand |

Offsets keep their established meanings: COM offsets use world axes, origin
offsets use body-local axes, and beam offsets use along/perpendicular axes.
Beam fractions are additive and are not clamped. Changing a reference preserves
the numerical offset, not its resolved world position.

Union-member access requires the appropriate current mode. For example, a beam
fraction edit on a world location returns `ERROR_ENGINE_STATE_INVALID`, and a
between-location getter requires a between-locations orientation. Change modes
atomically with the complete typed reference setter. This avoids exposing a tag
change with stale data in the newly selected union member.

Getters can return stored reference IDs after a referenced entity is deleted;
validate those IDs before using them. Setters use the same validation as complete
source replacement. An unrelated edit fails while a reference is stale; the
appropriate reference setter can repair it. Source ownership is independent of
references, and editing the force component preserves any torque component on
the same source entity.

## Other physics properties

The expanded getter inventory covers the following runtime properties. Existing
full-description getters provide copied configurations, including ownership,
visual metadata and related flags; they also allocate nothing.

| Properties | Access |
|---|---|
| Velocity and acceleration | Complete typed vectors and `_x_get` / `_y_get` |
| World position, local/world COM, particle origin, local/world joint-anchor position, local soft-body node position | Existing complete vectors plus `_x_get` / `_y_get` |
| Gravity | Complete global vector plus `_x_get` / `_y_get` |
| Orientation and angular acceleration | Scalar getters in degrees and `_radians_get` |
| Angular velocity and its limit, mass, inertia, particle radius | Existing typed getters |
| Friction and restitution | Scalar getters; beam entities read their independent beam materials |
| Collision category and response mask | Typed mask getters; unset filters retain existing default semantics |
| Dynamic/static/held/collision state | `_dynamic_check`, `_static_check`, `_entity_held_check`, `_collision_check`; gravity and kinematic checks already exist |
| Angle lock | Full `AngleLockResult`, minimum/maximum scalar getters, and radians variants |
| Axis lock | Axis and point vectors and X/Y getters; the axis reflects stored normalization |
| Transform lock | Driver, local offset and X/Y, local angle and radians, and the three flag checks |
| Joint | Type, anchor A/B, resolved rest length/angle, stiffness and damping; angle has a radians variant |
| Soft-body node | Radius and local position; generic entity motion/material getters and existing descriptions cover other fields |
| Soft-body beam | Endpoint A/B, resolved rest length, stiffness, damping, thickness and collision-enabled check; generic material and existing filter getters apply |
| Soft-body area | Copied geometry and current order in its owning body's area list |
| Soft-body membership | Node/beam/area count getters; existing `soft_body_get` returns the copied ID arrays |
| Particle geometry quality | `particle_rigid_vertices_get`; origin/radius use existing getters and new axis reads |
| Generic physics target | `target_get`; returns the stored borrowed ID |
| Hitboxes, bindings, overlaps and contacts | Existing typed geometry, ID, list and interaction getters |

Creation conveniences such as autoshape dimensions, impulses and
`toward_position` setters do not retain separate configuration. Read their
resulting geometry or motion instead. Ownership uses existing entity/description
accessors rather than a second physics ownership API.

Invalid/stale entity IDs and absent required storage return errors in typed
getters. A missing motion value is not silently returned as zero; the axis
setters' documented initialization behavior remains separate. Returned values
are independent copies; a later physics update or setter affects the next read,
not a previous result. Serialize reads and mutations with engine updates.

## Remaining partial setters

Milestone 19 Goal 3 adds the following focused edits. Prefix each name with
`rohr_physics_` or `physics_`; all setters return `EngineResult`. They require
the corresponding existing component, rather than creating a configuration
with guessed values for its other fields.

```c
EngineResult changed = rohr_physics_joint_stiffness_set(joint_id, 25.0f);
if(rohr_error_check(changed)) goto done;

changed = rohr_physics_transform_lock_local_offset_x_set(entity_id, 8.0f);
if(rohr_error_check(changed)) goto done;
```

| Configuration | Focused setters |
|---|---|
| Angle lock | `angle_lock_min_set`, `angle_lock_max_set`, and radians variants |
| Axis lock | `axis_lock_axis_set`, `axis_lock_axis_x_set`, `axis_lock_axis_y_set`; `axis_lock_point_on_axis_set` and X/Y variants |
| Transform lock | `transform_lock_driver_set`, `transform_lock_local_offset_set` and X/Y variants, `transform_lock_local_angle_set` and radians variant; `transform_lock_lock_position_set`, `transform_lock_lock_orientation_set`, `transform_lock_inherit_velocity_set` |
| Joint | `joint_type_set`, `joint_anchor_a_set`, `joint_anchor_b_set`, `joint_rest_length_set`, `joint_stiffness_set`, `joint_damping_set`, `joint_rest_angle_set` and radians variant |
| Joint anchor | `joint_anchor_entity_set`; the matching `joint_anchor_entity_get` returns `EntityResult`. Existing local-offset setters remain available. |
| Standalone particle | `particle_rigid_vertices_set`; existing radius, origin and general body setters cover its other physics fields |
| Soft-body node | `soft_body_node_radius_set`; existing local-position and general body setters cover its other physics fields |
| Soft-body beam | `soft_body_beam_node_a_set`, `soft_body_beam_node_b_set`, `soft_body_beam_rest_length_set`, `soft_body_beam_stiffness_set`, `soft_body_beam_damping_set` |

Joint and beam description getters expose their **resolved** rest values with
explicit flags. Field edits retain those values, including zero, even after the
connected bodies move or an endpoint is disconnected/replaced. Changing a
joint's type also preserves them. To deliberately measure a new rest state, use
the complete description setter with the corresponding explicit flag false.

Anchor transfer preserves its local offset and ID and updates connected joints'
body references. It can change the anchor's world position and lifetime parent;
the old entity no longer owns that anchor. Joint ownership and rest values are
unchanged. Joint endpoint setters accept `JOINT_ANCHOR_INVALID`; beam endpoint
setters accept `ENTITY_INVALID` to disconnect that endpoint. Stale IDs fail.

Axis direction is normalized by the existing full setter. Editing X or Y first
retains the other stored component, then normalizes the resulting vector; both
returned components can therefore change. A zero direction is rejected. Point
edits retain the direction, subject to existing normalization precision. Angle
limits retain the full setter's finite-value semantics. Radian conversion that
overflows is rejected without changing the configuration.

Particle vertex count changes only the active polygon through the existing
geometry preparation/commit path. Zero selects the engine default; other values
must lie between `MIN_VERTICIES` and `MAX_VERTICIES`. Circle radius, local origin,
body state, inactive variants, hitbox IDs and animation bindings are preserved;
geometry caches and derived mass properties refresh as usual. Soft-body node
radius changes update its circle while retaining its independently editable
polygon. These calls introduce no allocation of their own; underlying geometry
or component setters retain their existing allocation and failure contracts.

## Completed field-access inventory

The new operations supplement existing focused access rather than creating
duplicate names for every resource-specific spelling:

- Motion, positions, materials and collision filters use the general entity
  APIs. This includes soft-body nodes; generic material/filter setters dispatch
  to independent beam state where applicable.
- Collision category and response each have a setter preserving the other
  mask. Collision/gravity modes use existing enable/disable operations;
  static/dynamic and kinematic state retain their existing mode operations.
  Beam collision enable/disable and thickness already preserve its other fields.
  Generic filter edits now also update the node's `SoftBodyNode` snapshot,
  keeping it consistent with the effective collision filter.
- Soft-body transforms use position/orientation setters. Area geometry and
  order already have focused setters. Polygon and area topology remain complete
  validated geometry values, rather than independently mutable counts/vertices.
- Entity ownership and lifetime use their existing APIs. Visual colors,
  overrides and visibility use graphics/entity APIs and complete descriptions.
  Focused physics edits preserve them.
- World COM, world anchor positions, inertia, membership counts and other
  derived values are read through their getters. Change their underlying
  geometry, local coordinates, masses, membership or body transforms instead.

## Verification

Milestone 19 Goal 2 adds 31 setters and 109 getters/checks, each exposed through
both direct-engine and facade APIs. Existing scalar result types remain
unchanged for compatibility; newly added errorable scalar getters, including
torque and soft-body node radius, use shared `ScalarResult`.

- Eleven Release regressions pass: force field access, physics getters, axis
  setters, COM, origin offsets, force sources, shared sources, description
  creation, physics recovery, entity snapshots and runtime persistence.
- Both new suites pass ASan/UBSan with leak detection. Tests cover preservation,
  union modes, invalid/stale IDs, finite/range validation, copied values,
  allocation-free access, source lifetime and reference-frame response.
- Public-header consumers pass against installed Nix, sanitizer and generic
  Linux SDKs. The engine, suites and SDK consumers cross-build for Windows;
  native Windows execution is unverified.
- Player-controller and soft-body examples build independently against the
  installed Release SDK and run from `/tmp` with SDL dummy drivers. Both also
  cross-build against the installed Windows SDK.
- The car reaches its full 3,600-frame / 60-simulated-second endpoint:
  14,400 substeps, 7,222,164 candidates and 1,195,541 contacts, matching the
  existing baseline counts. Its existing `realtime_pass=0` remains visible;
  this is functional verification, not solver or performance acceptance.
- Final builds have no compiler warnings or sanitizer findings. The API
  inventory audit retains 77 pre-existing diagnostics, with none added.

Commands and complete logs are retained in `sandbox/milestone_19/goal_2/`.

## Goal 3 verification

The goal adds 35 setters and one anchor-entity getter, each with direct-engine
and facade entry points. The focused suite exercises both spellings, required
components, invalid/stale IDs, rejected edits, ownership/lifetime preservation,
automatic rest-value preservation after deformation and disconnection, anchor
transfer, particle geometry and inactive hitbox variants, and collision-filter
snapshot consistency. Ordinary edits are checked with allocation forbidden.

Fourteen Release regressions pass: `physics_fields`, `physics_getters`,
`physics_force_setters`, `physics_axis_setters`, `soft_body`, `soft_body_frame`,
`soft_body_mass`, `soft_body_areas`, `particle_collision_mode`, `center_of_mass`,
`origin_offsets`, `descriptor_creation`, `runtime_persistence` and
`shared_sources`. The new installed Release SDK consumer also passes.

The audit enabled assertions in the existing joint, soft-body frame and mass
tests, whose setup calls were otherwise removed by Release's `NDEBUG`. The
frame and mass regressions pass. The separate `joints` test still aborts at
`tests/joints.c:340`, an angular-velocity tolerance in its high-speed independent
motion scenario. Linking the same assertion-enabled test object against the
unchanged Goal 2 SDK (`8479729e1330760e890547868cb598bcb95e696b`)
reproduces the identical failure. Both runs are **incomplete**
at that assertion; neither establishes full-run numerical bounds. This existing
solver failure is preserved outside the additive field-access goal. It is not
reported as a passing regression or performance acceptance.

- The focused suite and installed SDK consumer pass ASan/UBSan with leak
  detection. No final compiler warnings or sanitizer findings remain.
- Public-only consumers build and run against installed Nix and generic Linux
  SDKs. The engine, suite and installed SDK consumer cross-build for Windows;
  native Windows execution remains unverified.
- Player-controller and soft-body examples independently build against the
  installed SDK, run from `/tmp` with SDL dummy drivers, and cross-build for
  Windows. The car completes 3,600 frames / 60 simulated seconds with 14,400
  substeps, 7,222,164 candidates and 1,195,541 contacts, matching the baseline.
  Its existing `realtime_pass=0` remains unmet and outside this API goal.
- All 72 new direct/facade declarations are classified in the API inventory.
  Its 77 pre-existing diagnostics remain unchanged.

Evidence, including both joint diagnostic logs, is in
`sandbox/milestone_19/goal_3/`.
