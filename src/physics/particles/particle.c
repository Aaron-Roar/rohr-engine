/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "physics/physics_internal.h"

#include <math.h>

ParticleConfig physics_particle_config_default_get(void) {
    return (ParticleConfig){
        .radius = 1.0f,
        .rigid_vertices = ROHR_PARTICLE_RIGID_VERTICES_DEFAULT,
        .mass_value = 1.0f,
        .friction = 0.5f,
        .collision_filter = physics_collision_filter_config_default_get(),
        .collision_enabled = true
    };
}

EntityResult physics_particle_create(ParticleConfig config) {
    uint32_t rigid_vertices = config.rigid_vertices == 0 ?
        ROHR_PARTICLE_RIGID_VERTICES_DEFAULT : config.rigid_vertices;
    Shape hitbox;
    EntityResult added;
    EngineResult result;
    Entity particle;

    if(!physics_world_position_check(config.position) ||
            !physics_world_position_check(config.local_origin) ||
            !isfinite(config.velocity.x) || !isfinite(config.velocity.y) ||
            !isfinite(config.acceleration.x) ||
            !isfinite(config.acceleration.y) ||
            !isfinite(config.radius) || config.radius <= 0.0f ||
            rigid_vertices < MIN_VERTICIES || rigid_vertices > MAX_VERTICIES ||
            !isfinite(config.mass_value) || config.mass_value < 0.0f ||
            !isfinite(config.friction) || config.friction < 0.0f ||
            !isfinite(config.restitution) || config.restitution < 0.0f ||
            config.restitution > 1.0f)
        return ERROR_RESULT_MAKE_ERROR(
            EntityResult, ERROR_ENGINE_STATE_INVALID);
    added = entity_add();
    if(error_check(added)) return added;
    particle = added.result.value;
    hitbox = math_circle_create(config.radius, (uint8_t)rigid_vertices);
    for(uint16_t i = 0; i < hitbox.amount_of_vertices; i += 1) {
        hitbox.vertices[i].x += config.local_origin.x;
        hitbox.vertices[i].y += config.local_origin.y;
    }
#define PARTICLE_APPLY(call) do { \
    result = (call); \
    if(error_check(result)) goto fail; \
} while(0)
    PARTICLE_APPLY(physics_position_set(particle, config.position));
    PARTICLE_APPLY(physics_hitbox_set(particle, hitbox));
    PARTICLE_APPLY(physics_particle_origin_set(
        particle, config.local_origin));
    PARTICLE_APPLY(physics_particle_radius_set(particle, config.radius));
    if(config.static_body)
        PARTICLE_APPLY(physics_static_set(particle));
    else {
        PARTICLE_APPLY(physics_mass_set(particle, config.mass_value));
        PARTICLE_APPLY(physics_velocity_set(particle, config.velocity));
        PARTICLE_APPLY(physics_acceleration_set(particle, config.acceleration));
        PARTICLE_APPLY(physics_dynamic_set(particle));
        if(config.gravity_enabled)
            PARTICLE_APPLY(physics_gravity_enable(particle));
    }
    PARTICLE_APPLY(physics_friction_set(particle, config.friction));
    PARTICLE_APPLY(physics_restitution_set(particle, config.restitution));
    PARTICLE_APPLY(physics_collision_filter_set(
        particle, config.collision_filter));
    if(!config.collision_enabled)
        PARTICLE_APPLY(entity_components_delete(particle, ROHR_COLLISION));
#undef PARTICLE_APPLY
    return added;

fail:
#undef PARTICLE_APPLY
    (void)entity_delete(particle);
    return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
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
    float cosine = cosf(angle);
    float sine = sinf(angle);

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
