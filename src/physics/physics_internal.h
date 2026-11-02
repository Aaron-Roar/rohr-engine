/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_PHYSICS_INTERNAL_H
#define ROHR_PHYSICS_INTERNAL_H

#include "physics.h"

/* Membership is frozen only inside a synchronous complete substep. Component
 * values remain live; manual stages use the ordinary entity validation path. */
typedef struct PhysicsStepMembership {
    bool active;
    Entity handles[MAX_ENTITIES];
    EntityIndex colliders[MAX_ENTITIES];
    uint32_t collider_count;
} PhysicsStepMembership;
extern PhysicsStepMembership physics_step_membership;

static inline bool physics_step_entity_alive_check(EntityIndex index) {
    if(physics_step_membership.active)
        return index < MAX_ENTITIES &&
            physics_step_membership.handles[index] != ENTITY_INVALID;
    return entity_index_alive_check(index);
}

static inline bool physics_step_entity_components_check(EntityIndex index,
        RohrComponentMask components) {
    return physics_step_entity_alive_check(index) &&
        (entity_mask[index] & components) == components;
}

/* Solver paths already hold indices. Read current filters without converting
 * them back to public handles; beams own their filter in their beam component. */
static inline bool physics_collision_filter_by_index_get(EntityIndex index,
        CollisionFilterDescription *filter) {
    if(filter == NULL || !physics_step_entity_alive_check(index)) return false;
    RohrComponentMask mask = entity_mask[index];
    if((mask & ROHR_SOFT_BODY_BEAM) != 0) {
        if(index >= soft_body_beams_pool.capacity || !soft_body_beams_pool.used[index]) return false;
        *filter = (CollisionFilterDescription){soft_body_beams[index].category,
            soft_body_beams[index].responds_to};
    } else if((mask & ROHR_COLLISION_FILTER) != 0) {
        *filter = collision_filters[index];
    } else {
        *filter = (CollisionFilterDescription){ROHR_COLLISION_CATEGORY_DEFAULT,
            ROHR_COLLISION_CATEGORY_ALL};
    }
    return true;
}

static inline bool physics_collision_response_to_by_index_check(EntityIndex first,
        EntityIndex second) {
    CollisionFilterDescription a, b;
    return physics_collision_filter_by_index_get(first, &a) &&
        physics_collision_filter_by_index_get(second, &b) &&
        (a.responds_to & b.category) != 0;
}

static inline bool physics_collision_response_between_by_index_check(EntityIndex first,
        EntityIndex second) {
    CollisionFilterDescription a, b;
    return physics_collision_filter_by_index_get(first, &a) &&
        physics_collision_filter_by_index_get(second, &b) &&
        ((a.responds_to & b.category) != 0 || (b.responds_to & a.category) != 0);
}

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
