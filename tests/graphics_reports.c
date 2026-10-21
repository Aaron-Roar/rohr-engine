/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "graphics/texture_assets.h"
#include "graphics/text_assets.h"
#include "graphics/scene_image.h"
#include "test_png.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define OK(call) assert(!rohr_error_check(call))
static const Color white = {255,255,255,255};
static const char *texture_path = "graphics_reports_fixture.png";
static unsigned callbacks;
static bool recursive, bad_submission;

#ifdef ROHR_TEST_GRAPHICS_WRAP
static bool fail_allocation, fail_draw, fail_texture, fail_target, fail_present, fail_capture;
static bool fake_recording, fail_write, fail_flush;
static unsigned presents;
static bool trace_composition;
static double composition_angles[8];
static SDL_FRect composition_rectangles[8];
static size_t composition_count;
static bool check_camera_targets;
static bool trace_image_targets;
static unsigned image_source_count, image_composite_count;
static SDL_Texture *image_targets[2];
void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *p, size_t n) { return fail_allocation ? NULL : __real_realloc(p,n); }
bool __real_SDL_RenderFillRect(SDL_Renderer *, const SDL_FRect *);
bool __wrap_SDL_RenderFillRect(SDL_Renderer *r, const SDL_FRect *rect) {
    if(trace_image_targets && rect->x == -10) {
        assert(image_source_count < 2);
        SDL_Texture *target = SDL_GetRenderTarget(r);
        float width = 0, height = 0;
        assert(target && SDL_GetTextureSize(target, &width, &height));
        assert(width == 80 + image_source_count * 20 && height == 60);
        assert(!SDL_RenderClipEnabled(r));
        SDL_BlendMode blend;
        assert(SDL_GetRenderDrawBlendMode(r, &blend) && blend == SDL_BLENDMODE_BLEND);
        int lw = 0, lh = 0; SDL_RendererLogicalPresentation presentation;
        assert(SDL_GetRenderLogicalPresentation(r, &lw, &lh, &presentation));
        assert(presentation == SDL_LOGICAL_PRESENTATION_DISABLED);
        image_targets[image_source_count++] = target;
    }
    if(fail_draw) { fail_draw=false; return SDL_SetError("injected draw failure"); }
    return __real_SDL_RenderFillRect(r,rect);
}
bool __real_SDL_RenderTextureRotated(SDL_Renderer *, SDL_Texture *, const SDL_FRect *,
    const SDL_FRect *, double, const SDL_FPoint *, SDL_FlipMode);
bool __wrap_SDL_RenderTextureRotated(SDL_Renderer *r, SDL_Texture *t, const SDL_FRect *src,
        const SDL_FRect *dst, double angle, const SDL_FPoint *center, SDL_FlipMode flip) {
    if(trace_image_targets) {
        assert(image_composite_count < 2);
        assert(SDL_GetRenderTarget(r) == NULL && t == image_targets[image_composite_count]);
        SDL_BlendMode blend;
        assert(SDL_GetTextureBlendMode(t, &blend) && blend == SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        SDL_Rect clip = {0}; assert(SDL_GetRenderClipRect(r, &clip));
        assert(SDL_RenderClipEnabled(r) && clip.x == 11 && clip.y == 12 && clip.w == 200 && clip.h == 160);
        assert(angle == 35 + image_composite_count * 55);
        image_composite_count++;
    }
    if(trace_composition) {
        assert(composition_count < 8);
        composition_rectangles[composition_count] = *dst;
        composition_angles[composition_count++] = angle;
    }
    if(fail_texture) { fail_texture=false; return SDL_SetError("injected sprite draw failure"); }
    return __real_SDL_RenderTextureRotated(r,t,src,dst,angle,center,flip);
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

static void image_source_draw(void *context) {
    (void)context;
    rohr_graphics_layer_active_set(10);
    rohr_graphics_background_draw((Color){0});
    assert(rohr_graphics_screen_rect_draw(-10, -20, 130, 100, white));
    /* Capture must isolate even a callback's unbalanced clip stack. */
    assert(rohr_graphics_screen_clip_push(1,2,3,4));
    rohr_graphics_layer_active_set(99);
}

static void image_target_lifetime(void) {
    GraphicsSceneImage *images[2] = {graphics_scene_image_create(), graphics_scene_image_create()};
    assert(images[0] && images[1]);
    assert(rohr_graphics_screen_clip_push(11,12,200,160));
    rohr_graphics_layer_active_set(20);
    for(unsigned i = 0; i < 2; i++) {
        OK(graphics_scene_image_render(images[i], 80 + i*20, 60, image_source_draw, NULL));
        assert(rohr_graphics_layer_active_get() == 20);
        OK(graphics_scene_image_draw_rotated(images[i], (SDL_FRect){30,40,80,60}, 35 + i*55));
    }
    rohr_graphics_screen_clip_pop();
    /* The destination and source commands retain the resource independently. */
    graphics_scene_image_destroy(images[0]);
#ifdef ROHR_TEST_GRAPHICS_WRAP
    trace_image_targets = true;
#endif
    complete(rohr_graphics_pipeline_commands_execute());
#ifdef ROHR_TEST_GRAPHICS_WRAP
    trace_image_targets = false;
    assert(image_source_count == 2 && image_composite_count == 2);
#endif
    /* Reuse, resize, streaming upload and target conversion release prior images. */
    OK(graphics_scene_image_render(images[1], 120, 70, image_source_draw, NULL));
    complete(rohr_graphics_pipeline_commands_execute());
    unsigned char pixels[16] = {0};
    OK(graphics_scene_image_update(images[1], pixels, 2, 2, 8));
    OK(graphics_scene_image_render(images[1], 2, 2, image_source_draw, NULL));
    complete(rohr_graphics_pipeline_commands_execute());
    graphics_scene_image_destroy(images[1]);
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
#ifdef ROHR_TEST_GRAPHICS_WRAP
    if(check_camera_targets) {
        int count = 0, width = 0, height = 0, logical_width = 0, logical_height = 0;
        SDL_Window **windows = SDL_GetWindows(&count); assert(windows && count);
        SDL_Renderer *renderer = SDL_GetRenderer(windows[0]); SDL_free(windows);
        assert(SDL_GetRenderTarget(renderer));
        SDL_RendererLogicalPresentation mode;
        assert(SDL_GetCurrentRenderOutputSize(renderer, &width, &height));
        assert(SDL_GetRenderLogicalPresentation(renderer, &logical_width, &logical_height, &mode));
        assert(mode == SDL_LOGICAL_PRESENTATION_DISABLED);
        CameraDescription description = rohr_camera_description_get(camera).result.value;
        Position center = rohr_graphics_world_to_screen_get(description.position);
        assert(fabsf(center.x - width * .5f) < .001f && fabsf(center.y - height * .5f) < .001f);
    }
#endif
    assert(rohr_graphics_screen_rect_draw(1,2,8,9,white));
    if(bad_submission) rohr_graphics_screen_texture_draw((TextureAsset){.handle=123,.size={1,1}},
        (Position){0},(Scale){2,2},0);
}

static void reports_and_boundaries(void) {
    Report r=rohr_graphics_show();
    complete(r); assert(callbacks==1 && r.entry_count==4);
    assert(entry(&r,ROHR_GRAPHICS_RECORDING_REPORT_ID)==NULL);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_QUEUED_COMMAND_COUNT)==2);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&r,ROHR_GRAPHICS_PRESENTATION_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    Report saved=r, snapshot=r;
    complete(rohr_graphics_pipeline_cameras_update());
    Report commands=rohr_graphics_pipeline_commands_execute(); complete(commands);
    assert(metric(&commands,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    complete(rohr_graphics_pipeline_viewports_draw());
    r=rohr_graphics_pipeline_recording_update();
    assert(r.header.status==ROHR_REPORT_NOT_RUN && rohr_report_valid_check(&r));
    complete(rohr_graphics_pipeline_present());
    r=rohr_graphics_pipeline_commands_execute(); complete(r); assert(r.statistic_count==0);
    assert(memcmp(&saved,&snapshot,sizeof(saved))==0);
    recursive=true; bad_submission=true;
    r=rohr_graphics_show(); complete(r);
    assert(r.diagnostic_count==1 && r.diagnostics[0].report_id==ROHR_GRAPHICS_COMMANDS_REPORT_ID);
    assert(r.diagnostics[0].internal_stage==ROHR_GRAPHICS_SUBMISSION);
    assert(r.diagnostics[0].source.kind==ROHR_REPORT_OBJECT_TEXTURE);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_SKIPPED_COUNT)==1);
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
    TextureAssetResult loaded=rohr_graphics_texture_load((TextureDescription){texture_path,{4,4}}); OK(loaded);
    TextureAsset texture=loaded.result.value;
    EntityResult entity=rohr_entity_add(); OK(entity);
#ifdef ROHR_TEST_GRAPHICS_WRAP
    OK(graphics_sprite_add(entity.result.value, (SpriteDescription){.texture = texture, .scale = (Scale){1,1}, .direction = DIRECTION_RIGHT, .follow_entity_rotation = true, .visible = true}));
    EntityInfoResult drawable=entity_info_get(entity.result.value,ROHR_ENTITY_INFO_GRAPHICS_SPRITE); OK(drawable);
    EntityInfoIssueCountResult drawable_issues=entity_info_issues_count_get(&drawable.result.value,
        ROHR_ENTITY_INFO_GRAPHICS_SPRITE); OK(drawable_issues); assert(drawable_issues.result.value==0);
    assert(graphics_sprite_draw(entity.result.value));
    assert(graphics_screen_rect_draw(2,2,1,1,white));
    assert(graphics_texture_assets_stats_get().command_references==1);
    fail_texture=true;
    Report draw_failure=rohr_graphics_pipeline_commands_execute(); complete(draw_failure);
    assert(draw_failure.diagnostic_count==1 && draw_failure.diagnostics[0].source.kind==ROHR_REPORT_OBJECT_TEXTURE);
    assert(draw_failure.diagnostics[0].source.id==drawable.result.value.sprite.texture.handle);
    assert(draw_failure.diagnostics[0].code==ERROR_ENGINE_GRAPHICS_DRAW_FAILED);
    assert(draw_failure.diagnostics[0].recovery==ROHR_REPORT_CONTRIBUTION_SKIPPED);
    assert(metric(&draw_failure,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&draw_failure,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_SKIPPED_COUNT)==1);
    assert(graphics_texture_assets_stats_get().command_references==0 && graphics_texture_valid_check(texture));
    EntityInfoResult still_drawable=rohr_entity_info_get(entity.result.value,ROHR_ENTITY_INFO_GRAPHICS_SPRITE); OK(still_drawable);
    assert(memcmp(&drawable.result.value.sprite,&still_drawable.result.value.sprite,sizeof(Sprite))==0);
    ReportResult latest_draw=report_get(ROHR_GRAPHICS_COMMANDS_REPORT); OK(latest_draw);
    assert(memcmp(&latest_draw.result.value,&draw_failure,sizeof(Report))==0);
    SDL_SetError("unrelated replacement error");
    assert(graphics_sprite_draw(entity.result.value));
    Report retried=graphics_pipeline_commands_execute(); complete(retried);
    assert(retried.diagnostic_count==0 && metric(&retried,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(strstr(draw_failure.diagnostics[0].detail,"injected sprite draw failure"));
    OK(entity_info_release(&drawable.result.value)); OK(entity_info_release(&still_drawable.result.value));
#endif
    AnimatedSprite sprite={.scale={1,1},.visible=true};
    sprite.player.ticks_per_frame=1;
    AnimationFrameDescription frame=rohr_graphics_animation_frame_create((AnimationFrameDescription){.texture = texture, .scale = {1, 1}});
    OK(rohr_graphics_animated_sprite_value_frame_add(&sprite,frame));
    OK(rohr_graphics_animated_sprite_value_frame_add(&sprite,frame));
    OK(rohr_graphics_animated_sprite_add(entity.result.value, rohr_graphics_animated_sprite_value_description_get(sprite)));
    complete(rohr_graphics_show());
    assert(rohr_graphics_animated_sprite_frame_index_get(entity.result.value).result.value==0);
    Report animation=rohr_graphics_sprite_frames_update(1,0);
    complete(animation);
    assert(animation.header.id==ROHR_GRAPHICS_ANIMATIONS_REPORT_ID);
    assert(metric(&animation,ROHR_GRAPHICS_ANIMATIONS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(rohr_graphics_animated_sprite_frame_index_get(entity.result.value).result.value==1);
    EntityResult healthy=rohr_entity_add(); OK(healthy);
    OK(graphics_animated_sprite_add(healthy.result.value, graphics_animated_sprite_value_description_get(sprite)));
    EntityIndexResult index=rohr_entity_index_get(entity.result.value); OK(index);
    EntityInfoResult before=entity_info_get(entity.result.value,ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS); OK(before);
    /* The pipeline rejects mismatched player metadata without repairing it. */
    animated_sprites[index.result.value].player.frame_count=1;
    animation=graphics_sprite_frames_update(1,0);
    complete(animation);
    assert(animation.diagnostic_count==1 && animation.diagnostics[0].source.id==entity.result.value);
    assert(animation.diagnostics[0].code==ERROR_ENGINE_STATE_INVALID);
    assert(animation.diagnostics[0].internal_stage==ROHR_GRAPHICS_ANIMATION);
    assert(animation.diagnostics[0].recovery==ROHR_REPORT_CONTRIBUTION_SKIPPED);
    assert(metric(&animation,ROHR_GRAPHICS_ANIMATIONS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&animation,ROHR_GRAPHICS_ANIMATIONS_REPORT_ID,ROHR_REPORT_SKIPPED_COUNT)==1);
    assert(graphics_animated_sprite_frame_index_get(healthy.result.value).result.value==1);
    EntityInfoResult broken=rohr_entity_info_get(entity.result.value,ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS); OK(broken);
    assert(broken.result.value.animations[0].player.frame_count==1);
    EntityInfoIssue issue;
    EntityInfoIssueReadResult issues=entity_info_issues_get(&broken.result.value,
        ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS,&issue,1); OK(issues);
    assert(issues.result.value.total==1 && issue.reason==ROHR_ENTITY_INFO_VALUE_INVALID);
    assert(issue.reference.id==animation.diagnostics[0].source.id && issue.has_index && issue.index==0);
    ReportResult stored=report_get(ROHR_GRAPHICS_ANIMATIONS_REPORT); OK(stored);
    assert(memcmp(&stored.result.value,&animation,sizeof(animation))==0);
    Report retained=animation;
    animated_sprites[index.result.value].player.frame_count=2;
    animation=rohr_graphics_sprite_frames_update(2,0);
    assert(animation.diagnostic_count==0);
    assert(metric(&animation,ROHR_GRAPHICS_ANIMATIONS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==2);
    assert(graphics_animated_sprite_frame_index_get(entity.result.value).result.value==0);
    EntityInfoResult repaired=entity_info_get(entity.result.value,ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS); OK(repaired);
    EntityInfoIssueCountResult count=entity_info_issues_count_get(&repaired.result.value,
        ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS); OK(count); assert(count.result.value==0);
    OK(rohr_entity_delete(entity.result.value));
    OK(rohr_entity_delete(healthy.result.value));
    assert(before.result.value.animations[0].player.frame_count==2);
    assert(before.result.value.animations[0].player.frame_index==1);
    assert(broken.result.value.animations[0].player.frame_count==1);
    assert(memcmp(&stored.result.value,&retained,sizeof(retained))==0);
    OK(entity_info_release(&before.result.value)); OK(entity_info_release(&broken.result.value));
    OK(entity_info_release(&repaired.result.value));
    FontAsset font=rohr_graphics_font_default_get();
    TextAssetResult text=rohr_graphics_text_create((TextDescription){.font = font, .value = "queued text", .color = white}); OK(text);
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
static void viewport_layer_composition(ViewportId original, CameraId camera) {
    WindowPresentationDescription saved = rohr_graphics_window_presentation_description_get();
    WindowPresentationDescription presentation = saved; presentation.aspect_ratio_auto = true;
    OK(rohr_graphics_window_presentation_description_set(presentation));
    OK(rohr_viewport_disable_set(original));
    ViewportDescription desc = rohr_viewport_description_default_get(); desc.enabled = true;
    ViewportIdResult viewport = rohr_viewport_create(desc); OK(viewport);
    GraphicsLayerIdResult middle = rohr_graphics_layer_create((LayerDescription){
        .name = "composition_middle", .value = 1}); OK(middle);
    TextAssetResult asset = rohr_graphics_text_create((TextDescription){.value = "layer"}); OK(asset);
    ViewportUiTextDescription text = rohr_graphics_ui_text_description_default_get();
    text.text = asset.result.value; text.position = (Position){100,100};
    ViewportItemDescription item = rohr_viewport_item_description_default_get();
    item.layer_id = middle.result.value; item.orientation = 20;
    /* Create UI first so equal-layer ordering cannot accidentally pass by slot. */
    OK(rohr_viewport_ui_text_add(viewport.result.value, text, item));
    item.layer_id = 0; item.layer = 3; item.orientation = 40;
    ViewportItemIdResult upper = rohr_viewport_ui_text_add(viewport.result.value, text, item); OK(upper);
    item.layer = 0; item.orientation = 10; item.rectangle = (ViewportRectangle){-40,40,200,160};
    OK(rohr_viewport_camera_add(viewport.result.value, camera, item));
    item.layer = 2; item.orientation = 30; item.rectangle = (ViewportRectangle){100,80,300,220};
    ViewportItemIdResult right = rohr_viewport_camera_add(viewport.result.value, camera, item); OK(right);
    check_camera_targets = true;
    for(unsigned frame = 0; frame < 2; frame++) {
        complete(rohr_graphics_pipeline_cameras_update());
        complete(rohr_graphics_pipeline_commands_execute());
        WindowPresentationDescription current = rohr_graphics_window_presentation_description_get();
        assert(current.logical_width == presentation.logical_width);
        assert(current.logical_height == presentation.logical_height);
    }
    check_camera_targets = false;
    trace_composition = true; composition_count = 0;
    complete(rohr_graphics_pipeline_viewports_draw());
    assert(composition_count == 4);
    assert(composition_angles[0] == 10 && composition_angles[1] == 20 &&
        composition_angles[2] == 30 && composition_angles[3] == 40);
    OK(rohr_graphics_layer_name_set("composition_middle", 2));
    composition_count = 0;
    complete(rohr_graphics_pipeline_viewports_draw());
    assert(composition_count == 4);
    assert(composition_angles[0] == 10 && composition_angles[1] == 30 &&
        composition_angles[2] == 20 && composition_angles[3] == 40);
    item = rohr_viewport_item_description_get(right.result.value).result.value;
    item.content_offset = (Vec2D){40,-20};
    item.content_scale = (Scale){1.5f,.5f}; item.content_orientation = 15;
    OK(rohr_viewport_item_description_set(right.result.value, item));
    composition_count = 0;
    complete(rohr_graphics_pipeline_viewports_draw());
    SDL_FRect shifted = composition_rectangles[1];
    Vec2D offset = rohr_math_vector_rotate(item.content_offset, -item.orientation);
    assert(fabsf(shifted.x + shifted.w * .5f - (250 + offset.x)) < .001f);
    assert(fabsf(shifted.y + shifted.h * .5f - (190 + offset.y)) < .001f);
    assert(composition_angles[1] == 45);
    item.visible = false;
    OK(rohr_viewport_item_description_set(right.result.value, item));
    composition_count = 0;
    complete(rohr_graphics_pipeline_viewports_draw());
    assert(composition_count == 3 && composition_angles[1] == 20 && composition_angles[2] == 40);
    ViewportItemDescription hidden = rohr_viewport_item_description_get(upper.result.value).result.value;
    hidden.visible = false;
    OK(rohr_viewport_item_description_set(upper.result.value, hidden));
    composition_count = 0;
    complete(rohr_graphics_pipeline_viewports_draw());
    assert(composition_count == 2 && composition_angles[0] == 10 && composition_angles[1] == 20);
    trace_composition = false;
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_graphics_text_destroy(&asset.result.value));
    OK(rohr_graphics_layer_destroy(middle.result.value));
    OK(rohr_viewport_enable_set(original));
    OK(rohr_graphics_window_presentation_description_set(saved));
}

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
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    assert(metric(&r,ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_SKIPPED_COUNT)==1);
    OK(rohr_report_json_write(&r,"graphics_reports_test.json",true));
    FILE *json=fopen("graphics_reports_test.json","rb"); assert(json);
    char serialized[16384]; size_t size=fread(serialized,1,sizeof(serialized)-1,json);
    serialized[size]='\0'; assert(fclose(json)==0 && remove("graphics_reports_test.json")==0);
    assert(strstr(serialized,"ROHR_GRAPHICS_COMMANDS_REPORT_ID") &&
        strstr(serialized,"ROHR_GRAPHICS_COMMAND_EXECUTION") &&
        strstr(serialized,"ERROR_ENGINE_GRAPHICS_DRAW_FAILED") &&
        strstr(serialized,"ROHR_REPORT_QUEUED_COMMAND_COUNT"));
    Report retained=r;
    SDL_SetError("unrelated later backend error");
    r=graphics_pipeline_commands_execute(); complete(r);
    assert(strstr(retained.diagnostics[0].detail,"injected draw failure"));
    FontAsset font=graphics_font_default_get();
    TextAssetResult text=graphics_text_create((TextDescription){.font = font, .value = "discard me", .color = white}); OK(text);
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
    assert(entry(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT_ID)==NULL);
    complete(rohr_graphics_show());
    fail_target=true;
    r=graphics_pipeline_viewports_draw();
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.report_id==ROHR_GRAPHICS_VIEWPORTS_REPORT_ID);
    complete(graphics_pipeline_viewports_draw());
    fail_draw=true;
    r=graphics_pipeline_viewports_draw(); complete(r);
    assert(r.diagnostic_count==1 && r.diagnostics[0].report_id==ROHR_GRAPHICS_VIEWPORTS_REPORT_ID);
    assert(r.diagnostics[0].source.kind==ROHR_REPORT_OBJECT_VIEWPORT);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==1);
    fail_present=true;
    r=graphics_pipeline_present();
    assert(r.header.status==ROHR_REPORT_FAILED && r.first_fatal.report_id==ROHR_GRAPHICS_PRESENTATION_REPORT_ID);
    complete(graphics_pipeline_present());
    fake_recording=true;
    assert(graphics_recording_start("unused.mp4",60));
    before=presents; fail_capture=true;
    r=graphics_show();
    assert(r.header.status==ROHR_REPORT_FAILED && presents==before+1);
    assert(r.first_fatal.report_id==ROHR_GRAPHICS_RECORDING_REPORT_ID);
    assert(entry(&r,ROHR_GRAPHICS_PRESENTATION_REPORT_ID)->header.status==ROHR_REPORT_COMPLETE);
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
    ViewportIdResult viewport=rohr_viewport_create(rohr_viewport_description_default_get()); OK(viewport);
    OK(rohr_viewport_camera_set(viewport.result.value,camera));
    OK(rohr_viewport_enable_set(viewport.result.value));
    OK(rohr_camera_render_callback_set(camera,render,NULL));
    reports_and_boundaries(); retained_assets();
#ifdef ROHR_TEST_GRAPHICS_WRAP
    failures();
    viewport_layer_composition(viewport.result.value, camera);
#endif
    OK(rohr_camera_pause_with_engine_set(camera));
    rohr_engine_pause(); unsigned before=callbacks;
    r=rohr_graphics_show(); complete(r); assert(callbacks==before);
    rohr_engine_resume();
    OK(rohr_viewport_disable_set(viewport.result.value));
    r=rohr_graphics_show(); complete(r); assert(callbacks==before);
    assert(metric(&r,ROHR_GRAPHICS_VIEWPORTS_REPORT_ID,ROHR_REPORT_PROCESSED_COUNT)==0);
    image_target_lifetime();
    assert(!graphics_screen_shape_filled_draw((Shape){0},white));
    rohr_graphics_stop(); OK(rohr_graphics_start());
    r=rohr_graphics_pipeline_commands_execute(); complete(r); assert(r.diagnostic_count==0);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
