/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "../src/entity/entity_internal.h"
#include "../src/core/project_internal.h"
#define AUDIO_TEST_PATH "project_creation.wav"
#define AUDIO_TEST_MUSIC_PATH "project_creation.ogg"
#include "audio_fixture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef ROHR_TEST_PROJECT_INTERNAL
#include "entity/entity_internal.h"
#include "audio/audio_internal.h"
#include "graphics/texture_assets.h"
#include "graphics/text_assets.h"
#include "graphics/font_assets.h"
#endif

#define OK(call) do { if(rohr_error_check(call)) { fprintf(stderr,"failed %s:%d: %s\n",__FILE__,__LINE__,#call); abort(); } } while(0)
#define BAD(call) assert(rohr_error_check(call))
#ifdef ROHR_TEST_ALLOCATION_WRAP
static _Thread_local long allocation_countdown = -1;
static bool reject(void) {
    if(allocation_countdown < 0) return false;
    if(allocation_countdown-- == 0) { allocation_countdown = -1; return true; }
    return false;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void *__real_realloc(void *,size_t);
void *__wrap_malloc(size_t n) { return reject() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n,size_t s) { return reject() ? NULL : __real_calloc(n,s); }
void *__wrap_realloc(void *p,size_t n) { return reject() ? NULL : __real_realloc(p,n); }
#endif

typedef struct Fixture {
    ProjectDescription project; /* Canonical root of this hand-written catalog. */
    ObjectDescription left, right, body, camera, follower;
    const ObjectDescription *roots[2], *left_members[1], *right_members[2];
    ViewportCreateItemDescription items[3];
    ViewportCreateDescription viewport[2];
    ProjectViewportDescription viewports[2];
    ProjectControllerDescription controller;
    ProjectActionDescription actions[2];
    ProjectSoundDescription sounds[2];
    ProjectMusicDescription music;
    LayerDescription layer;
    char viewport_name[32], sound_name[32], music_name[32];
} Fixture;
static void fixture_init(Fixture *f) {
    *f=(Fixture){0};
    f->project.root=(ObjectDescription){.id=1,.name="project",.kind=OBJECT_CONTAINER,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.children=f->roots,.child_count=2};
    f->left=(ObjectDescription){.id=2,.name="left",.kind=OBJECT_CONTAINER,.parent=&f->project.root,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.children=f->left_members,.child_count=1};
    f->right=f->left; f->right.id=3; f->right.name="right";
    f->right.children=f->right_members; f->right.child_count=2;
    f->body=(ObjectDescription){.id=4,.name="body",.kind=OBJECT_PARTICLE,.parent=&f->left,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.position={20,30},.radius=8,.mass=1,
        .particle_vertices=12,.color={240,130,30,255},.border_color={255,255,255,255}};
    f->follower=f->body; f->follower.id=5; f->follower.name="follower"; f->follower.parent=&f->right;
    f->follower.entity_settings=(ObjectEntitySettings){.transform_driver={.id=4,.kind=OBJECT_PARTICLE},
        .transform_position=true,.transform_offset={30,10},.gameplay_target={.id=4,.kind=OBJECT_PARTICLE}};
    f->camera=(ObjectDescription){.id=6,.name="camera",.kind=OBJECT_CAMERA,.parent=&f->right,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.size={320,240},.zoom=1,
        .target=4,.follow_position=true};
    f->roots[0]=&f->left; f->roots[1]=&f->right;
    f->left_members[0]=&f->body; f->right_members[0]=&f->camera; f->right_members[1]=&f->follower;
    f->items[0]=(ViewportCreateItemDescription){.name="main",.kind=VIEWPORT_CREATE_SCREEN,
        .placement=rohr_viewport_item_description_default_get(),.camera_object=&f->right,.camera_member=&f->camera,
        .background_color={20,30,45,255},.render_width=320,.render_height=240};
    f->items[0].placement.rectangle=(ViewportRectangle){-15,10,270,210};
    f->items[0].placement.orientation=8;
    f->items[1]=f->items[0]; f->items[1].name="inset";
    f->items[1].placement.rectangle=(ViewportRectangle){180,100,120,100};
    f->items[1].placement.orientation=-12; f->items[1].placement.layer=2;
    f->items[1].placement.clip_enabled=true;
    f->items[1].placement.clip_rectangle=(ViewportRectangle){190,90,100,120};
    f->items[2]=(ViewportCreateItemDescription){.name="label",.kind=VIEWPORT_CREATE_TEXT,
        .placement=rohr_viewport_item_description_default_get(),
        .text=rohr_graphics_ui_text_description_default_get(),.text_value="Project instance",
        .text_color={255,255,255,255}};
    f->items[2].placement.rectangle=(ViewportRectangle){155,25,0,0}; f->items[2].placement.layer=1;
    f->layer=(LayerDescription){"project_ui",1};
    f->items[2].layer_name=f->layer.name;
    for(unsigned i=0;i<2;++i) {
        f->viewport[i]=(ViewportCreateDescription){.viewport=rohr_viewport_description_default_get(),
            .items=f->items,.item_count=3,.layers=&f->layer,.layer_count=1};
        f->viewport[i].viewport.enabled=true;
        f->viewport[i].viewport.rectangle=(ViewportRectangle){i*330.0f,0,320,240};
    }
    strcpy(f->viewport_name,"play"); strcpy(f->sound_name,"jump"); strcpy(f->music_name,"theme");
    f->viewports[0]=(ProjectViewportDescription){f->viewport_name,&f->viewport[0]};
    f->viewports[1]=(ProjectViewportDescription){"second",&f->viewport[1]};
    f->project.viewports=f->viewports; f->project.viewport_count=2;
    f->actions[0]=(ProjectActionDescription){.name="toggle",.type=INPUT_ACTION_BUTTON,
        .button_mode=INPUT_BUTTON_PERSISTENT,.button_initial_state=true};
    f->actions[0].bindings_default.count=1;
    f->actions[0].bindings_default.values[0]=rohr_input_binding_scancode_create(SDL_SCANCODE_SPACE);
    f->actions[1]=(ProjectActionDescription){.name="move",.type=INPUT_ACTION_AXIS_1D};
    f->controller=(ProjectControllerDescription){.controller={.name="player",.enabled=true},
        .actions=f->actions,.action_count=2};
    f->project.controllers=&f->controller; f->project.controller_count=1;
    f->sounds[0]=(ProjectSoundDescription){f->sound_name,{.path=AUDIO_TEST_PATH,.volume=.3f,.pan=-.25f,.loop=true,.playback_rate=1.2f}};
    f->sounds[1]=(ProjectSoundDescription){"land",{.path=AUDIO_TEST_PATH,.volume=.6f,.pan=.5f,.loop=false,.playback_rate=.8f}};
    f->music=(ProjectMusicDescription){f->music_name,{.path=AUDIO_TEST_MUSIC_PATH,.volume=.4f,.loop=true,.playback_rate=.9f}};
    f->project.sounds=f->sounds; f->project.sound_count=2;
    f->project.music=&f->music; f->project.music_count=1;
}
static ProjectId create(Fixture *f) { ProjectIdResult r=rohr_project_create(f->project); OK(r); return r.result.value; }
static Entity root(ProjectId p) { EntityResult r=project_root_get(p); OK(r); return r.result.value; }
static Entity member(Entity p,const char *name) { EntityResult r=object_member_get(p,name); OK(r); return r.result.value; }
static Entity body(ProjectId p) { return member(member(root(p),"left"),"body"); }
static ViewportId viewport(ProjectId p) { ViewportIdResult r=project_viewport_by_name_get(p,"play"); OK(r); return r.result.value; }
static InputControllerId controller(ProjectId p) { InputControllerIdResult r=project_controller_by_name_get(p,"player"); OK(r); return r.result.value; }
static InputActionId toggle(ProjectId p) { InputActionIdResult r=rohr_input_action_by_name_get(controller(p),"toggle"); OK(r); return r.result.value; }
static Sound sound(ProjectId p) { SoundResult r=project_sound_by_name_get(p,"jump"); OK(r); return r.result.value; }
static Music music(ProjectId p) { MusicResult r=project_music_by_name_get(p,"theme"); OK(r); return r.result.value; }
static ScreenDescription screen(ViewportId v,const char *name) {
    ViewportItemIdResult i=rohr_viewport_item_by_name_get(v,name); OK(i);
    ViewportItemDescriptionResult d=rohr_viewport_item_description_get(i.result.value); OK(d);
    ScreenDescriptionResult s=rohr_screen_description_get(d.result.value.screen); OK(s); return s.result.value;
}
static void show(ProjectId p) {
    ViewportListResult list=project_viewports_get(p); OK(list);
    Report r=rohr_graphics_viewports_show(list.result.value.values,list.result.value.count);
    assert(r.header.status==ROHR_REPORT_COMPLETE && r.diagnostic_count==0);
    bool cameras=false, commands=false;
    for(size_t i=0;i<r.statistic_count;++i) {
        ReportStatistic statistic=r.statistics[i];
        if(statistic.metric_id!=ROHR_REPORT_PROCESSED_COUNT || !statistic.value) continue;
        cameras |= statistic.report_id==ROHR_GRAPHICS_CAMERAS_REPORT_ID;
        commands |= statistic.report_id==ROHR_GRAPHICS_COMMANDS_REPORT_ID;
    }
    assert(cameras && commands);
}
static void wiring(ProjectId p) {
    Entity b=body(p), right=member(root(p),"right"), camera=member(right,"camera");
    CameraIdResult c=rohr_entity_object_camera_get(camera); OK(c);
    assert(rohr_camera_attachment_description_get(c.result.value).result.value.entity==b);
    assert(rohr_physics_transform_lock_description_get(member(right,"follower")).result.value.driver==b);
    assert(screen(viewport(p),"main").camera==c.result.value);
    assert(screen(viewport(p),"inset").camera_entity==camera);
    ViewportListResult list=project_viewports_get(p); OK(list);
    assert(list.result.value.count==2 && list.result.value.values[0]==viewport(p));
    assert(list.result.value.values[1]==project_viewport_by_name_get(p,"second").result.value);
}
static void audio_counts(size_t players,size_t resources,size_t tracks) {
#ifdef ROHR_TEST_PROJECT_INTERNAL
    AudioOwnershipStats s=audio_ownership_stats_get();
    assert(s.sound_players==players && s.sound_resources==resources && s.sound_references==players && s.music_instances==tracks);
#else
    (void)players; (void)resources; (void)tracks;
#endif
}
static void two_instances(void) {
    Fixture f; fixture_init(&f);
    Tick tick=rohr_engine_tick_get(); Time time=rohr_engine_time_get();
    OK(rohr_engine_time_per_tick_set(.02));
    float volume=rohr_audio_volume_get();
    Acceleration gravity=rohr_physics_gravity_get();
    WindowPresentationDescription window=rohr_graphics_window_presentation_description_get();
    ProjectId a=create(&f), b=create(&f);
    wiring(a); wiring(b); show(a); show(b);
    assert(body(a)!=body(b) && viewport(a)!=viewport(b) && controller(a)!=controller(b));
    assert(sound(a)!=sound(b) && music(a)!=music(b));
    assert(rohr_engine_tick_get()==tick && rohr_engine_time_get()==time);
    assert(rohr_engine_time_per_tick_get()==.02 && rohr_audio_volume_get()==volume);
    Acceleration current_gravity=rohr_physics_gravity_get();
    assert(current_gravity.x==gravity.x && current_gravity.y==gravity.y);
    WindowPresentationDescription current_window=rohr_graphics_window_presentation_description_get();
    assert(current_window.window_width==window.window_width && current_window.window_height==window.window_height &&
        current_window.mode==window.mode);
    assert(rohr_audio_sound_volume_get(sound(a)).result.value==.3f);
    assert(rohr_audio_sound_pan_get(sound(a)).result.value==-.25f);
    assert(rohr_audio_sound_playback_rate_get(sound(a)).result.value==1.2f);
    assert(rohr_audio_sound_loop_check(sound(a)) && rohr_audio_music_loop_check(music(a)));
    assert(rohr_audio_music_volume_get(music(a)).result.value==.4f);
    assert(rohr_audio_music_playback_rate_get(music(a)).result.value==.9f);
    assert(!rohr_audio_sound_playing_check(sound(a)) && !rohr_audio_music_playing_check(music(a)));
    audio_counts(4,1,2);
    assert(rohr_input_action_button_down_check(toggle(a)) && rohr_input_action_button_down_check(toggle(b)));
    OK(rohr_input_action_button_state_set(toggle(a),false));
    assert(!rohr_input_action_button_down_check(toggle(a)) && rohr_input_action_button_down_check(toggle(b)));
    OK(rohr_physics_position_set(body(a),(Position){80,90}));
    assert(rohr_physics_position_get(body(b)).result.value.x==20);
    ObjectAppearance appearance=rohr_entity_appearance_get(body(a)).result.value;
    appearance.visible=false; OK(rohr_entity_appearance_set(body(a),appearance));
    assert(rohr_entity_appearance_get(body(b)).result.value.visible);
    /* All names, mappings, and descriptors are borrowed only for creation. */
    strcpy(f.viewport_name,"changed"); strcpy(f.sound_name,"changed"); strcpy(f.music_name,"changed");
    strcpy(f.controller.controller.name,"changed");
    assert(viewport(a) && controller(a) && sound(a) && music(a));
    Entity old_body=body(a); InputActionId old_action=toggle(a); Sound old_sound=sound(a);
    OK(rohr_entity_delete(old_body)); BAD(object_member_get(member(root(a),"left"),"body"));
    Entity replacement=rohr_entity_create(f.body).result.value; assert(replacement && replacement!=old_body);
    OK(rohr_project_destroy(a)); BAD(project_root_get(a)); BAD(rohr_project_destroy(a));
    BAD(rohr_input_action_type_get(old_action)); assert(!rohr_audio_sound_valid_check(old_sound));
    assert(rohr_entity_alive_check(replacement)); audio_counts(2,1,1); wiring(b); show(b);
    OK(rohr_entity_delete(replacement)); OK(rohr_project_destroy(b)); audio_counts(0,0,0);
    OK(rohr_project_destroy(ROHR_PROJECT_INVALID));
    assert(rohr_entity_alive_count_get()==0);
}
static void failed(Fixture *f,ProjectId survivor,uint32_t alive) {
    BAD(rohr_project_create(f->project));
    assert(rohr_entity_alive_count_get()==alive);
    audio_counts(2,1,1); wiring(survivor);
    assert(controller(survivor) && sound(survivor) && music(survivor));
}
static void failures(void) {
    Fixture f; fixture_init(&f);
    ProjectId survivor=create(&f); uint32_t alive=rohr_entity_alive_count_get();
    BAD(rohr_internal_project_create(NULL)); BAD(project_root_get(UINT64_MAX));
    BAD(project_controller_by_name_get(survivor,NULL)); BAD(project_sound_by_name_get(survivor,"absent"));
    BAD(project_music_by_name_get(survivor,"absent")); BAD(project_viewport_by_name_get(survivor,"absent"));
    f.project.root.parent=&f.right; failed(&f,survivor,alive); f.project.root.parent=NULL;
    f.project.viewport_count=MAX_VIEWPORTS+1; failed(&f,survivor,alive); f.project.viewport_count=2;
    f.viewports[1].name=f.viewports[0].name; failed(&f,survivor,alive); f.viewports[1].name="second";
    f.items[0].camera_object=&f.body; failed(&f,survivor,alive); f.items[0].camera_object=&f.right;
    ObjectDescription external=f.right; f.items[0].camera_object=&external;
    failed(&f,survivor,alive); f.items[0].camera_object=&f.right;
    f.viewport[1].template=&f.viewport[0]; failed(&f,survivor,alive); f.viewport[1].template=NULL;
    strcpy(f.actions[1].name,"toggle"); failed(&f,survivor,alive); strcpy(f.actions[1].name,"move");
    f.actions[1].bindings_default.count=ROHR_INPUT_BINDING_LIMIT+1;
    failed(&f,survivor,alive); f.actions[1].bindings_default.count=0;
    f.sounds[1].description.path="missing_project_sound.wav"; failed(&f,survivor,alive);
    f.sounds[1].description.path=AUDIO_TEST_PATH;
    f.music.description.path="missing_project_music.ogg";
    strcpy(f.body.layer_name,"failed_project_layer"); f.body.layer=7;
    f.layer.name="failed_viewport_layer"; f.items[2].layer_name=f.layer.name;
    ProjectIdResult missing=rohr_project_create(f.project); BAD(missing);
    assert(missing.result.error==ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    failed(&f,survivor,alive);
    BAD(rohr_graphics_layer_name_id_get("failed_project_layer"));
    BAD(rohr_graphics_layer_name_id_get("failed_viewport_layer"));
    f.music.description.path=AUDIO_TEST_MUSIC_PATH; f.body.layer_name[0]=0;
    f.layer.name="project_ui"; f.items[2].layer_name=f.layer.name;
#ifdef ROHR_TEST_ALLOCATION_WRAP
#ifdef ROHR_TEST_PROJECT_INTERNAL
    GraphicsTextAssetStats text_before=graphics_text_assets_stats_get();
    GraphicsFontAssetStats font_before=graphics_font_assets_stats_get();
#endif
    unsigned rejected=0; bool complete=false;
    for(long point=0;point<2048;++point) {
        allocation_countdown=point;
        ProjectIdResult r=rohr_project_create(f.project);
        allocation_countdown=-1;
        if(!rohr_error_check(r)) { OK(rohr_project_destroy(r.result.value)); complete=true; break; }
        ++rejected; assert(rohr_entity_alive_count_get()==alive); audio_counts(2,1,1);
        assert(sound(survivor) && controller(survivor));
#ifdef ROHR_TEST_PROJECT_INTERNAL
        GraphicsTextAssetStats text_after=graphics_text_assets_stats_get();
        GraphicsFontAssetStats font_after=graphics_font_assets_stats_get();
        assert(text_before.live_resources==text_after.live_resources &&
            text_before.public_owners==text_after.public_owners &&
            text_before.dependency_references==text_after.dependency_references &&
            text_before.live_payloads==text_after.live_payloads);
        assert(font_before.live_resources==font_after.live_resources && font_before.owner_references==font_after.owner_references);
#endif
    }
    assert(complete && rejected>10);
    printf("allocation rollback points=%u\n",rejected);
#endif
    ProjectId recovery=create(&f); OK(rohr_project_destroy(recovery));
    OK(rohr_project_destroy(survivor)); audio_counts(0,0,0); assert(rohr_entity_alive_count_get()==0);
}
static void capacities(void) {
    Fixture f; fixture_init(&f);
    /* One sound slot remains: failure after loading the project's first sound. */
    Sound sounds[ROHR_AUDIO_SOUND_LIMIT-1];
    for(size_t i=0;i<ROHR_AUDIO_SOUND_LIMIT-1;++i) { SoundResult s=rohr_audio_sound_create(f.sounds[0].description); OK(s); sounds[i]=s.result.value; }
    ProjectIdResult r=rohr_project_create(f.project); BAD(r); assert(r.result.error==ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
    assert(rohr_entity_alive_count_get()==0); audio_counts(ROHR_AUDIO_SOUND_LIMIT-1,1,0);
    for(size_t i=0;i<ROHR_AUDIO_SOUND_LIMIT-1;++i) OK(rohr_audio_sound_destroy(&sounds[i]));
    Music tracks[ROHR_AUDIO_MUSIC_LIMIT];
    for(size_t i=0;i<ROHR_AUDIO_MUSIC_LIMIT;++i) { MusicResult m=rohr_audio_music_create(f.music.description); OK(m); tracks[i]=m.result.value; }
    r=rohr_project_create(f.project); BAD(r); assert(r.result.error==ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED);
    audio_counts(0,0,ROHR_AUDIO_MUSIC_LIMIT);
    for(size_t i=0;i<ROHR_AUDIO_MUSIC_LIMIT;++i) OK(rohr_audio_music_destroy(&tracks[i]));
    InputControllerId controllers[ROHR_INPUT_CONTROLLER_LIMIT];
    for(size_t i=0;i<ROHR_INPUT_CONTROLLER_LIMIT;++i) {
        InputControllerDescription d={.enabled=true}; snprintf(d.name,sizeof(d.name),"external_%zu",i);
        InputControllerIdResult c=rohr_input_controller_create(d); OK(c); controllers[i]=c.result.value;
    }
    r=rohr_project_create(f.project); BAD(r); assert(r.result.error==ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED);
    assert(rohr_entity_alive_count_get()==0);
    for(size_t i=0;i<ROHR_INPUT_CONTROLLER_LIMIT;++i) OK(rohr_input_controller_destroy(controllers[i]));
    ViewportId views[MAX_VIEWPORTS-1];
    for(size_t i=0;i<MAX_VIEWPORTS-1;++i) {
        ViewportIdResult v=rohr_viewport_create(rohr_viewport_description_default_get()); OK(v); views[i]=v.result.value;
    }
    BAD(rohr_project_create(f.project)); assert(rohr_entity_alive_count_get()==0);
    for(size_t i=0;i<MAX_VIEWPORTS-1;++i) OK(rohr_viewport_destroy(views[i]));
    ProjectDescription empty={.root={.id=1,.name="empty",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL}};
    ProjectId owners[ROHR_PROJECT_LIMIT];
    for(size_t i=0;i<ROHR_PROJECT_LIMIT;++i) { ProjectIdResult p=rohr_project_create(empty); OK(p); owners[i]=p.result.value; }
    BAD(rohr_project_create(empty));
    for(size_t i=0;i<ROHR_PROJECT_LIMIT;++i) OK(rohr_project_destroy(owners[i]));
    ProjectId recovery=create(&f); OK(rohr_project_destroy(recovery));
}
static void expired_root(void) {
    Fixture f; fixture_init(&f); ProjectId p=create(&f);
    Entity old=root(p);
    OK(rohr_entity_lifetime_description_set(old,(EntityLifetimeDescription){
        .tick_enabled=true,.expiry_tick=rohr_engine_tick_get()}));
    Report report=rohr_engine_lifecycle_update(); assert(report.header.status==ROHR_REPORT_COMPLETE);
    BAD(project_root_get(p)); assert(!rohr_entity_alive_check(old));
    EntityResult replacement=rohr_object_create(f.project); OK(replacement);
    assert(replacement.result.value!=old);
    OK(rohr_project_destroy(p)); assert(rohr_entity_alive_check(replacement.result.value));
    OK(rohr_entity_delete(replacement.result.value)); audio_counts(0,0,0);
}
static void lifecycle(void) {
    Fixture f; fixture_init(&f);
    ProjectId survivor=create(&f);
    uint32_t alive=rohr_entity_alive_count_get();
#ifdef ROHR_TEST_PROJECT_INTERNAL
    GraphicsTextAssetStats text_before=graphics_text_assets_stats_get();
    GraphicsFontAssetStats font_before=graphics_font_assets_stats_get();
    GraphicsTextureAssetStats texture_before=graphics_texture_assets_stats_get();
#endif
    for(unsigned i=0;i<1000;++i) {
        ProjectId p=create(&f);
        ViewportUiShapeDescription button=rohr_graphics_ui_shape_description_default_get();
        button.shape=rohr_math_square_create(30,15); button.border_enabled=true;
        ViewportItemDescription placement=rohr_viewport_item_description_default_get();
        placement.rectangle=(ViewportRectangle){40,45,0,0}; placement.layer=5;
        OK(rohr_viewport_ui_shape_add(viewport(p),button,placement));
        if(i==0 || i==999) { wiring(p); show(p); }
        ViewportId old=viewport(p); InputControllerId old_controller=controller(p);
        OK(rohr_project_destroy(p));
        BAD(project_viewports_get(p)); BAD(rohr_viewport_description_get(old));
        BAD(rohr_input_controller_description_get(old_controller));
        audio_counts(2,1,1);
        assert(rohr_entity_alive_check(body(survivor)));
        assert(rohr_entity_alive_count_get()==alive);
#ifdef ROHR_TEST_PROJECT_INTERNAL
        GraphicsTextAssetStats text_after=graphics_text_assets_stats_get();
        GraphicsFontAssetStats font_after=graphics_font_assets_stats_get();
        GraphicsTextureAssetStats texture_after=graphics_texture_assets_stats_get();
        assert(text_before.live_resources==text_after.live_resources &&
            text_before.public_owners==text_after.public_owners &&
            text_before.dependency_references==text_after.dependency_references &&
            text_before.live_payloads==text_after.live_payloads);
        assert(font_before.live_resources==font_after.live_resources && font_before.owner_references==font_after.owner_references);
        assert(texture_before.live_resources==texture_after.live_resources &&
            texture_before.owner_references==texture_after.owner_references &&
            texture_before.command_references==texture_after.command_references);
#endif
    }
    OK(rohr_project_destroy(survivor)); audio_counts(0,0,0);
    assert(rohr_entity_alive_count_get()==0);
}
static void restart(void) {
    Fixture f; fixture_init(&f);
    ProjectId old=create(&f); InputControllerId c=controller(old); InputActionId a=toggle(old);
    Sound s=sound(old); Music m=music(old); ViewportId v=viewport(old);
    /* Ordinary app order destroys graphics before engine; remaining projects
     * must still release input/audio and forget stale graphics handles safely. */
    rohr_graphics_stop(); rohr_engine_stop();
    BAD(project_root_get(old)); BAD(rohr_project_destroy(old));
    OK(rohr_engine_start()); OK(rohr_graphics_start()); OK(rohr_audio_start());
    ProjectId fresh=create(&f); assert(fresh!=old && controller(fresh)!=c && toggle(fresh)!=a);
    BAD(rohr_input_controller_description_get(c)); BAD(rohr_input_action_type_get(a));
    BAD(rohr_viewport_description_get(v)); assert(!rohr_audio_sound_valid_check(s) && !rohr_audio_music_valid_check(m));
    OK(rohr_project_destroy(fresh));
}
static void prerequisites(void) {
    Fixture f; fixture_init(&f); BAD(rohr_project_create(f.project));
    OK(rohr_engine_start()); BAD(rohr_project_create(f.project));
    ProjectDescription empty={.root={.id=1,.name="empty",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL}};
    ProjectIdResult p=rohr_project_create(empty); OK(p); OK(rohr_project_destroy(p.result.value));
    /* Empty viewport collections do not conceal camera/texture prerequisites. */
    f.project.viewport_count=0; BAD(rohr_project_create(f.project)); f.project.viewport_count=2;
    OK(rohr_graphics_start()); BAD(rohr_project_create(f.project));
    assert(rohr_entity_alive_count_get()==0); OK(rohr_audio_start());
}
static void runtime_names_and_reuse(void) {
    Fixture f; fixture_init(&f);
    ProjectId first = create(&f), second = create(&f);
    Entity old_root = root(first), old_body = body(first);
    ViewportId old_viewport = viewport(first);
    InputControllerId old_controller = controller(first);
    Sound old_sound = sound(first); Music old_music = music(first);
    assert(!strcmp(rohr_entity_name_get(old_root).result.value.value, "project"));
    assert(!strcmp(rohr_entity_name_get(old_body).result.value.value, "body"));
    assert(rohr_entity_by_name_get("body_1").result.value == body(second));
    assert(rohr_viewport_by_name_get("play").result.value == old_viewport);
    assert(rohr_viewport_by_name_get("play_1").result.value == viewport(second));
    assert(rohr_input_controller_by_name_get("player").result.value == old_controller);
    assert(rohr_input_controller_by_name_get("player_1").result.value == controller(second));
    assert(rohr_audio_sound_by_name_get("jump").result.value == old_sound);
    assert(rohr_audio_sound_by_name_get("jump_1").result.value == sound(second));
    assert(rohr_audio_music_by_name_get("theme").result.value == old_music);
    assert(rohr_audio_music_by_name_get("theme_1").result.value == music(second));
    wiring(first); wiring(second);

    Entity right = member(root(second), "right"), camera = member(right, "camera");
    OK(rohr_entity_name_set(camera, "renamed_camera"));
#ifdef ROHR_TEST_PROJECT_INTERNAL
    /* Even the temporary legacy member alias is not an authored binding key. */
    OK(entity_object_member_restore(right, camera, "changed_alias"));
    BAD(object_member_get(right, "camera"));
#endif
    ViewportObjectBinding binding = {&f.right, right};
    ViewportCreateDescription bound = {.template = &f.viewport[0], .objects = &binding, .object_count = 1};
    ViewportIdResult extra = rohr_viewport_create(bound); OK(extra);
    assert(screen(extra.result.value, "main").camera_entity == camera);
    OK(rohr_viewport_destroy(extra.result.value));
#ifdef ROHR_TEST_PROJECT_INTERNAL
    OK(entity_object_member_restore(right, camera, "camera"));
#endif

    OK(rohr_entity_delete(old_root)); OK(rohr_viewport_destroy(old_viewport));
    OK(rohr_input_controller_destroy(old_controller));
    Sound removed_sound = old_sound; Music removed_music = old_music;
    OK(rohr_audio_sound_destroy(&removed_sound)); OK(rohr_audio_music_destroy(&removed_music));
    EntityResult replacement = rohr_entity_description_add((EntityDescription){.name = "project"}); OK(replacement);
    ViewportIdResult replacement_viewport = rohr_viewport_create((ViewportDescription){.name = "play"}); OK(replacement_viewport);
    InputControllerIdResult replacement_controller = rohr_controller_create(f.controller); OK(replacement_controller);
    SoundDescription sd = f.sounds[0].description; strcpy(sd.name, "jump");
    MusicDescription md = f.music.description; strcpy(md.name, "theme");
    SoundResult replacement_sound = rohr_audio_sound_create(sd); OK(replacement_sound);
    MusicResult replacement_music = rohr_audio_music_create(md); OK(replacement_music);
    assert(replacement.result.value != old_root);
    assert(replacement_viewport.result.value != old_viewport);
    assert((replacement_viewport.result.value & 0xffu) == (old_viewport & 0xffu));
    assert(replacement_controller.result.value != old_controller);
    assert((replacement_controller.result.value & 0xffffu) == (old_controller & 0xffffu));
    assert(replacement_sound.result.value != old_sound);
    assert((replacement_sound.result.value & 0xffffu) == (old_sound & 0xffffu));
    assert(replacement_music.result.value != old_music);
    assert((replacement_music.result.value & 0xffffu) == (old_music & 0xffffu));
    OK(rohr_project_destroy(first));
    assert(rohr_entity_alive_check(replacement.result.value));
    assert(rohr_viewport_by_name_get("play").result.value == replacement_viewport.result.value);
    assert(rohr_input_controller_by_name_get("player").result.value == replacement_controller.result.value);
    assert(rohr_audio_sound_by_name_get("jump").result.value == replacement_sound.result.value);
    assert(rohr_audio_music_by_name_get("theme").result.value == replacement_music.result.value);
    wiring(second); OK(rohr_project_destroy(second));
    OK(rohr_entity_delete(replacement.result.value)); OK(rohr_viewport_destroy(replacement_viewport.result.value));
    OK(rohr_input_controller_destroy(replacement_controller.result.value));
    OK(rohr_audio_sound_destroy(&replacement_sound.result.value)); OK(rohr_audio_music_destroy(&replacement_music.result.value));
    assert(rohr_entity_alive_count_get() == 0);
}

static void preview(void) {
    Fixture f; fixture_init(&f);
    WindowPresentationDescription window=rohr_graphics_window_presentation_description_get();
    window.window_width=700; window.window_height=620; window.logical_width=700; window.logical_height=620;
    OK(rohr_graphics_window_presentation_description_set(window));
    f.items[2].text_value="First project - orange body toggles";
    ProjectId a=create(&f);
    f.items[2].text_value="Second project - blue body stays visible";
    f.project.root.position.x=1000;
    f.body.color=(Color){50,180,255,255}; f.follower.color=f.body.color;
    ProjectId b=create(&f);
    ProjectId projects[]={a,b}; ViewportId batch[4];
    for(unsigned row=0;row<2;++row) {
        ViewportListResult list=project_viewports_get(projects[row]); OK(list);
        for(unsigned col=0;col<2;++col) {
            ViewportId id=list.result.value.values[col]; batch[row*2+col]=id;
            ViewportDescriptionResult d=rohr_viewport_description_get(id); OK(d);
            d.result.value.rectangle.y=40+row*290; d.result.value.rectangle.x+=20;
            OK(rohr_viewport_description_set(id,d.result.value));
        }
    }
    OK(rohr_graphics_frame_limit_set(60));
    Uint64 started=SDL_GetTicks(); bool running=true;
    while(running && SDL_GetTicks()-started<60000) {
        SDL_Event event; while(SDL_PollEvent(&event)) if(event.type==SDL_EVENT_QUIT) running=false;
        double t=(SDL_GetTicks()-started)/1000.0;
        ObjectAppearance appearance=rohr_entity_appearance_get(body(a)).result.value;
        appearance.visible=((unsigned)(t/6)%2)==0; OK(rohr_entity_appearance_set(body(a),appearance));
        for(unsigned i=0;i<2;++i) {
            Entity driven=member(member(root(projects[i]),"right"),"follower");
            OK(rohr_physics_position_set(driven,(Position){(float)(20+i*1000+60*cos(t)),(float)(30+40*sin(t))}));
        }
        Report report=rohr_graphics_viewports_show(batch,4); assert(report.header.status==ROHR_REPORT_COMPLETE);
    }
    OK(rohr_project_destroy(a)); OK(rohr_project_destroy(b));
}
int main(int argc,char **argv) {
    assert(audio_test_wav_write() && audio_test_ogg_write());
    prerequisites(); OK(rohr_graphics_vsync_set(false)); OK(rohr_graphics_frame_limit_set(100000));
    if(argc>1 && !strcmp(argv[1],"--preview")) preview();
    else { runtime_names_and_reuse(); two_instances(); failures(); capacities(); expired_root(); lifecycle(); restart(); }
    rohr_graphics_stop(); rohr_engine_stop();
    assert(remove(AUDIO_TEST_PATH)==0 && remove(AUDIO_TEST_MUSIC_PATH)==0);
    return 0;
}
