/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>
#include <stdio.h>

static bool close_float(float first, float second) {
    return fabsf(first - second) < 0.0001f;
}

static bool particle_config_test(void) {
    ParticleConfig defaults = rohr_physics_particle_config_default_get();
    ParticleConfig first_config = defaults;
    ParticleConfig second_config = defaults;
    EntityResult first_result;
    EntityResult second_result;
    ShapeResult first_hitbox;
    ShapeResult second_hitbox;
    PositionResult first_position;
    ParticleRadiusResult first_radius;
    PositionResult second_origin;
    CollisionFilterConfigResult second_filter;

    if(defaults.radius != 1.0f ||
            defaults.rigid_vertices != ROHR_PARTICLE_RIGID_VERTICES_DEFAULT ||
            defaults.mass_value != 1.0f || defaults.friction != 0.5f ||
            defaults.restitution != 0.0f || !defaults.collision_enabled ||
            defaults.gravity_enabled ||
            defaults.collision_filter.category !=
                ROHR_COLLISION_CATEGORY_DEFAULT ||
            defaults.collision_filter.collides_with !=
                ROHR_COLLISION_CATEGORY_ALL) return false;
    first_config.position = (Position){12.0f, -8.0f};
    first_config.velocity = (Velocity){3.0f, 4.0f};
    first_config.radius = 2.5f;
    first_config.rigid_vertices = 0;
    first_config.gravity_enabled = true;
    first_result = rohr_physics_particle_create(first_config);
    if(rohr_error_check(first_result)) return false;
    second_config.position = (Position){-20.0f, 6.0f};
    second_config.local_origin = (Position){1.5f, -2.0f};
    second_config.radius = 4.0f;
    second_config.rigid_vertices = 7;
    second_config.static_body = true;
    second_config.collision_enabled = false;
    second_config.collision_filter = (CollisionFilterConfig){
        .category = UINT64_C(4), .collides_with = UINT64_C(2)};
    second_result = rohr_physics_particle_create(second_config);
    if(rohr_error_check(second_result)) {
        (void)rohr_entity_delete(first_result.result.value);
        return false;
    }
    first_hitbox = rohr_physics_hitbox_get(first_result.result.value);
    second_hitbox = rohr_physics_hitbox_get(second_result.result.value);
    first_position = rohr_physics_position_get(first_result.result.value);
    first_radius = rohr_physics_particle_radius_get(first_result.result.value);
    second_origin = rohr_physics_particle_origin_get(second_result.result.value);
    second_filter = rohr_physics_collision_filter_get(second_result.result.value);
    if(rohr_error_check(first_hitbox) || rohr_error_check(second_hitbox) ||
            rohr_error_check(first_position) || rohr_error_check(first_radius) ||
            rohr_error_check(second_origin) || rohr_error_check(second_filter) ||
            first_hitbox.result.value.amount_of_vertices !=
                ROHR_PARTICLE_RIGID_VERTICES_DEFAULT ||
            second_hitbox.result.value.amount_of_vertices != 7 ||
            !close_float(first_hitbox.result.value.vertices[0].x, 2.5f) ||
            !close_float(second_hitbox.result.value.vertices[0].x, 5.5f) ||
            !close_float(first_position.result.value.x, 12.0f) ||
            !close_float(first_position.result.value.y, -8.0f) ||
            !close_float(first_radius.result.value, 2.5f) ||
            !close_float(second_origin.result.value.x, 1.5f) ||
            !close_float(second_origin.result.value.y, -2.0f) ||
            !rohr_entity_components_check(first_result.result.value,
                ROHR_PARTICLE | ROHR_HIT_BOX | ROHR_DYNAMIC | ROHR_MASS |
                    ROHR_COLLISION | ROHR_GRAVITY) ||
            !rohr_entity_components_check(second_result.result.value,
                ROHR_PARTICLE | ROHR_HIT_BOX | ROHR_STATIC) ||
            rohr_entity_components_check(second_result.result.value,
                ROHR_DYNAMIC) ||
            rohr_entity_components_check(second_result.result.value,
                ROHR_MASS) ||
            rohr_entity_components_check(second_result.result.value,
                ROHR_COLLISION) ||
            rohr_entity_components_check(second_result.result.value,
                ROHR_GRAVITY) ||
            second_filter.result.value.category != UINT64_C(4) ||
            second_filter.result.value.collides_with != UINT64_C(2)) {
        (void)rohr_entity_delete(first_result.result.value);
        (void)rohr_entity_delete(second_result.result.value);
        return false;
    }
    first_config.radius = -1.0f;
    if(!rohr_error_check(rohr_physics_particle_create(first_config))) {
        (void)rohr_entity_delete(first_result.result.value);
        (void)rohr_entity_delete(second_result.result.value);
        return false;
    }
    return !rohr_error_check(rohr_entity_delete(first_result.result.value)) &&
        !rohr_error_check(rohr_entity_delete(second_result.result.value));
}

static Shape lower_triangle_get(void) {
    return (Shape){
        .amount_of_vertices = 3,
        .vertices = {{0.0f, 0.0f}, {2.0f, 0.0f}, {0.0f, 2.0f}}
    };
}

static Shape upper_triangle_get(void) {
    return (Shape){
        .amount_of_vertices = 3,
        .vertices = {{2.0f, 2.0f}, {2.0f, 0.5f}, {0.5f, 2.0f}}
    };
}

int main(void) {
    EntityResult first_result;
    EntityResult second_result;
    Entity first;
    Entity second;
    PositionResult particle_origin;
    ParticleRadiusResult particle_radius;
    HitboxIndexResult hitbox_index;
    HitboxIndexResult hitbox_count;

    if(rohr_physics_sat_overlap_get(
            lower_triangle_get(), upper_triangle_get()).detected) {
        fprintf(stderr, "test polygons unexpectedly overlap\n");
        return 1;
    }
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(!particle_config_test()) goto fail;
    first_result = rohr_entity_add();
    second_result = rohr_entity_add();
    if(rohr_error_check(first_result) || rohr_error_check(second_result)) goto fail;
    first = first_result.result.value;
    second = second_result.result.value;
    if(rohr_error_check(rohr_physics_hitbox_set(first, lower_triangle_get())) ||
            rohr_error_check(rohr_physics_hitbox_set(second, upper_triangle_get())) ||
            rohr_error_check(rohr_physics_position_set(
                first, (Position){0})) ||
            rohr_error_check(rohr_physics_position_set(
                second, (Position){0})) ||
            rohr_error_check(rohr_physics_static_set(first)) ||
            rohr_error_check(rohr_physics_static_set(second)) ||
            rohr_error_check(rohr_physics_particle_origin_set(
                first, (Position){0.1f, -0.2f})) ||
            rohr_error_check(rohr_physics_particle_radius_set(first, 1.5f)))
        goto fail;
    if(rohr_error_check(rohr_physics_hitbox_add(first, upper_triangle_get())))
        goto fail;
    hitbox_count = rohr_physics_hitbox_count_get(first);
    hitbox_index = rohr_physics_hitbox_active_index_get(first);
    if(rohr_error_check(hitbox_count) || hitbox_count.result.value != 2 ||
            rohr_error_check(hitbox_index) || hitbox_index.result.value != 0 ||
            rohr_error_check(rohr_physics_hitbox_active_index_set(first, 1)) ||
            rohr_error_check(rohr_physics_hitbox_at_remove(first, 0))) goto fail;
    hitbox_count = rohr_physics_hitbox_count_get(first);
    hitbox_index = rohr_physics_hitbox_active_index_get(first);
    if(rohr_error_check(hitbox_count) || hitbox_count.result.value != 1 ||
            rohr_error_check(hitbox_index) || hitbox_index.result.value != 0 ||
            rohr_error_check(rohr_physics_hitbox_set(first,
                lower_triangle_get()))) goto fail;
    particle_origin = rohr_physics_particle_origin_get(first);
    particle_radius = rohr_physics_particle_radius_get(first);
    if(rohr_error_check(particle_origin) || rohr_error_check(particle_radius) ||
            particle_origin.result.value.x != 0.1f ||
            particle_origin.result.value.y != -0.2f ||
            particle_radius.result.value != 1.5f) goto fail;

    rohr_system_physics_update(0.0);
    if(rohr_physics_overlap_check(first, second)) {
        ShapeResult first_shape = rohr_physics_global_hit_box_get(first);
        ShapeResult second_shape = rohr_physics_global_hit_box_get(second);
        fprintf(stderr, "mixed particle/rigid pair used circle overlap: second_particle=%d\n",
            rohr_entity_components_check(second, ROHR_PARTICLE));
        if(!rohr_error_check(first_shape) && !rohr_error_check(second_shape))
            fprintf(stderr, "global sat=%d particle=%d\n",
                rohr_physics_sat_overlap_get(first_shape.result.value,
                    second_shape.result.value).detected,
                rohr_physics_particle_overlap_get(first_shape.result.value,
                    second_shape.result.value).detected);
        goto fail;
    }

    if(rohr_error_check(rohr_entity_components_add(second, ROHR_PARTICLE)))
        goto fail;
    if(rohr_error_check(rohr_physics_particle_radius_set(second, 1.5f)) ||
            rohr_error_check(rohr_physics_position_set(
                second, (Position){10.0f, 1.5f})) ||
            rohr_error_check(rohr_physics_particle_origin_set(
                second, (Position){-8.0f, 0.0f}))) goto fail;
    rohr_system_physics_update(0.0);
    if(!rohr_physics_overlap_check(first, second)) {
        fprintf(stderr, "particle pair did not use circle overlap\n");
        goto fail;
    }
    if(rohr_error_check(rohr_physics_hitbox_at_remove(second, 0)) ||
            rohr_entity_components_check(second, ROHR_HIT_BOX)) {
        fprintf(stderr, "removing final variant retained hitbox component\n");
        goto fail;
    }

    rohr_engine_stop();
    return 0;

fail:
    rohr_engine_stop();
    return 1;
}
