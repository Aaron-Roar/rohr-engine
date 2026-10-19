/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "graphics/font_assets.h"
#include "graphics/text_assets.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifndef ROHR_TEST_FONT_PATH
#error "ROHR_TEST_FONT_PATH must name the font fixture"
#endif

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "error %d: %s\n", (int)result.result.error,
        rohr_error_message_get(result));
    return false;
}

static bool stats_check(size_t resources, size_t owners,
        size_t dependencies, size_t payloads, size_t commands,
        const char *context) {
    GraphicsTextAssetStats stats = graphics_text_assets_stats_get();
    if(stats.live_resources == resources && stats.public_owners == owners &&
            stats.dependency_references == dependencies &&
            stats.live_payloads == payloads &&
            stats.command_references == commands) return true;
    fprintf(stderr,
        "%s: text stats were resources=%zu owners=%zu dependencies=%zu "
        "payloads=%zu commands=%zu; expected %zu/%zu/%zu/%zu/%zu\n",
        context, stats.live_resources, stats.public_owners,
        stats.dependency_references, stats.live_payloads,
        stats.command_references, resources, owners, dependencies, payloads,
        commands);
    return false;
}

static int fail(const char *message, bool graphics_started,
        bool engine_started) {
    fprintf(stderr, "%s\n", message);
    if(graphics_started) rohr_graphics_stop();
    if(engine_started) rohr_engine_stop();
    return 1;
}

#ifdef ROHR_TEST_TEXT_WRAP
static bool fail_payload_allocation;
void *__real_calloc(size_t count, size_t size);
void *__wrap_calloc(size_t count, size_t size) {
    if(fail_payload_allocation) { fail_payload_allocation = false; return NULL; }
    return __real_calloc(count, size);
}
#endif

static void description_checks(void) {
    TextDescription defaults = rohr_graphics_text_description_default_get();
    assert(defaults.value != NULL && defaults.value[0] == '\0');
    assert(defaults.color.alpha == 255 && defaults.wrap_width == 0);
    TextAssetResult made = rohr_graphics_text_create(defaults);
    assert(!rohr_error_check(made));
    TextAsset text = made.result.value;
    TextHandle handle = text.handle;
    TextDescriptionResult current = rohr_graphics_text_description_get(text);
    assert(!rohr_error_check(current));
    assert(current.result.value.font.handle == rohr_graphics_font_default_get().handle);
    assert(text.size.x == 0 && text.size.y == 0);
    assert(rohr_error_check(graphics_text_create((TextDescription){0})));
    assert(rohr_error_check(graphics_text_description_get((TextAsset){0})));
    assert(rohr_error_check(graphics_text_description_set(NULL, defaults)));

    char input[] = "hello world";
    defaults.value = input;
    defaults.wrap_width = 40;
    assert(!rohr_error_check(rohr_graphics_text_description_set(&text, defaults)));
    input[0] = 'X';
    current = graphics_text_description_get(text);
    assert(strcmp(current.result.value.value, "hello world") == 0);
    assert(text.handle == handle && text.size.x == 40 && text.size.y == 16);
    GraphicsTextPayload *payload = graphics_text_internal_payload_get(handle);
    const char *borrowed = current.result.value.value;
    assert(!rohr_error_check(graphics_text_description_set(&text, current.result.value)));
    assert(graphics_text_internal_payload_get(handle) == payload);
    assert(graphics_text_description_get(text).result.value.value == borrowed);
    assert(graphics_text_assets_stats_get().live_payloads == 1);

    TextDescription invalid = current.result.value;
    invalid.font.handle = UINT32_MAX;
    assert(rohr_error_check(graphics_text_description_set(&text, invalid)));
    invalid = current.result.value;
    invalid.wrap_width = -1;
    assert(rohr_error_check(graphics_text_description_set(&text, invalid)));
    invalid = current.result.value;
    invalid.value = NULL;
    assert(rohr_error_check(graphics_text_description_set(&text, invalid)));
#ifdef ROHR_TEST_TEXT_WRAP
    invalid = current.result.value;
    invalid.value = "replacement";
    fail_payload_allocation = true;
    assert(rohr_error_check(graphics_text_description_set(&text, invalid)));
    assert(!fail_payload_allocation);
#endif
    assert(graphics_text_internal_payload_get(handle) == payload);
    assert(graphics_text_description_get(text).result.value.value == borrowed);
    assert(text.size.x == 40 && text.size.y == 16);
    /* Borrowed interior strings are copied before the prior storage is freed. */
    current.result.value.value += 6;
    current.result.value.color = (Color){12, 34, 56, 78};
    assert(!rohr_error_check(graphics_text_description_set(&text, current.result.value)));
    assert(strcmp(graphics_text_description_get(text).result.value.value, "world") == 0);
    assert(graphics_text_description_get(text).result.value.color.alpha == 78);
    assert(text.size.x == 40 && text.size.y == 8);
    assert(rohr_graphics_text_wrap_width_set(&text, 1));
    assert(text.size.x == 8 && text.size.y == 40);
    assert(rohr_graphics_text_wrap_width_set(&text, 24));
    assert(rohr_graphics_text_value_set(&text, "abc   "));
    assert(text.size.x == 24 && text.size.y == 8);
    assert(rohr_graphics_text_value_set(&text, "abc \ndef"));
    assert(text.size.x == 24 && text.size.y == 16);
    assert(rohr_graphics_text_value_set(&text, "world"));
    assert(rohr_graphics_text_wrap_width_set(&text, 0));
    assert(text.size.x == 40 && text.size.y == 8);
    assert(rohr_graphics_text_value_set(&text, "ab\ncd\n"));
    assert(text.size.x == 16 && text.size.y == 24);
    assert(rohr_graphics_text_value_set(&text, "a\r\nb\tX"));
    assert(text.size.x == 48 && text.size.y == 16);
    assert(rohr_graphics_text_value_set(&text, "éé"));
    assert(text.size.x == 16 && text.size.y == 8);
    assert(!rohr_error_check(graphics_text_description_set(&text,
        (TextDescription){.value = "literal"})));
    current = graphics_text_description_get(text);
    assert(current.result.value.color.alpha == 0 && current.result.value.wrap_width == 0);
    assert(current.result.value.font.handle == rohr_graphics_font_default_get().handle);

    FontAssetResult font = rohr_graphics_font_load((FontDescription){
        .file = ROHR_TEST_FONT_PATH, .point_size = 18});
    assert(!rohr_error_check(font));
    FontAsset old_font = font.result.value;
    defaults.font = old_font;
    defaults.value = "custom text with several words";
    defaults.wrap_width = 45;
    assert(!rohr_error_check(graphics_text_description_set(&text, defaults)));
    assert(!rohr_error_check(rohr_graphics_font_release(&font.result.value)));
    assert(graphics_font_assets_stats_get().owner_references == 1);
    assert(graphics_text_description_get(text).result.value.wrap_width == 45);
    assert(text.size.y > 18);
    GraphicsUiIdResult ui = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .text = text, .scale = {1, 1}});
    assert(!rohr_error_check(ui));
    assert(rohr_graphics_text_draw(&text, (Position){0}));
    /* Replace a revision once, then change fonts while the older revision is queued. */
    assert(rohr_graphics_text_value_set(&text, "another revision"));
    assert(graphics_font_assets_stats_get().owner_references == 2);
    assert(!rohr_error_check(rohr_graphics_text_description_set(&text,
        (TextDescription){.value = "built in", .color = {255, 255, 255, 255}})));
    assert(rohr_graphics_font_valid_check(old_font));
    assert(graphics_font_assets_stats_get().owner_references == 1);
    Scale size;
    assert(graphics_text_internal_size_get(handle, &size));
    assert(size.x == text.size.x && size.y == text.size.y);
    assert(rohr_graphics_ui_text_get(ui.result.value).result.value.text.handle == handle);
    rohr_graphics_show();
    assert(!rohr_graphics_font_valid_check(old_font));
    TextAsset stale = text;
    assert(!rohr_error_check(rohr_graphics_text_destroy(&text)));
    assert(rohr_error_check(graphics_text_description_get(stale)));
    assert(rohr_error_check(graphics_text_description_set(&stale, defaults)));
    assert(!rohr_error_check(rohr_graphics_ui_destroy(ui.result.value)));
    assert(stats_check(0, 0, 0, 0, 0, "description cleanup"));
}

int main(void) {
    FontAsset default_font;
    FontAssetResult custom_font;
    FontAsset custom_stale;
    TextAssetResult first;
    TextAssetResult second;
    TextAssetResult failed;
    TextAsset stale;
    TextAsset stale_destroy;
    TextAsset queued_stale;
    TextAsset shutdown_stale;
    TextAsset invalid = {.handle = UINT32_MAX};
    TextAsset zero = {0};
    GraphicsUiIdResult ui;
    GraphicsUiIdResult shape_ui;
    GraphicsUiTextResult ui_value;
    TextHandle expired;
    Scale previous_size;
    bool engine_started = false;
    bool graphics_started = false;

    if(!result_ok(rohr_engine_start()))
        return fail("engine start failed", false, false);
    engine_started = true;
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics start failed", false, true);
    graphics_started = true;
    default_font = rohr_graphics_font_default_get();
    if(!stats_check(0, 0, 0, 0, 0, "initial state"))
        return fail("text registry did not start empty", true, true);
    description_checks();
    failed = rohr_graphics_text_create((TextDescription){.font = default_font, .value = NULL, .color = (Color){255, 255, 255, 255}});
    if(!rohr_error_check(failed) ||
            !stats_check(0, 0, 0, 0, 0, "creation failure"))
        return fail("failed creation changed registry state", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "same", .color = (Color){255, 255, 255, 255}});
    second = graphics_text_create((TextDescription){.font = default_font, .value = "same", .color = (Color){255, 255, 255, 255}});
    if(rohr_error_check(first) || rohr_error_check(second) ||
            first.result.value.handle == second.result.value.handle ||
            !rohr_graphics_text_valid_check(first.result.value) ||
            !graphics_text_valid_check(second.result.value) ||
            !stats_check(2, 2, 0, 2, 0, "unique instances"))
        return fail("identical text did not create unique instances", true, true);
    if(!result_ok(graphics_text_destroy(&first.result.value)) ||
            !result_ok(rohr_graphics_text_destroy(&second.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, "unique destruction"))
        return fail("direct and wrapper text destruction failed", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "", .color = (Color){255, 255, 255, 255}});
    if(rohr_error_check(first) || first.result.value.size.x != 0.0f ||
            first.result.value.size.y != 0.0f ||
            !rohr_graphics_text_draw(&first.result.value, (Position){0}) ||
            !stats_check(1, 1, 0, 0, 0, "empty text"))
        return fail("empty text contract failed", true, true);
    previous_size = first.result.value.size;
    if(rohr_graphics_text_value_set(&first.result.value, NULL) ||
            first.result.value.size.x != previous_size.x ||
            first.result.value.size.y != previous_size.y ||
            !rohr_graphics_text_valid_check(first.result.value) ||
            !stats_check(1, 1, 0, 0, 0, "transactional failure"))
        return fail("failed mutation changed empty text", true, true);
    if(!result_ok(rohr_graphics_text_destroy(&first.result.value)))
        return fail("empty text destruction failed", true, true);

    custom_font = rohr_graphics_font_load((FontDescription){
        .file = ROHR_TEST_FONT_PATH, .point_size = 18.0f});
    if(rohr_error_check(custom_font))
        return fail("custom font load failed", true, true);
    custom_stale = custom_font.result.value;
    first = rohr_graphics_text_create((TextDescription){.font = custom_font.result.value, .value = "custom font", .color = (Color){230, 234, 242, 255}});
    if(rohr_error_check(first) ||
            !result_ok(rohr_graphics_font_release(&custom_font.result.value)) ||
            !rohr_graphics_font_valid_check(custom_stale) ||
            !rohr_graphics_text_wrap_width_set(&first.result.value, 40) ||
            !rohr_graphics_text_value_set(&first.result.value, "wrapped value"))
        return fail("custom text did not own or reuse its font", true, true);
    if(!result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            rohr_graphics_font_valid_check(custom_stale))
        return fail("text did not release its font dependency", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "ui owner", .color = (Color){255, 255, 255, 255}});
    second = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "replacement", .color = (Color){255, 255, 255, 255}});
    ui = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .text = first.result.value, .scale = {1.0f, 1.0f}});
    if(rohr_error_check(first) || rohr_error_check(second) ||
            rohr_error_check(ui) ||
            !stats_check(2, 2, 1, 2, 0, "ui dependency"))
        return fail("persistent UI did not acquire text", true, true);
    if(rohr_error_check(rohr_graphics_ui_text_set(ui.result.value,
                (ViewportUiTextConfig){.text = second.result.value,
                    .scale = {1.0f, 1.0f}})) ||
            !stats_check(2, 2, 1, 2, 0, "ui replacement"))
        return fail("persistent UI replacement failed", true, true);
    ui_value = rohr_graphics_ui_text_get(ui.result.value);
    if(rohr_error_check(ui_value) ||
            ui_value.result.value.text.handle != second.result.value.handle ||
            !rohr_error_check(rohr_graphics_ui_text_set(ui.result.value,
                (ViewportUiTextConfig){.text = invalid,
                    .scale = {1.0f, 1.0f}})) ||
            rohr_graphics_ui_text_get(ui.result.value).result.value.text.handle !=
                second.result.value.handle ||
            !stats_check(2, 2, 1, 2, 0, "failed UI replacement"))
        return fail("persistent UI getter did not return borrowed text", true, true);
    stale = second.result.value;
    if(!result_ok(rohr_graphics_text_destroy(&second.result.value)) ||
            rohr_graphics_text_valid_check(stale) ||
            !stats_check(2, 1, 1, 2, 0, "ui deferred destruction") ||
            !result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            !stats_check(1, 0, 1, 1, 0, "ui sole dependency") ||
            !result_ok(rohr_graphics_ui_destroy(ui.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, "ui destruction"))
        return fail("persistent UI dependency cleanup failed", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "shape label", .color = (Color){255, 255, 255, 255}});
    shape_ui = rohr_graphics_ui_shape_create((ViewportUiShapeConfig){
        .shape = {.amount_of_vertices = 3,
            .vertices = {{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 10.0f}}},
        .text = {.text = first.result.value, .scale = {1.0f, 1.0f}}});
    if(rohr_error_check(first) || rohr_error_check(shape_ui) ||
            !result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            !stats_check(1, 0, 1, 1, 0, "shape text dependency") ||
            !result_ok(rohr_graphics_ui_destroy(shape_ui.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, "shape text destruction"))
        return fail("shape-contained text dependency cleanup failed", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "queued revision", .color = (Color){255, 255, 255, 255}});
    queued_stale = first.result.value;
    if(rohr_error_check(first) || !rohr_graphics_text_draw(&first.result.value,
                (Position){10.0f, 10.0f}) ||
            !stats_check(1, 1, 0, 1, 1, "queued revision") ||
            !rohr_graphics_text_value_set(&first.result.value, "new revision") ||
            !stats_check(1, 1, 0, 2, 1, "mutated queued revision") ||
            !result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            rohr_graphics_text_valid_check(queued_stale) ||
            !stats_check(1, 0, 0, 2, 1, "destroyed queued owner"))
        return fail("queued revision ownership failed", true, true);
    rohr_graphics_show();
    if(!stats_check(0, 0, 0, 0, 0, "executed queued revision"))
        return fail("queued revision was not released", true, true);

    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "stale", .color = (Color){255, 255, 255, 255}});
    stale = first.result.value;
    expired = stale.handle;
    if(!result_ok(rohr_graphics_text_destroy(&first.result.value)))
        return fail("stale fixture destruction failed", true, true);
    stale_destroy = stale;
    if(!rohr_error_check(rohr_graphics_text_destroy(NULL)) ||
            !rohr_error_check(rohr_graphics_text_destroy(&stale_destroy)) ||
            stale_destroy.handle != stale.handle ||
            !result_ok(rohr_graphics_text_destroy(&zero)) ||
            zero.handle != TEXT_HANDLE_INVALID ||
            rohr_graphics_text_valid_check(stale))
        return fail("zero or stale destruction contract failed", true, true);
    ui = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .text = stale, .scale = {1.0f, 1.0f}});
    if(!rohr_error_check(ui) || !stats_check(0, 0, 0, 0, 0,
            "stale UI rejection"))
        return fail("persistent UI accepted stale text", true, true);
    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "new generation", .color = (Color){255, 255, 255, 255}});
    if(rohr_error_check(first) || first.result.value.handle == expired)
        return fail("text generation did not advance", true, true);

    shutdown_stale = first.result.value;
    ui = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .text = first.result.value, .scale = {1.0f, 1.0f}});
    if(rohr_error_check(ui) ||
            !rohr_graphics_text_draw(&first.result.value, (Position){0}) ||
            !result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            !stats_check(1, 0, 1, 1, 1, "shutdown dependencies"))
        return fail("shutdown queue fixture failed", true, true);
    rohr_graphics_stop();
    graphics_started = false;
    if(rohr_graphics_text_valid_check(shutdown_stale) ||
            !stats_check(0, 0, 0, 0, 0, "graphics shutdown"))
        return fail("graphics shutdown did not clear text", false, true);
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics restart failed", false, true);
    graphics_started = true;
    default_font = rohr_graphics_font_default_get();
    first = rohr_graphics_text_create((TextDescription){.font = default_font, .value = "restart", .color = (Color){255, 255, 255, 255}});
    if(rohr_error_check(first) ||
            first.result.value.handle == shutdown_stale.handle ||
            rohr_graphics_text_valid_check(shutdown_stale) ||
            !result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, "graphics restart"))
        return fail("graphics restart text generation failed", true, true);

    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
