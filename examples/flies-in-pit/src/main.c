/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "example_viewport.h"
#include "example_input.h"
#include <stdio.h>

const Color background_color = {255,255,255,255};
const Mass large_fly_mass = 50.0f;
const float large_fly_control_acceleration = 240.0f;
const Torque large_fly_control_torque = 2000000.0f;

#define PRINT_ENGINE_ERROR(engine_result) \
    fprintf(stderr, "error %d: %s\n", (int)(engine_result).result.error, \
        rohr_error_message_get(engine_result))

typedef struct RenderContext {
    Entity walls[3];
    bool phase_1;
    bool phase_2;
    bool phase_3;
} RenderContext;

static void render_scene(CameraId camera, void *context_value) {
    RenderContext *context = context_value;
    (void)camera;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw(background_color);
    rohr_graphics_layer_active_set(0);
    for(size_t i = 0; i < 3; i += 1)
        rohr_graphics_hit_box_draw(context->walls[i], GRAPHICS_FILLED);
    rohr_graphics_sprite_frames_update(rohr_engine_tick_get(), rohr_engine_time_get());
    rohr_graphics_animated_sprites_draw();
    if(context->phase_1) rohr_graphics_hit_boxes_draw();
    if(context->phase_2) rohr_graphics_particles_draw();
    rohr_graphics_layer_active_set(100);
    rohr_graphics_aabb_tree_draw();
    rohr_graphics_contacts_draw();
    if(context->phase_3) rohr_graphics_local_origins_draw();
    rohr_graphics_layer_active_set(0);
}

int main(void) {
    if(rohr_error_check(rohr_directory_working_set(
            rohr_directory_base_get()))) return 1;
    InputControllerId input_controller = INPUT_CONTROLLER_INVALID;
    InputActionId exit_action, debug_action, move_action, turn_action;
    ViewportId viewport = VIEWPORT_INVALID;
    RenderContext render_context = {0};
    bool broadphase_debug = true;

    {
        EngineResult init_result = rohr_engine_start();
        if(rohr_error_check(init_result)) {
            PRINT_ENGINE_ERROR(init_result);
            return 1;
        }
    }
    {
        InputBinding exit_binding = example_input_key_binding(SDL_SCANCODE_ESCAPE,
            1.0f, (Vec2D){0});
        InputBinding debug_binding = example_input_key_binding(SDL_SCANCODE_B,
            1.0f, (Vec2D){0});
        InputBinding move_bindings[] = {
            example_input_key_binding(SDL_SCANCODE_W, 1.0f, (Vec2D){0, 1}),
            example_input_key_binding(SDL_SCANCODE_A, 1.0f, (Vec2D){-1, 0}),
            example_input_key_binding(SDL_SCANCODE_S, 1.0f, (Vec2D){0, -1}),
            example_input_key_binding(SDL_SCANCODE_D, 1.0f, (Vec2D){1, 0})};
        InputBinding turn_bindings[] = {
            example_input_key_binding(SDL_SCANCODE_LEFT, -1.0f, (Vec2D){0}),
            example_input_key_binding(SDL_SCANCODE_RIGHT, 1.0f, (Vec2D){0})};
        if(!example_input_controller_create("flies", &input_controller) ||
                !example_input_action_create(input_controller, "exit", INPUT_ACTION_BUTTON,
                    &exit_binding, 1, &exit_action) ||
                !example_input_action_create(input_controller, "toggle_debug",
                    INPUT_ACTION_BUTTON, &debug_binding, 1, &debug_action) ||
                !example_input_action_create(input_controller, "move", INPUT_ACTION_AXIS_2D,
                    move_bindings, 4, &move_action) ||
                !example_input_action_create(input_controller, "turn", INPUT_ACTION_AXIS_1D,
                    turn_bindings, 2, &turn_action)) goto fail;
    }
    {
        EngineResult tick_result = rohr_engine_time_per_tick_set(1.0 / 120.0);
        if(rohr_error_check(tick_result)) {
            PRINT_ENGINE_ERROR(tick_result);
            rohr_engine_stop();
            return 1;
        }
    }
    {
        EngineResult graphics_result = rohr_graphics_start();
        if(rohr_error_check(graphics_result)) {
            PRINT_ENGINE_ERROR(graphics_result);
            rohr_engine_stop();
            return 1;
        }
    }
    EngineResult load_result = rohr_game_state_file_load(
        "assets/flies-in-pit/game.json"
    );
    if(rohr_error_check(load_result)) {
        PRINT_ENGINE_ERROR(load_result);
        goto fail;
    }

    EntityResult wall_1_result = rohr_entity_by_name_get("wall_1");
    if(rohr_error_check(wall_1_result)) {
        PRINT_ENGINE_ERROR(wall_1_result);
        goto fail;
    }
    Entity wall_1 = wall_1_result.result.value;
    EntityResult wall_2_result = rohr_entity_by_name_get("wall_2");
    if(rohr_error_check(wall_2_result)) {
        PRINT_ENGINE_ERROR(wall_2_result);
        goto fail;
    }
    Entity wall_2 = wall_2_result.result.value;
    EntityResult wall_3_result = rohr_entity_by_name_get("wall_3");
    if(rohr_error_check(wall_3_result)) {
        PRINT_ENGINE_ERROR(wall_3_result);
        goto fail;
    }
    Entity wall_3 = wall_3_result.result.value;
    EntityResult large_fly_result = rohr_entity_by_name_get("large_fly");
    if(rohr_error_check(large_fly_result)) {
        PRINT_ENGINE_ERROR(large_fly_result);
        goto fail;
    }
    Entity large_fly = large_fly_result.result.value;
    render_context.walls[0] = wall_1;
    render_context.walls[1] = wall_2;
    render_context.walls[2] = wall_3;
    if(!example_viewport_create(render_scene, &render_context, &viewport)) goto fail;
    rohr_graphics_aabb_tree_debug_set(broadphase_debug);
    rohr_graphics_contacts_debug_set(broadphase_debug);
    CameraAttachmentResult attachment_result = rohr_graphics_camera_attachment_get();
    if(rohr_error_check(attachment_result)
            || attachment_result.result.value.entity != large_fly
            || !attachment_result.result.value.follow_position
            || attachment_result.result.value.follow_orientation) {
        fprintf(stderr, "Camera is not attached to large_fly\n");
        goto fail;
    }

    rohr_engine_clock_reset();
    //Game Loop
    rohr_graphics_recording_start("flies_in_pit_recording.mp4",60);
    bool phase_1 = false;
    bool phase_2 = false;
    bool phase_3 = false;
    while(true) {
        SDL_Event event;
        bool exit_requested = false;
        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT) exit_requested = true;
        }
        if(exit_requested ||
                rohr_input_action_button_pressed_check(exit_action)) break;
        if(rohr_input_action_button_pressed_check(debug_action)) {
            broadphase_debug = !broadphase_debug;
            rohr_graphics_aabb_tree_debug_set(broadphase_debug);
            rohr_graphics_contacts_debug_set(broadphase_debug);
        }
        Tick ticks_advanced = rohr_system_tick_update();
        if(!phase_1 && rohr_engine_time_get() > 3) {
            phase_1 = true;
        }
        if(!phase_2 && rohr_engine_time_get() > 5) {
            phase_2 = true;
        }
        if(!phase_3 && rohr_engine_time_get() > 7) {
            phase_3 = true;
        }
        render_context.phase_1 = phase_1;
        render_context.phase_2 = phase_2;
        render_context.phase_3 = phase_3;

        //Game Code
        InputAxis2DResult move_result = rohr_input_action_axis_2d_get(move_action);
        InputAxis1DResult turn_result = rohr_input_action_axis_1d_get(turn_action);
        Vec2D move_axis = rohr_error_check(move_result) ? (Vec2D){0} :
            move_result.result.value;
        float turn_axis = rohr_error_check(turn_result) ? 0.0f :
            turn_result.result.value;
        if(ticks_advanced > 0 && (move_axis.x != 0.0f || move_axis.y != 0.0f)) {
            EngineResult force_result = rohr_physics_force_for_one_tick_apply(large_fly, (Force){
                .x = move_axis.x * large_fly_mass * large_fly_control_acceleration,
                .y = move_axis.y * large_fly_mass * large_fly_control_acceleration
            });
            if(rohr_error_check(force_result)) {
                PRINT_ENGINE_ERROR(force_result);
                goto fail;
            }
        }
        if(ticks_advanced > 0 && turn_axis != 0.0f) {
            EngineResult torque_result = rohr_physics_torque_for_one_tick_apply(large_fly, -turn_axis * large_fly_control_torque);
            if(rohr_error_check(torque_result)) {
                PRINT_ENGINE_ERROR(torque_result);
                goto fail;
            }
        }

        //physics
        if(rohr_error_check(rohr_physics_update(ticks_advanced))) goto fail;

        rohr_graphics_show();

    }
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;

fail:
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_graphics_stop();
    rohr_engine_stop();
    return 1;
}
