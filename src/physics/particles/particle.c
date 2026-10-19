/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "entity/entity_internal.h"
#include "physics/physics_internal.h"

#include <math.h>
#include <stdlib.h>
#include "physics/physics_step_internal.h"
#include "physics/collision/shape_decomposition.h"

ParticleDescription physics_particle_description_default_get(void) {
    return (ParticleDescription){
        .radius = 1.0f,
        .rigid_vertices = ROHR_PARTICLE_RIGID_VERTICES_DEFAULT,
        .mass_value = 1.0f,
        .friction = 0.5f,
        .collision_filter = physics_collision_filter_description_default_get(),
        .collision_enabled = true
    };
}

/* A geometry replacement preserves variant IDs, inactive variants, and bindings.
 * Allocate a missing first variant before publishing any geometry or ownership. */
typedef struct ParticleShapeChange {
    Shape shape;
    HitboxVariant *allocated;
    bool replace;
} ParticleShapeChange;

static bool particle_standalone_check(EntityIndex index) {
    return entity_index_components_check(index, ROHR_PARTICLE) &&
        particle_geometries_pool.used[index] && particle_geometries_pool.objects[index].standalone &&
        !(entity_mask[index] & (ROHR_SOFT_BODY | ROHR_SOFT_BODY_NODE | ROHR_SOFT_BODY_BEAM | ROHR_SOFT_BODY_AREA));
}

static uint32_t particle_vertices_get(EntityIndex index) {
    if(hitbox_variants_pool.used[index]) {
        const HitboxVariantList *variants = &hitbox_variants_pool.objects[index];
        if(variants->values && variants->active_index < variants->count)
            return variants->values[variants->active_index].shape.amount_of_vertices;
    }
    return ROHR_PARTICLE_RIGID_VERTICES_DEFAULT;
}

static bool particle_description_check(ParticleDescription description) {
    uint32_t vertices = description.rigid_vertices ? description.rigid_vertices : ROHR_PARTICLE_RIGID_VERTICES_DEFAULT;
    return physics_world_position_check(description.position) && physics_world_position_check(description.local_origin) &&
        isfinite(description.velocity.x) && isfinite(description.velocity.y) &&
        isfinite(description.acceleration.x) && isfinite(description.acceleration.y) &&
        isfinite(description.radius) && description.radius > 0 && vertices >= MIN_VERTICIES && vertices <= MAX_VERTICIES &&
        isfinite(description.mass_value) && description.mass_value >= 0 &&
        isfinite(description.friction) && description.friction >= 0 &&
        isfinite(description.restitution) && description.restitution >= 0 && description.restitution <= 1;
}

static EngineResult particle_shape_prepare(EntityIndex index, Position origin, float radius,
        uint32_t vertices, ParticleShapeChange *change) {
    *change = (ParticleShapeChange){0};
    HitboxVariantList *variants = &hitbox_variants_pool.objects[index];
    bool existing = hitbox_variants_pool.used[index] && variants->count;
    if(existing && (!variants->values || variants->active_index >= variants->count))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    ParticleGeometry old = particle_geometries_pool.objects[index];
    change->replace = !existing || !(entity_mask[index] & ROHR_HIT_BOX) ||
        !particle_geometries_pool.used[index] || !old.origin_explicit || !old.radius_explicit ||
        origin.x != old.local_origin.x || origin.y != old.local_origin.y || radius != old.radius ||
        vertices != particle_vertices_get(index);
    if(!change->replace) return error_result_value(true);
    Shape shape = math_circle_create(radius, (uint8_t)vertices);
    for(uint16_t i = 0; i < shape.amount_of_vertices; ++i) {
        shape.vertices[i].x += origin.x;
        shape.vertices[i].y += origin.y;
    }
    if(!physics_shape_collision_prepare(shape, &change->shape))
        return error_result_error(ERROR_ENGINE_INVALID_SHAPE);
    if(!existing && (!hitbox_variants_pool.used[index] || !variants->values || !variants->capacity)) {
        change->allocated = malloc(sizeof(*change->allocated));
        if(!change->allocated) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    return error_result_value(true);
}

static void particle_shape_commit(EntityIndex index, Position origin, float radius,
        ParticleShapeChange *change) {
    (void)ParticleGeometryPool_store_at(&particle_geometries_pool, index, (ParticleGeometry){
        .local_origin = origin, .radius = radius, .origin_explicit = true,
        .radius_explicit = true, .standalone = true});
    entity_mask[index] |= ROHR_PARTICLE;
    if(!change->replace) return;
    HitboxVariantList *variants = &hitbox_variants_pool.objects[index];
    if(!hitbox_variants_pool.used[index]) {
        (void)HitboxVariantListPool_store_at(&hitbox_variants_pool, index, (HitboxVariantList){.next_id = 1});
    }
    if(change->allocated) {
        free(variants->values);
        variants->values = change->allocated;
        variants->capacity = 1;
        change->allocated = NULL;
    }
    if(!variants->count) {
        if(!variants->next_id) variants->next_id = 1;
        variants->values[0].id = variants->next_id++;
        variants->count = 1;
        variants->active_index = 0;
    }
    variants->values[variants->active_index].shape = change->shape;
    /* Entity creation reserves all indexed pools; these stores cannot allocate. */
    (void)ShapePool_store_at(&hit_boxes_pool, index, change->shape);
    (void)ShapePool_store_at(&world_hit_boxes_pool, index, change->shape);
    entity_mask[index] |= ROHR_HIT_BOX;
    physics_mass_properties_geometry_set(index, &change->shape);
    physics_step_hitbox_dirty_add(index);
}

static EngineResult particle_description_apply(Entity entity, EntityIndex index, ParticleDescription description) {
    if(!particle_description_check(description)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    ParticleShapeChange change;
    uint32_t vertices = description.rigid_vertices ? description.rigid_vertices : ROHR_PARTICLE_RIGID_VERTICES_DEFAULT;
    EngineResult result = particle_shape_prepare(index, description.local_origin, description.radius, vertices, &change);
    if(error_check(result)) return result;
    result = entity_owner_set(entity, description.owner);
    if(error_check(result)) { free(change.allocated); return result; }
    /* No fallible operations remain after ownership has committed. */
    particle_shape_commit(index, description.local_origin, description.radius, &change);
    (void)PositionPool_store_at(&positions_pool, index, description.position);
    (void)VelocityPool_store_at(&velocities_pool, index, description.velocity);
    (void)AccelerationPool_store_at(&accelerations_pool, index, description.acceleration);
    (void)MassPool_store_at(&mass_pool, index, description.mass_value);
    (void)FrictionPool_store_at(&frictions_pool, index, description.friction);
    (void)RestitutionPool_store_at(&restitutions_pool, index, description.restitution);
    (void)CollisionFilterDescriptionPool_store_at(&collision_filters_pool, index, description.collision_filter);
    entity_mask[index] &= ~(ROHR_STATIC | ROHR_DYNAMIC | ROHR_GRAVITY | ROHR_COLLISION);
    entity_mask[index] |= ROHR_MASS | ROHR_COLLISION_FILTER | (description.static_body ? ROHR_STATIC : ROHR_DYNAMIC);
    if(description.gravity_enabled) entity_mask[index] |= ROHR_GRAVITY;
    if(description.collision_enabled) entity_mask[index] |= ROHR_COLLISION;
    physics_body_state_sync(index);
    return error_result_value(true);
}

EntityResult physics_particle_add(ParticleDescription description) {
    if(!particle_description_check(description))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    if(description.owner != ENTITY_INVALID && !entity_alive_check(description.owner))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_INVALID_ENTITY);
    EntityResult added = entity_default_create("particle", ENTITY_INVALID);
    if(error_check(added)) return added;
    EntityIndex index;
    (void)entity_index_get(added.result.value, &index);
    EngineResult result = particle_description_apply(added.result.value, index, description);
    if(error_check(result)) {
        (void)entity_delete(added.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    (void)OrientationPool_store_at(&orientations_pool, index, 0);
    return added;
}

EngineResult physics_particle_description_set(Entity entity, ParticleDescription description) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return result;
    if(!particle_standalone_check(index)) return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    return particle_description_apply(entity, index, description);
}

ParticleDescriptionResult physics_particle_description_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ParticleDescriptionResult, result.result.error);
    if(!particle_standalone_check(index))
        return ERROR_RESULT_MAKE_ERROR(ParticleDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    EntityResult owner = entity_owner_get(entity);
    if(error_check(owner)) return ERROR_RESULT_MAKE_ERROR(ParticleDescriptionResult, owner.result.error);
    CollisionFilterDescriptionResult filter = physics_collision_filter_description_get(entity);
    if(error_check(filter)) return ERROR_RESULT_MAKE_ERROR(ParticleDescriptionResult, filter.result.error);
    ParticleGeometry geometry = particle_geometries_pool.objects[index];
    ParticleDescription description = {
        .owner = owner.result.value,
        .position = positions_pool.used[index] ? positions[index] : (Position){0},
        .local_origin = geometry.local_origin,
        .velocity = velocities_pool.used[index] ? velocities[index] : (Velocity){0},
        .acceleration = accelerations_pool.used[index] ? accelerations[index] : (Acceleration){0},
        .radius = geometry.radius, .rigid_vertices = particle_vertices_get(index),
        .mass_value = entity_index_components_check(index, ROHR_MASS) && mass_pool.used[index] ? mass[index] : 0,
        .friction = frictions_pool.used[index] ? frictions[index] : 0,
        .restitution = restitutions_pool.used[index] ? restitutions[index] : 0,
        .collision_filter = filter.result.value,
        .static_body = entity_index_components_check(index, ROHR_STATIC),
        .gravity_enabled = entity_index_components_check(index, ROHR_GRAVITY),
        .collision_enabled = entity_index_components_check(index, ROHR_COLLISION)
    };
    return ERROR_RESULT_MAKE_VALUE(ParticleDescriptionResult, description);
}

static EngineResult particle_geometry_set(EntityIndex index, Position origin, float radius) {
    ParticleShapeChange change;
    EngineResult result = particle_shape_prepare(index, origin, radius, particle_vertices_get(index), &change);
    if(error_check(result)) return result;
    particle_shape_commit(index, origin, radius, &change);
    return error_result_value(true);
}

static ParticleGeometry physics_particle_geometry_effective_get(EntityIndex index) {
    ParticleGeometry geometry = {0};

    if(index < particle_geometries_pool.capacity &&
            particle_geometries_pool.used[index])
        geometry = particle_geometries_pool.objects[index];
    if(!geometry.radius_explicit && index < hit_boxes_pool.capacity &&
            hit_boxes_pool.used[index]) {
        Position center = math_polygon_centroid(hit_boxes[index]);
        geometry.radius = math_circle_radius(hit_boxes[index], center);
    }
    return geometry;
}

Position physics_particle_world_origin_by_index_get(EntityIndex index) {
    ParticleGeometry geometry = physics_particle_geometry_effective_get(index);
    float angle = index < orientations_pool.capacity && orientations_pool.used[index]
        ? orientations[index] : 0.0f;
    Position body = index < positions_pool.capacity && positions_pool.used[index]
        ? positions[index] : (Position){0};
    float cosine = cosf(math_degrees_to_radians(angle));
    float sine = -sinf(math_degrees_to_radians(angle));

    return (Position){
        body.x + geometry.local_origin.x * cosine - geometry.local_origin.y * sine,
        body.y + geometry.local_origin.x * sine + geometry.local_origin.y * cosine
    };
}

float physics_particle_radius_by_index_get(EntityIndex index) {
    return physics_particle_geometry_effective_get(index).radius;
}

EngineResult physics_particle_origin_set(Entity entity, Position local_origin) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    ParticleGeometry geometry;

    if(error_check(result)) return result;
    if(!physics_world_position_check(local_origin))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    geometry = physics_particle_geometry_effective_get(index);
    if(particle_standalone_check(index))
        return particle_geometry_set(index, local_origin, geometry.radius);
    geometry.local_origin = local_origin;
    geometry.origin_explicit = true;
    (void)ParticleGeometryPool_store_at(
        &particle_geometries_pool, index, geometry);
    entity_mask[index] |= ROHR_PARTICLE;
    return error_result_value(true);
}

PositionResult physics_particle_origin_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(error_check(result))
        return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_PARTICLE))
        return ERROR_RESULT_MAKE_ERROR(
            PositionResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(PositionResult,
        physics_particle_geometry_effective_get(index).local_origin);
}

EngineResult physics_particle_radius_set(Entity entity, float radius) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    ParticleGeometry geometry;

    if(error_check(result)) return result;
    if(!isfinite(radius) || radius <= 0.0f)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    geometry = physics_particle_geometry_effective_get(index);
    if(particle_standalone_check(index))
        return particle_geometry_set(index, geometry.local_origin, radius);
    geometry.radius = radius;
    geometry.radius_explicit = true;
    if(ParticleGeometryPool_store_at(
            &particle_geometries_pool, index, geometry).kind == ERROR_RESULT_ERROR)
        return error_result_error(ERROR_ENGINE_TABLE_EXPANSION_FAILED);
    entity_mask[index] |= ROHR_PARTICLE;
    if(index < soft_body_nodes_pool.capacity && soft_body_nodes_pool.used[index]) {
        soft_body_nodes[index].radius = radius;
        for(EntityIndex beam = 0; beam < soft_body_beams_pool.capacity; beam += 1) {
            EntityIndex other;
            float maximum;
            if(!soft_body_beams_pool.used[beam] ||
                    (soft_body_beams[beam].node_a != entity &&
                        soft_body_beams[beam].node_b != entity)) continue;
            if(!entity_index_get(soft_body_beams[beam].node_a == entity ?
                    soft_body_beams[beam].node_b : soft_body_beams[beam].node_a,
                    &other) || !soft_body_nodes_pool.used[other]) continue;
            maximum = 2.0f * fminf(radius, soft_body_nodes[other].radius);
            if(maximum < ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN) {
                soft_body_beams[beam].collision_enabled = false;
                soft_body_beams[beam].collision_thickness =
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN;
            } else if(soft_body_beams[beam].collision_thickness > maximum)
                soft_body_beams[beam].collision_thickness = maximum;
        }
    }
    return error_result_value(true);
}

ParticleRadiusResult physics_particle_radius_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(error_check(result))
        return ERROR_RESULT_MAKE_ERROR(
            ParticleRadiusResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_PARTICLE))
        return ERROR_RESULT_MAKE_ERROR(
            ParticleRadiusResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ParticleRadiusResult,
        physics_particle_radius_by_index_get(index));
}

static OverlapInfo physics_particle_values_overlap_get(
    Position first_center,
    float first_radius,
    Position second_center,
    float second_radius
) {
    Vec2D delta = math_vector_subtract(second_center, first_center);
    float distance_squared = math_dot_product(delta, delta);
    float radius_sum = first_radius + second_radius;
    float distance;
    Vec2D normal;

    if(distance_squared >= radius_sum * radius_sum)
        return (OverlapInfo){.detected = false};
    distance = sqrtf(distance_squared);
    normal = distance > 0.00001f
        ? (Vec2D){delta.x / distance, delta.y / distance}
        : (Vec2D){1.0f, 0.0f};
    return (OverlapInfo){
        .detected = true,
        .normal = normal,
        .depth = radius_sum - distance
    };
}

OverlapInfo physics_particle_entities_overlap_get(
    EntityIndex first,
    EntityIndex second
) {
    return physics_particle_values_overlap_get(
        physics_particle_world_origin_by_index_get(first),
        physics_particle_radius_by_index_get(first),
        physics_particle_world_origin_by_index_get(second),
        physics_particle_radius_by_index_get(second));
}

OverlapInfo physics_particle_overlap_get(Shape first, Shape second) {
    Position first_center = math_polygon_centroid(first);
    Position second_center = math_polygon_centroid(second);
    float first_radius = math_circle_radius(first, first_center);
    float second_radius = math_circle_radius(second, second_center);
    return physics_particle_values_overlap_get(
        first_center, first_radius, second_center, second_radius);
}

Vec1D physics_circle_moment_of_inertia(Shape circle, Mass mass_value) {
    Vec1D radius = math_circle_radius(circle, math_polygon_centroid(circle));
    Vec1D area = PI_F * radius * radius;
    Vec1D density = mass_value / fabsf(area);
    Vec1D area_moment = 0.5f * area * radius * radius;

    return density * area_moment;
}
