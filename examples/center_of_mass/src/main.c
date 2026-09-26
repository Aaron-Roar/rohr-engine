/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_viewports.h"
#include <stdio.h>

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
    return false;
}

static void marker_draw(Position position, float size, Color color) {
    (void)rohr_graphics_screen_quad_draw(rohr_graphics_world_to_screen_get(position),
        size, size, 0, color);
}

static void render_scene(CameraId camera, void *context) {
    ProjectObjects *objects = context;
    const Entity bodies[] = {objects->mass_demo.automatic_body,
        objects->mass_demo.explicit_body};
    const JointAnchorId anchors[] = {objects->mass_demo.anchor_automatic_tip,
        objects->mass_demo.anchor_explicit_tip};
    (void)camera;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw((Color){18, 22, 30, 255});
    rohr_graphics_layer_active_set(0);
    project_objects_draw_all(objects);
    rohr_graphics_layer_active_set(10);
    for(size_t i = 0; i < 2; i += 1) {
        PositionResult origin = rohr_physics_position_get(bodies[i]);
        PositionResult com = rohr_physics_center_of_mass_world_position_get(bodies[i]);
        JointAnchorPositionResult anchor = rohr_physics_joint_anchor_world_position_get(anchors[i]);
        if(!rohr_error_check(origin)) marker_draw(origin.result.value, 6, (Color){255,255,255,255});
        if(!rohr_error_check(com)) marker_draw(com.result.value, 10, (Color){45,140,255,255});
        if(!rohr_error_check(anchor)) marker_draw(anchor.result.value, 7, (Color){255,215,70,255});
    }
    rohr_graphics_layer_active_set(0);
}

int main(void) {
    ProjectObjects objects = {0};
    ProjectViewports viewports = {0};
    int status = 1;
    if(!result_ok(rohr_directory_working_set(rohr_directory_base_get())) ||
            !result_ok(rohr_engine_start()) || !result_ok(rohr_graphics_start())) goto done;
    if(!result_ok(project_objects_create_all(&objects)) ||
            !result_ok(project_viewports_create(&viewports, &objects)) ||
            !result_ok(rohr_camera_render_callback_set(objects.mass_demo.camera_main_camera,
                render_scene, &objects))) goto done;
    puts("Left: automatic COM. Right: explicit COM. White: origin; blue: COM; yellow: local anchor.");
    puts("Both COMs remain fixed while the origins and anchors orbit. Escape exits.");
    rohr_engine_clock_reset();
    for(;;) {
        SDL_Event event;
        bool quit = false;
        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0)
            if(event.type == SDL_EVENT_QUIT) quit = true;
        if(quit || rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) break;
        if(!result_ok(rohr_physics_update(rohr_system_tick_update()))) goto done;
        rohr_graphics_show();
    }
    status = 0;
done:
    project_viewports_destroy(&viewports);
    project_objects_destroy_all(&objects);
    rohr_graphics_stop();
    rohr_engine_stop();
    return status;
}
