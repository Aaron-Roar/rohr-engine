/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "graphics/font_assets.h"
#include "graphics/text_assets.h"

#include <stdio.h>

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
    failed = rohr_graphics_text_create(&default_font, NULL,
        (Color){255, 255, 255, 255});
    if(!rohr_error_check(failed) ||
            !stats_check(0, 0, 0, 0, 0, "creation failure"))
        return fail("failed creation changed registry state", true, true);

    first = rohr_graphics_text_create(&default_font, "same",
        (Color){255, 255, 255, 255});
    second = graphics_text_create(&default_font, "same",
        (Color){255, 255, 255, 255});
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

    first = rohr_graphics_text_create(&default_font, "",
        (Color){255, 255, 255, 255});
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

    custom_font = rohr_graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_PATH, .point_size = 18.0f});
    if(rohr_error_check(custom_font))
        return fail("custom font load failed", true, true);
    custom_stale = custom_font.result.value;
    first = rohr_graphics_text_create(&custom_font.result.value, "custom font",
        (Color){230, 234, 242, 255});
    if(rohr_error_check(first) ||
            !result_ok(rohr_graphics_font_release(&custom_font.result.value)) ||
            !rohr_graphics_font_valid_check(custom_stale) ||
            !rohr_graphics_text_wrap_width_set(&first.result.value, 40) ||
            !rohr_graphics_text_value_set(&first.result.value, "wrapped value"))
        return fail("custom text did not own or reuse its font", true, true);
    if(!result_ok(rohr_graphics_text_destroy(&first.result.value)) ||
            rohr_graphics_font_valid_check(custom_stale))
        return fail("text did not release its font dependency", true, true);

    first = rohr_graphics_text_create(&default_font, "ui owner",
        (Color){255, 255, 255, 255});
    second = rohr_graphics_text_create(&default_font, "replacement",
        (Color){255, 255, 255, 255});
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

    first = rohr_graphics_text_create(&default_font, "shape label",
        (Color){255, 255, 255, 255});
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

    first = rohr_graphics_text_create(&default_font, "queued revision",
        (Color){255, 255, 255, 255});
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

    first = rohr_graphics_text_create(&default_font, "stale",
        (Color){255, 255, 255, 255});
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
    first = rohr_graphics_text_create(&default_font, "new generation",
        (Color){255, 255, 255, 255});
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
    first = rohr_graphics_text_create(&default_font, "restart",
        (Color){255, 255, 255, 255});
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
