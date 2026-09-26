/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <stdio.h>
#include <math.h>
#include "rohr.h"

#define PRINT_ENGINE_ERROR(result_value) \
    fprintf(stderr, "error %d: %s\n", (int)(result_value).result.error, \
        rohr_error_message_get(result_value))

static bool center_of_mass_check(Entity entity) {
    PositionResult local = rohr_physics_center_of_mass_local_position_get(entity);
    MomentOfInertiaResult inertia = rohr_physics_moment_of_inertia_get(entity);
    ShapeResult shape = rohr_physics_hitbox_get(entity);
    return !rohr_error_check(local) && !rohr_error_check(inertia) &&
        !rohr_error_check(shape) && fabsf(local.result.value.x - 3) < 0.001f &&
        fabsf(local.result.value.y - 1) < 0.001f &&
        fabsf(inertia.result.value - 146.66667f) < 0.001f &&
        shape.result.value.vertices[0].x == 0 && shape.result.value.vertices[0].y == 0;
}

static bool angular_state_check(Entity entity) {
    EntityIndexResult index = rohr_entity_index_get(entity);
    AngularVelocityResult rate = rohr_physics_angular_velocity_radians_get(entity);
    return !rohr_error_check(index) && !rohr_error_check(rate) &&
        fabsf(rohr_math_degrees_to_radians(orientations[index.result.value]) + 0.35f) < 0.0001f &&
        fabsf(rate.result.value + 0.8f) < 0.0001f;
}

int main(void) {
    if(rohr_error_check(rohr_directory_working_set(
            rohr_directory_base_get()))) return 1;
    const char *paths[] = {
        "assets/game-state/world.json",
        "assets/game-state/relationships.json"
    };
    EntityIndex seeker_index;
    CameraAttachment camera_attachment;

    {
        EngineResult init_result = rohr_engine_start();
        if(rohr_error_check(init_result)) {
            PRINT_ENGINE_ERROR(init_result);
            return 1;
        }
    }

    {
        EngineResult load_result = rohr_game_state_files_load(paths, 2);
        if(rohr_error_check(load_result)) {
            PRINT_ENGINE_ERROR(load_result);
            rohr_engine_stop();
            return 1;
        }
    }

    EntityResult seeker_result = rohr_entity_by_name_get("seeker");
    EntityResult player_result = rohr_entity_by_name_get("player");
    if(rohr_error_check(seeker_result) || rohr_error_check(player_result)) {
        rohr_engine_stop();
        return 1;
    }
    Entity seeker = seeker_result.result.value;
    Entity player = player_result.result.value;
    EntityIndexResult index_result = rohr_entity_index_get(seeker);
    CameraAttachmentResult attachment_result = rohr_graphics_camera_attachment_get();
    if(rohr_error_check(index_result) || rohr_error_check(attachment_result)) {
        if(rohr_error_check(index_result)) {
            PRINT_ENGINE_ERROR(index_result);
        }
        if(rohr_error_check(attachment_result)) {
            PRINT_ENGINE_ERROR(attachment_result);
        }
        rohr_engine_stop();
        return 1;
    }
    seeker_index = index_result.result.value;
    camera_attachment = attachment_result.result.value;
    if(targets[seeker_index] != player
            || camera_attachment.entity != player
            || camera_attachment.position_offset.x != 10.0f
            || camera_attachment.position_offset.y != -20.0f
            || fabsf(camera_attachment.orientation_offset +
                rohr_math_radians_to_degrees(0.25f)) > 0.0001f) {
        fprintf(stderr, "Loaded entity relationships do not match\n");
        rohr_engine_stop();
        return 1;
    }

    /* Teleporting the origin retains authored geometry, COM, and inertia. */
    if(!angular_state_check(player) || !center_of_mass_check(player) ||
            rohr_error_check(rohr_physics_position_set(player, (Position){70,90})) ||
            !center_of_mass_check(player)) {
        fprintf(stderr, "Origin-relative mass properties do not match\n");
        rohr_engine_stop();
        return 1;
    }

    EngineResult save_result = rohr_game_state_file_save("saved_game_state.json");
    if(rohr_error_check(save_result)) {
        PRINT_ENGINE_ERROR(save_result);
        rohr_engine_stop();
        return 1;
    }
    EngineResult template_result = rohr_game_state_template_file_save(
        "saved_game_state_template.json"
    );
    if(rohr_error_check(template_result)) {
        PRINT_ENGINE_ERROR(template_result);
    }
    rohr_engine_stop();
    if(rohr_error_check(template_result)) return 1;
    if(rohr_error_check(rohr_engine_start())) return 1;
    EngineResult reload = rohr_game_state_file_load("saved_game_state.json");
    EntityResult restored = rohr_entity_by_name_get("player");
    bool valid = !rohr_error_check(reload) && !rohr_error_check(restored) &&
        center_of_mass_check(restored.result.value) && angular_state_check(restored.result.value);
    rohr_engine_stop();
    return valid ? 0 : 1;
}
