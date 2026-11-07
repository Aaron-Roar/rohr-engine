/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_OBJECT_H
#define ROHR_OBJECT_H

#include "physics.h"
#include "graphics.h"

/* Packed colors use RGBA (0xRRGGBBAA). Unspecified styles use marker defaults. */
typedef struct ObjectVisualStyle {
    bool specified;
    uint32_t fill_color, border_color;
    float border_thickness;
    Scale scale;
    Position offset;
    Orientation rotation;
} ObjectVisualStyle;
typedef struct ObjectVisuals {
    ObjectVisualStyle main, endpoint_a, endpoint_b;
} ObjectVisuals;

typedef enum ObjectKind {
    OBJECT_CONTAINER, OBJECT_BODY, OBJECT_PARTICLE, OBJECT_SOFT_BODY,
    OBJECT_NODE, OBJECT_BEAM, OBJECT_AREA, OBJECT_ANCHOR, OBJECT_JOINT,
    OBJECT_SPRITE, OBJECT_ANIMATION, OBJECT_CAMERA, OBJECT_FORCE, OBJECT_TORQUE
} ObjectKind;

/** Stable authored identity. A zero id disconnects the reference. In compiled
 * catalogs id is the catalog description id; soft_body is then zero. */
typedef struct ObjectEntityReference {
    ObjectKind kind;
    uint64_t id;
    uint64_t soft_body;
} ObjectEntityReference;

/** Enabled deadlines default to durations after spawn. Absolute mode uses the
 * same clock as EntityLifetimeDescription; neither mode starts a private clock. */
typedef struct ObjectLifetimeDescription {
    EntityLifetimeDescription deadline;
    bool time_absolute;
    bool tick_absolute;
} ObjectLifetimeDescription;

/** Optional initial entity configuration, copied independently per spawn.
 * Axis points default to top-level object coordinates. Transform offsets are
 * driver-local; angles use clockwise degrees. References never own entities. */
typedef struct ObjectEntitySettings {
    ObjectLifetimeDescription lifetime;
    AngularAcceleration angular_acceleration;
    AngularVelocity angular_velocity_maximum;
    bool angular_velocity_maximum_enabled;
    ObjectEntityReference gameplay_target;
    bool axis_lock_enabled;
    bool axis_lock_world_fixed;
    AxisLockDescription axis_lock;
    ObjectEntityReference transform_driver;
    Vec2D transform_offset;
    Orientation transform_angle;
    bool transform_position;
    bool transform_orientation;
    bool transform_inherit_velocity;
} ObjectEntitySettings;

/** Authored source references are stable document/description IDs, never entities. */
typedef enum ObjectSourceReferenceKind {
    OBJECT_SOURCE_NONE, OBJECT_SOURCE_BODY, OBJECT_SOURCE_PARTICLE,
    OBJECT_SOURCE_SOFT_BODY, OBJECT_SOURCE_NODE, OBJECT_SOURCE_BEAM
} ObjectSourceReferenceKind;
typedef struct ObjectSourceReference {
    ObjectSourceReferenceKind kind;
    uint64_t id;
    uint64_t soft_body; /* Required for nodes/beams; zero otherwise. */
} ObjectSourceReference;
typedef enum ObjectSourceMode {
    OBJECT_SOURCE_PERSISTENT, OBJECT_SOURCE_INITIAL
} ObjectSourceMode;
typedef struct ObjectForceLocation {
    ForcePositionReferenceKind kind;
    Vec2D offset;
    ObjectSourceReference reference;
    ForceBeamAnchor anchor;
    float fraction;
} ObjectForceLocation;
typedef struct ObjectForceOrientation {
    ForceOrientationReferenceKind kind;
    Orientation offset;
    ObjectSourceReference reference;
    ObjectForceLocation from;
    ObjectForceLocation to;
} ObjectForceOrientation;
typedef struct ObjectForceDescription {
    ObjectSourceMode mode;
    ObjectSourceReference target;
    ObjectForceLocation location;
    ObjectForceOrientation orientation;
    float magnitude;
} ObjectForceDescription;
typedef struct ObjectTorqueDescription {
    ObjectSourceMode mode;
    ObjectSourceReference target;
    Torque magnitude;
} ObjectTorqueDescription;

typedef uint32_t EntityObjectFlags;
enum {
    ROHR_ENTITY_OBJECT_PHYSICS = 1u << 0,
    ROHR_ENTITY_OBJECT_VISUALS = 1u << 1,
    ROHR_ENTITY_OBJECT_ALL = ROHR_ENTITY_OBJECT_PHYSICS | ROHR_ENTITY_OBJECT_VISUALS
};

typedef struct ObjectHitbox {
    HitboxId id;
    Shape shape;
} ObjectHitbox;
typedef struct ObjectFrame {
    AnimationFrameId id;
    const char *path;
    Scale scale;
    Position offset;
    Orientation rotation;
} ObjectFrame;
typedef struct ObjectAreaLoop {
    uint64_t nodes[SOFT_BODY_MAX_NODES];
    uint32_t node_count;
} ObjectAreaLoop;
typedef struct ObjectAreaGeometry {
    ObjectAreaLoop outer;
    ObjectAreaLoop holes[SOFT_BODY_MAX_AREA_HOLES];
    uint32_t hole_count;
} ObjectAreaGeometry;

typedef struct ObjectDescription ObjectDescription;

/* Shared initial union member for generated typed views. Generated code exposes
 * these fields directly beside its named children, with no duplicated state. */
#define ROHR_OBJECT_DESCRIPTION_FIELDS \
    uint64_t id; \
    const char *name; \
    ObjectKind kind; \
    EntityObjectFlags categories; \
    const ObjectDescription *parent; \
    const ObjectDescription *const *children; \
    size_t child_count; \
    Position position; \
    Orientation orientation; \
    Velocity velocity; \
    Acceleration acceleration; \
    AngularVelocity angular_velocity; \
    ObjectEntitySettings entity_settings; \
    Mass mass; \
    float radius; \
    float friction; \
    float restitution; \
    bool static_body; \
    bool kinematic_driven; \
    bool gravity_enabled; \
    bool collision_enabled; \
    bool rotation_locked; \
    bool visible; \
    bool center_of_mass_explicit; \
    Position center_of_mass; \
    Position particle_origin; \
    uint32_t particle_vertices; \
    CollisionFilterDescription collision_filter; \
    const ObjectHitbox *geometry; \
    size_t geometry_count; \
    size_t active_geometry; \
    const HitboxAnimationBinding *hitbox_bindings; \
    size_t hitbox_binding_count; \
    Color color; \
    Color border_color; \
    bool color_overridden; \
    int layer; \
    uint64_t target; \
    uint64_t endpoint_a; \
    uint64_t endpoint_b; \
    JointType joint_type; \
    float rest_length; \
    float stiffness; \
    float damping; \
    float rest_angle; \
    float thickness; \
    const ObjectAreaGeometry *area; \
    const char *asset_path; \
    Scale size; \
    Scale scale; \
    const ObjectFrame *frames; \
    size_t frame_count; \
    Tick ticks_per_frame; \
    Time time_per_frame; \
    uint32_t starting_frame; \
    Direction direction; \
    bool playing; \
    bool follow_position; \
    bool follow_orientation; \
    bool pause_with_engine; \
    float zoom; \
    ObjectForceDescription force; \
    ObjectTorqueDescription torque; \
    ObjectVisuals visuals

struct ObjectDescription { ROHR_OBJECT_DESCRIPTION_FIELDS; };
ERROR_DECLARE_RESULT_TYPE(ObjectDescriptionResult, const ObjectDescription *);

/** Immutable member template. Shares catalog storage with ObjectDescription;
 * unlike EntityDescription, this describes components, not just metadata. */
typedef ObjectDescription EntityCreateDescription;

/** Borrowed for creation only. The description must belong to the same catalog
 * as the template being created; the live entity must match its kind. */
typedef struct ObjectCreateBinding {
    const ObjectDescription *description;
    Entity entity;
} ObjectCreateBinding;

typedef struct EntityCreateOptions {
    Entity owner;
    Position offset;
    const ObjectCreateBinding *bindings;
    size_t binding_count;
} EntityCreateOptions;

typedef struct ObjectCreateOptions {
    Entity owner;
    Position offset;
    EntityObjectFlags flags;
    const ObjectCreateBinding *bindings;
    size_t binding_count;
} ObjectCreateOptions;

EntityCreateOptions entity_create_options_default_get(void);
ObjectCreateOptions object_create_options_default_get(void);
/** Create one member without its separately authored children or siblings.
 * Container and soft-body aggregate templates are rejected.
 * Input is borrowed during the call; returned state and asset references are
 * independently owned. Creation may allocate/load assets. Failure rolls back
 * the attempt without altering borrowed instances. Delete with entity_delete. */
EntityResult entity_create(const EntityCreateDescription *description);
EntityResult entity_create_with_options(const EntityCreateDescription *description,
    EntityCreateOptions options);
/** Create a container/aggregate and its members. Internal references resolve
 * within this instance; external references require explicit bindings. */
EntityResult object_create(const ObjectDescription *description);
EntityResult object_create_with_options(const ObjectDescription *description,
    ObjectCreateOptions options);
/** Get a borrowed direct member by authored name. Does not search descendants;
 * missing/ambiguous names fail. Visual aliases may name the same body entity. */
EntityResult object_member_get(Entity object, const char *name);

/* Generated views begin with ObjectDescription (possibly in their first union).
 * Pointer conversion addresses that initial member and evaluates input once.
 * The parenthesized function names also remain usable as function pointers. */
#define entity_create(description) \
    (entity_create)((const EntityCreateDescription *)(description))
#define entity_create_with_options(description, options) \
    (entity_create_with_options)((const EntityCreateDescription *)(description), (options))
#define object_create(description) \
    (object_create)((const ObjectDescription *)(description))
#define object_create_with_options(description, options) \
    (object_create_with_options)((const ObjectDescription *)(description), (options))

/* Accept a generated view lvalue or a common description pointer. Generated
 * views start with a union whose first member is ObjectDescription. */
#define object_description_ref(value) _Generic((value), \
    ObjectDescription *: (value), const ObjectDescription *: (value), \
    default: (const ObjectDescription *)&(value))

/** Resolve a compiled description without loading or spawning anything. */
const ObjectDescription *object_description_get(const ObjectDescription *description);
#define object_get(value) object_description_get(object_description_ref(value))
ObjectDescriptionResult object_description_name_get(const ObjectDescription *scope, const char *name);
#define object_description_by_name_get(scope, name) \
    object_description_name_get(object_description_ref(scope), name)

/** Load an immutable file-backed catalog; release each returned owning reference. */
ObjectDescriptionResult object_load(const char *path);
/** Explicitly allocate an editable deep copy. Release only the returned root. */
ObjectDescriptionResult object_description_copy(const ObjectDescription *description, EntityObjectFlags flags);
#define object_copy(value) object_description_copy(object_description_ref(value), ROHR_ENTITY_OBJECT_ALL)
#define object_copy_with_options(value, flags) object_description_copy(object_description_ref(value), flags)
EngineResult object_destroy(const ObjectDescription *description);

typedef struct ObjectMaterialDescription {
    float friction;
    float restitution;
} ObjectMaterialDescription;
ERROR_DECLARE_RESULT_TYPE(ObjectMaterialDescriptionResult, ObjectMaterialDescription);
typedef struct ObjectCollisionDescription {
    bool enabled;
    CollisionFilterDescription filter;
} ObjectCollisionDescription;
ERROR_DECLARE_RESULT_TYPE(ObjectCollisionDescriptionResult, ObjectCollisionDescription);
typedef struct ObjectColorDescription {
    Color color;
    Color border_color;
    bool overridden;
} ObjectColorDescription;
ERROR_DECLARE_RESULT_TYPE(ObjectColorDescriptionResult, ObjectColorDescription);

/** Full replacement on editable copies only; affects future spawns. Getters
 * also accept immutable generated and loaded descriptions. Material/collision
 * require a physics body, particle, node, or beam. Color retains inheritance. */
EngineResult object_material_description_set(const ObjectDescription *description, ObjectMaterialDescription value);
ObjectMaterialDescriptionResult object_material_description_get(const ObjectDescription *description);
EngineResult object_collision_description_set(const ObjectDescription *description, ObjectCollisionDescription value);
ObjectCollisionDescriptionResult object_collision_description_get(const ObjectDescription *description);
EngineResult object_color_description_set(const ObjectDescription *description, ObjectColorDescription value);
ObjectColorDescriptionResult object_color_description_get(const ObjectDescription *description);
EngineResult object_force_description_set(const ObjectDescription *description, ObjectForceDescription value);
EngineResult object_torque_description_set(const ObjectDescription *description, ObjectTorqueDescription value);
EngineResult object_mass_set(const ObjectDescription *description, Mass mass);
EngineResult object_position_set(const ObjectDescription *description, Position position);
EngineResult object_velocity_set(const ObjectDescription *description, Velocity velocity);
EngineResult object_acceleration_set(const ObjectDescription *description, Acceleration acceleration);
EngineResult object_orientation_set(const ObjectDescription *description, Orientation orientation);
EngineResult object_angular_velocity_set(const ObjectDescription *description, AngularVelocity value);
EngineResult object_radius_set(const ObjectDescription *description, float radius);
EngineResult object_visibility_set(const ObjectDescription *description, bool visible);
EngineResult object_scale_set(const ObjectDescription *description, Scale scale);

typedef struct ObjectSpawnDescription {
    EntityObjectFlags flags;
    Position offset;
} ObjectSpawnDescription;
/** Select all categories at zero offset. An explicitly empty flags mask is invalid. */
ObjectSpawnDescription object_spawn_description_default_get(void);
/** Retrieve the camera resource owned by a spawned camera entity. */
CameraIdResult entity_object_camera_get(Entity entity);
/** Retrieve an anchor resource owned by a spawned anchor entity. */
JointAnchorIdResult entity_object_anchor_get(Entity entity);
/** Draw retained instance appearance and owned descendants in the active camera. */
bool entity_object_draw(Entity entity);
EntityResult entity_object_description_add(const ObjectDescription *description, ObjectSpawnDescription options);
#define entity_object_add(value) entity_object_description_add(object_description_ref(value), \
    (ObjectSpawnDescription){.flags = ROHR_ENTITY_OBJECT_ALL})
#define entity_object_add_with_options(value, flags_value) entity_object_description_add(object_description_ref(value), \
    (ObjectSpawnDescription){.flags = (flags_value)})
EntityResult entity_object_description_by_name_add(const ObjectDescription *scope, const char *name, ObjectSpawnDescription options);
#define entity_object_by_name_add(scope, name) entity_object_description_by_name_add(object_description_ref(scope), name, \
    (ObjectSpawnDescription){.flags = ROHR_ENTITY_OBJECT_ALL})
#define entity_object_by_name_add_with_options(scope, name, flags_value) \
    entity_object_description_by_name_add(object_description_ref(scope), name, (ObjectSpawnDescription){.flags = (flags_value)})

#endif
