/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(value) do { if(!(value)) { fprintf(stderr, "camera description check %d: %s\n", __LINE__, #value); return false; } } while(0)
#define OK(value) CHECK(!rohr_error_check(value))

#ifdef ROHR_TEST_CAMERA_WRAP
static Uint64 counter = 1024;
static bool fail_logical;
Uint64 __wrap_SDL_GetPerformanceCounter(void) { return counter; }
Uint64 __wrap_SDL_GetPerformanceFrequency(void) { return 1024; }
bool __real_SDL_SetRenderLogicalPresentation(SDL_Renderer *, int, int, SDL_RendererLogicalPresentation);
bool __wrap_SDL_SetRenderLogicalPresentation(SDL_Renderer *renderer, int w, int h,
        SDL_RendererLogicalPresentation mode) {
    if(fail_logical) { fail_logical = false; return SDL_SetError("injected logical presentation failure"); }
    return __real_SDL_SetRenderLogicalPresentation(renderer, w, h, mode);
}
#endif

static bool near(float a, float b) { return fabsf(a - b) < 0.002f; }
static bool point(Position a, Position b) { return near(a.x, b.x) && near(a.y, b.y); }
static void draw(CameraId camera, void *context) { (void)camera; *(int *)context += 1; }

static bool advance(Tick ticks) {
#ifdef ROHR_TEST_CAMERA_WRAP
    counter += ticks * 16;
    CHECK(rohr_engine_tick_update() == ticks);
#else
    Tick target = rohr_engine_tick_get() + ticks;
    Uint64 deadline = SDL_GetTicks() + 2000;
    while(rohr_engine_tick_get() < target && SDL_GetTicks() < deadline) {
        SDL_Delay(1);
        rohr_engine_tick_update();
    }
    CHECK(rohr_engine_tick_get() >= target);
#endif
    CHECK(graphics_pipeline_cameras_update().header.status != ROHR_REPORT_FAILED);
    return true;
}

static bool descriptions(void) {
    CameraDescription defaults = rohr_camera_description_default_get();
    CHECK(defaults.enabled && !defaults.pause_with_engine && defaults.zoom == 1 &&
        defaults.dimensions.x == WINDOW_WIDTH && defaults.dimensions.y == WINDOW_HEIGHT);
    CameraAttachmentDescription attachment = rohr_camera_attachment_description_default_get();
    CHECK(attachment.entity == ENTITY_INVALID && attachment.follow_position && attachment.follow_orientation);
    int rendered = 0;
    CameraId active = rohr_camera_active_get();
    CameraIdResult made = rohr_camera_create((CameraDescription){.render_callback = draw, .render_context = &rendered});
    OK(made); CameraId id = made.result.value;
    CameraDescriptionResult current = rohr_camera_description_get(id); OK(current);
    CHECK(!current.result.value.enabled && current.result.value.zoom == 1 &&
        current.result.value.render_callback == draw && current.result.value.render_context == &rendered);
    EntityResult entity = rohr_entity_add(); OK(entity); Entity target = entity.result.value;
    OK(rohr_physics_position_set(target, (Position){10, 20}));
    OK(rohr_physics_orientation_set(target, 90));
    attachment.entity = target;
    attachment.position_offset = (Vec2D){0, 2};
    attachment.orientation_offset = 10;
    OK(rohr_camera_attachment_description_set(id, attachment));
    CHECK(rohr_camera_active_get() == active);
    current = rohr_camera_description_get(id); OK(current);
    CHECK(point(current.result.value.position, (Position){12, 20}) && near(current.result.value.orientation, 100));
    current.result.value.position = (Position){15, 20};
    current.result.value.orientation = 120;
    current.result.value.enabled = true;
    OK(rohr_camera_description_set(id, current.result.value));
    CameraAttachmentDescriptionResult attached = rohr_camera_attachment_description_get(id); OK(attached);
    CHECK(point(attached.result.value.position_offset, (Position){0, 5}) && near(attached.result.value.orientation_offset, 30));
    OK(rohr_physics_position_set(target, (Position){20, 30}));
    OK(rohr_physics_orientation_set(target, 180));
    current = rohr_camera_description_get(id); OK(current);
    CHECK(point(current.result.value.position, (Position){20, 25}) && near(current.result.value.orientation, 210));
    // A failed full edit cannot change callback, flags, offsets, or either motion.
    OK(rohr_camera_position_set(id, (Position){20, 15}, 1));
    OK(rohr_camera_zoom_set(id, 3, 1));
    CameraDescription invalid = current.result.value;
    invalid.position = (Position){999, 999}; invalid.zoom = NAN; invalid.render_callback = NULL;
    CHECK(rohr_error_check(rohr_camera_description_set(id, invalid)));
    CHECK(rohr_camera_moving_get(id).result.value);
    CHECK(rohr_camera_description_get(id).result.value.render_callback == draw);
    CameraAttachmentDescription bad = attached.result.value; bad.entity = ENTITY_INVALID;
    CHECK(rohr_error_check(rohr_camera_attachment_description_set(id, bad)));
    CHECK(rohr_camera_moving_get(id).result.value);
    attached = rohr_camera_attachment_description_get(id); OK(attached);
    OK(rohr_camera_attachment_description_set(id, attached.result.value));
    CHECK(rohr_camera_moving_get(id).result.value);
    attached.result.value.orientation_offset += 5;
    OK(rohr_camera_attachment_description_set(id, attached.result.value));
    CHECK(rohr_camera_moving_get(id).result.value);
    current = rohr_camera_description_get(id); OK(current);
    current.result.value.dimensions = (Vec2D){640, 480};
    OK(rohr_camera_description_set(id, current.result.value));
    CHECK(rohr_camera_moving_get(id).result.value);
    // Offset animation continues to inherit subsequent target movement and rotation.
    OK(rohr_physics_position_set(target, (Position){30, 40}));
    OK(rohr_physics_orientation_set(target, 90));
    CHECK(advance(64));
    CHECK(point(rohr_camera_get(id).result.value.position, (Position){45, 40}));
    CHECK(near(rohr_camera_zoom_get(id).result.value, 3));
    CHECK(!rohr_camera_moving_get(id).result.value);
    OK(rohr_camera_active_set(id));
    rohr_graphics_camera_rotate(10);
    CHECK(near(rohr_camera_attachment_description_get(id).result.value.orientation_offset, 45));
    rohr_graphics_camera_move((Vec2D){5, 0});
    CHECK(point(rohr_camera_get(id).result.value.position, (Position){50, 40}));
    CHECK(!rohr_error_check(rohr_camera_attachment_description_get(id)));
    // Position edits leave zoom running; zoom edits leave position running.
    OK(rohr_camera_zoom_set(id, 5, 1));
    OK(rohr_camera_position_move(id, (Vec2D){5, 0}, 0));
    CHECK(advance(64)); CHECK(near(rohr_camera_zoom_get(id).result.value, 5));
    OK(rohr_camera_position_move(id, (Vec2D){5, 0}, 1));
    current = rohr_camera_description_get(id); current.result.value.zoom = 2;
    OK(rohr_camera_description_set(id, current.result.value));
    CHECK(rohr_camera_moving_get(id).result.value);
    CHECK(advance(64)); CHECK(point(rohr_camera_get(id).result.value.position, (Position){60, 40}));
    // Invalid duration/translation cannot detach or cancel an existing move.
    OK(rohr_camera_position_move(id, (Vec2D){5, 0}, 1));
    CHECK(rohr_error_check(rohr_camera_position_set(id, (Position){0}, NAN)));
    CHECK(rohr_error_check(rohr_camera_position_move(id, (Vec2D){INFINITY, 0}, 0)));
    CHECK(rohr_camera_moving_get(id).result.value);
    OK(rohr_camera_detach(id));
    CHECK(!rohr_camera_moving_get(id).result.value);
    CHECK(rohr_error_check(rohr_camera_attachment_description_get(id)));
    CHECK(point(rohr_camera_get(id).result.value.position, (Position){60, 40}));
    // Each follow-flag combination converts world edits back into matching offsets.
    for(int flags = 0; flags < 4; ++flags) {
        attachment.follow_position = (flags & 1) != 0;
        attachment.follow_orientation = (flags & 2) != 0;
        OK(rohr_camera_attachment_description_set(id, attachment));
        current = rohr_camera_description_get(id); current.result.value.position = (Position){7, 9};
        current.result.value.orientation = 50;
        OK(rohr_camera_description_set(id, current.result.value));
        Camera view = rohr_camera_get(id).result.value;
        CHECK(point(view.position, (Position){7, 9}) && near(view.orientation, 50));
        OK(rohr_camera_description_set(id, rohr_camera_description_get(id).result.value));
        CHECK(!rohr_error_check(rohr_camera_attachment_description_get(id)));
    }
    // A missing target freezes the view but retains its exact generational handle.
    Camera frozen = rohr_camera_get(id).result.value;
    OK(rohr_entity_delete(target));
    CHECK(!rohr_graphics_camera_attached_get());
    CHECK(rohr_camera_attachment_description_get(id).result.value.entity == target);
    CHECK(point(rohr_camera_get(id).result.value.position, frozen.position));
    current = rohr_camera_description_get(id); current.result.value.enabled = false;
    OK(rohr_camera_description_set(id, current.result.value));
    current.result.value.position.x += 1;
    CHECK(rohr_error_check(rohr_camera_description_set(id, current.result.value)));
    CHECK(point(rohr_camera_get(id).result.value.position, frozen.position));
    entity = rohr_entity_add(); OK(entity); CHECK(entity.result.value != target);
    OK(rohr_physics_position_set(entity.result.value, (Position){900, 900}));
    CHECK(point(rohr_camera_get(id).result.value.position, frozen.position));
    OK(rohr_entity_delete(entity.result.value));
    /* Loading an explicitly unattached saved view can replace a stale attachment. */
    const char *state = "{\"version\":8,\"entities\":[],\"camera\":{\"transform\":{\"position\":{\"x\":8,\"y\":9},\"orientation\":25}}}";
    CHECK(SDL_SaveFile("camera_description_state.json", state, strlen(state)));
    OK(rohr_game_state_file_load("camera_description_state.json"));
    CHECK(remove("camera_description_state.json") == 0);
    CHECK(rohr_error_check(rohr_camera_attachment_description_get(id)));
    CHECK(point(rohr_camera_get(id).result.value.position, (Position){8, 9}));
    OK(rohr_camera_description_set(id, current.result.value));
    for(int invalid_case = 0; invalid_case < 4; ++invalid_case) {
        invalid = defaults;
        if(invalid_case == 0) invalid.zoom = -1;
        if(invalid_case == 1) invalid.dimensions.x = -1;
        if(invalid_case == 2) invalid.orientation = INFINITY;
        if(invalid_case == 3) invalid.position.x = NAN;
        CHECK(rohr_error_check(rohr_camera_create(invalid)));
        CHECK(rohr_error_check(rohr_camera_description_set(id, invalid)));
    }
    OK(rohr_camera_active_set(active)); OK(rohr_camera_destroy(id));
    CHECK(rohr_error_check(rohr_camera_description_get(id)));
    CHECK(rohr_error_check(rohr_camera_attachment_description_get(id)));
    return true;
}

static bool presentation(void) {
    WindowPresentationDescription before = rohr_graphics_window_presentation_description_get();
    WindowPresentationDescription changed = before;
    changed.window_width = 640; changed.window_height = 480;
    changed.logical_width = 320; changed.logical_height = 240; changed.aspect_ratio_auto = true;
    WindowPresentationDescription bad = changed; bad.mode = (GraphicsWindowMode)99;
    CHECK(rohr_error_check(rohr_graphics_window_presentation_description_set(bad)));
    CHECK(rohr_graphics_window_presentation_description_get().window_width == before.window_width);
    bad = changed; bad.logical_height = 0;
    CHECK(rohr_error_check(rohr_graphics_window_presentation_description_set(bad)));
#ifdef ROHR_TEST_CAMERA_WRAP
    fail_logical = true;
    CHECK(rohr_error_check(rohr_graphics_window_presentation_description_set(changed)));
    WindowPresentationDescription after = rohr_graphics_window_presentation_description_get();
    CHECK(after.window_width == before.window_width && after.window_height == before.window_height &&
        after.logical_width == before.logical_width && after.logical_height == before.logical_height &&
        after.mode == before.mode && after.aspect_ratio_auto == before.aspect_ratio_auto);
#endif
    OK(rohr_graphics_window_presentation_description_set(changed));
    CHECK(rohr_graphics_window_presentation_description_get().logical_width == 320);
    CHECK(rohr_graphics_window_presentation_description_get().aspect_ratio_auto);
#ifdef ROHR_TEST_CAMERA_WRAP
    fail_logical = true;
    OK(rohr_graphics_window_presentation_description_set(rohr_graphics_window_presentation_description_get()));
    CHECK(fail_logical); fail_logical = false;
#endif
    OK(rohr_graphics_window_presentation_description_set(before));
    return true;
}

int main(void) {
    if(rohr_error_check(rohr_engine_start()) || rohr_error_check(rohr_graphics_start())) return 1;
    if(rohr_error_check(rohr_engine_time_per_tick_set(1.0 / 64.0))) return 1;
    bool passed = descriptions() && presentation();
    rohr_graphics_stop(); rohr_engine_stop();
    return passed ? 0 : 1;
}
