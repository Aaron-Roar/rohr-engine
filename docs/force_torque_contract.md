# Unified force and torque contract

This is the approved contract for the **Unified Forces and Torques** milestone.
The shared description APIs, references, rigid-body/particle response,
beam-target loads, whole-soft-body loads, and editor authoring are implemented.
All targets use shared descriptions; obsolete whole-body vector/scalar helpers
are removed without compatibility aliases. Impulse APIs remain separate.
The authoring clarifications below define the implemented behavior. Runtime
soft-body persistence was introduced in state version 6; see the
[runtime topology contract](game_state.md#version-6-runtime-topology-contract).

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
    Entity owner;
    Entity target;
    ForceLocation location;
    ForceOrientation orientation;
    float magnitude;
} ForceDescription;

typedef struct TorqueDescription {
    Entity owner;
    Entity target;
    Torque magnitude;
} TorqueDescription;
```

`Entity` zero is the existing `ENTITY_INVALID` sentinel. In a description's
`target` field it deliberately means no target. `(ForceDescription){0}` and
`(TorqueDescription){0}` are valid sources, not incomplete configurations:
no target, zero magnitude, and (for forces) world origin and world orientation
zero. No description-default getter or separate create function is needed.

Goal 5 added `ENTITY_NONE` as an alias of `ENTITY_INVALID`, with the same
zero value. Use `ENTITY_NONE` to express an intentionally absent optional target;
retain `ENTITY_INVALID` for invalid-handle results and internal validity checks.
This adds no entity type or distinct sentinel and changes no validation rules.
`NONE` is accepted only where an API already permits an absent entity. Explicit
COM/origin/beam and entity-based orientation references still require suitable
entities; world references do not require an entity.

| Position reference | Offset meaning |
| --- | --- |
| World | Absolute world coordinates. `{0, 0}` is world origin. |
| COM | World-axis offset from the explicitly referenced entity's current COM. |
| Origin | Local offset following the referenced rigid or soft body's current origin and rotation. |
| Beam | `x` is along distance; `y` is perpendicular distance to the beam's left. Anchor and fraction establish the base point. |

Beam anchors map to fractions 0, 0.5, and 1. The stored fraction adds to the
anchor fraction; distance and fraction are additive, not mutually exclusive.
Fractions follow current length; distances retain their physical length.
Start/end follow stored endpoint order. External application points are allowed
without clamping. A sideways offset remains a fixed distance during stretching.
Soft-body origins follow a private tracking frame: its position is the equal-weight
average of directly owned nodes, and its incremental clockwise angle is the
best-fit rotation between successive centered node positions. The usable origin
retains its position and angle offsets from that frame, so origin-relative force
locations and body-relative directions follow node motion. This frame is not a
selectable entity and does not replace mass-weighted COM or physical inertia.

One node or collapsed/uncorrelated geometry follows translation while retaining
the previous angle; an empty body retains its pose. Adding, deleting, or
transferring nodes recalibrates tracking without moving the usable origin. Beam
connections alone never change frame membership. Explicit whole-body transforms
move nodes once and recalibrate the samples. Tracking observes node motion and
never changes nodes, velocities, or masses. It refreshes before source evaluation
and after integration and constraint corrections. Save/load and independent
spawns reconstruct the private samples without new serialized fields. Rotation
uses the shortest observed incremental turn; motion exceeding half a turn
between samples cannot be distinguished from a turn in the opposite direction.

World orientation zero points up. Other orientation references supply a rigid
or soft body's orientation, a beam's start-to-end direction, or the direction from one
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

Descriptions now include optional lifetime `owner`, independent of targets and
references. Generic entity ownership is authoritative. Full description setters
replace the owner atomically with source configuration; field setters preserve
it, and getters report its current value. A shared force/torque entity has one
owner for both components. One-tick `apply` requires `ENTITY_INVALID` ownership;
use `add` for persistent owned sources. Ownership changes preserve connections.

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
The result layout and section inventory are defined by the
[entity inspection contract](entity_inspection_contract.md).

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

For current positive node masses and finite positions, calculate total mass
`M`, mass-weighted COM `C`, and point-mass rotational inertia `I` using double
intermediates and coordinates relative to a node. With `r_i = position_i - C`:

```text
M = sum(m_i)
I = sum(m_i * |r_i|^2)
tau = requested pure torque, or moment(P - C, F)
node_force_i = (m_i / M) * F + (m_i * tau / I) * (r_i.y, -r_i.x)
```

Omit the turning term when `tau` is zero. Accept any positive inertia when
required node loads and simulated-node acceleration accumulators are finite
and representable; there is no minimum inertia cutoff. Single-node and
coincident-node bodies accept translation, but nonzero turning at zero inertia
suspends the entire contribution. Empty bodies, invalid or duplicate membership,
nonpositive/nonfinite masses, and nonfinite positions suspend nonzero loads.
Include fixed/held nodes in mass, COM, inertia, and assigned-load validation;
only simulated nodes receive acceleration changes. All contributions are
prepared before any are committed, without allocation or child sources.
Valid zero loads and held sources remain inactive after explicit reference
validation; unusable explicit references still suspend them. Recompute from
current membership, masses, and geometry every substep, including on retries.
A force referenced to its target's COM retains its COM-relative offset when
computing the moment, avoiding rounding through a world-space float COM.
COM references retain their existing ability to resolve zero-mass members when
total mass is positive; receiving a nonzero whole-body load requires every mass
to be positive. The tracked origin/orientation is a reference frame, not a rigid
constraint on the deforming nodes.

Beams remain two-node axial springs, with no added bending model. For every
resolved application point, use the following equivalent endpoint loads. Fraction, anchor, and distance offsets only locate the point; coincident
points with the same force and beam geometry produce identical loads.
Let `A`, `B` be endpoint positions, `L = |B - A|`, `t = (B - A)/L`, and
`n = (-t.y, t.x)` (left perpendicular in the engine's world coordinates).

```text
P     = resolved world application point
s     = dot(P - A, t)
h     = dot(P - A, n)
alpha = s / L

F     = signed magnitude * resolved orientation unit vector
F_parallel      = dot(F, t)
F_perpendicular = dot(F, n)

B_force = alpha * F_parallel * t
        + ((s * F_perpendicular - h * F_parallel) / L) * n
A_force = F - B_force
```

The axial component splits according to the resolved fraction `s/L`; the
perpendicular component and sideways offset reproduce the requested moment.
This is not a mass-based split. Endpoint mass determines acceleration afterward;
fixed/held endpoints keep their assigned shares without redistribution.

Use this formula for all position references, including the receiving beam,
world positions, other entities, and other beams. Recompute each substep without
clamping external points. Fractional and fixed-distance descriptions at the same
point have the same effect now, but their points may diverge during stretching.
References do not receive automatic reactions or constrain the receiving beam.

Work equivalence uses the receiving beam's instantaneous virtual attachment
`P = A + alpha*L*t + h*n`. Hold the resolved `alpha` and `h` constant during
virtual endpoint displacement. This does not assert work equivalence to the
motion of a fixed-distance reference during stretching, an unrelated entity,
or fixed world coordinates. Actual supplied power is the endpoint-load power.

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

The rigid-body and beam examples use the implemented API. Check every result
before using its value.

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
presentation for forces is now fixed: a constant screen-size arrow and application
dot, with magnitude edited numerically. Dot/shaft dragging updates the location
offset; the shared rotation handle updates orientation offset while preserving
references and magnitude sign. Zero arrows remain selectable with muted styling.
Editor eyes are independent of persisted app visibility. Runtime forces draw
signed arrows; runtime torques draw static 270-degree arrows at target COM or
beam midpoint. Zero sources are muted, and unresolved geometry is omitted.
Visibility never disables a source. Torque editor markers follow target COM or
beam midpoint plus an editor-only world-axis offset, falling back to the object
origin when the target cannot resolve. Changing targets preserves this offset.
Dragging changes only that offset; an app-preview click only selects the source.
The editor arc is 270 degrees at a fixed 44-pixel radius, with 4-pixel strokes
and 14-pixel arrowhead legs. Positive is clockwise, negative counterclockwise,
and zero is muted clockwise. Editor controls are cyan (yellow when selected);
force and torque app visuals default to red-orange while preserving explicit colors.

App markers and editor app previews share world-space filled geometry. Their
Visuals properties are fill/border color, border thickness, signed X/Y scale,
local X/Y offset, and clockwise rotation. Editing controls remain above the app
preview at their established screen size. The editor eye hides both; app
visibility hides the preview and runtime marker independently of editing controls.
Force offsets use the signed arrow's local axes; torque offsets use world axes.
Visual transforms never mutate source descriptions, targets, or physics.

Joints use the same visual style. Springs additionally have independently styled
Endpoint A/B markers, with the coil connecting their offset visual centers.
Endpoint scale/rotation affects each marker; assembly scale/rotation acts about
the physical midpoint, and assembly offset moves the complete graphic. Pin and
weld joints retain a single marker. Torque Properties exposes Visuals, Source,
and Editor Marker accordions, with Visible in App first below the header/name.
The editor eye, ordering, clipboard, multi-edit, history, navigation, and CLI
replay follow the shared source interactions. Project format 11 stores torque
editor visibility and marker offsets. Only the current project version is accepted. Editor metadata never enters runtime descriptions.

### Goal 5: authored definitions and persistence

Goal 5 implements these authored-model requirements through object-owned
collections, commands, the current project document format, and loadable/editable object
descriptions. New entries default to persistent, no target, and zero magnitude;
forces default to world-origin application and world-zero orientation.
Goal 6 enables runtime spawning and generated catalogs through the shared
object spawner, including named persistent sources and expiring initial sources.
Runtime source descriptions continue to serialize in game-state version 9.

**Ownership and reference scope.** Forces and torques are top-level items owned
by an object. Their receiving targets and every entity used by location or
orientation references must belong to that same object. This includes nodes,
beams, whole soft bodies, and both locations in a between-locations orientation.
World references and an explicit no-target selection remain available.
Developer C can connect runtime entities across objects through the existing
API; the authored restriction does not limit that API.

Persist stable authored identifiers and resolve them to the appropriate runtime
entities when an object is instantiated. Preserve these relationships through
renaming and reordering. Whole-object copies and duplicates must resolve their
internal references within the new object; they must not share the original's
mutable sources or bind accidentally to an unrelated matching identifier.

**Authored deletion.** Retain a source when one of its referenced elements is
deleted. Apply the following field-specific fallbacks in the same history
transaction as the deletion:

| Affected source field | Replacement |
| --- | --- |
| Receiving target | No target (`ENTITY_NONE`, the existing zero sentinel). |
| Force application location | World position with offset `{0, 0}`: world origin. |
| Force orientation | World orientation with offset `0`: world up. |

Reset the affected location or orientation as a complete value, discarding its
old entity reference and relative offsets. An entity reference lost inside a
between-locations orientation also resets that orientation to world zero.
If the deleted element appears in multiple fields, reset each affected field;
retain unaffected fields, signed magnitude, and initial/persistent mode.
Undo restores both the deleted element and the previous source descriptions;
redo repeats the same fallbacks. Deleting a containing soft body includes its
removed node/beam references. A world location is an application point, not a
receiving target; only the target's no-target option disables target contribution.

These are intentional editor/CLI document edits. They do not change the runtime
API's stale-handle validation or suspension behavior, and malformed serialized
references must not be silently repaired using deletion fallbacks.

**Persistence and prerequisite.** Project saving already describes soft-body
nodes and beam attachments. Runtime game-state saving/loading must also preserve
soft-body shape, topology, and physical state. Complete the separate single-goal
Runtime Soft-Body Persistence milestone before implementing Goal 5. Its scope
includes current node positions, masses, motion and fixed/held state; beam
endpoints, rest lengths, stiffness, damping and collision settings; body
ownership/origin; area loops and node references; and supported attachments.
Rest lengths must survive independently of current node positions so loading a
deformed body does not silently change its resting shape or spring response.

Save force/torque descriptions with stable serialized references and reconnect
them to the restored bodies, nodes, and beams. Old runtime entity handles are
not persistent identities. The prerequisite includes validation, failure
cleanup, round-trip state/reference checks, and continued-simulation checks.
Goal 5 adds authored source definitions, CLI operations, project serialization,
validation, history, copying, duplication and
deletion, including the agreed fallbacks and `ENTITY_NONE` alias. Preserve
zero-magnitude and no-target sources and initial versus persistent mode.
Existing engine-tick lifetime rules remain authoritative. Generated source
creation and instance cleanup are implemented in Goal 6; detailed editor presentation
remains Goals 7 and 8.

Goal 5 verification must cover object-local reference validation; source and
reference round trips; independent whole-object copies; target/location/
orientation deletion fallbacks, including nested orientation references;
undo/redo; zero/no-target and initial/persistent preservation; and alias parity
without weakening explicit-reference validation. Runtime save/load checks must
reconnect source descriptions to restored soft-body topology. Authored source
instantiation uses the same runtime source components and lifetime serialization.

### Goal 6: source instantiation and the car example

Create sources after instance topology and joint bindings are ready. Resolve
every authored reference within that instance, and roll back all newly created
entities if any step fails. Persistent sources belong to the containing object,
independently of their target. Initial sources have that same ownership and the
existing creation-tick-plus-one expiry; cancellation and normal expiry remove
the source completely. Public `apply()` still requires absent ownership.

Loaded descriptions, generated catalogs, and generated source-bearing constructor
wrappers share spawning behavior. Visual-only spawning excludes sources. World
references remain absolute; instance placement does not shift world application
points or headings. Runtime saves retain source ownership and remaining expiry,
and loading does not replay expired initial definitions.

The existing soft-body car example stores its geometry, joints, and rear-wheel
drive torque in an editable project. A/D updates the persistent rear-disk torque.
Each Space press applies an upward one-tick force at chassis COM only when both
wheels currently contact external rigid bodies. Disk, tire-node, and tire-beam
contacts qualify; self-contacts, standalone particles, and soft-body targets do
not. The wheels may contact different rigid bodies. An accepted press can wait
for the next physics update, but rejected airborne presses are never buffered.
The default project remains unchanged. Graphical force editing is implemented
in Goal 7 and torque editing in Goal 8.

### Milestone acceptance

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
- Coincident fractional, distance, combined-offset, and world points yield equal
  endpoint loads and motion; after stretching, compare each newly resolved point
  with its equivalent world-point description.
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
