/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_PHYSICS_INTERNAL_H
#define ROHR_PHYSICS_INTERNAL_H

#include "physics.h"

typedef struct ParticleShapeChange {
    Shape shape;
    HitboxVariant *allocated;
    bool replace;
} ParticleShapeChange;

/* Prepare may allocate; free change.allocated on rollback. Commit cannot fail. */
EngineResult physics_particle_shape_prepare(EntityIndex index, Position origin, float radius,
    uint32_t vertices, ParticleShapeChange *change);
void physics_particle_shape_commit(EntityIndex index, Position origin, float radius,
    ParticleShapeChange *change, bool standalone);

void physics_tables_destroy(void);
EngineResult physics_mass_properties_reserve(size_t capacity);
void physics_mass_properties_destroy(void);
void physics_mass_properties_clear(EntityIndex index);
bool physics_shape_mass_properties_get(const Shape *shape, Position *centroid,
    double *unit_inertia);
void physics_mass_properties_geometry_set(EntityIndex index, const Shape *shape);
Position physics_com_local_by_index_get(EntityIndex index);
Position physics_com_world_by_index_get(EntityIndex index);
float physics_inverse_inertia_by_index_get(EntityIndex index);
Velocity physics_point_velocity_by_index_get(EntityIndex index, Position point);
EngineResult physics_live_index_get(Entity entity, EntityIndex *index);
typedef EngineResult (*PhysicsGroupEntityFn)(Entity entity);
typedef EngineResult (*PhysicsGroupEntityTargetFn)(
    Entity entity,
    float magnitude,
    Entity target
);
EngineResult physics_group_entity_apply(GroupId group, PhysicsGroupEntityFn fn);
EngineResult physics_group_entity_target_apply(
    GroupId group,
    float magnitude,
    Entity target,
    PhysicsGroupEntityTargetFn fn
);
Vec2D physics_direction_between_positions(Position from, Position to);
void physics_body_state_table_init(void);
void physics_body_state_entity_clear(EntityIndex index);
void physics_body_state_sync(EntityIndex index);
/* Stored intent, distinct from the mass-dependent derived component flag. */
bool physics_kinematic_explicit_check(EntityIndex index);
void physics_force_state_init(void);
/* Read-only marker geometry; never evaluates or applies a simulation load. */
bool physics_source_geometry_get(Entity source, bool torque, Position *point,
    Vec2D *direction, float *magnitude);
void physics_sources_apply(Report *report, bool torque);
ForceDescription physics_force_vector_description(Entity target, Force vector);
bool physics_entity_nonrotating_check(EntityIndex index);
/* Validated raw restore for saved solver state; never accepts body-local attachments. */
EngineResult physics_joint_component_restore(Entity entity, Joint joint);
void physics_joint_state_init(void);
void physics_joint_entity_clear(Entity entity, EntityIndex index);
void physics_soft_body_entity_clear(Entity entity, EntityIndex index);
/* Validate before ownership mutation; commit updates only derived membership. */
EngineResult physics_soft_body_membership_validate(EntityIndex index, Entity owner);
void physics_soft_body_membership_update(Entity entity, EntityIndex index, Entity owner);
EngineResult physics_soft_body_origin_position_set(
    EntityIndex body_index, Position position);
EngineResult physics_soft_body_origin_orientation_set(
    EntityIndex body_index, Orientation orientation);
EngineResult physics_interaction_state_init(void);
void physics_interaction_state_destroy(void);
void physics_config_init(void);
Position physics_particle_world_origin_by_index_get(EntityIndex index);
float physics_particle_radius_by_index_get(EntityIndex index);
OverlapInfo physics_particle_entities_overlap_get(
    EntityIndex first,
    EntityIndex second
);

#endif
