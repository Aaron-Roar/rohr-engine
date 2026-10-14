/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool forbid_allocation;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { assert(!forbid_allocation); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { assert(!forbid_allocation); return __real_calloc(n, s); }
void *__wrap_realloc(void *p, size_t n) { assert(!forbid_allocation); return __real_realloc(p, n); }
#endif

static const UIRect bounds = {0, 0, 100, 30};
static uint64_t metric(const Report *r, ReportMetricId id) {
    assert(report_valid_check(r));
    for(size_t i = 0; i < r->statistic_count; ++i) {
        assert(r->statistics[i].report_id == ROHR_UI_REPORT);
        if(r->statistics[i].metric_id == id) return r->statistics[i].value;
    }
    return 0;
}
static void complete(const Report *r) {
    assert(report_valid_check(r) && r->header.id == ROHR_UI_REPORT);
    assert(r->header.status == ROHR_REPORT_COMPLETE && !r->has_fatal);
    assert(r->entry_count == 0 && r->header.total_ms >= 0);
}
static void clean(Report r) { complete(&r); assert(r.diagnostic_count == 0); }
static uint64_t warnings(const Report *r, EngineError code) {
    uint64_t n = 0;
    complete(r);
    for(size_t i = 0; i < r->diagnostic_count; ++i) {
        assert(r->diagnostics[i].severity == ROHR_REPORT_SEVERITY_WARNING);
        assert(r->diagnostics[i].report_id == ROHR_UI_REPORT);
        if(r->diagnostics[i].code == code) n += r->diagnostics[i].occurrence_count;
    }
    return n;
}
static void key(void) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN}; e.key.key = SDLK_A;
    ui_event_add(&e);
}
static void text(const char *s) {
    SDL_Event e = {.type = SDL_EVENT_TEXT_INPUT}; e.text.text = s;
    rohr_ui_event_add(&e);
}
static void input(MouseButtonState button) {
    rohr_ui_input_set((UIInput){.pointer = {10, 10}, .primary_button = button});
}

static void lifecycle(void) {
    /* First widget works with default input, before any explicit setup. */
    assert(ui_interaction("first", bounds).hovered);
    Report r = ui_update(); clean(r);
    assert(metric(&r, ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT) == 1);
    assert(metric(&r, ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK) == 1);
    r = rohr_ui_update(); clean(r); assert(r.statistic_count == 0);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    forbid_allocation = true;
#endif
    key(); text("abc");
    input(MOUSE_BUTTON_STATE_PRESSED);
    assert(ui_interaction("held", bounds).pressed);
    assert(ui_pointer_consumed_get());
    input(MOUSE_BUTTON_STATE_PRESSED);
    assert(ui_key_pressed_check(SDLK_A));
    assert(!ui_interaction("behind", bounds).pressed);
    assert(ui_pointer_consumed_get());
    r = rohr_ui_update(); clean(r);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    forbid_allocation = false;
#endif
    assert(metric(&r, ROHR_REPORT_UI_EVENT_RECEIVED_COUNT) == 2);
    assert(metric(&r, ROHR_REPORT_UI_TEXT_BYTES_PEAK) == 3);
    assert(metric(&r, ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT) == 2);
    assert(metric(&r, ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK) == 2);
    assert(!ui_pointer_consumed_get() && !ui_primary_pressed_check());
    assert(!ui_key_pressed_check(SDLK_A));
    assert(!ui_interaction("held", bounds).pressed); /* No replay without input. */
    clean(ui_update());
    input(MOUSE_BUTTON_STATE_DOWN);
    assert(ui_interaction("held", bounds).pressed); clean(ui_update());
    input(MOUSE_BUTTON_STATE_RELEASED);
    assert(ui_interaction("held", bounds).clicked); clean(ui_update());
    assert(!ui_interaction("held", bounds).clicked); clean(ui_update());
    assert(ui_navigation_activate());
    assert(ui_interaction("held", bounds).keyboard_activated); clean(ui_update());
    clean(ui_update()); /* Missing controls lose focus even without input/widgets. */
    UIRect focused;
    assert(!ui_navigation_focus_bounds_get(&focused) && !ui_navigation_activate());

    char value[32] = "";
    UIFieldBinding binding = {.kind = UI_FIELD_STRING, .string = value,
        .string_capacity = sizeof(value)};
    ui_field_focus_set("field", binding, NULL, false);
    text("hello"); input(MOUSE_BUTTON_STATE_UP); input(MOUSE_BUTTON_STATE_UP);
    (void)ui_field("field", binding, NULL, bounds, NULL);
    assert(strcmp(value, "hello") == 0); clean(ui_update());
    (void)ui_field("field", binding, NULL, bounds, NULL);
    assert(strcmp(value, "hello") == 0); clean(ui_update());
    clean(ui_update());
    UISliderConfig slider = ui_slider_config_default_get();
    slider.center = (Position){100, 100}; slider.length = 100;
    ui_input_set((UIInput){.pointer = {100, 100}, .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    UISliderResult drag = ui_slider("drag", 0, &slider); assert(drag.pressed);
    clean(ui_update());
    ui_input_set((UIInput){.pointer = {140, 100}, .primary_button = MOUSE_BUTTON_STATE_DOWN});
    drag = ui_slider("drag", drag.value, &slider); assert(drag.value > 0.8f);
    clean(ui_update());
    float held_value = drag.value;
    drag = ui_slider("drag", held_value, &slider);
    assert(drag.value == held_value && !drag.pressed); clean(ui_update());
    ui_input_set((UIInput){.pointer = {150, 100}, .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    drag = ui_slider("drag", held_value, &slider); assert(drag.value == 1.0f);
    clean(ui_update());

    SDL_Event wheel = {.type = SDL_EVENT_MOUSE_WHEEL}; wheel.wheel.y = -1;
    ui_event_add(&wheel); input(MOUSE_BUTTON_STATE_UP);
    UIScrollRegionResult scroll = ui_scroll_region_begin("wheel", bounds, 100, 0, 10);
    assert(scroll.offset == 10); ui_scroll_region_end(); clean(ui_update());
    scroll = ui_scroll_region_begin("wheel", bounds, 100, scroll.offset, 10);
    assert(scroll.offset == 10 && !scroll.changed);
    ui_scroll_region_end(); clean(ui_update());

    /* Equivalent direct/public paths collect the same work. */
    input(MOUSE_BUTTON_STATE_UP); key(); (void)ui_interaction("parity", bounds);
    Report direct = ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10, 10}}); key();
    (void)rohr_ui_interaction("parity", bounds); Report wrapped = rohr_ui_update();
    assert(direct.header.id == wrapped.header.id && direct.header.status == wrapped.header.status);
    assert(direct.statistic_count == wrapped.statistic_count);
    for(size_t i = 0; i < direct.statistic_count; ++i)
        assert(metric(&wrapped, direct.statistics[i].metric_id) == direct.statistics[i].value);
    assert(direct.diagnostic_count == wrapped.diagnostic_count);

}

static void capacities(void) {
    SDL_Event ignored = {.type = SDL_EVENT_KEY_UP};
    ui_event_add(NULL); ui_event_add(&ignored);
    for(size_t i = 0; i < UI_FIELD_KEY_EVENT_MAX + 3; ++i) key();
    for(size_t i = 0; i < UI_FIELD_KEY_EVENT_MAX + 2; ++i) text("x");
    SDL_Event wheel = {.type = SDL_EVENT_MOUSE_WHEEL}; wheel.wheel.y = 1;
    ui_event_add(&wheel);
    Report r = ui_update(), saved = r;
    assert(metric(&r, ROHR_REPORT_UI_EVENT_RECEIVED_COUNT) == 2 * UI_FIELD_KEY_EVENT_MAX + 6);
    assert(metric(&r, ROHR_REPORT_UI_EVENT_DROPPED_COUNT) == 5);
    assert(metric(&r, ROHR_REPORT_UI_KEY_EVENT_SLOTS_PEAK) == UI_FIELD_KEY_EVENT_MAX);
    assert(metric(&r, ROHR_REPORT_UI_TEXT_EVENT_SLOTS_PEAK) == UI_FIELD_KEY_EVENT_MAX);
    assert(metric(&r, ROHR_REPORT_UI_TEXT_BYTES_PEAK) == UI_FIELD_KEY_EVENT_MAX);
    assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 5);
    char full[UI_FIELD_EDIT_MAX]; memset(full, 'x', sizeof(full) - 1); full[sizeof(full) - 1] = 0;
    text(full); text("x");
    Report bytes = rohr_ui_update();
    assert(metric(&bytes, ROHR_REPORT_UI_TEXT_BYTES_PEAK) == UI_FIELD_EDIT_MAX - 1);
    assert(metric(&bytes, ROHR_REPORT_UI_EVENT_DROPPED_COUNT) == 1);
    assert(warnings(&bytes, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 1);
    assert(!error_check(report_merge(&r, &bytes)));
    assert(metric(&r, ROHR_REPORT_UI_EVENT_DROPPED_COUNT) == 6);
    assert(metric(&r, ROHR_REPORT_UI_TEXT_BYTES_PEAK) == UI_FIELD_EDIT_MAX - 1);
    Report empty = ui_update(); clean(empty); assert(empty.statistic_count == 0);
    assert(saved.diagnostic_count == 2 && metric(&saved, ROHR_REPORT_UI_EVENT_DROPPED_COUNT) == 5);
    for(size_t i = 0; i < 515; ++i) (void)ui_interaction("same-id", bounds);
    r = ui_update();
    assert(metric(&r, ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT) == 515);
    assert(metric(&r, ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK) == 512);
    assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 3);
    const TextAsset *options[513] = {0};
    (void)ui_dropdown("too-many", options, 513, 0, (UIDropdownConfig){0}, bounds, NULL);
    r = ui_update(); assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 1);
    for(size_t i = 0; i < 65; ++i) {
        (void)ui_scroll_region_begin("scroll", bounds, 30, 0, 1);
        ui_scroll_region_end();
    }
    r = ui_update();
    assert(metric(&r, ROHR_REPORT_UI_SCROLL_RECORDS_PEAK) == 64);
    assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 1);
    for(size_t i = 0; i < 9; ++i) ui_translation_y_push(1);
    for(size_t i = 0; i < 9; ++i) (void)ui_scroll_region_begin("nested", bounds, 30, 0, 1);
    r = ui_update();
    assert(metric(&r, ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK) == 8);
    assert(metric(&r, ROHR_REPORT_UI_SCROLL_SCOPE_DEPTH_PEAK) == 8);
    assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 2);
    assert(warnings(&r, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 16);
    input(MOUSE_BUTTON_STATE_UP);
    assert(ui_interaction("untranslated", bounds).hovered); clean(ui_update());
    ui_translation_y_pop(); ui_scroll_region_end(); ui_clip_end(); ui_modal_controls_end();
    r = ui_update(); assert(warnings(&r, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 4);
    ui_translation_y_push(1); ui_modal_controls_begin(); ui_translation_y_pop();
    r = ui_update(); assert(warnings(&r, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 3);
    clean(ui_update());
    for(size_t i = 0; i < 65; ++i) ui_modal_controls_begin();
    r = ui_update();
    assert(warnings(&r, ERROR_ENGINE_UI_CAPACITY_EXCEEDED) == 1);
    assert(warnings(&r, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 64);
    assert(!error_check(rohr_report_text_write(&saved, "ui_report.txt")));
    assert(!error_check(rohr_report_json_write(&saved, "ui_report.json", true)));
    const char *files[] = {"ui_report.txt", "ui_report.json"};
    for(size_t i = 0; i < 2; ++i) {
        char buffer[16384] = {0}; FILE *f = fopen(files[i], "rb"); assert(f);
        size_t n = fread(buffer, 1, sizeof(buffer) - 1, f); assert(n > 0); fclose(f);
        assert(strstr(buffer, i == 0 ? "UI_EVENT_DROPPED_COUNT" : "ROHR_REPORT_UI_EVENT_DROPPED_COUNT"));
        assert(strstr(buffer, "ERROR_ENGINE_UI_CAPACITY_EXCEEDED"));
        assert(remove(files[i]) == 0);
    }
}

static void graphics_ownership(void) {
    assert(!error_check(rohr_engine_start())); assert(!error_check(rohr_graphics_start()));
    /* Discard earlier draw-without-renderer evidence, which UI never duplicated. */
    (void)rohr_graphics_pipeline_commands_execute();
    assert(graphics_screen_clip_push(0, 0, 200, 200)); /* Surrounding graphics scope. */
    assert(ui_clip_begin(bounds));
    ui_translation_y_push(5);
    (void)ui_scroll_region_begin("nested", bounds, 30, 0, 1);
    ui_modal_set(bounds); ui_modal_controls_begin();
    Report saved = ui_update(), snapshot = saved;
    assert(warnings(&saved, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 4);
    /* Graphics has 16 slots: exactly 15 remain, proving only our clips were popped. */
    for(size_t i = 0; i < 15; ++i) assert(graphics_screen_clip_push(0, 0, 200, 200));
    assert(!ui_clip_begin(bounds));
    (void)ui_scroll_region_begin("failed-clip", bounds, 30, 0, 1);
    ui_scroll_region_end(); /* Failed clip must never pop a surrounding scope. */
    assert(!graphics_screen_clip_push(0, 0, 200, 200));
    clean(ui_update());
    for(size_t i = 0; i < 16; ++i) graphics_screen_clip_pop();
    ui_clip_end(); /* An unmatched UI end also leaves graphics ownership alone. */
    Report r = ui_update(); assert(warnings(&r, ERROR_ENGINE_UI_SCOPE_UNBALANCED) == 1);
    ui_surface((UIRect){NAN, 0, 1, 1}, (Color){0});
    clean(ui_update());
    Report graphics = graphics_pipeline_commands_execute();
    assert(graphics.diagnostic_count > 0);
    uint64_t clip_failures = 0;
    for(size_t i = 0; i < graphics.diagnostic_count; ++i)
        if(graphics.diagnostics[i].code == ERROR_ENGINE_GRAPHICS_STATE_FAILED)
            clip_failures += graphics.diagnostics[i].occurrence_count;
    assert(clip_failures == 3);
    for(size_t i = 0; i < graphics.diagnostic_count; ++i)
        assert(graphics.diagnostics[i].report_id != ROHR_UI_REPORT);
    rohr_graphics_stop(); rohr_engine_stop();
    assert(memcmp(&saved, &snapshot, sizeof(saved)) == 0);
    clean(ui_update());
}

int main(void) {
    lifecycle(); capacities(); graphics_ownership();
    return 0;
}
