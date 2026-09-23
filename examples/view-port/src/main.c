/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "example_viewport.h"
#include "example_input.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "elderfly_descriptors.h"

const Color background_color = {255,255,255,255};
AnimationAsset animation_elderfly = {0};
AnimatedSprite sprite_elderfly = {0};
const float camera_move_speed = 100.0f;
const float camera_turn_speed = PI_F * 0.5f;

#define PRINT_ENGINE_ERROR(engine_result) \
    fprintf(stderr, "error %d: %s\n", (int)(engine_result).result.error, \
        rohr_error_message_get(engine_result))

static void render_scene(CameraId camera, void *context) {
    (void)camera;
    (void)context;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw(background_color);
    rohr_graphics_layer_active_set(0);
    rohr_graphics_sprite_frames_update(rohr_engine_tick_get(), rohr_engine_time_get());
    rohr_graphics_animated_sprites_draw();
    rohr_graphics_layer_active_set(100);
    rohr_graphics_aabb_tree_draw();
    rohr_graphics_contacts_draw();
    rohr_graphics_layer_active_set(0);
}

int main(void) {
    if(rohr_error_check(rohr_directory_working_set(
            rohr_directory_base_get()))) return 1;
    {
        EngineResult init_result = rohr_engine_init();
        if(rohr_error_check(init_result)) {
            PRINT_ENGINE_ERROR(init_result);
            return 1;
        }
    }
    InputControllerId input_controller = INPUT_CONTROLLER_INVALID;
    InputActionId exit_action, debug_action, move_action, camera_move_action;
    InputActionId camera_turn_action, drag_action, rotate_action;
    ViewportId viewport = VIEWPORT_INVALID;
    bool broadphase_debug = true;
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
        InputBinding camera_move_bindings[] = {
            example_input_key_binding(SDL_SCANCODE_I, 1.0f, (Vec2D){0, 1}),
            example_input_key_binding(SDL_SCANCODE_J, 1.0f, (Vec2D){-1, 0}),
            example_input_key_binding(SDL_SCANCODE_K, 1.0f, (Vec2D){0, -1}),
            example_input_key_binding(SDL_SCANCODE_L, 1.0f, (Vec2D){1, 0})};
        InputBinding camera_turn_bindings[] = {
            example_input_key_binding(SDL_SCANCODE_Q, -1.0f, (Vec2D){0}),
            example_input_key_binding(SDL_SCANCODE_E, 1.0f, (Vec2D){0})};
        InputBinding drag_binding =
            example_input_mouse_button_binding(INPUT_MOUSE_BUTTON_LEFT);
        InputBinding rotate_binding =
            example_input_mouse_button_binding(INPUT_MOUSE_BUTTON_RIGHT);
        if(!example_input_controller_create("viewport", &input_controller) ||
                !example_input_action_create(input_controller, "exit", INPUT_ACTION_BUTTON,
                    &exit_binding, 1, &exit_action) ||
                !example_input_action_create(input_controller, "toggle_debug",
                    INPUT_ACTION_BUTTON, &debug_binding, 1, &debug_action) ||
                !example_input_action_create(input_controller, "move", INPUT_ACTION_AXIS_2D,
                    move_bindings, 4, &move_action) ||
                !example_input_action_create(input_controller, "camera_move",
                    INPUT_ACTION_AXIS_2D, camera_move_bindings, 4,
                    &camera_move_action) ||
                !example_input_action_create(input_controller, "camera_turn",
                    INPUT_ACTION_AXIS_1D, camera_turn_bindings, 2,
                    &camera_turn_action) ||
                !example_input_action_create(input_controller, "drag", INPUT_ACTION_BUTTON,
                    &drag_binding, 1, &drag_action) ||
                !example_input_action_create(input_controller, "rotate",
                    INPUT_ACTION_BUTTON, &rotate_binding, 1, &rotate_action))
            goto fail;
    }
    {
        EngineResult graphics_result = rohr_graphics_start();
        if(rohr_error_check(graphics_result)) {
            PRINT_ENGINE_ERROR(graphics_result);
            rohr_engine_shutdown();
            return 1;
        }
    }
    EntityResult water_smash_result = rohr_entity_add();
    if(rohr_error_check(water_smash_result)) {
        PRINT_ENGINE_ERROR(water_smash_result);
        goto fail;
    }
    Entity water_smash = water_smash_result.result.value;
    rohr_physics_position_set(water_smash, (Position){.x = 0, .y = 0});
    rohr_physics_orientation_set(water_smash, 0);
    rohr_physics_mass_set(water_smash, 50);
    rohr_physics_velocity_set(water_smash, (Velocity){0, 0});
    rohr_physics_restitution_set(water_smash, 0.1);
    Shape shape4 = rohr_math_square_create(150, 220);
    rohr_physics_hitbox_set(water_smash, shape4);
    rohr_physics_friction_set(water_smash, 0.4);
    rohr_physics_dynamic_set(water_smash);
    EngineResult camera_result = rohr_graphics_camera_attach(
            water_smash,
            (Vec2D){.x = 0.0f, .y = 100.0f},
            0.0f);
    if(rohr_error_check(camera_result)) {
        PRINT_ENGINE_ERROR(camera_result);
        goto fail;
    }
    AnimationAssetResult animation_result = rohr_graphics_animation_load(elderfly_fly);
    if(rohr_error_check(animation_result)) {
        PRINT_ENGINE_ERROR(animation_result);
        goto fail;
    }
    animation_elderfly = animation_result.result.value;
    sprite_elderfly = rohr_graphics_animated_sprite_create(animation_elderfly, (Scale){10,10});
    rohr_graphics_animated_sprite_add(water_smash, sprite_elderfly);
    if(!example_viewport_create(render_scene, NULL, &viewport)) goto fail;
    rohr_graphics_aabb_tree_debug_set(broadphase_debug);
    rohr_graphics_contacts_debug_set(broadphase_debug);

    rohr_engine_clock_reset();
    //Game Loop
    //rohr_graphics_recording_start("examples/view-port/recording.mp4",60);
    while(true) {

        //physics
        Tick ticks_advanced = rohr_system_tick_update();
        Time tick_time = rohr_engine_time_per_tick_get() * (Time)ticks_advanced;
        if(rohr_error_check(rohr_physics_update(ticks_advanced))) goto fail;

        rohr_graphics_show();

        SDL_Event sdl_event;
        bool exit_requested = false;

        rohr_input_frame_begin();
        while((sdl_event = rohr_engine_event_poll()).type != 0) {
            if(sdl_event.type == SDL_EVENT_QUIT) exit_requested = true;
        }
        if(rohr_input_action_button_pressed_check(debug_action)) {
            broadphase_debug = !broadphase_debug;
            rohr_graphics_aabb_tree_debug_set(broadphase_debug);
            rohr_graphics_contacts_debug_set(broadphase_debug);
        }
        if(exit_requested ||
                rohr_input_action_button_pressed_check(exit_action)) break;
        InputAxis2DResult move_result = rohr_input_action_axis_2d_get(move_action);
        InputAxis2DResult camera_move_result =
            rohr_input_action_axis_2d_get(camera_move_action);
        InputAxis1DResult camera_turn_result =
            rohr_input_action_axis_1d_get(camera_turn_action);
        Vec2D move_axis = rohr_error_check(move_result) ? (Vec2D){0} :
            move_result.result.value;
        Vec2D camera_move_axis = rohr_error_check(camera_move_result) ?
            (Vec2D){0} : camera_move_result.result.value;
        float camera_turn_axis = rohr_error_check(camera_turn_result) ? 0.0f :
            camera_turn_result.result.value;
        rohr_physics_velocity_set(water_smash, (Velocity){
            .x = move_axis.x * 100.0f,
            .y = move_axis.y * 100.0f
        });
        rohr_graphics_camera_move((Vec2D){
            .x = camera_move_axis.x * camera_move_speed * tick_time,
            .y = camera_move_axis.y * camera_move_speed * tick_time
        });
        rohr_graphics_camera_rotate(
            camera_turn_axis * camera_turn_speed * tick_time
        );

        if(rohr_input_action_button_down_check(drag_action)) {
            Vec2D pointer = rohr_input_mouse_position_get();
            rohr_physics_position_set(
                water_smash,
                rohr_graphics_screen_to_world_get((Position){pointer.x, pointer.y})
            );
        }
        if(rohr_input_action_button_down_check(rotate_action)) {
            EntityIndexResult index_result = rohr_entity_index_get(water_smash);
            if(!rohr_error_check(index_result)) {
                rohr_physics_orientation_set(water_smash, orientations[index_result.result.value] + 10*(2*PI_F/360));
            }
        }


    }
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_graphics_end();
    rohr_engine_shutdown();
    return 0;

fail:
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_graphics_end();
    rohr_engine_shutdown();
    return 1;
}
