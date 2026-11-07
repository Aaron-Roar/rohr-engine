/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "test_png.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) do { if(rohr_error_check(call)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#call); abort(); } } while(0)
#define BAD(call) assert(rohr_error_check(call))
static Entity entity_value(EntityResult r) { OK(r); return r.result.value; }
static ViewportId viewport_value(ViewportIdResult r) { OK(r); return r.result.value; }
static uint64_t metric(Report r, ReportId owner, ReportMetricId id) {
    assert(rohr_report_valid_check(&r));
    for(size_t i=0;i<r.statistic_count;++i)
        if(r.statistics[i].report_id==owner && r.statistics[i].metric_id==id) return r.statistics[i].value;
    return 0;
}
static Report show(ViewportId v) {
    Report r=rohr_graphics_viewport_show(v);
    assert(r.header.status!=ROHR_REPORT_FAILED);
    return r;
}
#ifdef ROHR_TEST_VIEWPORT_WRAP
#include "graphics/font_assets.h"
#include "graphics/text_assets.h"
static unsigned present_count, clear_count;
static int fail_texture_after=-1;
static bool trace;
static unsigned color_order[256], color_count;
static float pointer_x,pointer_y;
static bool pointer_down;
bool __real_SDL_RenderPresent(SDL_Renderer *r);
bool __wrap_SDL_RenderPresent(SDL_Renderer *r) { present_count++; return __real_SDL_RenderPresent(r); }
bool __real_SDL_RenderClear(SDL_Renderer *r);
bool __wrap_SDL_RenderClear(SDL_Renderer *r) { clear_count++; return __real_SDL_RenderClear(r); }
SDL_Texture *__real_SDL_CreateTexture(SDL_Renderer *,SDL_PixelFormat,SDL_TextureAccess,int,int);
SDL_Texture *__wrap_SDL_CreateTexture(SDL_Renderer *r,SDL_PixelFormat f,SDL_TextureAccess a,int w,int h) {
    if(fail_texture_after>=0 && fail_texture_after--==0) { SDL_SetError("injected screen allocation"); return NULL; }
    return __real_SDL_CreateTexture(r,f,a,w,h);
}
bool __real_SDL_RenderGeometry(SDL_Renderer *,SDL_Texture *,const SDL_Vertex *,int,const int *,int);
bool __wrap_SDL_RenderGeometry(SDL_Renderer *r,SDL_Texture *t,const SDL_Vertex *v,int n,const int *i,int m) {
    if(trace && !t && n && color_count<256) color_order[color_count++]=(unsigned)(v[0].color.r*255+.5f);
    return __real_SDL_RenderGeometry(r,t,v,n,i,m);
}
SDL_MouseButtonFlags __wrap_SDL_GetMouseState(float *x,float *y) {
    if(x) *x=pointer_x;
    if(y) *y=pointer_y;
    return pointer_down ? SDL_BUTTON_LMASK : 0;
}
#endif
static const ObjectDescription catalog, camera_template, particle_template;
static const ObjectDescription *const children[]={&camera_template,&particle_template};
static const ObjectDescription catalog={.id=100,.name="actors",.kind=OBJECT_CONTAINER,
    .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.children=children,.child_count=2};
static const ObjectDescription camera_template={.id=101,.name="camera",.kind=OBJECT_CAMERA,
    .parent=&catalog,.categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.size={400,300},.zoom=1};
static const ObjectDescription particle_template={.id=102,.name="particle",.kind=OBJECT_PARTICLE,
    .parent=&catalog,.categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.radius=15,.mass=1,
    .particle_vertices=12,.color={201,70,40,255},.border_color={255,255,255,255}};
static Entity member(Entity root,const char *name) { return entity_value(rohr_object_member_get(root,name)); }
static ViewportCreateItemDescription screen_item(const char *name) {
    ViewportCreateItemDescription item={.name=name,.kind=VIEWPORT_CREATE_SCREEN,
        .placement=rohr_viewport_item_description_default_get(),.render_width=320,.render_height=240,
        .background_color={18,22,30,255},.camera_object=&catalog,.camera_member=&camera_template};
    item.placement.rectangle=(ViewportRectangle){0,0,320,240};
    return item;
}
static ViewportCreateDescription recipe(ViewportCreateItemDescription *items,size_t count) {
    ViewportCreateDescription d={.viewport=rohr_viewport_description_default_get(),.items=items,.item_count=count};
    d.viewport.enabled=true; d.viewport.rectangle=(ViewportRectangle){20,20,600,400}; return d;
}
static ScreenId screen_get(ViewportId viewport,const char *name) {
    ViewportItemIdResult item=rohr_viewport_item_by_name_get(viewport,name); OK(item);
    ViewportItemDescriptionResult d=rohr_viewport_item_description_get(item.result.value); OK(d);
    return d.result.value.screen;
}
static unsigned callbacks;
static bool recurse;
static ViewportId recursion_viewport;
static void callback(CameraId camera,void *context) {
    (void)camera; (void)context; callbacks++;
    if(recurse) {
        Report r=rohr_graphics_viewport_show(recursion_viewport);
        assert(r.header.status==ROHR_REPORT_FAILED);
    }
    rohr_graphics_background_draw((Color){1,2,3,255});
}
static void selection_and_rendering(Entity root) {
    ViewportCreateItemDescription items[2]={screen_item("world"),screen_item("detail")};
    items[1].placement.rectangle=(ViewportRectangle){250,80,240,180};
    items[1].placement.orientation=25; items[1].placement.layer=20;
    items[1].placement.clip_enabled=true; items[1].placement.clip_rectangle=(ViewportRectangle){0,0,420,300};
    items[1].render_width=240; items[1].render_height=180;
    ViewportCreateDescription authored=recipe(items,2);
    ViewportObjectBinding binding={&catalog,root};
    ViewportCreateDescription d={.template=&authored,.objects=&binding,.object_count=1};
    ViewportId a=viewport_value(rohr_viewport_create(&d));
    ViewportId b=viewport_value(graphics_viewport_create(&d));
    ScreenId first=screen_get(a,"world"),second=screen_get(a,"detail");
    CameraId camera=rohr_entity_object_camera_get(member(root,"camera")).result.value;
    OK(rohr_camera_render_callback_set(camera,callback,NULL));
    callbacks=0;
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.rectangle=(ViewportRectangle){0,0,60,60}; placement.layer=30;
    OK(rohr_viewport_screen_add(b,first,placement));
    Report r=rohr_graphics_viewports_show((ViewportId[]){a,b},2);
    assert(r.header.status!=ROHR_REPORT_FAILED && callbacks==0);
    assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==4);
    assert(metric(r,ROHR_GRAPHICS_PRESENTATION_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==8);
    ObjectAppearance appearance=rohr_entity_appearance_get(root).result.value;
    appearance.visible=false; OK(rohr_entity_appearance_set(root,appearance));
    r=show(a); assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    assert(rohr_entity_appearance_get(member(root,"particle")).result.value.visible);
    appearance.visible=true; OK(rohr_entity_appearance_set(root,appearance));
    Entity later=entity_value(rohr_entity_create(&particle_template));
    OK(rohr_graphics_layer_entity_set(later,-20));
    ObjectAppearance style=rohr_entity_appearance_get(later).result.value; style.color.red=101;
    OK(rohr_entity_appearance_set(later,style));
#ifdef ROHR_TEST_VIEWPORT_WRAP
    color_count=0; trace=true;
#endif
    r=show(a);
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==8);
#ifdef ROHR_TEST_VIEWPORT_WRAP
    trace=false; assert(color_count==4 && color_order[0]==101 && color_order[1]==101 && color_order[2]==201 && color_order[3]==201);
#endif
    OK(rohr_entity_delete(later));
    /* Invalid selections preserve queued work; selected frames discard window work. */
    assert(rohr_graphics_screen_rect_draw(0,0,10,10,(Color){255,0,0,255}));
#ifdef ROHR_TEST_VIEWPORT_WRAP
    unsigned presents=present_count;
#endif
    assert(rohr_graphics_viewports_show((ViewportId[]){a,a},2).header.status==ROHR_REPORT_FAILED);
    assert(rohr_graphics_viewports_show(NULL,1).header.status==ROHR_REPORT_FAILED);
    assert(rohr_graphics_viewport_show(UINT32_MAX).header.status==ROHR_REPORT_FAILED);
#ifdef ROHR_TEST_VIEWPORT_WRAP
    assert(present_count==presents);
#endif
    r=rohr_graphics_pipeline_commands_execute();
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(rohr_graphics_screen_rect_draw(0,0,10,10,(Color){255,0,0,255}));
    r=show(a); assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_SKIPPED_COUNT)==1);
    r=rohr_graphics_pipeline_cameras_update_for_viewports(&a,1);
    assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    r=rohr_graphics_pipeline_commands_execute();
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==4);
    r=rohr_graphics_pipeline_viewports_draw_selected(&a,1); assert(r.header.status!=ROHR_REPORT_FAILED);
    /* Screen mode is independent of the camera callback. */
    ScreenDescription screen=rohr_screen_description_get(second).result.value;
    screen.content_mode=SCREEN_CONTENT_CALLBACK; OK(rohr_screen_description_set(second,screen));
    recurse=true; recursion_viewport=a; callbacks=0;
    r=show(a); assert(callbacks==1); recurse=false;
    OK(rohr_viewport_disable_set(a));
#ifdef ROHR_TEST_VIEWPORT_WRAP
    unsigned clears=clear_count;
#endif
    r=show(a); assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
#ifdef ROHR_TEST_VIEWPORT_WRAP
    assert(clear_count==clears+1);
#endif
    r=rohr_graphics_viewports_show(NULL,0); assert(r.header.status!=ROHR_REPORT_FAILED);
    OK(rohr_viewport_destroy(a)); BAD(rohr_screen_description_get(first));
    assert(!rohr_error_check(rohr_camera_get(camera)));
    OK(rohr_viewport_destroy(b));
}
static void creation_and_lifetime(Entity root,Entity other) {
#ifdef ROHR_TEST_VIEWPORT_WRAP
    GraphicsFontAssetStats fonts_before=graphics_font_assets_stats_get();
    GraphicsTextAssetStats texts_before=graphics_text_assets_stats_get();
#endif
    ViewportCreateItemDescription items[2]={screen_item("world"),{0}};
    items[1]=(ViewportCreateItemDescription){.name="button",.kind=VIEWPORT_CREATE_SHAPE,
        .placement=rohr_viewport_item_description_default_get(),.shape=rohr_graphics_ui_shape_description_default_get(),
        .text_value="Toggle",.text_color={255,255,255,255}};
    items[1].placement.rectangle=(ViewportRectangle){100,80,0,0};
    items[1].shape.shape=(Shape){.amount_of_vertices=4,.vertices={{-40,-15},{40,-15},{40,15},{-40,15}}};
    items[1].shape.button_enabled=true; items[1].shape.fill_color=(Color){30,60,110,255};
    ViewportCreateDescription authored=recipe(items,2);
    ViewportObjectBinding binding={&catalog,root};
    ViewportCreateDescription d={.template=&authored,.objects=&binding,.object_count=1};
    BAD(rohr_viewport_create(&authored));
    ViewportCameraBinding override={.item_name="world",.entity=member(other,"camera")};
    d.cameras=&override; d.camera_count=1;
    ViewportId v=viewport_value(rohr_viewport_create(&d));
    ScreenDescription screen=rohr_screen_description_get(screen_get(v,"world")).result.value;
    assert(screen.camera_entity==override.entity);
    ViewportItemId button=rohr_viewport_item_by_name_get(v,"button").result.value;
    GraphicsUiId ui=rohr_viewport_item_description_get(button).result.value.ui;
    TextAsset text=rohr_graphics_ui_shape_description_get(ui).result.value.text.text;
#ifdef ROHR_TEST_VIEWPORT_WRAP
    pointer_x=120; pointer_y=100; pointer_down=false; (void)show(v);
    pointer_down=true; (void)show(v); assert(rohr_viewport_ui_pressed_check(button));
    pointer_down=false; (void)show(v);
#endif
    OK(rohr_viewport_item_remove(button));
    BAD(rohr_viewport_item_by_name_get(v,"button"));
    OK(rohr_graphics_ui_shape_description_get(ui));
    OK(rohr_viewport_ui_add(v,ui,items[1].placement));
    CameraId camera=screen.camera;
    ViewportItemIdResult added=rohr_viewport_camera_add(v,camera,items[0].placement); OK(added);
    ScreenId added_screen=rohr_viewport_item_description_get(added.result.value).result.value.screen;
    assert(rohr_screen_description_get(added_screen).result.value.content_mode==SCREEN_CONTENT_AUTOMATIC);
    OK(rohr_camera_disable_set(camera));
    Report paused=show(v); assert(metric(paused,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    OK(rohr_camera_enable_set(camera));
    (void)show(v); /* Initialize both targets before testing paused reuse. */
    CameraDescription camera_config=rohr_camera_description_get(camera).result.value;
    camera_config.pause_with_engine=true; OK(rohr_camera_description_set(camera,camera_config));
    rohr_engine_pause(); paused=show(v);
    assert(metric(paused,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    rohr_engine_resume(); camera_config.pause_with_engine=false; OK(rohr_camera_description_set(camera,camera_config));
    OK(rohr_viewport_destroy(v)); BAD(rohr_screen_description_get(added_screen));
    BAD(rohr_graphics_ui_shape_description_get(ui)); BAD(rohr_graphics_text_description_get(text));
    OK(rohr_camera_get(camera));
    override.item_name="missing"; BAD(rohr_viewport_create(&d)); override.item_name="world";
    override.camera=camera; BAD(rohr_viewport_create(&d)); override.camera=0;
    override.entity=member(root,"particle"); BAD(rohr_viewport_create(&d));
    override.entity=member(other,"camera");
    ViewportCameraBinding duplicate[2]={override,override}; d.cameras=duplicate; d.camera_count=2; BAD(rohr_viewport_create(&d));
    d.cameras=NULL; d.camera_count=0;
    binding.instance=member(root,"particle"); BAD(rohr_viewport_create(&d)); binding.instance=root;
    LayerDescription layer={"recipe_rollback",17}; authored.layers=&layer; authored.layer_count=1;
    items[1].shape.shape.amount_of_vertices=2;
    BAD(rohr_viewport_create(&d)); BAD(rohr_graphics_layer_name_id_get(layer.name));
    OK(rohr_camera_get(camera)); items[1].shape.shape.amount_of_vertices=4;
    layer.name="recipe_shared";
    items[0].layer_name=layer.name;
#ifdef ROHR_TEST_VIEWPORT_WRAP
    for(int fail=0;fail<2;++fail) {
        fail_texture_after=fail;
        ViewportIdResult attempt=rohr_viewport_create(&d);
        if(!rohr_error_check(attempt)) OK(rohr_viewport_destroy(attempt.result.value));
        fail_texture_after=-1;
    }
#endif
    for(unsigned i=0;i<1000;++i) {
        v=viewport_value(rohr_viewport_create(&d));
        OK(rohr_viewport_camera_add(v,camera,items[0].placement));
        if(i%100==0) (void)show(v);
        OK(rohr_viewport_destroy(v));
    }
    GraphicsLayerIdResult named=rohr_graphics_layer_name_id_get(layer.name); OK(named);
    layer.value=19; BAD(rohr_viewport_create(&d));
    assert(rohr_graphics_layer_get(named.result.value).result.value==17);
    OK(rohr_graphics_layer_destroy(named.result.value));
    /* Borrowed low-level screens survive viewport deletion. */
    ScreenIdResult borrowed=rohr_screen_create(rohr_screen_description_default_get()); OK(borrowed);
    ViewportDescription scalar=rohr_viewport_description_default_get();
    ViewportIdResult (*legacy)(ViewportDescription)=rohr_viewport_create;
    v=viewport_value(legacy(scalar)); OK(rohr_viewport_screen_add(v,borrowed.result.value,items[0].placement));
    OK(rohr_viewport_destroy(v)); OK(rohr_screen_description_get(borrowed.result.value));
    OK(rohr_screen_destroy(borrowed.result.value));
#ifdef ROHR_TEST_VIEWPORT_WRAP
    GraphicsFontAssetStats fonts_after=graphics_font_assets_stats_get();
    GraphicsTextAssetStats texts_after=graphics_text_assets_stats_get();
    assert(fonts_before.live_resources==fonts_after.live_resources && fonts_before.owner_references==fonts_after.owner_references);
    assert(texts_before.live_resources==texts_after.live_resources && texts_before.live_payloads==texts_after.live_payloads &&
        texts_before.public_owners==texts_after.public_owners && texts_before.dependency_references==texts_after.dependency_references &&
        texts_before.command_references==texts_after.command_references);
#endif
}
static void stale_camera(Entity root) {
    ViewportCreateItemDescription item=screen_item("world");
    ViewportCreateDescription d=recipe(&item,1);
    ViewportObjectBinding binding={&catalog,root}; d.objects=&binding; d.object_count=1;
    ViewportId v=viewport_value(rohr_viewport_create(&d));
    ScreenId screen=screen_get(v,"world");
    Entity camera_entity=member(root,"camera");
    OK(rohr_entity_delete(camera_entity));
    Report r=show(v); assert(r.header.status==ROHR_REPORT_COMPLETE && r.diagnostic_count>0);
    Entity next=entity_value(rohr_entity_create(&camera_template));
    r=show(v); assert(r.header.status==ROHR_REPORT_COMPLETE && r.diagnostic_count>0);
    OK(rohr_screen_camera_set(screen,rohr_entity_object_camera_get(next).result.value));
    assert(rohr_screen_description_get(screen).result.value.camera_entity==0);
    r=show(v); assert(r.header.status!=ROHR_REPORT_FAILED);
    OK(rohr_viewport_destroy(v)); OK(rohr_entity_delete(next));
}
static void visual_components(Entity root) {
    ViewportCreateItemDescription item=screen_item("world");
    ViewportCreateDescription d=recipe(&item,1);
    ViewportObjectBinding binding={&catalog,root}; d.objects=&binding; d.object_count=1;
    ViewportId v=viewport_value(rohr_viewport_create(&d));
    Entity plain=entity_value(rohr_physics_particle_add((ParticleDescription){.mass_value=1,.radius=5}));
    Report r=show(v); assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    assert(SDL_SaveFile("automatic_pixel.png",test_png,sizeof(test_png)));
    TextureAssetResult texture=rohr_graphics_texture_load((TextureDescription){.file="automatic_pixel.png",.size={10,10}}); OK(texture);
    SpriteDescription sprite=rohr_graphics_sprite_description_default_get(); sprite.texture=texture.result.value;
    sprite.visible=true; OK(rohr_graphics_sprite_add(plain,sprite));
    OK(rohr_graphics_sprite_add(member(root,"particle"),sprite));
    OK(rohr_graphics_texture_release(&texture.result.value));
    r=show(v); assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==4);
    sprite=rohr_graphics_sprite_description_get(plain).result.value; sprite.visible=false;
    OK(rohr_graphics_sprite_description_set(plain,sprite));
    r=show(v); assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==3);
    OK(rohr_entity_delete(plain)); OK(rohr_entity_components_delete(member(root,"particle"),ROHR_SPRITE));
    OK(rohr_viewport_destroy(v)); assert(remove("automatic_pixel.png")==0);
}
static void restored_bindings(void) {
    Entity root=entity_value(rohr_object_create(&catalog));
    EntityName name=rohr_entity_name_get(root).result.value;
    OK(rohr_game_state_file_save("automatic_state.json")); OK(rohr_entity_delete(root));
    OK(rohr_game_state_file_load("automatic_state.json"));
    root=entity_value(rohr_entity_by_name_get(name.value));
    ViewportCreateItemDescription item=screen_item("world"); ViewportCreateDescription d=recipe(&item,1);
    ViewportObjectBinding binding={&catalog,root}; d.objects=&binding; d.object_count=1;
    ViewportId viewport=viewport_value(rohr_viewport_create(&d)); (void)show(viewport);
    OK(rohr_viewport_destroy(viewport)); OK(rohr_entity_delete(root));
    assert(remove("automatic_state.json")==0);
}
static void first_frame_paused(Entity root) {
    CameraId camera=rohr_entity_object_camera_get(member(root,"camera")).result.value;
    CameraDescription config=rohr_camera_description_get(camera).result.value;
    config.pause_with_engine=true; OK(rohr_camera_description_set(camera,config));
    ViewportCreateItemDescription item=screen_item("paused");
    ViewportCreateDescription d=recipe(&item,1);
    ViewportObjectBinding binding={&catalog,root}; d.objects=&binding; d.object_count=1;
    ViewportId viewport=viewport_value(rohr_viewport_create(&d));
    rohr_engine_pause();
    Report r=show(viewport);
    /* A new paused target clears once; it does not discover scene content. */
    assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    r=show(viewport);
    assert(metric(r,ROHR_GRAPHICS_CAMERAS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    rohr_engine_resume();
    r=show(viewport);
    assert(metric(r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    OK(rohr_viewport_destroy(viewport));
    config.pause_with_engine=false; OK(rohr_camera_description_set(camera,config));
}
static void graphics_restart(void) {
    CameraDescription camera_config=rohr_camera_description_default_get();
    CameraIdResult old_camera=rohr_camera_create(camera_config); OK(old_camera);
    ScreenIdResult old_screen=rohr_screen_create(rohr_screen_description_default_get()); OK(old_screen);
    ViewportId old_viewport=viewport_value(rohr_viewport_create(rohr_viewport_description_default_get()));
    ViewportItemIdResult old_item=rohr_viewport_screen_add(old_viewport,old_screen.result.value,
        rohr_viewport_item_description_default_get()); OK(old_item);
    rohr_graphics_stop(); rohr_engine_stop();
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    CameraIdResult camera=rohr_camera_create(camera_config); OK(camera);
    ScreenIdResult screen=rohr_screen_create(rohr_screen_description_default_get()); OK(screen);
    ViewportId viewport=viewport_value(rohr_viewport_create(rohr_viewport_description_default_get()));
    OK(rohr_viewport_screen_add(viewport,screen.result.value,rohr_viewport_item_description_default_get()));
    BAD(rohr_camera_get(old_camera.result.value));
    BAD(rohr_screen_description_get(old_screen.result.value));
    BAD(rohr_viewport_description_get(old_viewport));
    BAD(rohr_viewport_item_description_get(old_item.result.value));
    OK(rohr_viewport_destroy(viewport)); OK(rohr_screen_destroy(screen.result.value));
    OK(rohr_camera_destroy(camera.result.value));
}
static void preview(void) {
    Entity root=entity_value(rohr_object_create(&catalog));
    ViewportCreateItemDescription items[2]={screen_item("world"),screen_item("inset")};
    items[0].render_width=800; items[0].render_height=600;
    items[0].placement.rectangle=(ViewportRectangle){0,0,800,600};
    items[1].placement.rectangle=(ViewportRectangle){630,100,360,240};
    items[1].placement.orientation=12; items[1].placement.layer=20;
    items[1].background_color=(Color){24,38,52,255};
    ViewportCreateDescription d=recipe(items,2); d.viewport.rectangle=(ViewportRectangle){20,20,1200,660};
    ViewportObjectBinding binding={&catalog,root}; d.objects=&binding; d.object_count=1;
    ViewportId viewport=viewport_value(rohr_viewport_create(&d));
    TextAssetResult title=rohr_graphics_text_create((TextDescription){.value="Automatic viewports - click Toggle to hide/show particles",.color={240,240,240,255}}); OK(title);
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.rectangle=(ViewportRectangle){600,25,0,0}; placement.layer=50;
    ViewportUiTextDescription label=rohr_graphics_ui_text_description_default_get(); label.text=title.result.value;
    OK(rohr_viewport_ui_text_add(viewport,label,placement));
    ViewportUiShapeDescription button=rohr_graphics_ui_shape_description_default_get();
    button.shape=rohr_math_square_create(160,44); button.button_enabled=true;
    button.fill_color=(Color){45,80,130,255}; button.hover_fill_color=(Color){70,110,170,255}; button.click_fill_color=(Color){90,140,190,255};
    TextAssetResult caption=rohr_graphics_text_create((TextDescription){.value="Toggle",.color={255,255,255,255}}); OK(caption); button.text.text=caption.result.value;
    placement.rectangle=(ViewportRectangle){950,500,0,0};
    ViewportItemIdResult control=rohr_viewport_ui_shape_add(viewport,button,placement); OK(control);
    Entity swarm[24];
    for(unsigned i=0;i<24;++i) swarm[i]=entity_value(rohr_entity_create(&particle_template));
    OK(rohr_graphics_frame_limit_set(60));
    Uint64 start=SDL_GetTicks(); bool running=true,was_pressed=false;
    while(running && SDL_GetTicks()-start<20000) {
        SDL_Event event; while(SDL_PollEvent(&event)) if(event.type==SDL_EVENT_QUIT) running=false;
        double t=(SDL_GetTicks()-start)/1000.0;
        for(unsigned i=0;i<24;++i) OK(rohr_physics_position_set(swarm[i],(Position){
            (float)(130*cos(t*.5+i*.27)),(float)(90*sin(t*.8+i*.27))}));
        (void)show(viewport);
        bool pressed=rohr_viewport_ui_pressed_check(control.result.value);
        if(pressed && !was_pressed) for(unsigned i=0;i<24;++i) {
            ObjectAppearance a=rohr_entity_appearance_get(swarm[i]).result.value; a.visible=!a.visible;
            OK(rohr_entity_appearance_set(swarm[i],a));
        }
        was_pressed=pressed;
    }
    OK(rohr_viewport_destroy(viewport)); OK(rohr_graphics_text_destroy(&title.result.value));
    OK(rohr_graphics_text_destroy(&caption.result.value));
    for(unsigned i=0;i<24;++i) OK(rohr_entity_delete(swarm[i]));
    OK(rohr_entity_delete(root));
}
int main(int argc,char **argv) {

    OK(rohr_engine_start()); OK(rohr_graphics_start());
    OK(rohr_graphics_vsync_set(false)); OK(rohr_graphics_frame_limit_set(100000));
    if(argc>1 && !strcmp(argv[1],"--preview")) { preview(); rohr_graphics_stop(); rohr_engine_stop(); return 0; }
    Entity a=entity_value(rohr_object_create(&catalog));
    Entity b=entity_value(rohr_object_create(&catalog));
    /* Keep one instance's visual hidden so command counts are exact. */
    ObjectAppearance appearance=rohr_entity_appearance_get(member(b,"particle")).result.value;
    appearance.visible=false; OK(rohr_entity_appearance_set(member(b,"particle"),appearance));
    selection_and_rendering(a); visual_components(a); creation_and_lifetime(a,b); first_frame_paused(a); stale_camera(a);
    OK(rohr_entity_delete(a)); OK(rohr_entity_delete(b));
    restored_bindings();
    assert(rohr_entity_alive_count_get()==0);
    graphics_restart();
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
