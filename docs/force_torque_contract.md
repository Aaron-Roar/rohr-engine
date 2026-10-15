# Unified force and torque contract

This is the approved contract for the **Unified Forces and Torques** milestone.
The shared description APIs, references, and rigid-body/particle response are
implemented in Goal 2. Receiving beam loads, whole-soft-body distribution, and
editor authoring remain in their subsequent [roadmap](../NEXT_STEPS.md) goals.
Whole-soft-body vector/scalar helpers remain until Goal 4; nodes use the shared
API. A shared source targeting a beam or whole soft body currently suspends with
an unsupported target contribution warning.

## Types and defaults

Positions and orientations each contain an offset and an independent reference.
The reference determines the offset's meaning. World is zero/default for both,
including inside a force description. There is no implicit target-COM override.
Anonymous unions expose `.reference.entity`, `.reference.beam`, and
`.reference.between` without a `.data` member. The tag selects the valid member;
these are plain copied values, with no owned pointers or allocation.
Anonymous unions require standard C11. The engine exports this language
requirement to SDK consumers, and generated project scaffolds select C11.

```c
typedef enum ForcePositionReferenceKind {
    ROHR_FORCE_WORLD_POSITION = 0,
    ROHR_FORCE_COM_POSITION,
    ROHR_FORCE_ORIGIN_POSITION,
    ROHR_FORCE_BEAM_POSITION
} ForcePositionReferenceKind;

typedef enum ForceBeamAnchor {
    ROHR_FORCE_BEAM_START = 0,
    ROHR_FORCE_BEAM_CENTER,
    ROHR_FORCE_BEAM_END
} ForceBeamAnchor;

typedef struct ForcePositionReference {
    ForcePositionReferenceKind kind;
    union {
        Entity entity; /* COM or origin. */
        struct {
            Entity entity;
            ForceBeamAnchor anchor;
            float fraction;
        } beam;
    };
} ForcePositionReference;

typedef struct ForceLocation {
    Vec2D offset;
    ForcePositionReference reference;
} ForceLocation;

typedef enum ForceOrientationReferenceKind {
    ROHR_FORCE_WORLD_ORIENTATION = 0,
    ROHR_FORCE_BODY_ORIENTATION,
    ROHR_FORCE_BEAM_ORIENTATION,
    ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION
} ForceOrientationReferenceKind;

typedef struct ForceOrientationReference {
    ForceOrientationReferenceKind kind;
    union {
        Entity entity; /* Body or beam. */
        struct {
            ForceLocation from;
            ForceLocation to;
        } between;
    };
} ForceOrientationReference;

typedef struct ForceOrientation {
    Orientation offset;
    ForceOrientationReference reference;
} ForceOrientation;

typedef struct ForceDescription {
    Entity target;
    ForceLocation location;
    ForceOrientation orientation;
    float magnitude;
} ForceDescription;

typedef struct TorqueDescription {
    Entity target;
    Torque magnitude;
} TorqueDescription;
```

`Entity` zero is the existing `ENTITY_INVALID` sentinel. In a description's
`target` field it deliberately means no target. `(ForceDescription){0}` and
`(TorqueDescription){0}` are valid sources, not incomplete configurations:
no target, zero magnitude, and (for forces) world origin and world orientation
zero. No description-default getter or separate create function is needed.

| Position reference | Offset meaning |
| --- | --- |
| World | Absolute world coordinates. `{0, 0}` is world origin. |
| COM | World-axis offset from the explicitly referenced entity's current COM. |
| Origin | Local offset following the referenced entity's origin transform; rigid-body offsets follow rotation. |
| Beam | `x` is along distance; `y` is perpendicular distance to the beam's left. Anchor and fraction establish the base point. |

Beam anchors map to fractions 0, 0.5, and 1. The stored fraction adds to the
anchor fraction; distance and fraction are additive, not mutually exclusive.
Fractions follow current length; distances retain their physical length.
Start/end follow stored endpoint order. External application points are allowed
without clamping. A sideways offset remains a fixed distance during stretching.
Soft-body origin/orientation remains an authoring transform, not an inferred
physical pose; do not infer orientation-following from a deforming soft body.

World orientation zero points up. Other orientation references supply a rigid
body's orientation, a beam's start-to-end direction, or the direction from one
resolved location to another. The orientation offset is clockwise degrees from
that base, following the existing [angle convention](angle_convention_contract.md).
Ordinary vector-to-angle construction uses existing math helpers; no separate
fixed-vector direction variant is required. Coincident between-locations points
cannot supply a direction and are temporarily unresolvable.

Magnitude is signed. Negative force magnitude reverses the resolved direction;
negative torque is counterclockwise and positive torque clockwise. Preserve
signed values in descriptions/getters rather than rewriting orientation.
Physical torque units are unchanged; angles remain degrees. Torque marker
position is editor-only data, not a physical pivot or a runtime torque field.

## API and ownership

All functions below have direct equivalents with the same signature and no
`rohr_` prefix. Result types use the existing `ERROR_DECLARE_RESULT_TYPE` pattern:
`ForceDescriptionResult`, `TorqueDescriptionResult`, `ForceLocationResult`,
`ForceOrientationResult`, and `ForceMagnitudeResult` wrap their respective
values (the latter wraps `float`). Reuse existing `EntityResult`, `TorqueResult`,
and `EngineResult`. All getters are errorable except that no new default-value
getter is necessary.

```c
EntityResult rohr_physics_force_add(ForceDescription description);
EntityResult rohr_physics_torque_add(TorqueDescription description);

ForceDescriptionResult rohr_physics_force_description_get(Entity source);
TorqueDescriptionResult rohr_physics_torque_description_get(Entity source);
EngineResult rohr_physics_force_description_set(Entity source, ForceDescription description);
EngineResult rohr_physics_torque_description_set(Entity source, TorqueDescription description);

EngineResult rohr_physics_force_target_set(Entity source, Entity target);
EntityResult rohr_physics_force_target_get(Entity source);
EngineResult rohr_physics_force_location_set(Entity source, ForceLocation location);
ForceLocationResult rohr_physics_force_location_get(Entity source);
EngineResult rohr_physics_force_orientation_set(Entity source, ForceOrientation orientation);
ForceOrientationResult rohr_physics_force_orientation_get(Entity source);
EngineResult rohr_physics_force_magnitude_set(Entity source, float magnitude);
ForceMagnitudeResult rohr_physics_force_magnitude_get(Entity source);

EngineResult rohr_physics_torque_target_set(Entity source, Entity target);
EntityResult rohr_physics_torque_target_get(Entity source);
EngineResult rohr_physics_torque_magnitude_set(Entity source, Torque magnitude);
TorqueResult rohr_physics_torque_magnitude_get(Entity source);

EngineResult rohr_physics_force_apply(ForceDescription description);
EngineResult rohr_physics_torque_apply(TorqueDescription description);
```

`add()` immediately creates an ECS source and copies configuration. A description
can be stack-local, reused, or discarded afterward; editing it never edits the
source. Referenced entities remain handles, not cloned entities. Zero and fully
populated descriptions use the same workflow. No nullable pointer overload,
separate `create()`, or `from_description_add()` remains.

Field setters and atomic whole-description replacement share validation with
creation. They update the source rather than accumulating another source.
Deleting through `rohr_entity_delete(source)` stops future contributions without
undoing motion or deleting referenced entities. Each persistent source owns one
entity ID; distributed loads never create child force/torque entities.

## Validation, evaluation, and lifetime

- Invalid tags, malformed numeric values (including NaN/infinity), and stale
  handles supplied to an API return immediate errors. Failed creation leaves no
  source; failed setters leave existing configuration unchanged. Validate the
  selected union member, not unused storage. Explicit reference kinds require
  a suitable entity; target zero alone is the valid no-target case.
- Reference entities need only supply the required position/orientation data.
  They may be static or ineligible to receive forces themselves. References
  never imply reaction forces; those belong to developer code or collisions.
- Temporarily unusable geometry is accepted as configuration, then diagnosed
  during evaluation, including for one-tick applications. A deleted reference
  suspends the affected contribution until repaired. Reused entity slots must
  not silently retarget a source.
- Resolve explicit references even at zero magnitude or with no target. A
  collapsed beam reference or unusable direction still warns. Valid no-target
  sources produce no target contribution and no warning; valid zero loads are
  inactive. Changing a reference to world, removing the source's force/torque
  component, or deleting the source is an explicit developer choice.
- Retry suspended contributions each scheduled substep. Do not invent a
  direction, clamp a force couple, or silently retain a resolved point from a
  previous frame. Reject the entire source contribution for that substep when
  either endpoint result or another coupled contribution is invalid. Other
  sources and successful earlier work remain intact.
- One-tick `apply()` accepts valid no-target configurations as successful no-ops,
  while still validating explicit references. Malformed inputs fail immediately;
  runtime geometry failures belong in the physics report.
- Preserve existing engine-tick lifetime: a one-tick source created at engine
  tick `T` expires at `T + 1` through engine lifecycle cleanup. Physics updates
  alone do not advance expiry. It participates in eligible evaluations while
  alive; references resolve every substep. Failed evaluations do not extend
  lifetime. Do not change tick catch-up or focus-loss behavior in this milestone.

Physics reports aggregate contributing, inactive, and suspended source
**evaluations per substep**, not unique source counts. Normal zero/no-target
behavior is not a warning; unresolvable explicit references are. Use the shared
report aggregation, bounded copied evidence, overflow, and first-fatal rules.
Record source/target context even if a one-tick source later expires. Do not add
another error queue or force-specific public status getters.

Current configuration/eligibility inspection belongs to the separate entity-info
API. It must not apply forces or change reports. The agreed direction is one
errorable entity getter, section selectors combined with `|`, and
`ROHR_ENTITY_INFO` for all sections, retaining individual component details.
The exact result layout and section inventory remain in the
[Entity Inspection milestone](../NEXT_STEPS.md#milestone-entity-inspection).

## Target behavior and beam calculation

Rigid-body forces apply at the resolved world point and derive torque about
current COM; a force through COM has no derived torque. Rigid bodies containing
particles retain angular response. Standalone particles and soft nodes ignore
application position for physical response and treat torque as a successful
no-op, while still checking explicitly configured references.

Whole-soft-body sources distribute translation and turning across nodes using
current masses and geometry every substep. A force through COM gives uniform
linear acceleration; off-center force also turns; pure torque has zero net
linear force. Preserve the requested net force/torque without imposing rigid
transforms or overwriting velocities. Geometry unable to represent a requested
turning contribution suspends the whole source contribution, not merely its
turning portion. Fixed/held nodes keep their assigned share; their constraints
absorb it rather than redistributing it to movable nodes.

Beams remain two-node axial springs, with no added bending model. For a location
attached to the receiving beam, use the following virtual-work-consistent loads.
Let `A`, `B` be endpoint positions, `L = |B - A|`, `t = (B - A)/L`, and
`n = (-t.y, t.x)` (left perpendicular in the engine's world coordinates).

```text
alpha = anchor_fraction + location.reference.beam.fraction
d     = location.offset.x
h     = location.offset.y
s     = alpha * L + d
P     = A + s*t + h*n

F     = signed magnitude * resolved orientation unit vector
F_parallel      = dot(F, t)
F_perpendicular = dot(F, n)

B_force = alpha * F_parallel * t
        + ((s * F_perpendicular - h * F_parallel) / L) * n
A_force = F - B_force
```

Thus a fractional centerline attachment divides the full vector by its
fraction, while a fixed-distance attachment places its axial component at the
anchor according to `alpha`. This distinction follows attachment motion during
stretching. It is not a mass-based endpoint split. Sideways offsets introduce
opposing forces accounting for torque. Endpoint mass determines acceleration
after loading; anchored endpoints retain their assigned reactions.

For an application reference not attached to the receiving beam (world, another
entity, or another beam), resolve its world point `P`, project onto the receiving
beam, and use `s = dot(P-A,t)`, `h = dot(P-A,n)`, `alpha = s/L`, `d = 0` in the
same force formula. Recompute each substep. This is equivalent loading at the
current point; it does not constrain that point to the receiving beam or transfer
work/reaction to the reference entity. Attachment-work equivalence is measured
against the receiving beam's virtual attachment, not the motion of an unrelated
reference or the zero motion of fixed world coordinates.

For signed clockwise torque `tau` alone:

```text
B_force = -(tau / L) * n
A_force = -B_force
```

Check moments using the clockwise convention:
`moment(r, F) = r.y*F.x - r.x*F.y`. The endpoint loads must reproduce the requested
force and moment about any common origin. A negative magnitude reverses every
contribution, including derived turning.

Define the top-level compile-time constant
`ROHR_PHYSICS_BEAM_MIN_LENGTH` with default `0.0001f` world units. Lengths at or
below it cannot resolve beam load/reference geometry: suspend and warn, even for
zero loads with explicit beam references. No runtime tolerance setter/getter is
introduced. Check derived numeric results too; a length above the threshold does
not make overflow acceptable. Resume on a later valid evaluation.

## Usage examples

The rigid-body examples use the implemented API; beam-target loading remains
Goal 3. Check every result before using its value.

```c
/* Valid no-target source; setters can configure it later. */
EntityResult empty = rohr_physics_force_add((ForceDescription){0});
if(rohr_error_check(empty)) return;
Entity source = empty.result.value;
EngineResult result = rohr_physics_force_target_set(source, body_a);
if(rohr_error_check(result)) return;
/* Its location is still world origin; selecting a target does not select COM. */

/* Force at A's COM, direction follows B's orientation. */
ForceDescription description = {
    .target = body_a,
    .location = {
        .reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = body_a}
    },
    .orientation = {
        .offset = 0.0f,
        .reference = {.kind = ROHR_FORCE_BODY_ORIENTATION, .entity = body_b}
    },
    .magnitude = 100.0f
};
EntityResult added = rohr_physics_force_add(description);
if(rohr_error_check(added)) return;
/* Changing this local value does not change added.result.value. */
description.magnitude = -100.0f;
result = rohr_physics_force_apply(description);
if(rohr_error_check(result)) return;

/* 25% along the beam plus 10 along and 5 left; globally downward. */
description = (ForceDescription){
    .target = beam,
    .location = {
        .offset = {10.0f, 5.0f},
        .reference = {
            .kind = ROHR_FORCE_BEAM_POSITION,
            .beam = {.entity = beam, .anchor = ROHR_FORCE_BEAM_START,
                     .fraction = 0.25f}
        }
    },
    .orientation = {.offset = 180.0f},
    .magnitude = 100.0f
};
result = rohr_physics_force_description_set(source, description);
if(rohr_error_check(result)) return;

/* Follow the beam instead, 90 degrees clockwise from start-to-end. */
ForceOrientation orientation = {
    .offset = 90.0f,
    .reference = {.kind = ROHR_FORCE_BEAM_ORIENTATION, .entity = beam}
};
result = rohr_physics_force_orientation_set(source, orientation);
if(rohr_error_check(result)) return;
result = rohr_physics_torque_apply((TorqueDescription){
    .target = body_a, .magnitude = -20.0f
});
if(rohr_error_check(result)) return;
```

## Migration, authoring, and acceptance

Replace redundant vector/scalar source signatures and soft-body force/torque
helpers when their shared equivalents are implemented; provide no compatibility
aliases. Keep impulse APIs separate. No backward compatibility for saved project
or runtime formats is required: remake examples and regenerate generated code.
Migrate documentation and direct/public callers with each relevant goal.

Editor authoring supports persistent and initial one-tick sources as top-level
object items, including zero sources later changed by developer C. Forces use an
application dot and arrow showing the actual signed direction; torque uses a
movable 270-degree marker whose position is visual only. Preserve selection,
visibility, ordering, clipboard, multi-edit, history, persistence, and independent
generated instances. No delayed event system is introduced. Exact handle
presentation remains with the editor goals.

Implementation acceptance includes:

- Zero initialization, no-target add/apply, later configuration, signed round
  trips, atomic replacement, copied descriptions, ownership, stale handles and
  slot reuse, malformed fields, and direct/public parity.
- World/origin/COM/beam positions, independent orientation references, moving
  bodies, global versus following direction, between-location coincidence,
  negative magnitudes, and reference validation at zero/no target.
- One-tick versus persistent parity, repeated application, substep resolution,
  engine lifecycle expiry, manual physics updates without expiry advancement,
  missing-reference repair, unchanged expiry on failure, and report counters.
- Rigid COM and offset response, particle/node torque no-ops, particle-containing
  rigid bodies, whole-soft-body COM acceleration and torque, changing masses and
  geometry, fixed nodes, and no child source entities.
- Beam fractional/distance/combined offsets, every anchor, sideways and external
  points, arbitrary angles, pure torque, unequal masses, fixed endpoints,
  threshold boundary/near-collapse recovery, and coupled numeric rejection.
- Analytic net-force and clockwise-moment checks plus finite-difference checks
  of endpoint work against attachment displacement. Motion tests cover free and
  constrained nodes and stretching. Energy tests compare external work with
  kinetic/elastic energy changes, accounting for damping, constraint reactions,
  and timestep error; force application does not imply constant energy. Verify
  convergence with smaller timesteps rather than exact discrete conservation.
- Applicable sanitizer and Linux/Windows builds, installed-SDK consumers,
  standalone/generated applications, serialization and independent instances,
  plus editor visual/manual review in the editor goals.

Entity inspection captures copied descriptions and current eligibility in its
force/torque sections. No force-specific status getter is added.

### Constructing a force from an X/Y vector

The description stores angle and signed magnitude. For a vector `v`, use
`hypotf(v.x, v.y)` for magnitude and
`rohr_math_radians_to_degrees(atan2f(v.x, v.y))` for the world orientation offset.
Choose offset zero when both vector components are zero. These standard math
operations require `<math.h>`; there is no separate vector overload. Select the
location explicitly, including a COM reference when that is the intended point.
