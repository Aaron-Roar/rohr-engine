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
Remaining partial setters belong to Goal 3 and await user instruction.
