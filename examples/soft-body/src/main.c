/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "example_viewport.h"
#include "example_input.h"
#include "car_controls.h"
#include "car_benchmark.h"

#include <math.h>
#include <stdio.h>

#define WHEEL_COUNT 2
#define RAMP_TRIANGLE_COUNT 4
#define LEVEL_WALL_COUNT 5
#define PIT_PARTICLE_COUNT 200
#define PIT_PARTICLE_COLUMNS 20

typedef struct Wheel {
    Entity hub;
    Entity disk;
    Entity soft_body;
} Wheel;

static const Color background_color;
static const Color wall_color;
static const Color chassis_color;
static const Color cabin_color;
static const Color hub_color;
static const Color disk_color;
static const Color surface_color;
static const Color beam_color;
static const Color node_color;
static const Color ramp_color;
static const Color particle_color;
static const RohrCollisionCategoryMask room_category;
static const RohrCollisionCategoryMask hub_category;
static const RohrCollisionCategoryMask particle_category;
static const float physics_tick_time;
static const Acceleration gravity;
static const float rigid_friction;
static const Restitution collision_restitution;
static const AngularVelocity maximum_wheel_angular_velocity;
static const Position level_wall_positions[LEVEL_WALL_COUNT];
static const Vec2D level_wall_dimensions[LEVEL_WALL_COUNT];
static const Position ramp_vertices[RAMP_TRIANGLE_COUNT][3];
static const Position ramp_weld_points[RAMP_TRIANGLE_COUNT - 1];
static const float particle_radius;
static const Mass particle_mass;
static const float particle_spacing;
static const Position particle_spawn_origin;
static const float camera_default_zoom;
static const float camera_collision_zoom;
static const Time camera_collision_zoom_duration;
static const Time camera_collision_zoom_out_duration;
static const float camera_collision_physics_scale;

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "error %d: %s\n", (int)result.result.error,
        rohr_error_message_get(result));
    return false;
}

static Entity wall_create(Position position, Vec2D dimensions) {
    EntityResult result = rohr_entity_add();
    Entity wall;
    if(rohr_error_check(result)) return ENTITY_INVALID;
    wall = result.result.value;
    if(!result_ok(rohr_physics_position_set(wall, position)) ||
            !result_ok(rohr_physics_hitbox_set(
                wall, rohr_math_square_create(dimensions.x, dimensions.y))) ||
            !result_ok(rohr_entity_components_add(wall, ROHR_COLLISION)) ||
            !result_ok(rohr_physics_static_set(wall)) ||
            !result_ok(rohr_physics_friction_set(wall, rigid_friction)) ||
            !result_ok(rohr_physics_restitution_set(wall, collision_restitution)) ||
            !result_ok(rohr_physics_collision_category_set(wall, room_category)) ||
            !result_ok(rohr_physics_collision_response_set(wall, ROHR_COLLISION_CATEGORY_ALL & ~hub_category))) return ENTITY_INVALID;
    return wall;
}

static Entity static_triangle_create(Position a, Position b, Position c) {
    Position center = {
        (a.x + b.x + c.x) / 3.0f,
        (a.y + b.y + c.y) / 3.0f
    };
    Shape triangle = {
        .amount_of_vertices = 3,
        .vertices = {
            {a.x - center.x, a.y - center.y},
            {b.x - center.x, b.y - center.y},
            {c.x - center.x, c.y - center.y}
        }
    };
    EntityResult result = rohr_entity_add();
    Entity entity;

    if(rohr_error_check(result)) return ENTITY_INVALID;
    entity = result.result.value;
    if(!result_ok(rohr_physics_position_set(entity, center)) ||
            !result_ok(rohr_physics_hitbox_set(entity, triangle)) ||
            !result_ok(rohr_entity_components_add(entity, ROHR_COLLISION)) ||
            !result_ok(rohr_physics_dynamic_set(entity)) ||
            !result_ok(rohr_physics_friction_set(entity, rigid_friction)) ||
            !result_ok(rohr_physics_restitution_set(entity, collision_restitution)) ||
            !result_ok(rohr_physics_collision_category_set(entity, room_category)) ||
            !result_ok(rohr_physics_collision_response_set(entity, ROHR_COLLISION_CATEGORY_ALL & ~hub_category))) return ENTITY_INVALID;
    return entity;
}

static Entity particle_create(ParticleDescription config, Position position) {
    EntityResult result;

    config.position = position;
    result = rohr_physics_particle_add(config);
    return rohr_error_check(result) ? ENTITY_INVALID : result.result.value;
}

static bool anchored_joint_set(Entity a, Vec2D offset_a, Entity b, Vec2D offset_b,
        JointType type) {
    JointAnchorIdResult anchor_a = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a, .local_offset = offset_a});
    JointAnchorIdResult anchor_b = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = b, .local_offset = offset_b});
    EntityResult joint = rohr_entity_add();
    if(rohr_error_check(anchor_a) || rohr_error_check(anchor_b) || rohr_error_check(joint)) {
        return false;
    }
    if(type == JOINT_WELD) {
        return result_ok(rohr_physics_joint_description_set(joint.result.value, (JointDescription){
            .type = JOINT_WELD,
            .anchor_a = anchor_a.result.value,
            .anchor_b = anchor_b.result.value}));
    }
    return result_ok(rohr_physics_joint_description_set(joint.result.value, (JointDescription){
            .type = JOINT_PIN,
            .anchor_a = anchor_a.result.value,
            .anchor_b = anchor_b.result.value}));
}

static const Color background_color = {18, 22, 30, 255};
static const Color wall_color = {90, 100, 115, 255};
static const Color chassis_color = {65, 145, 105, 255};
static const Color cabin_color = {80, 175, 125, 255};
static const Color hub_color = {75, 80, 90, 255};
static const Color disk_color = {190, 75, 65, 255};
static const Color surface_color = {55, 125, 175, 150};
static const Color beam_color = {235, 240, 245, 255};
static const Color node_color = {255, 170, 70, 255};
static const Color ramp_color = {145, 120, 85, 255};
static const Color particle_color = {45, 120, 230, 255};
static const RohrCollisionCategoryMask room_category = UINT64_C(1) << 1;
/* Hubs only contact pit particles; keep room/chassis exclusions explicit. */
static const RohrCollisionCategoryMask hub_category = UINT64_C(1) << 6;
static const RohrCollisionCategoryMask particle_category = UINT64_C(1) << 5;
static const float physics_tick_time = 1.0f / 240.0f;
static const Acceleration gravity = {0.0f, -175.0f};
static const float rigid_friction = 2.0f;
static const Restitution collision_restitution = 0.4f;
/* Preserve the physical limit of 7 radians/second in degree-based storage. */
static const AngularVelocity maximum_wheel_angular_velocity = 7.0f * 180.0f / PI_F;
static const Position level_wall_positions[LEVEL_WALL_COUNT] = {
    {-60.0f, -225.0f},
    {900.0f, 225.0f},
    {-305.0f, 0.0f},
    {600.0f, -370.0f},
    {800.0f, -320.0f}
};
static const Vec2D level_wall_dimensions[LEVEL_WALL_COUNT] = {
    {480.0f, 20.0f},
    {2400.0f, 20.0f},
    {20.0f, 470.0f},
    {400.0f, 10.0f},
    {10.0f, 100.0f}
};
static const Position ramp_vertices[RAMP_TRIANGLE_COUNT][3] = {
    {{180.0f, -215.0f}, {260.0f, -215.0f}, {260.0f, -180.0f}},
    {{260.0f, -215.0f}, {340.0f, -215.0f}, {260.0f, -180.0f}},
    {{260.0f, -180.0f}, {340.0f, -215.0f}, {340.0f, -170.0f}},
    {{340.0f, -215.0f}, {420.0f, -215.0f}, {340.0f, -170.0f}}
};
static const Position ramp_weld_points[RAMP_TRIANGLE_COUNT - 1] = {
    {260.0f, -197.5f},
    {300.0f, -197.5f},
    {340.0f, -180.0f}
};
static const float particle_radius = 3.0f;
static const Mass particle_mass = 0.3;
static const float particle_spacing = 17.0f;
static const Position particle_spawn_origin = {438.5f, -170.0f};
static const float camera_default_zoom = 2.5f;
static const float camera_collision_zoom = 5.0f;
static const Time camera_collision_zoom_duration = 5.0;
static const Time camera_collision_zoom_out_duration = 3.0;
static const float camera_collision_physics_scale = 0.2f;

typedef struct RenderContext {
    Entity *walls;
    Entity *ramp;
    Entity *particles;
    Entity chassis;
    Entity cabin;
    Wheel *wheels;
} RenderContext;

static void render_scene(CameraId camera, void *context_value) {
    RenderContext *context = context_value;
    (void)camera;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw(background_color);
    rohr_graphics_layer_active_set(100);
    rohr_graphics_aabb_tree_draw();
    rohr_graphics_contacts_draw();
    rohr_graphics_layer_active_set(0);
    for(uint32_t i = 0; i < LEVEL_WALL_COUNT; i += 1)
        rohr_graphics_hit_box_colored_draw(context->walls[i], GRAPHICS_FILLED,
            wall_color);
    for(uint32_t i = 0; i < RAMP_TRIANGLE_COUNT; i += 1)
        rohr_graphics_hit_box_colored_draw(context->ramp[i], GRAPHICS_FILLED,
            ramp_color);
    for(uint32_t i = 0; i < PIT_PARTICLE_COUNT; i += 1)
        rohr_graphics_hit_box_colored_draw(context->particles[i], GRAPHICS_FILLED,
            particle_color);
    for(uint32_t i = 0; i < WHEEL_COUNT; i += 1) {
        (void)rohr_graphics_soft_body_draw(context->wheels[i].soft_body,
            surface_color, beam_color, node_color);
        rohr_graphics_hit_box_colored_draw(context->wheels[i].disk,
            GRAPHICS_FILLED, disk_color);
    }
    rohr_graphics_hit_box_colored_draw(context->chassis, GRAPHICS_FILLED,
        chassis_color);
    rohr_graphics_hit_box_colored_draw(context->cabin, GRAPHICS_FILLED,
        cabin_color);
    for(uint32_t i = 0; i < WHEEL_COUNT; i += 1)
        rohr_graphics_hit_box_colored_draw(context->wheels[i].hub,
            GRAPHICS_FILLED, hub_color);
}

int main(int argc, char **argv) {
    static CarBenchmark benchmark;
    if(!car_benchmark_parse(&benchmark, argc, argv)) {
        fprintf(stderr, "usage: soft_body [--benchmark=headless|rendered] [--debug] [--slow-motion] [--frames=1..3600]\n");
        return 2;
    }
    bool graphics_enabled = !benchmark.enabled || benchmark.rendered;
    bool benchmark_pass = true;
    Entity walls[LEVEL_WALL_COUNT];
    Entity ramp[RAMP_TRIANGLE_COUNT];
    Entity particles[PIT_PARTICLE_COUNT];
    Entity chassis;
    Entity cabin;
    ParticleDescription particle_config;
    Wheel wheels[WHEEL_COUNT] = {0};
    InputControllerId input_controller = INPUT_CONTROLLER_INVALID;
    InputActionId exit_action, debug_action, torque_action, jump_action;
    Car car = {0};
    CarControls controls = {0};
    Tick zoom_end_tick = 0;
    Tick camera_return_end_tick = 0;
    bool collision_zoom_started = false;
    bool collision_slow_motion_active = false;
    bool camera_returning = false;
    bool broadphase_debug = benchmark.enabled ? benchmark.debug : true;
    ViewportId viewport = VIEWPORT_INVALID;

    if(!result_ok(rohr_directory_working_set(rohr_directory_base_get())) ||
            !result_ok(rohr_engine_start())) return 1;
    {
        InputBinding exit_binding = example_input_key_binding(SDL_SCANCODE_ESCAPE,
            1.0f, (Vec2D){0});
        InputBinding debug_binding = example_input_key_binding(SDL_SCANCODE_B,
            1.0f, (Vec2D){0});
        InputBinding jump_binding = example_input_key_binding(SDL_SCANCODE_SPACE, 1.0f, (Vec2D){0});
        InputBinding torque_bindings[] = {
            example_input_key_binding(SDL_SCANCODE_A, -1.0f, (Vec2D){0}),
            example_input_key_binding(SDL_SCANCODE_D, 1.0f, (Vec2D){0})};
        if(!example_input_controller_create("truck", &input_controller) ||
                !example_input_action_create(input_controller, "exit", INPUT_ACTION_BUTTON,
                    &exit_binding, 1, &exit_action) ||
                !example_input_action_create(input_controller, "toggle_debug",
                    INPUT_ACTION_BUTTON, &debug_binding, 1, &debug_action) ||
                !example_input_action_create(input_controller, "torque",
                    INPUT_ACTION_AXIS_1D, torque_bindings, 2, &torque_action) ||
                !example_input_action_create(input_controller, "jump",
                    INPUT_ACTION_BUTTON, &jump_binding, 1, &jump_action))
            goto fail;
    }
    if(!result_ok(rohr_engine_time_per_tick_set(physics_tick_time)) ||
            !result_ok(rohr_physics_substeps_set(4))) goto fail;
    if(graphics_enabled && !result_ok(rohr_graphics_start())) goto fail;
    if(benchmark.enabled && graphics_enabled) {
        WindowPresentationDescription window = rohr_graphics_window_presentation_description_get();
        window.mode = GRAPHICS_WINDOW_MODE_WINDOWED;
        window.window_width = window.logical_width = 1280;
        window.window_height = window.logical_height = 720;
        if(!result_ok(rohr_graphics_window_presentation_description_set(window)) ||
                !result_ok(rohr_graphics_vsync_set(false)) ||
                !result_ok(rohr_graphics_frame_limit_set(60))) goto fail;
    }
    particle_config = rohr_physics_particle_description_default_get();
    particle_config.acceleration = gravity;
    particle_config.radius = particle_radius;
    particle_config.rigid_vertices = 8;
    particle_config.mass_value = particle_mass;
    particle_config.friction = rigid_friction;
    particle_config.restitution = collision_restitution;
    particle_config.collision_filter.category = particle_category;
    particle_config.collision_filter.responds_to =
        ROHR_COLLISION_CATEGORY_ALL;
    rohr_graphics_aabb_tree_debug_set(broadphase_debug);
    rohr_graphics_contacts_debug_set(broadphase_debug);
    for(uint32_t i = 0; i < LEVEL_WALL_COUNT; i += 1) {
        walls[i] = wall_create(level_wall_positions[i], level_wall_dimensions[i]);
        if(walls[i] == ENTITY_INVALID) goto fail;
    }

    {
        Position centers[RAMP_TRIANGLE_COUNT];

        for(uint32_t i = 0; i < RAMP_TRIANGLE_COUNT; i += 1) {
            centers[i] = (Position){
                (ramp_vertices[i][0].x + ramp_vertices[i][1].x +
                    ramp_vertices[i][2].x) / 3.0f,
                (ramp_vertices[i][0].y + ramp_vertices[i][1].y +
                    ramp_vertices[i][2].y) / 3.0f
            };
            ramp[i] = static_triangle_create(
                ramp_vertices[i][0], ramp_vertices[i][1], ramp_vertices[i][2]);
            if(ramp[i] == ENTITY_INVALID) goto fail;
        }
        for(uint32_t i = 0; i + 1 < RAMP_TRIANGLE_COUNT; i += 1) {
            if(!anchored_joint_set(
                    ramp[i],
                    (Vec2D){ramp_weld_points[i].x - centers[i].x,
                        ramp_weld_points[i].y - centers[i].y},
                    ramp[i + 1],
                    (Vec2D){ramp_weld_points[i].x - centers[i + 1].x,
                        ramp_weld_points[i].y - centers[i + 1].y},
                    JOINT_WELD)) goto fail;
        }
    }
    if(!result_ok(car_create(&car, (Position){0}))) goto fail;
    chassis = car.chassis; cabin = car.cabin;
    wheels[0] = (Wheel){.hub=car.rear_hub,.disk=car.rear_disk,.soft_body=car.rear_tire};
    wheels[1] = (Wheel){.hub=car.front_hub,.disk=car.front_disk,.soft_body=car.front_tire};
    for(unsigned i=0;i<WHEEL_COUNT;++i)
        if(!result_ok(rohr_physics_angular_velocity_maximum_set(
                wheels[i].disk, maximum_wheel_angular_velocity))) goto fail;
    for(uint32_t i = 0; i < PIT_PARTICLE_COUNT; i += 1) {
        uint32_t column = i % PIT_PARTICLE_COLUMNS;
        uint32_t row = i / PIT_PARTICLE_COLUMNS;
        particles[i] = particle_create(particle_config, (Position){
            particle_spawn_origin.x + (float)column * particle_spacing,
            particle_spawn_origin.y + (float)row * particle_spacing
        });
        if(particles[i] == ENTITY_INVALID) goto fail;
    }
    if(graphics_enabled && !result_ok(rohr_graphics_camera_attach(
                (CameraAttachmentDescription){.entity = chassis, .follow_position = true}))) goto fail;
    if(graphics_enabled && !result_ok(rohr_camera_zoom_set(
                rohr_camera_active_get(), camera_default_zoom, 0.0))) goto fail;
    RenderContext render_context = {walls, ramp, particles, chassis, cabin, wheels};
    if(graphics_enabled && !example_viewport_create(render_scene, &render_context, &viewport)) goto fail;

    rohr_engine_clock_reset();
    benchmark.started = SDL_GetTicksNS();
    while(true) {
        uint64_t frame_start = SDL_GetTicksNS();
        SDL_Event event;
        bool exit_requested = false;

        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT) exit_requested = true;
        }
        if(!benchmark.enabled && rohr_input_action_button_pressed_check(debug_action)) {
            broadphase_debug = !broadphase_debug;
            rohr_graphics_aabb_tree_debug_set(broadphase_debug);
            rohr_graphics_contacts_debug_set(broadphase_debug);
        }
        if(rohr_input_action_button_pressed_check(exit_action))
            exit_requested = true;
        if(exit_requested) break;
        if(benchmark.enabled) {
            double script_time = benchmark.frames / 60.0;
            float drive = script_time < 2 ? 0 : script_time < 30 ? 1 : -1;
            car_jump_request(&controls, &car, benchmark.frames == 300 || benchmark.frames == 2100);
            if(!result_ok(car_controls_update(&controls, &car, drive))) goto fail;
            clock_t cpu = clock();
            Report physics = rohr_physics_update(4);
            benchmark.physics[benchmark.frames] = 1000.0 * (clock() - cpu) / CLOCKS_PER_SEC;
            car_benchmark_report_add(&benchmark, &physics);
            if(physics.header.status == ROHR_REPORT_FAILED || physics.has_fatal) goto fail;
            double scale = collision_slow_motion_active ? camera_collision_physics_scale : 1;
            benchmark.simulated += 4.0 * physics_tick_time * scale;
            if(benchmark.slow_motion && !collision_zoom_started) {
                for(unsigned i = 0; i < PIT_PARTICLE_COUNT; ++i) {
                    if(!rohr_physics_contact_check(chassis, particles[i])) continue;
                    collision_zoom_started = collision_slow_motion_active = true;
                    zoom_end_tick = benchmark.frames + 300;
                    if(!result_ok(rohr_physics_dt_per_tick_set(physics_tick_time * camera_collision_physics_scale))) goto fail;
                    break;
                }
            }
            if(collision_slow_motion_active && benchmark.frames >= zoom_end_tick) {
                rohr_physics_engine_time_per_tick_use();
                collision_slow_motion_active = false;
            }
            double wait_ms = 0;
            if(graphics_enabled) {
                uint64_t render_start = SDL_GetTicksNS();
                Report graphics = rohr_graphics_show();
                if(graphics.header.status == ROHR_REPORT_FAILED) goto fail;
                for(size_t i = 0; i < graphics.statistic_count; ++i)
                    if(graphics.statistics[i].metric_id == ROHR_REPORT_FRAME_LIMIT_WAIT_NS)
                        wait_ms += graphics.statistics[i].value / 1e6;
                benchmark.render[benchmark.frames] = (SDL_GetTicksNS() - render_start) / 1e6 - wait_ms;
            }
            benchmark.work[benchmark.frames] = (SDL_GetTicksNS() - frame_start) / 1e6 - wait_ms;
            ++benchmark.frames;
            benchmark.maximum_lag = fmax(benchmark.maximum_lag,
                (SDL_GetTicksNS() - benchmark.started) / 1e9 - benchmark.frames / 60.0);
            if(benchmark.frames % 60 == 0) {
                fprintf(stderr, "benchmark progress frame=%u simulated=%.3f elapsed=%.3f\n",
                    benchmark.frames, benchmark.simulated, (SDL_GetTicksNS() - benchmark.started) / 1e9);
            }
            if(benchmark.frames >= benchmark.limit) break;
            continue;
        }
        car_jump_request(&controls, &car, rohr_input_action_button_pressed_check(jump_action));
        {
            if(rohr_engine_update().header.status == ROHR_REPORT_FAILED) goto fail;
            Tick ticks = rohr_engine_tick_delta_get();
            Tick current_tick = rohr_engine_tick_get();
            if(camera_returning && current_tick >= camera_return_end_tick) {
                if(!result_ok(rohr_camera_attachment_description_set(
                            rohr_camera_active_get(),
                            (CameraAttachmentDescription){.entity = chassis, .follow_position = true}))) goto fail;
                camera_returning = false;
            }
            if(collision_slow_motion_active && current_tick >= zoom_end_tick) {
                rohr_physics_engine_time_per_tick_use();
                if(!result_ok(rohr_camera_position_from_entity_set(
                            rohr_camera_active_get(), chassis,
                            camera_collision_zoom_out_duration)) ||
                        !result_ok(rohr_camera_zoom_set(
                            rohr_camera_active_get(), camera_default_zoom,
                            camera_collision_zoom_out_duration))) goto fail;
                camera_return_end_tick = current_tick + (Tick)ceil(
                    camera_collision_zoom_out_duration / physics_tick_time);
                camera_returning = true;
                collision_slow_motion_active = false;
            }
            if(ticks > 0) {
                InputAxis1DResult torque_result =
                    rohr_input_action_axis_1d_get(torque_action);
                float torque_axis = rohr_error_check(torque_result) ? 0.0f :
                    torque_result.result.value;
                if(!result_ok(car_controls_update(
                            &controls, &car, torque_axis))) goto fail;
            }
            if(((rohr_physics_update(ticks)).header.status == ROHR_REPORT_FAILED)) goto fail;
            if(!collision_zoom_started) {
                for(uint32_t i = 0; i < PIT_PARTICLE_COUNT; i += 1) {
                    if(!rohr_physics_contact_check(chassis, particles[i])) continue;
                    /* This close-up intentionally leaves chassis following. */
                    if(!result_ok(rohr_camera_detach(rohr_camera_active_get())) ||
                            !result_ok(rohr_camera_position_from_entity_set(
                                rohr_camera_active_get(), wheels[1].hub,
                                camera_collision_zoom_duration)) ||
                            !result_ok(rohr_camera_zoom_set(
                                rohr_camera_active_get(), camera_collision_zoom,
                                camera_collision_zoom_duration)) ||
                            !result_ok(rohr_physics_dt_per_tick_set(
                                physics_tick_time * camera_collision_physics_scale))) goto fail;
                    zoom_end_tick = current_tick + (Tick)ceil(
                        camera_collision_zoom_duration / physics_tick_time);
                    collision_zoom_started = true;
                    collision_slow_motion_active = true;
                    break;
                }
            }
        }
        rohr_graphics_show();
    }
    if(benchmark.enabled) benchmark_pass = car_benchmark_finish(&benchmark);
    car_destroy(&car);
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    if(graphics_enabled) rohr_graphics_stop();
    rohr_engine_stop();
    return benchmark_pass ? 0 : 1;

fail:
    fprintf(stderr, "soft-body example failed\n");
    car_destroy(&car);
    example_viewport_destroy(&viewport);
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    if(graphics_enabled) rohr_graphics_stop();
    rohr_engine_stop();
    return 1;
}
