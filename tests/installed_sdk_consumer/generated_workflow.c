/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include "example_viewport.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK(call) do { if(rohr_error_check(call)) { fprintf(stderr,"Failed: %s\n",#call); return false; } } while(0)
typedef struct Workflow {
    ProjectId projects[2];
    Entity roots[2], bodies[2], late;
    CameraId cameras[2];
    ViewportId views[2];
    ScreenId owned_screen, borrowed_screen;
    ViewportItemId button, shared_mount;
    TextAsset caption;
    bool armed;
} Workflow;

static bool create(Workflow *w, bool whole_project) {
    for(unsigned i=0;i<2;++i) {
        if(whole_project) {
            ProjectIdResult instance=rohr_project_create(project); OK(instance);
            w->projects[i]=instance.result.value;
            EntityResult root=rohr_project_root_get(w->projects[i]); OK(root);
            EntityResult object=rohr_object_member_get(root.result.value,project.mass_demo.name); OK(object);
            w->roots[i]=object.result.value;
            /* Move runtime instances without modifying their immutable recipes. */
            const char *names[]={project.mass_demo.automatic_body.name,project.mass_demo.explicit_body.name};
            for(unsigned j=0;j<2;++j) {
                EntityResult member=rohr_object_member_get(w->roots[i],names[j]); OK(member);
                PositionResult position=rohr_physics_position_get(member.result.value); OK(position);
                position.result.value.x+=i*1800.0f;
                OK(rohr_physics_position_set(member.result.value,position.result.value));
            }
        } else {
            ObjectCreateOptions options=rohr_object_create_options_default_get();
            options.offset=(Position){i*1800.0f,0};
            EntityResult root=rohr_object_create_with_options(project.mass_demo,options); OK(root);
            w->roots[i]=root.result.value;
        }
        EntityResult body=rohr_object_member_get(w->roots[i],project.mass_demo.automatic_body.name); OK(body);
        w->bodies[i]=body.result.value;
        EntityResult camera=rohr_object_member_get(w->roots[i],project.mass_demo.main_camera.name); OK(camera);
        CameraIdResult camera_id=rohr_entity_object_camera_get(camera.result.value); OK(camera_id);
        w->cameras[i]=camera_id.result.value;
        if(whole_project) {
            CameraDescriptionResult camera_description=rohr_camera_description_get(w->cameras[i]); OK(camera_description);
            camera_description.result.value.position.x+=i*1800.0f;
            OK(rohr_camera_position_set(w->cameras[i],camera_description.result.value.position,0));
            ViewportIdResult view=rohr_project_viewport_by_name_get(w->projects[i],project._project.viewports[0].name); OK(view);
            w->views[i]=view.result.value;
        } else {
            ViewportObjectBinding binding={&project.mass_demo._description,w->roots[i]};
            ViewportCreateDescription description={.template=&project.mass_viewport._description,.objects=&binding,.object_count=1};
            ViewportIdResult view=rohr_viewport_create(description); OK(view); w->views[i]=view.result.value;
        }
        ViewportDescriptionResult viewport=rohr_viewport_description_get(w->views[i]); OK(viewport);
        viewport.result.value.rectangle=(ViewportRectangle){10+i*640.0f,10,620,700};
        OK(rohr_viewport_description_set(w->views[i],viewport.result.value));
        ViewportItemIdResult world=rohr_viewport_item_by_name_get(w->views[i],project.mass_viewport.items[0].name); OK(world);
        ViewportItemDescriptionResult placement=rohr_viewport_item_description_get(world.result.value); OK(placement);
        placement.result.value.rectangle=(ViewportRectangle){0,0,620,500};
        OK(rohr_viewport_item_description_set(world.result.value,placement.result.value));
        ScreenDescriptionResult screen=rohr_screen_description_get(placement.result.value.screen); OK(screen);
        assert(screen.result.value.camera==w->cameras[i]);
        assert(screen.result.value.content_mode==SCREEN_CONTENT_AUTOMATIC);
    }
    assert(w->roots[0]!=w->roots[1] && w->bodies[0]!=w->bodies[1] && w->cameras[0]!=w->cameras[1]);
    ObjectAppearanceResult color=rohr_entity_appearance_get(w->bodies[0]); OK(color);
    color.result.value.color=(Color){240,150,60,255};
    OK(rohr_entity_appearance_set(w->bodies[0],color.result.value));
    ObjectAppearanceResult other=rohr_entity_appearance_get(w->bodies[1]); OK(other);
    assert(other.result.value.color.red!=240);

    /* Extend after creation: this convenience-created screen belongs to view 0. */
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.rectangle=(ViewportRectangle){340,350,300,190};
    placement.orientation=12; placement.layer=20;
    placement.clip_enabled=true; placement.clip_rectangle=(ViewportRectangle){0,0,600,560};
    ViewportItemIdResult inset=rohr_viewport_camera_add(w->views[0],w->cameras[1],placement); OK(inset);
    ViewportItemDescriptionResult item=rohr_viewport_item_description_get(inset.result.value); OK(item);
    w->owned_screen=item.result.value.screen;
    ScreenDescriptionResult owned=rohr_screen_description_get(w->owned_screen); OK(owned);
    assert(owned.result.value.content_mode==SCREEN_CONTENT_AUTOMATIC);
    placement.orientation=-8; placement.rectangle=(ViewportRectangle){20,350,260,170};
    ViewportItemIdResult shared=rohr_viewport_screen_add(w->views[1],w->owned_screen,placement); OK(shared);
    w->shared_mount=shared.result.value;

    /* A separately created screen stays caller-owned when its mount is removed. */
    ScreenDescription screen=rohr_screen_description_default_get();
    screen.camera=w->cameras[0]; screen.content_mode=SCREEN_CONTENT_AUTOMATIC;
    screen.width=160; screen.height=90;
    ScreenIdResult borrowed=rohr_screen_create(screen); OK(borrowed); w->borrowed_screen=borrowed.result.value;
    ViewportItemIdResult mounted=rohr_viewport_screen_add(w->views[1],w->borrowed_screen,placement); OK(mounted);
    OK(rohr_viewport_item_remove(mounted.result.value));
    OK(rohr_screen_description_get(w->borrowed_screen));

    TextAssetResult text=rohr_graphics_text_create((TextDescription){
        .value="Toggle orange body",.color={255,255,255,255}}); OK(text); w->caption=text.result.value;
    w->button=example_button_add(w->views[0],w->caption,(UIRect){150,600,300,50},NULL);
    assert(w->button);
    EntityResult late=rohr_entity_create(project.mass_demo.explicit_body); OK(late); w->late=late.result.value;
    OK(rohr_physics_position_set(w->late,(Position){0,-180}));
    assert(rohr_entity_owner_get(w->late).result.value==ENTITY_INVALID);
    return true;
}
static bool toggle(Workflow *w) {
    ObjectAppearanceResult a=rohr_entity_appearance_get(w->bodies[0]); OK(a);
    a.result.value.visible=!a.result.value.visible;
    OK(rohr_entity_appearance_set(w->bodies[0],a.result.value));
    assert(rohr_entity_visible_check(w->bodies[1]));
    return true;
}
static bool draw(Workflow *w) {
    Report frame=rohr_graphics_viewports_show(w->views,2);
    if(frame.header.status==ROHR_REPORT_FAILED || frame.diagnostic_count) return false;
    uint64_t presents=0,cameras=0;
    for(size_t i=0;i<frame.statistic_count;++i) {
        ReportStatistic s=frame.statistics[i];
        if(s.metric_id!=ROHR_REPORT_PROCESSED_COUNT) continue;
        if(s.report_id==ROHR_GRAPHICS_PRESENTATION_REPORT_ID) presents=s.value;
        if(s.report_id==ROHR_GRAPHICS_CAMERAS_REPORT_ID) cameras=s.value;
    }
    assert(presents==1 && cameras==3); /* The mounted shared screen renders once. */
    return !example_button_clicked_check(w->button,&w->armed) || toggle(w);
}
static bool destroy(Workflow *w) {
    GraphicsUiId ui=rohr_viewport_item_description_get(w->button).result.value.ui;
    /* Remove the borrowed mount before its owning viewport is destroyed. */
    OK(rohr_viewport_item_remove(w->shared_mount));
    if(w->projects[0]) { OK(rohr_project_destroy(w->projects[0])); }
    else { OK(rohr_viewport_destroy(w->views[0])); }
    assert(rohr_error_check(rohr_screen_description_get(w->owned_screen)));
    assert(rohr_error_check(rohr_graphics_ui_shape_description_get(ui)));
    assert(rohr_entity_alive_check(w->bodies[1]));
    Report survivor=rohr_graphics_viewport_show(w->views[1]);
    assert(survivor.header.status!=ROHR_REPORT_FAILED && !survivor.diagnostic_count);
    if(w->projects[1]) { OK(rohr_project_destroy(w->projects[1])); }
    else { OK(rohr_viewport_destroy(w->views[1])); }
    OK(rohr_graphics_text_destroy(&w->caption));
    OK(rohr_screen_description_get(w->borrowed_screen));
    OK(rohr_screen_destroy(w->borrowed_screen));
    for(unsigned i=0;i<2;++i) {
        if(!w->projects[i]) {
            OK(rohr_camera_get(w->cameras[i]));
            OK(rohr_entity_delete(w->roots[i]));
        } else { assert(rohr_error_check(rohr_project_root_get(w->projects[i]))); }
        assert(!rohr_entity_alive_check(w->bodies[i]));
        assert(rohr_error_check(rohr_camera_get(w->cameras[i])));
    }
    assert(rohr_entity_alive_check(w->late));
    OK(rohr_entity_delete(w->late));
    assert(rohr_entity_alive_count_get()==0);
    return true;
}
static bool run(bool preview) {
    OK(rohr_engine_start()); OK(rohr_graphics_start()); OK(project_settings_apply());
    OK(rohr_graphics_vsync_set(false)); OK(rohr_graphics_frame_limit_set(preview?60:100000));
    unsigned count=preview?1:2000;
    for(unsigned i=0;i<count;++i) {
        Workflow workflow={0};
        if(!create(&workflow,preview || i>=1000) || !draw(&workflow) || !toggle(&workflow) || !draw(&workflow) || !toggle(&workflow)) return false;
        if(preview) {
            Uint64 start=SDL_GetTicks(); bool quit=false;
            rohr_engine_clock_reset();
            while(!quit && SDL_GetTicks()-start<60000) {
                SDL_Event event; rohr_input_frame_begin();
                while((event=rohr_engine_event_poll()).type) if(event.type==SDL_EVENT_QUIT) quit=true;
                if(rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) quit=true;
                if(quit) break;
                if(rohr_engine_update().header.status==ROHR_REPORT_FAILED ||
                    rohr_physics_update(rohr_engine_tick_delta_get()).header.status==ROHR_REPORT_FAILED || !draw(&workflow)) return false;
            }
        }
        if(!destroy(&workflow)) return false;
    }
    rohr_graphics_stop(); rohr_engine_stop();
    puts(preview?"PASS generated project workflow preview":"PASS 1000 object and 1000 project create/extend/use/destroy cycles, each with standalone entities");
    return true;
}
int main(int argc,char **argv) {
    bool preview=argc==2 && !strcmp(argv[1],"--preview");
    if(argc!=1 && !preview) return 2;
    bool success=run(preview);
    if(!success) { rohr_graphics_stop(); rohr_engine_stop(); }
    return success?0:1;
}
