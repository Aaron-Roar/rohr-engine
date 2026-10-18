/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ENTITY_INFO_H
#define ENTITY_INFO_H

#include "physics.h"
#include "graphics.h"

/** Allocating, immutable current-state snapshots. See docs/entity_inspection_contract.md.
 * Calls must be serialized between updates. Arrays and issues are snapshot-owned;
 * resource handles are borrowed identifiers. Plain assignment does not retain. */
typedef uint64_t EntityInfoFlags;
typedef uint64_t EntityInfoHandle;

#define ROHR_ENTITY_INFO_CORE_NAME (UINT64_C(1) << 0)
#define ROHR_ENTITY_INFO_CORE_COMPONENTS (UINT64_C(1) << 1)
#define ROHR_ENTITY_INFO_CORE_LIFETIME (UINT64_C(1) << 2)
#define ROHR_ENTITY_INFO_CORE_PARENT (UINT64_C(1) << 3)
#define ROHR_ENTITY_INFO_CORE_TARGET (UINT64_C(1) << 4)
#define ROHR_ENTITY_INFO_CORE_CHILDREN (UINT64_C(1) << 5)
#define ROHR_ENTITY_INFO_CORE_GROUPS (UINT64_C(1) << 6)
#define ROHR_ENTITY_INFO_PHYSICS_POSITION (UINT64_C(1) << 7)
#define ROHR_ENTITY_INFO_PHYSICS_ORIENTATION (UINT64_C(1) << 8)
#define ROHR_ENTITY_INFO_PHYSICS_VELOCITY (UINT64_C(1) << 9)
#define ROHR_ENTITY_INFO_PHYSICS_ACCELERATION (UINT64_C(1) << 10)
#define ROHR_ENTITY_INFO_PHYSICS_ANGULAR_VELOCITY (UINT64_C(1) << 11)
#define ROHR_ENTITY_INFO_PHYSICS_ANGULAR_ACCELERATION (UINT64_C(1) << 12)
#define ROHR_ENTITY_INFO_PHYSICS_ANGULAR_VELOCITY_MAXIMUM (UINT64_C(1) << 13)
#define ROHR_ENTITY_INFO_PHYSICS_MASS (UINT64_C(1) << 14)
#define ROHR_ENTITY_INFO_PHYSICS_COM_LOCAL (UINT64_C(1) << 15)
#define ROHR_ENTITY_INFO_PHYSICS_COM_WORLD (UINT64_C(1) << 16)
#define ROHR_ENTITY_INFO_PHYSICS_INERTIA (UINT64_C(1) << 17)
#define ROHR_ENTITY_INFO_PHYSICS_FRICTION (UINT64_C(1) << 18)
#define ROHR_ENTITY_INFO_PHYSICS_RESTITUTION (UINT64_C(1) << 19)
#define ROHR_ENTITY_INFO_PHYSICS_BODY_MODE (UINT64_C(1) << 20)
#define ROHR_ENTITY_INFO_PHYSICS_PARTICLE (UINT64_C(1) << 21)
#define ROHR_ENTITY_INFO_PHYSICS_COLLISION_FILTER (UINT64_C(1) << 22)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_ACTIVE (UINT64_C(1) << 23)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_LOCAL (UINT64_C(1) << 24)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_WORLD (UINT64_C(1) << 25)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOXES (UINT64_C(1) << 26)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_BINDINGS (UINT64_C(1) << 27)
#define ROHR_ENTITY_INFO_PHYSICS_AXIS_LOCK (UINT64_C(1) << 28)
#define ROHR_ENTITY_INFO_PHYSICS_ANGLE_LOCK (UINT64_C(1) << 29)
#define ROHR_ENTITY_INFO_PHYSICS_TRANSFORM_LOCK (UINT64_C(1) << 30)
#define ROHR_ENTITY_INFO_PHYSICS_JOINT (UINT64_C(1) << 31)
#define ROHR_ENTITY_INFO_PHYSICS_JOINT_ANCHORS (UINT64_C(1) << 32)
#define ROHR_ENTITY_INFO_PHYSICS_JOINT_ANCHORS_WORLD (UINT64_C(1) << 33)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY (UINT64_C(1) << 34)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_NODE (UINT64_C(1) << 35)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_BEAM (UINT64_C(1) << 36)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_AREA (UINT64_C(1) << 37)
#define ROHR_ENTITY_INFO_PHYSICS_OVERLAPS (UINT64_C(1) << 38)
#define ROHR_ENTITY_INFO_PHYSICS_CONTACTS (UINT64_C(1) << 39)
#define ROHR_ENTITY_INFO_PHYSICS_FORCE (UINT64_C(1) << 40)
#define ROHR_ENTITY_INFO_PHYSICS_TORQUE (UINT64_C(1) << 41)
#define ROHR_ENTITY_INFO_GRAPHICS_SPRITE (UINT64_C(1) << 42)
#define ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS (UINT64_C(1) << 43)
#define ROHR_ENTITY_INFO_GRAPHICS_ENTITY_LAYER (UINT64_C(1) << 44)
#define ROHR_ENTITY_INFO_GRAPHICS_SPRITE_LAYER (UINT64_C(1) << 45)
#define ROHR_ENTITY_INFO_GRAPHICS_ANIMATION_LAYER (UINT64_C(1) << 46)
#define ROHR_ENTITY_INFO_CORE_CHILD_COUNT (UINT64_C(1) << 47)
#define ROHR_ENTITY_INFO_CORE_GROUP_COUNT (UINT64_C(1) << 48)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_COUNT (UINT64_C(1) << 49)
#define ROHR_ENTITY_INFO_PHYSICS_HITBOX_BINDING_COUNT (UINT64_C(1) << 50)
#define ROHR_ENTITY_INFO_PHYSICS_JOINT_ANCHOR_COUNT (UINT64_C(1) << 51)
#define ROHR_ENTITY_INFO_PHYSICS_OVERLAP_COUNT (UINT64_C(1) << 52)
#define ROHR_ENTITY_INFO_PHYSICS_CONTACT_COUNT (UINT64_C(1) << 53)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_NODE_COUNT (UINT64_C(1) << 54)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_BEAM_COUNT (UINT64_C(1) << 55)
#define ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_AREA_COUNT (UINT64_C(1) << 56)
#define ROHR_ENTITY_INFO_GRAPHICS_ANIMATION_COUNT (UINT64_C(1) << 57)
#define ROHR_ENTITY_INFO_GRAPHICS_SPRITE_VISIBILITY (UINT64_C(1) << 58)
#define ROHR_ENTITY_INFO_GRAPHICS_SPRITE_SCALE (UINT64_C(1) << 59)
#define ROHR_ENTITY_INFO_GRAPHICS_PRESENCE (UINT64_C(1) << 60)
#define ROHR_ENTITY_INFO_CORE UINT64_C(0x180000000007f)
#define ROHR_ENTITY_INFO_PHYSICS UINT64_C(0x1fe03ffffffff80)
#define ROHR_ENTITY_INFO_GRAPHICS UINT64_C(0x1e007c0000000000)
#define ROHR_ENTITY_INFO (ROHR_ENTITY_INFO_CORE | ROHR_ENTITY_INFO_PHYSICS | ROHR_ENTITY_INFO_GRAPHICS)

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
    ROHR_ENTITY_INFO_CALCULATION_FAILED,
    /** Valid animation with no frames; playback and drawing are skipped. */
    ROHR_ENTITY_INFO_ANIMATION_EMPTY
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

/** Returned by value; treat all fields as immutable. Release each owner once. */
typedef struct EntityInfo {
    Entity entity;
    EntityInfoFlags requested;
    EntityInfoFlags available;
    EntityInfoHandle handle;
    EntityName name;
    RohrComponentMask components;
    LifeTime lifetime;
    Parent parent;
    Entity target;
    const Entity * children;
    const GroupId * groups;
    Position position;
    Orientation orientation;
    Velocity velocity;
    Acceleration acceleration;
    AngularVelocity angular_velocity;
    AngularAcceleration angular_acceleration;
    AngularVelocity angular_velocity_maximum;
    Mass mass_value;
    Position center_of_mass_local;
    Position center_of_mass_world;
    float moment_of_inertia;
    Friction friction;
    Restitution restitution;
    EntityInfoBodyMode body_mode;
    ParticleGeometry particle;
    CollisionFilterConfig collision_filter;
    EntityInfoActiveHitbox hitbox_active;
    Shape hitbox_local;
    Shape hitbox_world;
    const HitboxVariant * hitboxes;
    const HitboxAnimationBinding * hitbox_bindings;
    AxisLock axis_lock;
    AngleLock angle_lock;
    TransformLock transform_lock;
    Joint joint;
    const EntityInfoJointAnchor * joint_anchors;
    const EntityInfoJointAnchorWorld * joint_anchors_world;
    SoftBody soft_body;
    SoftBodyNode soft_body_node;
    SoftBodyBeam soft_body_beam;
    SoftBodyArea soft_body_area;
    const EntityInteraction * overlaps;
    const EntityContact * contacts;
    ForceDescription force;
    TorqueDescription torque;
    /** Current eligibility only; inspection does not execute a contribution. */
    bool force_eligible;
    bool torque_eligible;
    Sprite sprite;
    const AnimatedSprite * animations;
    EntityInfoLayer entity_layer;
    EntityInfoLayer sprite_layer;
    EntityInfoLayer animation_layer;
    size_t child_count;
    size_t group_count;
    size_t hitbox_count;
    size_t hitbox_binding_count;
    size_t joint_anchor_count;
    size_t overlap_count;
    size_t contact_count;
    size_t soft_body_node_count;
    size_t soft_body_beam_count;
    size_t soft_body_area_count;
    size_t animation_count;
    bool sprite_visible;
    Scale sprite_scale;
    bool has_sprite, has_animations, is_animated;
} EntityInfo;

ERROR_DECLARE_RESULT_TYPE(EntityInfoResult, EntityInfo);
ERROR_DECLARE_RESULT_TYPE(EntityInfoIssueCountResult, size_t);
ERROR_DECLARE_RESULT_TYPE(EntityInfoIssueReadResult, EntityInfoIssueRead);

/**
 * @brief Allocate a frozen current-state snapshot for the selected sections.
 * @param entity Live entity to inspect between updates.
 * @param flags Nonzero combination of supported ROHR_ENTITY_INFO selectors.
 * @return Owned EntityInfo or an error. Release every owning reference.
 */
EntityInfoResult entity_info_get(Entity entity, EntityInfoFlags flags);
/**
 * @brief Acquire another owner of the same immutable snapshot without reinspection.
 * @param info Live snapshot; plain C assignment does not retain ownership.
 * @return Another owning EntityInfo, or an error without changing ownership.
 */
EntityInfoResult entity_info_retain(const EntityInfo *info);
/**
 * @brief Release one snapshot owner and clear its value; zero is a no-op.
 * @param info Owning value. NULL and stale nonzero handles are errors.
 * @return Success or an error. Final release frees all snapshot arrays.
 */
EngineResult entity_info_release(EntityInfo *info);
/**
 * @brief Check whether every selected section was captured successfully.
 * @param info Live immutable snapshot.
 * @param flags Nonzero supported selectors; availability is not operation success.
 * @return False for invalid input or any unavailable section.
 */
bool entity_info_check(const EntityInfo *info, EntityInfoFlags flags);

/**
 * @brief Count captured issues matching the selected sections.
 * @param info Live immutable snapshot.
 * @param flags Supported section selectors; uncaptured sections match no issues.
 * @return Matching count, or an error for invalid input.
 */
EntityInfoIssueCountResult entity_info_issues_count_get(
    const EntityInfo *info, EntityInfoFlags flags);
/**
 * @brief Copy a prefix of matching issues, including owned explanatory text.
 * @param info Live immutable snapshot.
 * @param flags Nonzero supported section selectors.
 * @param destination Caller array; NULL is permitted only with zero capacity.
 * @param capacity Number of destination entries.
 * @return Written and total counts; insufficient capacity is successful partial output.
 */
EntityInfoIssueReadResult entity_info_issues_get(
    const EntityInfo *info, EntityInfoFlags flags,
    EntityInfoIssue *destination, size_t capacity);

#endif
