# Entity inspection contract

This is the implemented contract for the **Entity Inspection** milestone. Public
APIs are declared in [rohr.h](../include/rohr.h); types and direct APIs are in
[entity_info.h](../include/entity_info.h). The
[roadmap](../NEXT_STEPS.md#milestone-entity-inspection) records verification. New force/torque sources join this
API later under the [force/torque contract](force_torque_contract.md).

## Purpose and boundaries

Inspection captures immutable current-state information for one entity between
updates. It does not execute simulation, advance animation, repair components,
mutate caches or reports, or replace component getters. Reports describe executed
work; inspection describes captured configuration, values, and independently
observable eligibility issues. There is no combined entity-health value.

Calls to the inspection API must be serialized. Concurrent ECS mutation during
capture is unsupported. Capture checks direct
references but does not recursively snapshot referenced entities. Overlaps and
contacts copy the currently stored interaction results without recomputing them.
Resource identifiers are evidence at capture time, not guarantees of future
resource validity. GPU pixels and global animation asset libraries are excluded.

## API and ownership

```c
typedef uint64_t EntityInfoFlags;
typedef uint64_t EntityInfoHandle;

EntityInfoResult rohr_entity_info_get(Entity entity, EntityInfoFlags flags);
EntityInfoResult rohr_entity_info_retain(const EntityInfo *info);
EngineResult rohr_entity_info_release(EntityInfo *info);
bool rohr_entity_info_check(const EntityInfo *info, EntityInfoFlags flags);

EntityInfoIssueCountResult rohr_entity_info_issues_count_get(
    const EntityInfo *info, EntityInfoFlags flags);
EntityInfoIssueReadResult rohr_entity_info_issues_get(
    const EntityInfo *info, EntityInfoFlags flags,
    EntityInfoIssue *destination, size_t capacity);
```

Direct APIs have identical signatures without `rohr_`. Result wrappers use the
existing `ERROR_DECLARE_RESULT_TYPE` convention: `EntityInfoResult` contains
`EntityInfo`, `EntityInfoIssueCountResult` contains `size_t`, and
`EntityInfoIssueReadResult` contains `EntityInfoIssueRead`.

`get` explicitly allocates snapshot storage and returns its first ownership
reference. Scalars are returned by value; arrays and issues belong to the frozen
backing storage. `retain` returns another owning value sharing the same captured
storage, without reading the ECS again. Ordinary C assignment does not retain.
Release each owning reference exactly once; aliases are not additional owners.
Losing the last owning value without releasing it leaks storage.

`release` decrements ownership, clears the supplied value, and frees storage on
the final release. Releasing an already-cleared value succeeds as a no-op;
passing NULL is an error. Snapshots do not expire on updates, entity deletion,
or engine shutdown; their storage must remain releasable afterward. Resource
handles inside them do not retain textures, animations, or other live resources.

Use a validated opaque handle, with zero invalid and stale-handle reuse detection.
The backing registry/allocation layout is an implementation detail. Public values
and `const` array contents must be treated as immutable; changing a scalar does
not change the ECS, and callers must not modify ownership metadata or cast away
const. Arbitrary invalid C pointers and double-release through copied aliases
are caller errors, not supported ownership workflows.

## Selectors and flat result

`EntityInfo` starts with `Entity entity`, `EntityInfoFlags requested`,
`EntityInfoFlags available`, and `EntityInfoHandle handle`, followed by the
fields in the table below. All unrequested or unavailable fields are zeroed.
`components` is a stored ECS component mask, distinct from inspection availability.
Use `mass_value` because the existing public `mass` macro prevents a field named
`mass` from being used safely. The existing `children` pool macro also collides
with the approved `info.children[i]` spelling. That macro has been retired and its uses migrated to `children_pool.objects`.
This is a breaking change without a compatibility alias.

Each selector is `ROHR_ENTITY_INFO_` plus the suffix below, defined with
`UINT64_C(1) << bit` (not a narrow C enum). Combine selectors with `|`.
`ROHR_ENTITY_INFO_CORE`, `ROHR_ENTITY_INFO_PHYSICS`, and
`ROHR_ENTITY_INFO_GRAPHICS` combine their respective individual selectors;
`ROHR_ENTITY_INFO` combines all 61 implemented selectors. Bits 61–63 are
unsupported until explicitly assigned; future source fields are not included now.

| Bit | Selector suffix | Field type | Field name |
| --- | --- | --- | --- |
| 0 | `CORE_NAME` | `EntityName` | `name` |
| 1 | `CORE_COMPONENTS` | `RohrComponentMask` | `components` |
| 2 | `CORE_LIFETIME` | `LifeTime` | `lifetime` |
| 3 | `CORE_PARENT` | `Parent` | `parent` |
| 4 | `CORE_TARGET` | `Entity` | `target` |
| 5 | `CORE_CHILDREN` | `const Entity *` | `children` |
| 6 | `CORE_GROUPS` | `const GroupId *` | `groups` |
| 7 | `PHYSICS_POSITION` | `Position` | `position` |
| 8 | `PHYSICS_ORIENTATION` | `Orientation` | `orientation` |
| 9 | `PHYSICS_VELOCITY` | `Velocity` | `velocity` |
| 10 | `PHYSICS_ACCELERATION` | `Acceleration` | `acceleration` |
| 11 | `PHYSICS_ANGULAR_VELOCITY` | `AngularVelocity` | `angular_velocity` |
| 12 | `PHYSICS_ANGULAR_ACCELERATION` | `AngularAcceleration` | `angular_acceleration` |
| 13 | `PHYSICS_ANGULAR_VELOCITY_MAXIMUM` | `AngularVelocity` | `angular_velocity_maximum` |
| 14 | `PHYSICS_MASS` | `Mass` | `mass_value` |
| 15 | `PHYSICS_COM_LOCAL` | `Position` | `center_of_mass_local` |
| 16 | `PHYSICS_COM_WORLD` | `Position` | `center_of_mass_world` |
| 17 | `PHYSICS_INERTIA` | `float` | `moment_of_inertia` |
| 18 | `PHYSICS_FRICTION` | `Friction` | `friction` |
| 19 | `PHYSICS_RESTITUTION` | `Restitution` | `restitution` |
| 20 | `PHYSICS_BODY_MODE` | `EntityInfoBodyMode` | `body_mode` |
| 21 | `PHYSICS_PARTICLE` | `ParticleGeometry` | `particle` |
| 22 | `PHYSICS_COLLISION_FILTER` | `CollisionFilterConfig` | `collision_filter` |
| 23 | `PHYSICS_HITBOX_ACTIVE` | `EntityInfoActiveHitbox` | `hitbox_active` |
| 24 | `PHYSICS_HITBOX_LOCAL` | `Shape` | `hitbox_local` |
| 25 | `PHYSICS_HITBOX_WORLD` | `Shape` | `hitbox_world` |
| 26 | `PHYSICS_HITBOXES` | `const HitboxVariant *` | `hitboxes` |
| 27 | `PHYSICS_HITBOX_BINDINGS` | `const HitboxAnimationBinding *` | `hitbox_bindings` |
| 28 | `PHYSICS_AXIS_LOCK` | `AxisLock` | `axis_lock` |
| 29 | `PHYSICS_ANGLE_LOCK` | `AngleLock` | `angle_lock` |
| 30 | `PHYSICS_TRANSFORM_LOCK` | `TransformLock` | `transform_lock` |
| 31 | `PHYSICS_JOINT` | `Joint` | `joint` |
| 32 | `PHYSICS_JOINT_ANCHORS` | `const EntityInfoJointAnchor *` | `joint_anchors` |
| 33 | `PHYSICS_JOINT_ANCHORS_WORLD` | `const EntityInfoJointAnchorWorld *` | `joint_anchors_world` |
| 34 | `PHYSICS_SOFT_BODY` | `SoftBody` | `soft_body` |
| 35 | `PHYSICS_SOFT_BODY_NODE` | `SoftBodyNode` | `soft_body_node` |
| 36 | `PHYSICS_SOFT_BODY_BEAM` | `SoftBodyBeam` | `soft_body_beam` |
| 37 | `PHYSICS_SOFT_BODY_AREA` | `SoftBodyArea` | `soft_body_area` |
| 38 | `PHYSICS_OVERLAPS` | `const EntityInteraction *` | `overlaps` |
| 39 | `PHYSICS_CONTACTS` | `const EntityContact *` | `contacts` |
| 40 | `PHYSICS_FORCE` | `Force` | `force` |
| 41 | `PHYSICS_TORQUE` | `Torque` | `torque` |
| 42 | `GRAPHICS_SPRITE` | `Sprite` | `sprite` |
| 43 | `GRAPHICS_ANIMATIONS` | `const AnimatedSprite *` | `animations` |
| 44 | `GRAPHICS_ENTITY_LAYER` | `EntityInfoLayer` | `entity_layer` |
| 45 | `GRAPHICS_SPRITE_LAYER` | `EntityInfoLayer` | `sprite_layer` |
| 46 | `GRAPHICS_ANIMATION_LAYER` | `EntityInfoLayer` | `animation_layer` |
| 47 | `CORE_CHILD_COUNT` | `size_t` | `child_count` |
| 48 | `CORE_GROUP_COUNT` | `size_t` | `group_count` |
| 49 | `PHYSICS_HITBOX_COUNT` | `size_t` | `hitbox_count` |
| 50 | `PHYSICS_HITBOX_BINDING_COUNT` | `size_t` | `hitbox_binding_count` |
| 51 | `PHYSICS_JOINT_ANCHOR_COUNT` | `size_t` | `joint_anchor_count` |
| 52 | `PHYSICS_OVERLAP_COUNT` | `size_t` | `overlap_count` |
| 53 | `PHYSICS_CONTACT_COUNT` | `size_t` | `contact_count` |
| 54 | `PHYSICS_SOFT_BODY_NODE_COUNT` | `size_t` | `soft_body_node_count` |
| 55 | `PHYSICS_SOFT_BODY_BEAM_COUNT` | `size_t` | `soft_body_beam_count` |
| 56 | `PHYSICS_SOFT_BODY_AREA_COUNT` | `size_t` | `soft_body_area_count` |
| 57 | `GRAPHICS_ANIMATION_COUNT` | `size_t` | `animation_count` |
| 58 | `GRAPHICS_SPRITE_VISIBILITY` | `bool` | `sprite_visible` |
| 59 | `GRAPHICS_SPRITE_SCALE` | `Scale` | `sprite_scale` |
| 60 | `GRAPHICS_PRESENCE` | `bool` | `has_sprite, has_animations, is_animated` |

All collections use direct array pointers and separate `size_t` counts, never
`.values` wrappers. An empty captured array has a NULL pointer and count zero.
The snapshot owns the arrays; pointers remain valid while an owner retains it.

```c
/* Members of EntityInfo, illustrating the collection layout. */
const AnimatedSprite *animations;
size_t animation_count;
const HitboxVariant *hitboxes;
size_t hitbox_count;
const Entity *children;
size_t child_count;
```

A collection request also captures and marks its count selector available when
the count is obtained. `requested` preserves exactly the caller's flags;
`available` may therefore include these implied count bits. Count-only requests
do not copy collection contents. Local and world joint anchors share
`joint_anchor_count`; their arrays preserve the same source ordering. Soft-body
counts correspond to the copied descriptor's nodes, beams, and areas. Requesting
`PHYSICS_SOFT_BODY` also captures those three count selectors. No new dynamic
wrapper is added around the fixed arrays already inside `SoftBody` or shapes.

Use stored collection order; copy only occupied elements, not unused capacity.
If an element cannot be resolved, leave the entire requested resolved collection
unavailable rather than expose zero entries as valid values. Preserve any valid
count and independently captured local/configuration collection, and record each
independently detectable element issue with its source index.

```c
typedef struct EntityInfoBodyMode {
    bool is_static;
    bool is_dynamic;
    bool is_held;
    bool is_kinematic;
    bool has_gravity;
    bool has_collision;
} EntityInfoBodyMode;

typedef struct EntityInfoActiveHitbox {
    HitboxId id;
    size_t index;
} EntityInfoActiveHitbox;

typedef struct EntityInfoJointAnchor {
    JointAnchorId id;
    JointAnchor anchor;
} EntityInfoJointAnchor;

typedef struct EntityInfoJointAnchorWorld {
    JointAnchorId id;
    Position position;
} EntityInfoJointAnchorWorld;

typedef struct EntityInfoLayer {
    GraphicsLayerId id;
    int value;
} EntityInfoLayer;
```

Body-mode booleans reflect the corresponding component bits independently.
Computed COM, inertia, world shapes, and world anchor positions use the same
geometry and coordinate conventions as existing physics getters, without
publishing caches. Collision filtering reflects the effective configuration,
including existing defaults. Motion values use existing stored motion state;
inspection does not infer velocity from position history. Missing prerequisites
make a derived value unavailable, with issues explaining the dependency.

The current engine stores one `AnimatedSprite` per entity, so `animation_count`
is zero or one. `AnimatedSpriteSet` is not an entity component.
`animations` copies attached `AnimatedSprite` entries, including playback state,
frame values, visibility, scale, and transforms. It is not a list of all animation
assets. Mismatched sprite/player frame counts retain the captured configuration
and produce an invalid-value warning, matching the animation pipeline's rejection
of that configuration. `sprite_visible` and `sprite_scale` describe the static
sprite only; animation equivalents are accessed through `animations[i]`. The full sprite
request also supplies the visibility and scale selectors. Layer records capture
the existing entity/sprite/animation layer binding and resolved value; resolution
failure makes that layer section unavailable and preserves the ID in its issue.

`GRAPHICS_PRESENCE` supplies all three booleans: `has_sprite` means the static
sprite component exists; `has_animations` means at least one attached animation
entry exists; `is_animated` means the animated-sprite component exists. It does not
mean playback is advancing or visible. Presence and body-mode queries on a valid
entity succeed even when every boolean is false.

## Availability and issues

`check(info, flags)` returns true exactly when all supplied flags are available.
NULL, a cleared or expired handle, zero flags, or unsupported bits return false.
Availability means the value was obtained, not that an operation will succeed.
A captured configuration may remain available while its references prevent an
operation. Normal zero, false, and empty values are available, not errors.
Collections absent because the entity has no entries yield available empty
collections/counts; a requested absent component value instead yields an
informational issue and is unavailable. Soft-body topology counts require the
soft-body component; they are unavailable when it is absent.

```c
#define ROHR_ENTITY_INFO_DETAIL_CAPACITY 256

typedef enum EntityInfoIssueKind {
    ROHR_ENTITY_INFO_INFORMATION,
    ROHR_ENTITY_INFO_WARNING
} EntityInfoIssueKind;

typedef enum EntityInfoIssueReason {
    ROHR_ENTITY_INFO_COMPONENT_ABSENT,
    ROHR_ENTITY_INFO_REFERENCE_MISSING,
    ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING,
    ROHR_ENTITY_INFO_VALUE_INVALID,
    ROHR_ENTITY_INFO_GEOMETRY_UNRESOLVABLE,
    ROHR_ENTITY_INFO_CALCULATION_FAILED
} EntityInfoIssueReason;

typedef enum EntityInfoObjectKind {
    ROHR_ENTITY_INFO_OBJECT_NONE,
    ROHR_ENTITY_INFO_OBJECT_ENTITY,
    ROHR_ENTITY_INFO_OBJECT_GROUP,
    ROHR_ENTITY_INFO_OBJECT_HITBOX,
    ROHR_ENTITY_INFO_OBJECT_JOINT_ANCHOR,
    ROHR_ENTITY_INFO_OBJECT_TEXTURE,
    ROHR_ENTITY_INFO_OBJECT_ANIMATION,
    ROHR_ENTITY_INFO_OBJECT_GRAPHICS_LAYER
} EntityInfoObjectKind;

typedef enum EntityInfoField {
    ROHR_ENTITY_INFO_FIELD_VALUE,
    ROHR_ENTITY_INFO_FIELD_TARGET,
    ROHR_ENTITY_INFO_FIELD_APPLICATION_REFERENCE,
    ROHR_ENTITY_INFO_FIELD_ORIENTATION_REFERENCE,
    ROHR_ENTITY_INFO_FIELD_BEAM_START,
    ROHR_ENTITY_INFO_FIELD_BEAM_END,
    ROHR_ENTITY_INFO_FIELD_ANCHOR,
    ROHR_ENTITY_INFO_FIELD_SHAPE,
    ROHR_ENTITY_INFO_FIELD_ANIMATION_FRAME,
    ROHR_ENTITY_INFO_FIELD_LAYER_REFERENCE
} EntityInfoField;

typedef struct EntityInfoObjectRef {
    EntityInfoObjectKind kind;
    uint64_t id;
    Entity owner;
} EntityInfoObjectRef;

typedef struct EntityInfoIssue {
    EntityInfoFlags section;
    EntityInfoField field;
    EntityInfoIssueKind kind;
    EntityInfoIssueReason reason;
    EntityInfoObjectRef reference;
    EntityInfoObjectRef related_reference;
    bool has_index;
    size_t index;
    char detail[ROHR_ENTITY_INFO_DETAIL_CAPACITY];
    bool detail_truncated;
} EntityInfoIssue;

typedef struct EntityInfoIssueRead {
    size_t written;
    size_t total;
} EntityInfoIssueRead;
```

Each issue names one individual section, one independently detectable problem,
and its field/dependency. Multiple issues may share a section. `owner` identifies
the entity scope of local IDs such as hitboxes; unused references use NONE and
zero IDs. `index` is the original collection index when `has_index` is true.
Messages are NUL-terminated; `detail_truncated` exposes text truncation. Copied
issues own their text and remain usable after snapshot release.

Absent requested components are informational. Missing references, missing
required reference data, invalid numeric/configuration values, unresolvable
geometry, and failed calculations are warnings. Capture all independent issues;
do not fabricate downstream calculation failures after a prerequisite failed.
Issues may coexist with available stored configuration. No successful unrelated
section erases an issue. Allocate enough storage for every captured issue;
allocation or size-overflow failure fails the entire capture without partial
ownership or silent issue loss.

Issue getters filter the frozen records by the requested bitmask. Supported but
uncaptured sections match no issues and trigger no live inspection. Combined
flags return each matching record once. Preserve stable capture order: selector
bit order, then collection index, then field/reason enum order. Count and copy
observe the same immutable list. `total` is the matching count; `written` is the
copied prefix length. A small destination is a successful partial copy, explicitly
visible as `written < total`. No automatic printing or serialization occurs.

## Validation and usage

Invalid/deleted entities, zero or unsupported selection flags, allocation failure,
and size overflow return an error from `get`, with no snapshot. Missing components
or failed resolution return a successful snapshot with availability and issues.
Retain and issue getters reject NULL or invalidated snapshots. Issue getters reject
zero/unsupported flags; a NULL destination is permitted only with capacity zero.
Invalid copy inputs cause no destination writes. Zero capacity returns total with
written zero. A retain-count overflow must fail without changing ownership.

```c
EntityInfoResult result = rohr_entity_info_get(entity,
    ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS |
    ROHR_ENTITY_INFO_PHYSICS_VELOCITY);
if (result.kind == ERROR_RESULT_ERROR) {
    /* Handle result.result.error. */
    return;
}
EntityInfo info = result.result.value;
if (rohr_entity_info_check(&info, ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS)) {
    for (size_t i = 0; i < info.animation_count; ++i) {
        bool visible = info.animations[i].visible;
        (void)visible;
    }
}
EntityInfoIssueCountResult count =
    rohr_entity_info_issues_count_get(&info, ROHR_ENTITY_INFO);
if (count.kind == ERROR_RESULT_VALUE && count.result.value > 0) {
    /* Allocate count.result.value EntityInfoIssue entries, then copy with
       rohr_entity_info_issues_get. Check the result and written/total.
       The copied records can be printed and outlive info. */
}
(void)rohr_entity_info_release(&info);
```

## Force/torque integration boundary

Current selectors capture the existing Force vector and Torque scalar, with the
existing target under CORE_TARGET. The later source milestone replaces these
with its approved description/configuration and resolved details. That breaking
extension must update this contract and its selector inventory; do not implement
future source structures in the current inspection goal.

A zero-initialized future source is valid: no target, zero magnitude, and world
references. It is not unconfigured or broken. Validate its references even with
zero magnitude or no target. Preserve available source configuration when a
collapsed beam or missing reference makes resolved values unavailable. Record
those issues; do not substitute world references, disable sources, or introduce
separate force-status getters. Inspection does not apply reactions or retry work.

## Implementation acceptance criteria

- Public/direct equivalence for every API; single, combined, and all selectors;
  zero/unsupported flags and invalid/deleted entities; no simulation or report
  changes during capture, retain, check, issue reads, or release.
- Available zero/false/empty values versus missing components; valid stored
  configuration with unresolved dependencies; multiple independent issues;
  local/world geometry and layer resolution using existing conventions.
- Direct indexing for all collections, count-only capture without array copies,
  implied count availability, deterministic ordering, empty arrays, invalid
  element resolution, and full copies at configured maxima and dynamic sizes.
- Independent frozen results after ECS mutation, deletion, slot reuse, and
  engine shutdown; retain/release in different orders; cleared release no-op;
  resource IDs without resource retention; no leaked partial allocations under
  deterministic allocation-failure injection or overflow.
- Issue filtering, count/copy agreement, zero and insufficient capacity, invalid
  destination without writes, multiple issues per section, copied text lifetime,
  and visible message truncation without dropped issue records.
- Compile direct/public examples and measure snapshot scalar size, allocation
  size, and capture cost for sparse, all-section, and large-collection entities.
  Record actual measurements rather than assume maximum arrays are cheap.
- Run relevant entity/physics/graphics/report tests, sanitizers, Linux and Windows
  builds, affected examples, and installed-SDK consumers. Distinguish platform
  compilation from runtime verification. These are Goal 2 requirements; Goal 1
  verifies documentation, type consistency, examples, and links only.

## Storage and scaling

The adversarial inspection suite checks selector composition, independent broken
dependencies, malformed collection extents and active indices, nonfinite values,
component removal, stale targets, issue-buffer boundaries, allocation failure,
and retained ownership across engine restarts. Four fixed seeds exercise 8,000
ownership operations across 40 restarts; 128 selector combinations check filtered
evidence against a full capture. Inspection must preserve existing stored reports.
Collection extents are checked before copying or using legacy getters; malformed
storage is unavailable with an issue, while allocation-size overflow fails the
capture. Independent dependency failures must remain individually observable.

Each successful capture allocates one backing record containing the scalar
snapshot, plus only the requested nonempty arrays and issue storage. Count-only
queries allocate the backing record but no collection arrays. Allocation and
size-overflow failures clean all partial storage. Retain and release allocate
nothing. A process-lifetime monotonically increasing 64-bit handle prevents stale
aliases from resolving a later allocation; handle exhaustion returns an error.
Live snapshots use a linked registry, so handle lookup costs O(live snapshots).
No engine lifecycle operation destroys caller-owned snapshots.

Linux x64 measurements for the verification fixture: `sizeof(EntityInfo)` is
9,184 bytes; a count-only capture allocates 9,320 bytes. An all-section capture of
an entity with 64 hitboxes, one child, group membership, one anchor, and one
animation allocates 100,552 bytes in 10 allocations (including issue storage).
A 1,024-hitbox snapshot copies all 1,130,496 array bytes. In the local Linux
build, 100 captures/releases of the 64-hitbox all-section fixture took about
1.4 ms; the installed consumer measured about 2.8 ms. Timing is observational,
not a portable performance guarantee. These sizes are implementation/platform
measurements, not ABI size guarantees.
Prefer narrow selectors when polling, and release snapshots promptly. If profiling
shows many simultaneously retained snapshots, the registry can gain indexed
lookup without changing the public ownership contract.
