/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include "example_viewport.h"
#include <stdio.h>

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr,"%s\n",rohr_error_message_get(result));
    return false;
}

/* Diagnostic markers are viewport UI, so they add no simulated bodies. */
static bool markers_update(const MassDemo *demo, const ViewportItemId markers[6]) {
    const Entity bodies[]={demo->automatic_body,demo->explicit_body};
    const JointAnchorId anchors[]={demo->anchor_automatic_tip,demo->anchor_explicit_tip};
    for(size_t i=0;i<2;++i) {
        PositionResult origin=rohr_physics_position_get(bodies[i]);
        PositionResult com=rohr_physics_center_of_mass_world_position_get(bodies[i]);
        JointAnchorPositionResult anchor=rohr_physics_joint_anchor_world_position_get(anchors[i]);
        if(rohr_error_check(origin)||rohr_error_check(com)||rohr_error_check(anchor)) return false;
        Position points[]={origin.result.value,com.result.value,anchor.result.value};
        for(size_t j=0;j<3;++j) {
            Position point=rohr_graphics_world_to_screen_get(points[j]);
            ViewportItemDescriptionResult item=rohr_viewport_item_description_get(markers[i*3+j]);
            if(rohr_error_check(item)) return false;
            item.result.value.rectangle.x=point.x; item.result.value.rectangle.y=point.y;
            if(!result_ok(rohr_viewport_item_description_set(markers[i*3+j],item.result.value))) return false;
        }
    }
    return true;
}

int main(void) {
    MassDemo demo={0};
    ViewportId viewport=VIEWPORT_INVALID;
    ViewportItemId markers[6]={0};
    int status=1;
    if(!result_ok(rohr_directory_working_set(rohr_directory_base_get())) ||
            !result_ok(rohr_engine_start()) || !result_ok(rohr_graphics_start()) ||
            !result_ok(project_settings_apply())) goto done;
    EntityResult created=rohr_object_create(&project.mass_demo);
    if(rohr_error_check(created)) goto done;
    if(!result_ok(mass_demo_instance_bind(&demo,created.result.value))) {
        (void)rohr_entity_delete(created.result.value); goto done;
    }
    ViewportObjectBinding binding={&project.mass_demo._description,demo.source_instance};
    ViewportCreateDescription description={.template=&project.mass_viewport,.objects=&binding,.object_count=1};
    ViewportIdResult view=rohr_viewport_create(&description);
    if(rohr_error_check(view)) goto done;
    viewport=view.result.value;
    if(!result_ok(rohr_camera_active_set(demo.camera_main_camera))) goto done;
    const float sizes[]={6,10,7};
    const Color colors[]={{255,255,255,255},{45,140,255,255},{255,215,70,255}};
    for(size_t i=0;i<6;++i) {
        markers[i]=example_rectangle_add(viewport,(UIRect){0,0,sizes[i%3],sizes[i%3]},colors[i%3],10);
        if(!markers[i]) goto done;
    }
    puts("Left: automatic COM. Right: explicit COM. White: origin; blue: COM; yellow: local anchor.");
    puts("Both COMs remain fixed while the origins and anchors orbit. Escape exits.");
    rohr_engine_clock_reset();
    for(;;) {
        SDL_Event event; bool quit=false;
        rohr_input_frame_begin();
        while((event=rohr_engine_event_poll()).type!=0) if(event.type==SDL_EVENT_QUIT) quit=true;
        if(quit||rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) break;
        if(rohr_engine_update().header.status==ROHR_REPORT_FAILED ||
                rohr_physics_update(rohr_engine_tick_delta_get()).header.status==ROHR_REPORT_FAILED ||
                !markers_update(&demo,markers) ||
                rohr_graphics_viewport_show(viewport).header.status==ROHR_REPORT_FAILED) goto done;
    }
    status=0;
done:
    example_viewport_destroy(&viewport);
    if(demo.source_instance) (void)rohr_entity_delete(demo.source_instance);
    rohr_graphics_stop(); rohr_engine_stop();
    return status;
}
