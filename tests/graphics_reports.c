/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "graphics/texture_assets.h"
#include "graphics/text_assets.h"
#include "test_png.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define OK(call) assert(!rohr_error_check(call))
static const Color white = {255,255,255,255};
static const char *texture_path = "graphics_reports_fixture.png";
static unsigned callbacks;
static bool recursive, bad_submission;

#ifdef ROHR_TEST_GRAPHICS_WRAP
static bool fail_allocation, fail_draw, fail_target, fail_present, fail_capture;
static bool fake_recording, fail_write, fail_flush;
static unsigned presents;
void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *p, size_t n) { return fail_allocation ? NULL : __real_realloc(p,n); }
bool __real_SDL_RenderFillRect(SDL_Renderer *, const SDL_FRect *);
bool __wrap_SDL_RenderFillRect(SDL_Renderer *r, const SDL_FRect *rect) {
    if(fail_draw) { fail_draw=false; return SDL_SetError("injected draw failure"); }
    return __real_SDL_RenderFillRect(r,rect);
}
bool __real_SDL_SetRenderTarget(SDL_Renderer *, SDL_Texture *);
bool __wrap_SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) {
    if(fail_target) { fail_target=false; return SDL_SetError("injected target failure"); }
    return __real_SDL_SetRenderTarget(r,t);
}
bool __real_SDL_RenderPresent(SDL_Renderer *);
bool __wrap_SDL_RenderPresent(SDL_Renderer *r) {
    presents++;
    if(fail_present) { fail_present=false; return SDL_SetError("injected present failure"); }
    return __real_SDL_RenderPresent(r);
}
SDL_Surface *__real_SDL_RenderReadPixels(SDL_Renderer *, const SDL_Rect *);
SDL_Surface *__wrap_SDL_RenderReadPixels(SDL_Renderer *r, const SDL_Rect *rect) {
    if(fail_capture) { fail_capture=false; SDL_SetError("injected capture failure"); return NULL; }
    return __real_SDL_RenderReadPixels(r,rect);
}
bool __real_platform_process_command_check(const char *);
bool __wrap_platform_process_command_check(const char *s) {
    return fake_recording || __real_platform_process_command_check(s);
}
FILE *__real_platform_process_open_write(const char *);
FILE *__wrap_platform_process_open_write(const char *s) {
    return fake_recording ? tmpfile() : __real_platform_process_open_write(s);
}
int __real_platform_process_close(FILE *);
int __wrap_platform_process_close(FILE *f) {
    return fake_recording ? fclose(f) : __real_platform_process_close(f);
}
int __real_fflush(FILE *);
int __wrap_fflush(FILE *f) { return fail_flush ? EOF : __real_fflush(f); }
size_t __real_fwrite(const void *,size_t,size_t,FILE *);
size_t __wrap_fwrite(const void *p,size_t s,size_t n,FILE *f) {
    return fail_write ? 0 : __real_fwrite(p,s,n,f);
}
#endif

static uint64_t metric(const Report *r, ReportId owner, ReportMetricId id) {
    assert(rohr_report_valid_check(r));
    for(size_t i=0;i<r->statistic_count;i++)
        if(r->statistics[i].report_id==owner && r->statistics[i].metric_id==id) return r->statistics[i].value;
    return 0;
}
static const ReportEntry *entry(const Report *r,ReportId id) {
    for(size_t i=0;i<r->entry_count;i++) if(r->entries[i].header.id==id) return &r->entries[i];
    return NULL;
}
static void complete(Report r) {
    assert(rohr_report_valid_check(&r) && r.header.status==ROHR_REPORT_COMPLETE && !r.has_fatal);
}
static void render(CameraId camera,void *context) {
    (void)camera; (void)context; callbacks++;
    if(recursive) {
        Report (*calls[])(void) = {rohr_graphics_show,rohr_graphics_pipeline_cameras_update,
            rohr_graphics_pipeline_commands_execute,rohr_graphics_pipeline_viewports_draw,
            rohr_graphics_pipeline_recording_update,rohr_graphics_pipeline_present};
        for(size_t i=0;i<sizeof(calls)/sizeof(calls[0]);i++) {
            Report r=calls[i](); assert(rohr_report_valid_check(&r));
            assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.code==ERROR_ENGINE_STATE_INVALID);
        }
        complete(rohr_graphics_sprite_frames_update(0,0));
    }
    rohr_graphics_background_draw((Color){0,0,0,255});
    assert(rohr_graphics_screen_rect_draw(1,2,8,9,white));
    if(bad_submission) rohr_graphics_screen_texture_draw((TextureAsset){.handle=123,.size={1,1}},
        (Position){0},(Scale){2,2},0);
}

static void reports_and_boundaries(void) {
    Report r=rohr_graphics_show();
    complete(r); assert(callbacks==1 && r.entry_count==4);
    assert(entry(&r,ROHR_GRAPHICS_RECORDING_REPORT)==NULL);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_QUEUED_COMMAND_COUNT)==2);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==2);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&r,ROHR_GRAPHICS_PRESENTATION_REPORT,ROHR_REPORT_PROCESSED_COUNT)==1);
    Report saved=r, snapshot=r;
    complete(rohr_graphics_pipeline_cameras_update());
    Report commands=rohr_graphics_pipeline_commands_execute(); complete(commands);
    assert(metric(&commands,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==2);
    complete(rohr_graphics_pipeline_viewports_draw());
    r=rohr_graphics_pipeline_recording_update();
    assert(r.header.status==ROHR_REPORT_NOT_RUN && rohr_report_valid_check(&r));
    complete(rohr_graphics_pipeline_present());
    r=rohr_graphics_pipeline_commands_execute(); complete(r); assert(r.statistic_count==0);
    assert(memcmp(&saved,&snapshot,sizeof(saved))==0);
    recursive=true; bad_submission=true;
    r=rohr_graphics_show(); complete(r);
    assert(r.diagnostic_count==1 && r.diagnostics[0].report_id==ROHR_GRAPHICS_COMMANDS_REPORT);
    assert(r.diagnostics[0].internal_stage==ROHR_GRAPHICS_SUBMISSION);
    assert(r.diagnostics[0].source.kind==ROHR_REPORT_OBJECT_TEXTURE);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_SKIPPED_COUNT)==1);
    recursive=false; bad_submission=false;
    r=rohr_graphics_show(); complete(r); assert(r.diagnostic_count==0);
    for(unsigned i=0;i<ROHR_REPORT_DIAGNOSTIC_CAPACITY+2;i++)
        rohr_graphics_screen_texture_draw((TextureAsset){.handle=i+1000},(Position){0},(Scale){1,1},0);
    r=rohr_graphics_pipeline_commands_execute(); complete(r);
    assert(r.diagnostic_count==ROHR_REPORT_DIAGNOSTIC_CAPACITY && r.dropped_diagnostic_occurrences==2);
    r=rohr_graphics_pipeline_commands_execute(); complete(r); assert(r.diagnostic_count==0);
}

static void retained_assets(void) {
    FILE *f=fopen(texture_path,"wb"); assert(f);
    assert(fwrite(test_png,1,sizeof(test_png),f)==sizeof(test_png)); assert(fclose(f)==0);
    TextureAssetResult loaded=rohr_graphics_texture_load((TextureDescriptor){texture_path,{4,4}}); OK(loaded);
    TextureAsset texture=loaded.result.value;
    EntityResult entity=rohr_entity_add(); OK(entity);
    AnimatedSprite sprite={.scale={1,1},.visible=true};
    sprite.player.ticks_per_frame=1;
    AnimationFrame frame=rohr_graphics_animation_frame_create(texture);
    OK(rohr_graphics_animated_sprite_value_frame_add(&sprite,frame));
    OK(rohr_graphics_animated_sprite_value_frame_add(&sprite,frame));
    OK(rohr_graphics_animated_sprite_add(entity.result.value,sprite));
    complete(rohr_graphics_show());
    assert(rohr_graphics_animated_sprite_frame_index_get(entity.result.value).result.value==0);
    Report animation=rohr_graphics_sprite_frames_update(1,0);
    complete(animation);
    assert(animation.header.id==ROHR_GRAPHICS_ANIMATIONS_REPORT);
    assert(metric(&animation,ROHR_GRAPHICS_ANIMATIONS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(rohr_graphics_animated_sprite_frame_index_get(entity.result.value).result.value==1);
    OK(rohr_entity_delete(entity.result.value));
    FontAsset font=rohr_graphics_font_default_get();
    TextAssetResult text=rohr_graphics_text_create(&font,"queued text",white); OK(text);
    rohr_graphics_screen_texture_draw(texture,(Position){4,4},(Scale){4,4},0);
    assert(rohr_graphics_text_draw(&text.result.value,(Position){8,8}));
    OK(rohr_graphics_texture_release(&texture));
    OK(rohr_graphics_text_destroy(&text.result.value));
    assert(graphics_texture_assets_stats_get().command_references==1);
    assert(graphics_text_assets_stats_get().command_references==1);
    complete(rohr_graphics_pipeline_commands_execute());
    assert(graphics_texture_assets_stats_get().live_resources==0);
    assert(graphics_text_assets_stats_get().live_payloads==0);
    assert(remove(texture_path)==0);
}

#ifdef ROHR_TEST_GRAPHICS_WRAP
static void failures(void) {
    /* A new layer forces queue growth without changing the allocator globally. */
    graphics_layer_active_set(4567);
    fail_allocation=true;
    unsigned queued=0;
    while(graphics_screen_rect_draw(0,0,1,1,white)) { assert(++queued < 10000); }
    fail_allocation=false;
    Report r=rohr_graphics_pipeline_commands_execute(); complete(r);
    assert(r.diagnostic_count==1 && r.diagnostics[0].code==ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    assert(graphics_screen_rect_draw(0,0,1,1,white));
    assert(graphics_screen_rect_draw(2,2,1,1,white));
    fail_draw=true;
    r=graphics_pipeline_commands_execute(); complete(r);
    assert(r.diagnostic_count==1 && strstr(r.diagnostics[0].detail,"injected draw failure"));
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT,ROHR_REPORT_SKIPPED_COUNT)==1);
    OK(rohr_report_json_write(&r,"graphics_reports_test.json",true));
    FILE *json=fopen("graphics_reports_test.json","rb"); assert(json);
    char serialized[16384]; size_t size=fread(serialized,1,sizeof(serialized)-1,json);
    serialized[size]='\0'; assert(fclose(json)==0 && remove("graphics_reports_test.json")==0);
    assert(strstr(serialized,"ROHR_GRAPHICS_COMMANDS_REPORT") &&
        strstr(serialized,"ROHR_GRAPHICS_COMMAND_EXECUTION") &&
        strstr(serialized,"ERROR_ENGINE_GRAPHICS_DRAW_FAILED") &&
        strstr(serialized,"ROHR_REPORT_QUEUED_COMMAND_COUNT"));
    Report retained=r;
    SDL_SetError("unrelated later backend error");
    r=graphics_pipeline_commands_execute(); complete(r);
    assert(strstr(retained.diagnostics[0].detail,"injected draw failure"));
    FontAsset font=graphics_font_default_get();
    TextAssetResult text=graphics_text_create(&font,"discard me",white); OK(text);
    assert(graphics_text_draw(&text.result.value,(Position){0}));
    OK(graphics_text_destroy(&text.result.value));
    fail_target=true;
    r=graphics_pipeline_commands_execute();
    assert(r.header.status==ROHR_REPORT_FAILED && rohr_report_valid_check(&r));
    assert(graphics_text_assets_stats_get().command_references==0);
    assert(graphics_text_assets_stats_get().live_payloads==0);
    complete(graphics_pipeline_commands_execute());
    unsigned before=presents;
    fail_target=true;
    r=rohr_graphics_show();
    assert(r.header.status==ROHR_REPORT_FAILED && presents==before);
    assert(entry(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT)==NULL);
    complete(rohr_graphics_show());
    fail_target=true;
    r=graphics_pipeline_viewports_draw();
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.report_id==ROHR_GRAPHICS_VIEWPORTS_REPORT);
    complete(graphics_pipeline_viewports_draw());
    fail_draw=true;
    r=graphics_pipeline_viewports_draw(); complete(r);
    assert(r.diagnostic_count==1 && r.diagnostics[0].report_id==ROHR_GRAPHICS_VIEWPORTS_REPORT);
    assert(r.diagnostics[0].source.kind==ROHR_REPORT_OBJECT_VIEWPORT);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==1);
    fail_present=true;
    r=graphics_pipeline_present();
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.report_id==ROHR_GRAPHICS_PRESENTATION_REPORT);
    complete(graphics_pipeline_present());
    fake_recording=true;
    assert(graphics_recording_start("unused.mp4",60));
    before=presents; fail_capture=true;
    r=graphics_show();
    assert(r.header.status==ROHR_REPORT_FAILED && presents==before+1);
    assert(r.first_fatal.report_id==ROHR_GRAPHICS_RECORDING_REPORT);
    assert(entry(&r,ROHR_GRAPHICS_PRESENTATION_REPORT)->header.status==ROHR_REPORT_COMPLETE);
    r=graphics_pipeline_recording_update(); assert(r.header.status==ROHR_REPORT_NOT_RUN);
    assert(graphics_recording_start("unused.mp4",60));
    fail_write=true;
    r=graphics_pipeline_recording_update(); fail_write=false;
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.internal_stage==ROHR_GRAPHICS_RECORDING_WRITE);
    assert(graphics_recording_start("unused.mp4",60));
    fail_flush=true;
    r=graphics_pipeline_recording_update(); fail_flush=false;
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.code==ERROR_ENGINE_GRAPHICS_RECORDING_FAILED);
    assert(graphics_recording_start("unused.mp4",60));
    complete(graphics_pipeline_recording_update());
    graphics_recording_stop(); fake_recording=false;
}
#endif

int main(void) {
    Report r=rohr_graphics_show();
    assert(r.header.status==ROHR_REPORT_FAILED && rohr_report_valid_check(&r));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    OK(rohr_graphics_vsync_set(false)); OK(rohr_graphics_frame_limit_set(1000));
    CameraId camera=rohr_camera_active_get();
    ViewportIdResult viewport=rohr_viewport_create(rohr_viewport_config_default_get()); OK(viewport);
    OK(rohr_viewport_camera_set(viewport.result.value,camera));
    OK(rohr_viewport_enable_set(viewport.result.value));
    OK(rohr_camera_render_callback_set(camera,render,NULL));
    reports_and_boundaries(); retained_assets();
#ifdef ROHR_TEST_GRAPHICS_WRAP
    failures();
#endif
    OK(rohr_camera_pause_with_engine_set(camera));
    rohr_engine_pause(); unsigned before=callbacks;
    r=rohr_graphics_show(); complete(r); assert(callbacks==before);
    rohr_engine_resume();
    OK(rohr_viewport_disable_set(viewport.result.value));
    r=rohr_graphics_show(); complete(r); assert(callbacks==before);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT,ROHR_REPORT_PROCESSED_COUNT)==0);
    assert(!graphics_screen_shape_filled_draw((Shape){0},white));
    rohr_graphics_stop(); OK(rohr_graphics_start());
    r=rohr_graphics_pipeline_commands_execute(); complete(r); assert(r.diagnostic_count==0);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
