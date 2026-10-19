/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef PHYSICS_H
#define PHYSICS_H
#include <stddef.h>
#include "math2d.h"
#include "entity_components.h"

#include "report.h"

/** Diagnostic context within a runnable physics report; these are not report IDs. */
typedef enum PhysicsReportStage {
    ROHR_PHYSICS_ARGUMENTS = 1,
    ROHR_PHYSICS_PREPARATION,
    ROHR_PHYSICS_INTEGRATION,
    ROHR_PHYSICS_BROADPHASE,
    ROHR_PHYSICS_CONTACT_GATHER,
    ROHR_PHYSICS_JOINT_GATHER,
    ROHR_PHYSICS_FINALIZATION,
    ROHR_PHYSICS_CONSTRAINT_RESPONSE,
    ROHR_PHYSICS_JOINT_TARGET,
    ROHR_PHYSICS_POSITION,
    ROHR_PHYSICS_ORIENTATION,
    ROHR_PHYSICS_VELOCITY,
    ROHR_PHYSICS_ANGULAR_VELOCITY,
    ROHR_PHYSICS_FORCE_CONTRIBUTION,
    ROHR_PHYSICS_TORQUE_CONTRIBUTION,
    ROHR_PHYSICS_BEAM_CONTRIBUTION,
    ROHR_PHYSICS_SPRING_CONTRIBUTION,
    ROHR_PHYSICS_NORMAL_IMPULSE,
    ROHR_PHYSICS_FRICTION_IMPULSE,
    ROHR_PHYSICS_POSITION_CORRECTION,
    ROHR_PHYSICS_JOINT_POSITION,
    ROHR_PHYSICS_JOINT_VELOCITY,
    ROHR_PHYSICS_ANGLE_LOCK,
    ROHR_PHYSICS_AXIS_LOCK,
    ROHR_PHYSICS_TRANSFORM_LOCK,
    ROHR_PHYSICS_GEOMETRY
} PhysicsReportStage;

#define PHYSICS_SOLVER_ITERATIONS_DEFAULT 8u
#define PHYSICS_SUBSTEPS_DEFAULT 1u
#define ROHR_PHYSICS_GRAVITY_DEFAULT ((Acceleration){0.0f, -9.8f})
#define ROHR_WORLD_COORDINATE_MAX 1000000.0f

EngineResult physics_solver_iterations_set(uint32_t iterations);
uint32_t physics_solver_iterations_get(void);
EngineResult physics_substeps_set(uint32_t substeps);
uint32_t physics_substeps_get(void);

/** Begin a complete physics step and advance interaction transition state. */
void physics_pipeline_step_begin(void);
/** Clear transient constraints before assembling one physics substep. */
void physics_pipeline_substep_begin(void);
/** Clear force-derived acceleration accumulated during the previous substep. */
void physics_pipeline_accelerations_clear(void);
/** Apply global gravity to opted-in movable entities. */
void physics_pipeline_gravity_apply(void);
/** Apply spring/beam loads after acceleration reset; return owned diagnostics. */
Report physics_pipeline_forces_apply(void);
/** Integrate prepared accelerations; reject invalid operations without changing their prior state. */
Report physics_pipeline_integrate(double dt);
/** Gather rigid/soft contacts after substep_begin; stop dependent work on failure. */
Report physics_pipeline_contacts_gather(void);
/** Replace joint constraints with current pin/weld joints; return owned results. */
Report physics_pipeline_joints_gather(void);
/** Solve successfully gathered constraints; zero iterations returns NOT_RUN. */
Report physics_pipeline_constraints_solve(uint32_t iterations);
/** Run one substep; call step_begin first when starting a new interaction update. */
Report physics_pipeline_substep(double dt);
/** Run all substeps. Zero dt still solves; negative/nonfinite dt returns FAILED. */
Report physics_pipeline_update(double dt);
/** Result type for functions that return a Shape. */
ERROR_DECLARE_RESULT_TYPE(ShapeResult, Shape);
/** Result type for hitbox variant counts and indices. */
ERROR_DECLARE_RESULT_TYPE(HitboxIndexResult, size_t);

typedef uint32_t HitboxId;
#define HITBOX_ID_INVALID UINT32_C(0)
ERROR_DECLARE_RESULT_TYPE(HitboxIdResult, HitboxId);

typedef struct HitboxVariant {
    HitboxId id;
    Shape shape;
} HitboxVariant;

/** Ordered local-space hitbox variants owned by one entity. */
typedef struct HitboxVariantList {
    HitboxVariant *values;
    size_t count;
    size_t capacity;
    size_t active_index;
    HitboxId next_id;
} HitboxVariantList;

typedef struct HitboxAnimationBinding {
    AnimationId animation_id;
    AnimationFrameId frame_id;
    HitboxId hitbox_id;
} HitboxAnimationBinding;

typedef struct HitboxAnimationBindingList {
    HitboxAnimationBinding *values;
    size_t count;
    size_t capacity;
} HitboxAnimationBindingList;

/** Geometric overlap information from narrow-phase shape tests. */
typedef struct OverlapInfo {
    /** true when shapes overlap. */
    bool detected;
    /** Normal pointing from the first shape toward the second. */
    Axis normal;
    /** Penetration depth along the overlap normal. */
    Vec1D depth;
} OverlapInfo;

/** One current interaction between a queried entity and another entity. */
typedef struct EntityInteraction {
    /** The other entity in the interaction. */
    Entity target;
    /** Geometry oriented from the queried entity toward target. */
    OverlapInfo overlap;
} EntityInteraction;

/** Bit mask describing one or more collision categories. */
typedef uint64_t RohrCollisionCategoryMask;

/** Default collision category assigned to colliders. */
#define ROHR_COLLISION_CATEGORY_DEFAULT UINT64_C(1)
/** Mask matching every collision category. */
#define ROHR_COLLISION_CATEGORY_ALL UINT64_MAX
/** Mask matching no collision categories. */
#define ROHR_COLLISION_CATEGORY_NONE UINT64_C(0)
/** Engine-reserved category assigned to soft-body nodes by default. */
#define ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE (UINT64_C(1) << 63)

/** Collision filtering configuration owned by one collider entity. */
typedef struct CollisionFilterDescription {
    /** Categories represented by this collider. */
    RohrCollisionCategoryMask category;
    /** Categories this collider permits collision checks against. */
    RohrCollisionCategoryMask collides_with;
} CollisionFilterDescription;

/** Result type for functions returning collision filter configuration. */
ERROR_DECLARE_RESULT_TYPE(CollisionFilterDescriptionResult, CollisionFilterDescription);

/** Surface friction coefficient. */
typedef Vec1D Friction;

/** Collision restitution coefficient. */
typedef Vec1D Restitution;

/** Entity orientation angle in clockwise degrees. */
typedef Vec1D Orientation;
/** Entity world position. */
typedef Vec2D Position;

/** Check that both coordinates are finite and inside the supported world. */
bool physics_world_position_check(Position position);
EngineResult physics_hitbox_autoshape_circle_set(Entity entity, float radius);
EngineResult physics_hitbox_autoshape_rectangle_set(Entity entity,
    float width, float height);
EngineResult physics_hitbox_autoshape_triangle_set(Entity entity,
    float width, float height);
EngineResult physics_soft_body_autoshape_circle_set(Entity soft_body,
    float radius);
EngineResult physics_soft_body_autoshape_rectangle_set(Entity soft_body,
    float width, float height);
EngineResult physics_soft_body_autoshape_triangle_set(Entity soft_body,
    float width, float height);
/** Entity linear velocity. */
typedef Vec2D Velocity;
/** Particle circle geometry stored relative to its rigid body's origin. */
typedef struct ParticleGeometry {
    Position local_origin;
    float radius;
    bool origin_explicit;
    bool radius_explicit;
    /** Created as a standalone, nonrotating particle with automatic COM. */
    bool standalone;
} ParticleGeometry;
ERROR_DECLARE_RESULT_TYPE(ParticleRadiusResult, float);
#define PHYSICS_CONTACT_POINT_MAX 2
/** Solver information for one world-space point in a contact manifold. */
typedef struct ContactPointInfo {
    /** Stable engine feature identifier used to retain solver impulses. */
    uint32_t feature_id;
    /** World-space contact position. */
    Position position;
    /** Pre-resolution velocity of the second entity relative to the first. */
    Velocity relative_velocity;
    /** Normal impulse applied to the second entity at this point. */
    Vec2D normal_impulse;
    /** Friction impulse applied to the second entity at this point. */
    Vec2D friction_impulse;
} ContactPointInfo;

/** Physical response information for one entity contact. */
typedef struct ContactInfo {
    /** true when physical collision response was entered. */
    bool detected;
    /** Normal pointing from the first entity toward the second. */
    Axis normal;
    /** Penetration depth along the contact normal. */
    Vec1D depth;
    /** World-space points and solver results in the collision manifold. */
    ContactPointInfo points[PHYSICS_CONTACT_POINT_MAX];
    /** Number of valid manifold points. */
    uint8_t point_count;
} ContactInfo;

/** One current physical contact involving a queried entity. */
typedef struct EntityContact {
    /** The other entity in the contact. */
    Entity target;
    /** Contact data oriented from the queried entity toward target. */
    ContactInfo contact;
} EntityContact;
/** Entity angular velocity in clockwise degrees per second. */
typedef Orientation AngularVelocity;
/** Entity angular acceleration in clockwise degrees per second squared. */
typedef Orientation AngularAcceleration;
/** Entity linear acceleration. */
typedef Vec2D Acceleration;
/** Set the acceleration applied to entities carrying ROHR_GRAVITY. */
EngineResult physics_gravity_set(Acceleration gravity);
/** Return the current global gravity acceleration. */
Acceleration physics_gravity_get(void);
/** Add the ROHR_GRAVITY component to an entity. */
EngineResult physics_gravity_enable(Entity entity);
/** Remove the ROHR_GRAVITY component from an entity. */
EngineResult physics_gravity_disable(Entity entity);
/** Return whether an entity carries ROHR_GRAVITY. */
bool physics_gravity_check(Entity entity);
/** Linear force vector. */
typedef Vec2D Force;
/** Entity mass value. */
typedef float Mass;
ERROR_DECLARE_RESULT_TYPE(MassResult, Mass);
/** Torque value. */
typedef Orientation Torque;

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
    /** Lifetime owner, independent of target and references; invalid for apply. */
    Entity owner;
    Entity target;
    ForceLocation location;
    ForceOrientation orientation;
    float magnitude;
} ForceDescription;

typedef struct TorqueDescription {
    /** Lifetime owner, independent of target; invalid for apply. */
    Entity owner;
    Entity target;
    Torque magnitude;
} TorqueDescription;

#ifndef ROHR_PHYSICS_BEAM_MIN_LENGTH
#define ROHR_PHYSICS_BEAM_MIN_LENGTH 0.0001f
#endif

ERROR_DECLARE_RESULT_TYPE(ForceDescriptionResult, ForceDescription);
ERROR_DECLARE_RESULT_TYPE(TorqueDescriptionResult, TorqueDescription);
ERROR_DECLARE_RESULT_TYPE(ForceLocationResult, ForceLocation);
ERROR_DECLARE_RESULT_TYPE(ForceOrientationResult, ForceOrientation);
ERROR_DECLARE_RESULT_TYPE(ForceMagnitudeResult, float);


/** Default polygon vertex count used to approximate a particle circle. */
#define ROHR_PARTICLE_RIGID_VERTICES_DEFAULT 16u

/** Complete creation/replacement configuration for one standalone particle. */
typedef struct ParticleDescription {
    /** Optional lifetime owner; connections do not imply ownership. */
    Entity owner;
    /** Current world position of the particle entity. */
    Position position;
    /** Shared local offset of the particle circle and polygon hitbox. */
    Position local_origin;
    /** Linear velocity; retained while static for a later dynamic transition. */
    Velocity velocity;
    /** Linear acceleration; retained while static. */
    Acceleration acceleration;
    /** Radius shared by the particle circle and polygon hitbox. */
    float radius;
    /** Circle approximation quality; zero selects the engine default. */
    uint32_t rigid_vertices;
    /** Mass retained while static; static response still has zero inverse mass. */
    Mass mass_value;
    Friction friction;
    Restitution restitution;
    CollisionFilterDescription collision_filter;
    /** Whether to create a static rather than dynamic particle. */
    bool static_body;
    /** Whether a dynamic particle receives engine gravity. */
    bool gravity_enabled;
    /** Whether overlaps enter physical collision response. */
    bool collision_enabled;
} ParticleDescription;
ERROR_DECLARE_RESULT_TYPE(ParticleDescriptionResult, ParticleDescription);

ERROR_DECLARE_RESULT_TYPE(PositionResult, Position);
ERROR_DECLARE_RESULT_TYPE(BoolResult, bool);
ERROR_DECLARE_RESULT_TYPE(MomentOfInertiaResult, float);
ERROR_DECLARE_RESULT_TYPE(AngularVelocityResult, AngularVelocity);
ERROR_DECLARE_RESULT_TYPE(ForceResult, Force);
ERROR_DECLARE_RESULT_TYPE(TorqueResult, Torque);

/** Complete axis constraint configuration. The setter normalizes a finite,
 * nonzero axis; point_on_axis uses world coordinates. */
typedef struct AxisLockDescription {
    Axis axis;
    Position point_on_axis;
} AxisLockDescription;
ERROR_DECLARE_RESULT_TYPE(AxisLockDescriptionResult, AxisLockDescription);

/** Complete transform-following configuration. Offsets are driver-local and
 * local_angle is clockwise degrees. References do not imply lifetime ownership. */
typedef struct TransformLockDescription {
    Entity driver;
    Vec2D local_offset;
    Orientation local_angle;
    bool lock_position;
    bool lock_orientation;
    bool inherit_velocity;
} TransformLockDescription;
ERROR_DECLARE_RESULT_TYPE(TransformLockDescriptionResult, TransformLockDescription);

/** Constraint that locks movement onto an axis through a point. */
typedef struct AxisLock {
    /** Axis to constrain movement along. */
    Axis axis;
    /** Point that the locked axis passes through. */
    Position point_on_axis;
} AxisLock;

/** Constraint that clamps orientation to a min/max angle. */
typedef struct AngleLock {
    /** Minimum allowed orientation. */
    Orientation min;
    /** Maximum allowed orientation. */
    Orientation max;
} AngleLock;

/** Constraint that drives one entity from another entity's transform. */
typedef struct TransformLock {
    /** Entity that drives the transform. */
    Entity driver;

    /** Local position offset from the driver. */
    Vec2D local_offset;
    /** Local orientation offset from the driver. */
    Orientation local_angle;

    /** Whether position should follow the driver. */
    bool lock_position;
    /** Whether orientation should follow the driver. */
    bool lock_orientation;
    /** Whether velocity should be inherited from the driver. */
    bool inherit_velocity;
} TransformLock;

/** Joint behavior type. */
typedef enum JointType {
    /** Apply a damped spring between anchor points. */
    JOINT_SPRING,
    /** Preserve relative position and orientation. */
    JOINT_WELD,
    /** Keep anchor points together while allowing relative rotation. */
    JOINT_PIN
} JointType;

/** Stable handle for an entity-owned joint anchor. */
typedef uint64_t JointAnchorId;
#define JOINT_ANCHOR_INVALID UINT64_C(0)
#define MAX_JOINT_ANCHORS 10000
#define MAX_JOINT_ANCHORS_PER_ENTITY 32

/** Entity-owned point stored relative to the entity's stable local origin. */
typedef struct JointAnchor {
    Entity entity;
    Vec2D local_offset;
} JointAnchor;

typedef struct JointAnchorList {
    JointAnchorId values[MAX_JOINT_ANCHORS_PER_ENTITY];
    uint32_t count;
} JointAnchorList;

ERROR_DECLARE_RESULT_TYPE(JointAnchorIdResult, JointAnchorId);
ERROR_DECLARE_RESULT_TYPE(JointAnchorResult, JointAnchor);
ERROR_DECLARE_RESULT_TYPE(JointAnchorListResult, JointAnchorList);
ERROR_DECLARE_RESULT_TYPE(JointAnchorPositionResult, Position);

/** Joint component data stored on a joint entity. */
typedef struct Joint {
    /** Joint behavior type. */
    JointType type;

    /** First borrowed anchor handle; invalid means disconnected. */
    JointAnchorId anchor_a;
    /** Second borrowed anchor handle; invalid means disconnected. */
    JointAnchorId anchor_b;

    /** First constrained entity. */
    Entity a;
    /** Second constrained entity. */
    Entity b;

    /** Legacy serialization field; not used to connect or solve joints. */
    Vec2D local_anchor_a;
    /** Legacy serialization field; not used to connect or solve joints. */
    Vec2D local_anchor_b;

    /** Resting distance for spring joints. */
    float rest_length;

    /** Linear spring stiffness. */
    float stiffness;
    /** Linear damping. */
    float damping;

    /** Whether the joint locks relative angle. */
    bool lock_angle;
    /** Resting relative angle. */
    Orientation rest_angle;
    /** Angular spring stiffness. */
    float angular_stiffness;
    /** Angular damping. */
    float angular_damping;
} Joint;

#define SOFT_BODY_MAX_NODES 64
#define SOFT_BODY_MAX_BEAMS 256
#define SOFT_BODY_MAX_AREAS 128
#define SOFT_BODY_MAX_AREA_HOLES 16
/** Smallest stable full thickness accepted by beam collision. */
#define ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN 0.01f

/** Derived lists of directly owned soft-body elements; connections may cross owners. */
typedef struct SoftBody {
    /** Usable origin follows the equal-weight node centroid and incremental
     * best-fit node rotation, retaining its offset from that private frame.
     * Explicit origin transforms move nodes; tracking only observes them. */
    Entity origin;
    Entity nodes[SOFT_BODY_MAX_NODES];
    Entity beams[SOFT_BODY_MAX_BEAMS];
    Entity areas[SOFT_BODY_MAX_AREAS];
    uint32_t node_count;
    uint32_t beam_count;
    uint32_t area_count;
} SoftBody;

/** Lightweight point mass participating in soft-body collision. */
typedef struct SoftBodyNode {
    Entity soft_body;
    float radius;
    /** Mirrored for source compatibility; standard collision filters are authoritative. */
    RohrCollisionCategoryMask category;
    /** Mirrored for source compatibility; standard collision filters are authoritative. */
    RohrCollisionCategoryMask collides_with;
    Color draw_color;
    bool draw_color_overridden;
} SoftBodyNode;

/** Complete collision configuration stored by a soft-body beam. */
typedef struct SoftBodyBeamCollisionDescription {
    bool enabled;
    float thickness;
    CollisionFilterDescription filter;
} SoftBodyBeamCollisionDescription;
ERROR_DECLARE_RESULT_TYPE(SoftBodyBeamCollisionDescriptionResult, SoftBodyBeamCollisionDescription);

/** Elastic connection between two soft-body nodes. */
typedef struct SoftBodyBeam {
    Entity soft_body;
    Entity node_a;
    Entity node_b;
    float rest_length;
    float stiffness;
    float damping;
    /** Full width of the centroid-to-centroid collision segment. */
    float collision_thickness;
    /** Independent contact materials; endpoint node materials do not affect these. */
    Friction friction;
    Restitution restitution;
    bool collision_enabled;
    RohrCollisionCategoryMask category;
    RohrCollisionCategoryMask collides_with;
    Color draw_color;
    bool draw_color_overridden;
} SoftBodyBeam;

/** Ordered node boundary; the last node connects implicitly to the first. */
typedef struct SoftBodyAreaLoop {
    Entity nodes[SOFT_BODY_MAX_NODES];
    uint32_t node_count;
} SoftBodyAreaLoop;

/** Copied boundary description. Hole order and loop winding have no effect. */
typedef struct SoftBodyAreaGeometry {
    SoftBodyAreaLoop outer;
    SoftBodyAreaLoop holes[SOFT_BODY_MAX_AREA_HOLES];
    uint32_t hole_count;
} SoftBodyAreaGeometry;

/** Creation/replacement descriptions are copied by value. An invalid owner means independent
 * lifetime; any live owner is accepted. Connections never imply ownership. */
typedef struct SoftBodyDescription {
    Entity owner;
    Position position;
    /** Clockwise degrees. Explicit changes transform existing nodes. */
    Orientation orientation;
} SoftBodyDescription;
ERROR_DECLARE_RESULT_TYPE(SoftBodyDescriptionResult, SoftBodyDescription);

typedef struct SoftBodyNodeDescription {
    Entity owner;
    /** World position for node_add; owner-local position for node_local_add. */
    Position position;
    Mass mass;
    float radius;
    Velocity velocity;
    Acceleration acceleration;
    Friction friction;
    Restitution restitution;
    CollisionFilterDescription collision_filter;
    bool static_body;
    bool gravity_enabled;
    bool collision_enabled;
    Color draw_color;
    bool draw_color_overridden;
} SoftBodyNodeDescription;
ERROR_DECLARE_RESULT_TYPE(SoftBodyNodeDescriptionResult, SoftBodyNodeDescription);

typedef struct SoftBodyBeamDescription {
    Entity owner;
    Entity node_a;
    Entity node_b;
    float stiffness;
    float damping;
    /** Measured once from connected endpoints unless explicitly supplied. */
    float rest_length;
    bool rest_length_explicit;
    Friction friction;
    Restitution restitution;
    /** Collision defaults off; settings never derive from endpoint nodes. */
    SoftBodyBeamCollisionDescription collision;
    Color draw_color;
    bool draw_color_overridden;
} SoftBodyBeamDescription;
ERROR_DECLARE_RESULT_TYPE(SoftBodyBeamDescriptionResult, SoftBodyBeamDescription);

typedef struct SoftBodyAreaDescription {
    Entity owner;
    SoftBodyAreaGeometry geometry;
    Color draw_color;
    bool draw_color_overridden;
    bool visible;
} SoftBodyAreaDescription;
ERROR_DECLARE_RESULT_TYPE(SoftBodyAreaDescriptionResult, SoftBodyAreaDescription);

/** Complete joint configuration. Endpoints borrow body-owned anchor resources.
 * Invalid anchors disconnect an endpoint; ownership is independent of connections.
 * Automatic rest values are measured once by add/set (zero if disconnected).
 * Explicit flags allow authored rest values, including zero. */
typedef struct JointDescription {
    Entity owner;
    JointType type;
    JointAnchorId anchor_a;
    JointAnchorId anchor_b;
    float rest_length;
    float stiffness;
    float damping;
    Orientation rest_angle;
    bool rest_length_explicit;
    bool rest_angle_explicit;
} JointDescription;
ERROR_DECLARE_RESULT_TYPE(JointDescriptionResult, JointDescription);

/** Anchors are resources attached to an entity, not independently owned entities. */
typedef struct JointAnchorDescription {
    Entity entity;
    Vec2D local_offset;
} JointAnchorDescription;
ERROR_DECLARE_RESULT_TYPE(JointAnchorDescriptionResult, JointAnchorDescription);

/** Owner applies to the new joint; the new anchor remains attached to node. */
typedef struct SoftBodyNodeAnchorPinDescription {
    Entity owner;
    Entity node;
    JointAnchorId anchor;
} SoftBodyNodeAnchorPinDescription;

/** Value snapshot; generated triangles and caches remain private. */
typedef struct SoftBodyArea {
    Entity soft_body;
    SoftBodyAreaGeometry geometry;
    Color draw_color;
    bool draw_color_overridden;
    bool visible;
} SoftBodyArea;

/** Explicit handles created when pinning a soft-body node to an anchor. */
typedef struct SoftBodyNodeAnchorPin {
    Entity joint;
    JointAnchorId node_anchor;
} SoftBodyNodeAnchorPin;

ERROR_DECLARE_RESULT_TYPE(SoftBodyResult, SoftBody);
ERROR_DECLARE_RESULT_TYPE(SoftBodyNodeResult, SoftBodyNode);
ERROR_DECLARE_RESULT_TYPE(SoftBodyBeamResult, SoftBodyBeam);
ERROR_DECLARE_RESULT_TYPE(SoftBodyAreaResult, SoftBodyArea);
ERROR_DECLARE_RESULT_TYPE(SoftBodyNodeAnchorPinResult, SoftBodyNodeAnchorPin);

/** Pool storing positions by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(PositionPool, Position);
/** Pool storing explicit particle circle geometry by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(ParticleGeometryPool, ParticleGeometry);
/** Pool storing velocities by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(VelocityPool, Velocity);
/** Pool storing accelerations by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AccelerationPool, Acceleration);
/** Pool storing masses by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(MassPool, float);
/** Pool storing forces by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(ForcePool, ForceDescription);
/** Pool storing shapes by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(ShapePool, Shape);
/** Pool storing owned hitbox variant lists by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(HitboxVariantListPool, HitboxVariantList);
MEMORY_DECLARE_OBJECT_POOL(HitboxAnimationBindingListPool,
    HitboxAnimationBindingList);
/** Pool storing collision filters by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(CollisionFilterDescriptionPool, CollisionFilterDescription);
/** Pool storing orientations by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(OrientationPool, Orientation);
/** Pool storing angular velocities by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AngularVelocityPool, AngularVelocity);
/** Pool storing angular accelerations by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AngularAccelerationPool, AngularAcceleration);
/** Pool storing torques by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(TorquePool, TorqueDescription);
/** Pool storing friction values by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(FrictionPool, Friction);
/** Pool storing restitution values by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(RestitutionPool, Restitution);
/** Pool storing angle locks by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AngleLockPool, AngleLock);
/** Pool storing axis locks by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AxisLockPool, AxisLock);
/** Pool storing transform locks by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(TransformLockPool, TransformLock);
/** Pool storing joints by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(JointPool, Joint);
MEMORY_DECLARE_OBJECT_POOL(SoftBodyPool, SoftBody);
MEMORY_DECLARE_OBJECT_POOL(SoftBodyNodePool, SoftBodyNode);
MEMORY_DECLARE_OBJECT_POOL(SoftBodyBeamPool, SoftBodyBeam);

extern PositionPool positions_pool;
extern ParticleGeometryPool particle_geometries_pool;
extern VelocityPool velocities_pool;
extern AccelerationPool accelerations_pool;
extern MassPool mass_pool;
extern ForcePool forces_pool;
extern AccelerationPool force_accelerations_pool;
extern ShapePool hit_boxes_pool;
extern ShapePool world_hit_boxes_pool;
extern HitboxVariantListPool hitbox_variants_pool;
extern HitboxAnimationBindingListPool hitbox_animation_bindings_pool;
extern CollisionFilterDescriptionPool collision_filters_pool;
extern OrientationPool orientations_pool;
extern AngularVelocityPool angular_velocities_pool;
extern AngularVelocityPool angular_velocity_maximums_pool;
extern AngularAccelerationPool angular_accelerations_pool;
extern TorquePool torques_pool;
extern AngularVelocityPool torque_angular_accelerations_pool;
extern FrictionPool frictions_pool;
extern RestitutionPool restitutions_pool;
extern AngleLockPool angle_locks_pool;
extern AxisLockPool axis_locks_pool;
extern TransformLockPool transform_locks_pool;
extern JointPool joints_pool;
extern SoftBodyPool soft_bodies_pool;
extern SoftBodyNodePool soft_body_nodes_pool;
extern SoftBodyBeamPool soft_body_beams_pool;
#define positions positions_pool.objects
#define velocities velocities_pool.objects
#define accelerations accelerations_pool.objects
/** Current mass table; refreshed after engine-managed table growth. */
extern Mass *mass;
#define forces forces_pool.objects
#define force_accelerations force_accelerations_pool.objects
#define hit_boxes hit_boxes_pool.objects
#define world_hit_boxes world_hit_boxes_pool.objects
#define collision_filters collision_filters_pool.objects
#define orientations orientations_pool.objects
#define angular_velocities angular_velocities_pool.objects
#define angular_velocity_maximums angular_velocity_maximums_pool.objects
#define angular_accelerations angular_accelerations_pool.objects
#define torques torques_pool.objects
#define torque_angular_accelerations torque_angular_accelerations_pool.objects
#define frictions frictions_pool.objects
#define restitutions restitutions_pool.objects
#define angle_locks angle_locks_pool.objects
#define axis_locks axis_locks_pool.objects
#define transform_locks transform_locks_pool.objects
#define joints joints_pool.objects
#define soft_bodies soft_bodies_pool.objects
#define soft_body_nodes soft_body_nodes_pool.objects
#define soft_body_beams soft_body_beams_pool.objects

/**
 * Translate a local shape into world coordinates.
 *
 * @param shape Shape with origin-relative local vertices; no recentering occurs.
 * @param position World position of the local origin.
 * @param angle World orientation in clockwise degrees.
 * @return World-space shape.
 */
Shape physics_shape_world_translate(Shape shape, Position position, Orientation angle);
/** Clockwise-positive radians alternative; other parameters retain their units. */
Shape physics_shape_world_radians_translate(Shape shape, Position position, Orientation angle);

/**
 * Approximate polygon moment of inertia.
 *
 * @param shape Shape whose vertices define the body.
 * @param mass_value Body mass.
 * @return Moment of inertia.
 */
float physics_polygon_moment_of_inertia(Shape shape, Mass mass_value);

/**
 * Run SAT collision detection between two shapes.
 *
 * @param shape_1 First world-space shape.
 * @param shape_2 Second world-space shape.
 * @return Geometric overlap information.
 */
OverlapInfo physics_sat_overlap_get(Shape shape_1, Shape shape_2);

/**
 * Compute circle moment of inertia from a circle-like shape.
 *
 * @param circle Shape representing a circle.
 * @param mass_value Body mass.
 * @return Moment of inertia.
 */
Vec1D physics_circle_moment_of_inertia(Shape circle, Mass mass_value);

/** Check whether an entity index has ROHR_HOLD. */
bool physics_entity_held_get(EntityIndex index);
/** Check whether an entity index can be moved by physics update stages. */
bool physics_entity_movable_get(EntityIndex index);
/** Check whether forces and constraints may alter an entity's motion. */
bool physics_entity_simulated_get(EntityIndex index);

/** Set an entity's base linear acceleration. */
EngineResult physics_acceleration_set(Entity entity, Acceleration a);
/** Set an entity's angular acceleration and mark it dynamic. */
EngineResult physics_angular_acceleration_set(
        Entity entity,
        AngularAcceleration acceleration
);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult physics_angular_acceleration_radians_set(
        Entity entity,
        AngularAcceleration acceleration
);
/** Set acceleration toward a world position using a scalar magnitude. */
EngineResult physics_acceleration_toward_position_set(Entity entity, float acceleration_magnitude, Position position);
/** Set acceleration toward another entity's current world position. */
EngineResult physics_acceleration_toward_entity_set(Entity entity, float acceleration_magnitude, Entity target);
/** Set acceleration away from a world position using a scalar magnitude. */
EngineResult physics_acceleration_away_from_position_set(Entity entity, float acceleration_magnitude, Position position);
/** Set acceleration away from another entity's current world position. */
EngineResult physics_acceleration_away_from_entity_set(Entity entity, float acceleration_magnitude, Entity target);
/** Set acceleration toward an entity for every live entity in a group. */
EngineResult physics_group_acceleration_toward_entity_set(GroupId group, float acceleration_magnitude, Entity target);
/** Set acceleration away from an entity for every live entity in a group. */
EngineResult physics_group_acceleration_away_from_entity_set(GroupId group, float acceleration_magnitude, Entity target);
/** Set an entity's linear velocity. */
EngineResult physics_velocity_set(Entity entity, Velocity v);
/** Set velocity toward a world position using a scalar speed. */
EngineResult physics_velocity_toward_position_set(Entity entity, float speed, Position position);
/** Set velocity toward another entity's current world position. */
EngineResult physics_velocity_toward_entity_set(Entity entity, float speed, Entity target);
/** Set velocity away from a world position using a scalar speed. */
EngineResult physics_velocity_away_from_position_set(Entity entity, float speed, Position position);
/** Set velocity away from another entity's current world position. */
EngineResult physics_velocity_away_from_entity_set(Entity entity, float speed, Entity target);
/** Set velocity toward an entity for every live entity in a group. */
EngineResult physics_group_velocity_toward_entity_set(GroupId group, float speed, Entity target);
/** Set velocity away from an entity for every live entity in a group. */
EngineResult physics_group_velocity_away_from_entity_set(GroupId group, float speed, Entity target);
/** Set an entity's velocity to zero. */
EngineResult physics_entity_stop(Entity entity);
/** Set velocity to zero for every live entity in a group. */
EngineResult physics_group_entities_stop(GroupId group);
/** Apply an immediate linear impulse to an entity's velocity. */
EngineResult physics_impulse_apply(Entity entity, Vec2D impulse);
/**
 * Apply an immediate angular impulse (mass * distance squared / second)
 * about the effective COM. Adds degrees(impulse * inverse inertia) to clockwise angular velocity
 * without changing motion type. Zero inverse inertia is a successful no-op.
 * Nonfinite input or resulting velocity is rejected without mutation.
 */
EngineResult physics_angular_impulse_apply(Entity entity, float impulse);
/** Set the world origin, preserving local geometry and attachment offsets. */
EngineResult physics_position_set(Entity entity, Position p);
PositionResult physics_position_get(Entity entity);

/**
 * @brief Selects explicit COM using a finite origin-relative offset.
 * Geometry is optional. Standalone particles reject overrides.
 * Soft bodies redistribute positive node masses at fixed total mass to the
 * requested COM, preserving their prior proportions as closely as possible.
 * Unreachable targets and numerical failures leave every node unchanged.
 * Failure preserves configuration; success preserves origin, geometry, and velocities.
 */
EngineResult physics_center_of_mass_local_position_set(Entity entity, Position local_offset);
/**
 * @brief Returns the effective COM in local origin axes.
 * Automatic COM requires valid active geometry; an explicit offset does not.
 * Standalone particles use their circle centroid.
 * Soft bodies derive COM from current node positions and masses; an empty
 * or zero-total-mass soft body has no available COM.
 */
PositionResult physics_center_of_mass_local_position_get(Entity entity);
/**
 * @brief Returns origin plus rotated local COM.
 * Requires a transform and an available local COM.
 */
PositionResult physics_center_of_mass_world_position_get(Entity entity);
/** Discards explicit COM for ordinary bodies. Soft bodies: successful no-op. */
EngineResult physics_center_of_mass_automatic_set(Entity entity);
/** Returns the COM mode (always true for soft bodies), or an invalid-entity error. */
BoolResult physics_center_of_mass_automatic_check(Entity entity);
/**
 * @brief Returns derived inertia about the effective COM.
 * Requires valid geometry and stored mass, including zero mass.
 * Static and locked bodies may report inertia without enabling angular response.
 */
MomentOfInertiaResult physics_moment_of_inertia_get(Entity entity);

/** Set finite non-negative ordinary mass. For soft bodies, proportionally
 * scale positive node masses to a positive total; failures preserve all nodes.
 * No ROHR_MASS component is added to the soft-body container. */
EngineResult physics_mass_set(Entity entity, Mass m);
/** Stored ordinary mass or summed soft-body node mass (zero when empty). */
MassResult physics_mass_get(Entity entity);
EngineResult physics_mass_remove(Entity entity);
bool physics_mass_check(Entity entity);
EngineResult physics_kinematic_driven_set(Entity entity);
EngineResult physics_kinematic_driven_remove(Entity entity);
bool physics_kinematic_driven_check(Entity entity);
/** Create a persistent force source. The caller owns its entity_delete lifetime. */
EntityResult physics_force_add(ForceDescription description);
EntityResult physics_torque_add(TorqueDescription description);

ForceDescriptionResult physics_force_description_get(Entity source);
TorqueDescriptionResult physics_torque_description_get(Entity source);
EngineResult physics_force_description_set(Entity source, ForceDescription description);
EngineResult physics_torque_description_set(Entity source, TorqueDescription description);

EngineResult physics_force_target_set(Entity source, Entity target);
EntityResult physics_force_target_get(Entity source);
EngineResult physics_force_location_set(Entity source, ForceLocation location);
ForceLocationResult physics_force_location_get(Entity source);
EngineResult physics_force_orientation_set(Entity source, ForceOrientation orientation);
ForceOrientationResult physics_force_orientation_get(Entity source);
EngineResult physics_force_magnitude_set(Entity source, float magnitude);
ForceMagnitudeResult physics_force_magnitude_get(Entity source);

EngineResult physics_torque_target_set(Entity source, Entity target);
EntityResult physics_torque_target_get(Entity source);
EngineResult physics_torque_magnitude_set(Entity source, Torque magnitude);
TorqueResult physics_torque_magnitude_get(Entity source);

/** One-shot apply requires an invalid owner; persistent owned sources use add. */
EngineResult physics_force_apply(ForceDescription description);
/** One-shot apply requires an invalid owner; persistent owned sources use add. */
EngineResult physics_torque_apply(TorqueDescription description);

/**
 * Set an entity's simple polygon hitbox and prepare any required convex
 * decomposition. This rejects degenerate and self-intersecting outlines and
 * does not enable physical collision response.
 */
EngineResult physics_hitbox_set(Entity entity, Shape hitbox);
ShapeResult physics_hitbox_get(Entity entity);
EngineResult physics_hitbox_remove(Entity entity);
EngineResult physics_hitbox_add(Entity entity, Shape hitbox);
ShapeResult physics_hitbox_at_get(Entity entity, size_t index);
EngineResult physics_hitbox_at_set(Entity entity, size_t index, Shape hitbox);
EngineResult physics_hitbox_at_remove(Entity entity, size_t index);
HitboxIndexResult physics_hitbox_count_get(Entity entity);
HitboxIndexResult physics_hitbox_active_index_get(Entity entity);
EngineResult physics_hitbox_active_index_set(Entity entity, size_t index);
HitboxIdResult physics_hitbox_id_at_get(Entity entity, size_t index);
EngineResult physics_hitbox_id_at_set(Entity entity, size_t index, HitboxId id);
ShapeResult physics_hitbox_by_id_get(Entity entity, HitboxId id);
EngineResult physics_hitbox_by_id_set(Entity entity, HitboxId id, Shape hitbox);
EngineResult physics_hitbox_by_id_remove(Entity entity, HitboxId id);
HitboxIdResult physics_hitbox_active_id_get(Entity entity);
EngineResult physics_hitbox_active_id_set(Entity entity, HitboxId id);
EngineResult physics_hitbox_animation_binding_set(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id, HitboxId hitbox_id);
EngineResult physics_hitbox_animation_binding_remove(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id);
HitboxIdResult physics_hitbox_animation_binding_get(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id);
EngineResult physics_hitbox_animation_binding_at_set(Entity entity,
    size_t frame_index, size_t hitbox_index);
EngineResult physics_hitbox_animation_binding_at_remove(Entity entity,
    size_t frame_index);
HitboxIndexResult physics_hitbox_animation_binding_at_get(Entity entity,
    size_t frame_index);
void physics_hitbox_animation_bindings_update(void);
void physics_hitbox_animation_binding_hitbox_remove(EntityIndex index,
    HitboxId hitbox_id);
void physics_hitbox_animation_bindings_entity_clear(EntityIndex index);
/** Return the default collision filter: default category against all categories. */
CollisionFilterDescription physics_collision_filter_description_default_get(void);
/** Replace an entity's complete collision filter, with or without geometry. */
EngineResult physics_collision_filter_description_set(Entity entity, CollisionFilterDescription config);
/** Return an entity's collision filter. Entities without an override use defaults. */
CollisionFilterDescriptionResult physics_collision_filter_description_get(Entity entity);
/** Set the categories represented by an entity. */
EngineResult physics_collision_category_set(Entity entity, RohrCollisionCategoryMask category);
/** Set the category whitelist an entity can collide with. */
EngineResult physics_collision_with_set(Entity entity, RohrCollisionCategoryMask categories);
/** Allow an entity to collide with every category. */
EngineResult physics_collision_with_all_set(Entity entity);
/** Prevent an entity from colliding with every category. */
EngineResult physics_collision_with_none_set(Entity entity);
/** Return whether two entities mutually permit collision checks. */
bool physics_collision_between_check(Entity entity_1, Entity entity_2);
/** Set an entity's orientation in clockwise degrees. */
EngineResult physics_orientation_set(Entity entity, Orientation angle);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult physics_orientation_radians_set(Entity entity, Orientation angle);
/** Set an entity's angular velocity. */
EngineResult physics_angular_velocity_set(Entity entity, AngularVelocity v);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult physics_angular_velocity_radians_set(Entity entity, AngularVelocity v);
/** Return an entity's angular velocity. */
AngularVelocityResult physics_angular_velocity_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
AngularVelocityResult physics_angular_velocity_radians_get(Entity entity);
EngineResult physics_angular_velocity_maximum_set(Entity entity, AngularVelocity maximum);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult physics_angular_velocity_maximum_radians_set(Entity entity, AngularVelocity maximum);
AngularVelocityResult physics_angular_velocity_maximum_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
AngularVelocityResult physics_angular_velocity_maximum_radians_get(Entity entity);
/** Return the active hitbox at the current origin and orientation. */
ShapeResult physics_global_hit_box_get(Entity entity);
/** Set an entity's collision restitution. */
EngineResult physics_restitution_set(Entity entity, Restitution restitution);
/** Mark an entity dynamic and remove ROHR_STATIC. */
EngineResult physics_dynamic_set(Entity entity);
/** Mark an entity static and remove ROHR_DYNAMIC. */
EngineResult physics_static_set(Entity entity);
/** Add ROHR_HOLD so physics update stages preserve current values. */
EngineResult physics_entity_hold(Entity entity);
/** Remove ROHR_HOLD without changing ROHR_STATIC or ROHR_DYNAMIC state. */
EngineResult physics_entity_unhold(Entity entity);
/** Add ROHR_HOLD to every live entity in a group. */
EngineResult physics_group_entities_hold(GroupId group);
/** Remove ROHR_HOLD from every live entity in a group. */
EngineResult physics_group_entities_unhold(GroupId group);
/** Add or update an angle lock constraint. */
EngineResult physics_angle_lock_set(Entity entity, Orientation min, Orientation max);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult physics_angle_lock_radians_set(Entity entity, Orientation min, Orientation max);
/** Atomically replace the axis constraint; finite nonzero axis required.
 * Stores a normalized axis without changing current motion or ownership. */
EngineResult physics_axis_lock_description_set(Entity entity, AxisLockDescription description);
/** Get stored configuration; returns component-missing when no axis lock exists. */
AxisLockDescriptionResult physics_axis_lock_description_get(Entity entity);
/** Remove the axis lock without changing current motion. Idempotent for live entities. */
EngineResult physics_axis_lock_remove(Entity entity);
/** Set an entity's friction value. */
EngineResult physics_friction_set(Entity entity, float friction);

/** Atomically replace transform-following configuration using clockwise degrees.
 * Requires a live driver even when all flags are false. Does not capture the
 * current transform, move entities, or alter ownership. Use explicit conversion
 * helpers when supplying radians. */
EngineResult physics_transform_lock_description_set(Entity driven, TransformLockDescription description);
/** Get stored configuration, including its driver handle; component-missing
 * when absent. A deleted driver can remain in the stored configuration. */
TransformLockDescriptionResult physics_transform_lock_description_get(Entity driven);

/** Remove a transform lock from an entity. */
EngineResult physics_transform_lock_remove(Entity entity);

/**
 * Add a transform lock using the current relative transform as the offset.
 */
EngineResult physics_transform_lock_current_transform_set(
        Entity driven,
        Entity driver,
        bool lock_position,
        bool lock_orientation,
        bool inherit_velocity
);

/** Set the target component used by force and torque source entities. */
EngineResult physics_target_set(Entity entity, Entity target);

/** Atomically replace configuration and owner; failure leaves both unchanged. */
EngineResult physics_joint_description_set(Entity entity, JointDescription description);
/** Return current owner and resolved rest values with both explicit flags true. */
JointDescriptionResult physics_joint_description_get(Entity entity);

/** Full replacements are atomic; ownership never follows connections. */
SoftBodyDescription physics_soft_body_description_default_get(void);
/** Atomically replace soft body configuration. */
EngineResult physics_soft_body_description_set(Entity entity, SoftBodyDescription description);
/** Return current soft body configuration. */
SoftBodyDescriptionResult physics_soft_body_description_get(Entity entity);
/** Full node descriptions use world positions; local creation remains explicit. */
SoftBodyNodeDescription physics_soft_body_node_description_default_get(void);
/** Atomically replace soft body node configuration. */
EngineResult physics_soft_body_node_description_set(Entity entity, SoftBodyNodeDescription description);
/** Return current soft body node configuration. */
SoftBodyNodeDescriptionResult physics_soft_body_node_description_get(Entity entity);
/** Return defaults for soft body beam configuration. */
SoftBodyBeamDescription physics_soft_body_beam_description_default_get(void);
/** Atomically replace soft body beam configuration. */
EngineResult physics_soft_body_beam_description_set(Entity entity, SoftBodyBeamDescription description);
/** Resolved rest length is returned with rest_length_explicit true. */
SoftBodyBeamDescriptionResult physics_soft_body_beam_description_get(Entity entity);
/** Return defaults for soft body beam collision configuration. */
SoftBodyBeamCollisionDescription physics_soft_body_beam_collision_description_default_get(void);
/** Return current soft body beam collision configuration. */
SoftBodyBeamCollisionDescriptionResult physics_soft_body_beam_collision_description_get(Entity entity);
/** Return defaults for soft body area configuration. */
SoftBodyAreaDescription physics_soft_body_area_description_default_get(void);
/** Atomically replace soft body area configuration. */
EngineResult physics_soft_body_area_description_set(Entity entity, SoftBodyAreaDescription description);
/** Return current soft body area configuration. */
SoftBodyAreaDescriptionResult physics_soft_body_area_description_get(Entity entity);
/** Reattach the same anchor handle and all its joint references atomically.
 * Keep joint owners and rest values; offsets are local to the new entity. */
EngineResult physics_joint_anchor_description_set(JointAnchorId anchor, JointAnchorDescription description);
/** Return current joint anchor configuration. */
JointAnchorDescriptionResult physics_joint_anchor_description_get(JointAnchorId anchor);

JointAnchorIdResult physics_joint_anchor_add(JointAnchorDescription description);
JointAnchorListResult physics_joint_anchors_get(Entity entity);
JointAnchorPositionResult physics_joint_anchor_local_position_get(JointAnchorId anchor);
JointAnchorPositionResult physics_joint_anchor_world_position_get(JointAnchorId anchor);
EngineResult physics_joint_anchor_local_position_set(JointAnchorId anchor, Vec2D local_offset);
EngineResult physics_joint_anchor_remove(JointAnchorId anchor);

EntityResult physics_soft_body_add(SoftBodyDescription description);
SoftBodyResult physics_soft_body_get(Entity soft_body);
EntityResult physics_soft_body_node_add(SoftBodyNodeDescription description);
EntityResult physics_soft_body_node_local_add(SoftBodyNodeDescription description);
SoftBodyNodeResult physics_soft_body_node_get(Entity node);
PositionResult physics_soft_body_node_local_position_get(Entity node);
EngineResult physics_soft_body_node_local_position_set(
    Entity node, Position local_position);
/** Atomically replace soft body node collision filter configuration. */
EngineResult physics_soft_body_node_collision_filter_description_set(
    Entity node, CollisionFilterDescription description);
/** Return current soft body node collision filter configuration. */
CollisionFilterDescriptionResult physics_soft_body_node_collision_filter_description_get(Entity node);
EngineResult physics_soft_body_node_impulse_apply(Entity node, Vec2D impulse);
SoftBodyNodeAnchorPinResult physics_soft_body_node_to_anchor_pin_add(SoftBodyNodeAnchorPinDescription description);
EntityResult physics_soft_body_beam_add(SoftBodyBeamDescription description);
SoftBodyBeamResult physics_soft_body_beam_get(Entity beam);
/** Replace endpoints; ENTITY_INVALID leaves that endpoint disconnected. */
EngineResult physics_soft_body_beam_endpoints_set(Entity beam, Entity node_a, Entity node_b);
/** Atomically replace soft body beam collision configuration. */
EngineResult physics_soft_body_beam_collision_description_set(
    Entity beam, SoftBodyBeamCollisionDescription config);
EngineResult physics_soft_body_beam_collision_enable(Entity beam);
EngineResult physics_soft_body_beam_collision_disable(Entity beam);
EngineResult physics_soft_body_beam_collision_thickness_set(
    Entity beam, float thickness);
/** Atomically replace soft body beam collision filter configuration. */
EngineResult physics_soft_body_beam_collision_filter_description_set(
    Entity beam, CollisionFilterDescription description);
/** Return current soft body beam collision filter configuration. */
CollisionFilterDescriptionResult physics_soft_body_beam_collision_filter_description_get(Entity beam);
EntityResult physics_soft_body_area_add(SoftBodyAreaDescription description);
SoftBodyAreaResult physics_soft_body_area_get(Entity area);
EngineResult physics_soft_body_area_geometry_set(Entity area, SoftBodyAreaGeometry geometry);
EngineResult physics_soft_body_area_order_set(Entity area, uint32_t index);

/**
 * Create a joint entity connecting two live entities.
 *
 * @return EntityResult containing the new joint entity, or an error.
 */
EntityResult physics_joint_add(JointDescription description);

/**
 * Run particle collision detection between two circle-like shapes.
 */
OverlapInfo physics_particle_overlap_get(Shape shape_1, Shape shape_2);
/** Return a valid standalone particle configuration with engine defaults. */
ParticleDescription physics_particle_description_default_get(void);
/** Create one translation-only particle entity from a reusable configuration. */
EntityResult physics_particle_add(ParticleDescription config);
/** Atomically replace a standalone particle's current configuration and owner.
 * Rebuild changed circle/polygon geometry together, retaining hitbox IDs and
 * inactive variants. Preserve unrelated state; omitted fields are zero/false. */
EngineResult physics_particle_description_set(Entity entity, ParticleDescription description);
/** Get current values, including stored static motion and active polygon quality.
 * Non-standalone particles and soft-body nodes return component-missing. */
ParticleDescriptionResult physics_particle_description_get(Entity entity);
/** Update circle origin and, for standalone particles, its active polygon too. */
EngineResult physics_particle_origin_set(Entity entity, Position local_origin);
PositionResult physics_particle_origin_get(Entity entity);
/** Update circle radius and, for standalone particles, its active polygon too. */
EngineResult physics_particle_radius_set(Entity entity, float radius);
ParticleRadiusResult physics_particle_radius_get(Entity entity);

/** Return whether two entities overlap during the current physics step. */
bool physics_overlap_check(Entity entity, Entity target);
/** Return current overlap geometry in the requested entity order. */
OverlapInfo physics_overlap_get(Entity entity, Entity target);
/** Return whether an overlap began during the current physics step. */
bool physics_overlap_entered_check(Entity entity, Entity target);
/** Return whether an overlap continued from the previous physics step. */
bool physics_overlap_stayed_check(Entity entity, Entity target);
/** Return whether an overlap ended during the current physics step. */
bool physics_overlap_exited_check(Entity entity, Entity target);
/** Return the number of current overlaps involving an entity. */
size_t physics_overlap_count_get(Entity entity);
/** Write up to capacity current overlaps and return the number written. */
size_t physics_overlaps_get(
    Entity entity,
    EntityInteraction *results,
    size_t capacity
);

/** Return whether two entities physically contacted during the current physics step. */
bool physics_contact_check(Entity entity, Entity target);
/** Return current contact geometry in the requested entity order. */
ContactInfo physics_contact_get(Entity entity, Entity target);
/** Return the sum of a contact's normal and friction impulses. */
Vec2D physics_contact_total_impulse_get(ContactInfo contact);
/** Return whether a physical contact began during the current physics step. */
bool physics_contact_entered_check(Entity entity, Entity target);
/** Return whether a physical contact continued from the previous physics step. */
bool physics_contact_stayed_check(Entity entity, Entity target);
/** Return whether a physical contact ended during the current physics step. */
bool physics_contact_exited_check(Entity entity, Entity target);
/** Return the number of current physical contacts involving an entity. */
size_t physics_contact_count_get(Entity entity);
/** Write up to capacity current contacts and return the number written. */
size_t physics_contacts_get(
    Entity entity,
    EntityContact *results,
    size_t capacity
);

/** Set an explicit simulation delta for each engine tick. */
EngineResult physics_dt_per_tick_set(Time dt);
/** Return the explicit delta or the engine time per tick when using defaults. */
Time physics_dt_per_tick_get(void);
/** Return physics timing to the engine time-per-tick default. */
void physics_engine_time_per_tick_use(void);
/** Advance physics for a number of elapsed engine ticks. */
Report physics_update(Tick ticks);
/** Advance physics once using an explicit exceptional delta. */
Report physics_dt_update(Time dt);
#endif
