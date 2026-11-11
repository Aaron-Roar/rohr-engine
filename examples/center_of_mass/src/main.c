/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include "example_viewport.h"
#include <stdio.h>

int main(void) {
    ProjectId instance = ROHR_PROJECT_INVALID;
    ViewportId viewport = VIEWPORT_INVALID;
    Entity bodies[2] = {0};
    JointAnchorId anchors[2] = {0};
    ViewportItemId markers[6] = {0};
    int status = 1;
    EngineResult result = rohr_directory_working_set(rohr_directory_base_get());
    if(rohr_error_check(result)) { fprintf(stderr, "%s\n", rohr_error_message_get(result)); return 1; }
    result = rohr_engine_start();
    if(rohr_error_check(result)) goto failure;
    result = rohr_graphics_start();
    if(rohr_error_check(result)) goto failure;
    result = project_settings_apply();
    if(rohr_error_check(result)) goto failure;
    ProjectIdResult created = rohr_project_create(project);
    if(rohr_error_check(created)) { fprintf(stderr, "%s\n", rohr_error_message_get(created)); goto done; }
    instance = created.result.value;
    ViewportIdResult view = rohr_viewport_by_name_get(project._project.viewports[0].name);
    if(rohr_error_check(view)) { fprintf(stderr, "%s\n", rohr_error_message_get(view)); goto done; }
    viewport = view.result.value;
    const char *body_names[] = {"automatic_body", "explicit_body"};
    const char *anchor_names[] = {"automatic_tip", "explicit_tip"};
    for(size_t i = 0; i < 2; ++i) {
        EntityResult body = rohr_entity_by_name_get(body_names[i]);
        if(rohr_error_check(body)) { fprintf(stderr, "%s\n", rohr_error_message_get(body)); goto done; }
        bodies[i] = body.result.value;
        EntityResult entity = rohr_entity_by_name_get(anchor_names[i]);
        if(rohr_error_check(entity)) { fprintf(stderr, "%s\n", rohr_error_message_get(entity)); goto done; }
        JointAnchorIdResult anchor = rohr_entity_object_anchor_get(entity.result.value);
        if(rohr_error_check(anchor)) { fprintf(stderr, "%s\n", rohr_error_message_get(anchor)); goto done; }
        anchors[i] = anchor.result.value;
    }
    EntityResult camera_entity = rohr_entity_by_name_get("main_camera");
    if(rohr_error_check(camera_entity)) { fprintf(stderr, "%s\n", rohr_error_message_get(camera_entity)); goto done; }
    CameraIdResult camera = rohr_entity_object_camera_get(camera_entity.result.value);
    if(rohr_error_check(camera)) { fprintf(stderr, "%s\n", rohr_error_message_get(camera)); goto done; }
    result = rohr_camera_active_set(camera.result.value);
    if(rohr_error_check(result)) goto failure;
    const float sizes[] = {6, 10, 7};
    const Color colors[] = {{255,255,255,255}, {45,140,255,255}, {255,215,70,255}};
    for(size_t i = 0; i < 6; ++i) {
        markers[i] = example_rectangle_add(viewport, (UIRect){0,0,sizes[i%3],sizes[i%3]}, colors[i%3], 10);
        if(!markers[i]) goto done;
    }
    puts("Left: automatic COM. Right: explicit COM. White: origin; blue: COM; yellow: local anchor.");
    puts("Both COMs remain fixed while the origins and anchors orbit. Escape exits.");
    rohr_engine_clock_reset();
    for(;;) {
        SDL_Event event;
        bool quit = false;
        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) if(event.type == SDL_EVENT_QUIT) quit = true;
        if(quit || rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) break;
        Report report = rohr_engine_update();
        if(!rohr_report_check(report)) { (void)rohr_report_console_write(&report); goto done; }
        Tick ticks = rohr_engine_tick_delta_get();
        report = rohr_physics_update(ticks);
        if(!rohr_report_check(report) && !(ticks == 0 && report.header.status == ROHR_REPORT_NOT_RUN && rohr_report_valid_check(&report))) {
            (void)rohr_report_console_write(&report); goto done;
        }
        /* Diagnostic markers are viewport UI, so they add no simulated bodies. */
        for(size_t i = 0; i < 2; ++i) {
            PositionResult origin = rohr_physics_position_get(bodies[i]);
            PositionResult com = rohr_physics_center_of_mass_world_position_get(bodies[i]);
            JointAnchorPositionResult anchor = rohr_physics_joint_anchor_world_position_get(anchors[i]);
            if(rohr_error_check(origin) || rohr_error_check(com) || rohr_error_check(anchor)) goto done;
            Position points[] = {origin.result.value, com.result.value, anchor.result.value};
            for(size_t j = 0; j < 3; ++j) {
                Position point = rohr_graphics_world_to_screen_get(points[j]);
                ViewportItemDescriptionResult item = rohr_viewport_item_description_get(markers[i*3+j]);
                if(rohr_error_check(item)) { fprintf(stderr, "%s\n", rohr_error_message_get(item)); goto done; }
                item.result.value.rectangle.x = point.x;
                item.result.value.rectangle.y = point.y;
                result = rohr_viewport_item_description_set(markers[i*3+j], item.result.value);
                if(rohr_error_check(result)) goto failure;
            }
        }
        report = rohr_graphics_viewport_show(viewport);
        if(!rohr_report_check(report)) { (void)rohr_report_console_write(&report); goto done; }
    }
    status = 0;
    goto done;
failure:
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
done:
    result = rohr_project_destroy(instance);
    if(rohr_error_check(result)) { fprintf(stderr, "%s\n", rohr_error_message_get(result)); status = 1; }
    rohr_graphics_stop();
    rohr_engine_stop();
    return status;
}
