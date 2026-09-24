/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "graphics/font_assets.h"

#include <math.h>
#include <stdio.h>

#ifndef ROHR_TEST_FONT_PATH
#error "ROHR_TEST_FONT_PATH must name the font fixture"
#endif

#ifndef ROHR_TEST_FONT_ALIAS_PATH
#error "ROHR_TEST_FONT_ALIAS_PATH must name an equivalent normalized path"
#endif

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "error %d: %s\n", (int)result.result.error,
        rohr_error_message_get(result));
    return false;
}

static bool stats_check(size_t live_resources, size_t owner_references,
        size_t pinned_resources, const char *context) {
    GraphicsFontAssetStats stats = graphics_font_assets_stats_get();
    if(stats.live_resources == live_resources &&
            stats.owner_references == owner_references &&
            stats.pinned_resources == pinned_resources) return true;
    fprintf(stderr,
        "%s: font stats were live=%zu owners=%zu pinned=%zu; "
        "expected %zu/%zu/%zu\n",
        context, stats.live_resources, stats.owner_references,
        stats.pinned_resources, live_resources, owner_references,
        pinned_resources);
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
    FontAsset default_alias;
    FontAssetResult first;
    FontAssetResult shared;
    FontAssetResult different_size;
    FontAsset retained;
    FontAsset stale;
    FontAsset stale_release;
    FontAsset shutdown_stale;
    FontAsset old_default;
    FontAsset zero = {0};
    TextAssetResult text;
    TextAssetResult failed_text;
    FontHandle expired_handle;
    bool engine_started = false;
    bool graphics_started = false;

    if(!result_ok(rohr_engine_start()))
        return fail("engine start failed", false, false);
    engine_started = true;
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics start failed", false, true);
    graphics_started = true;
    if(!stats_check(1, 0, 1, "initial pinned font"))
        return fail("font registry did not pin its built-in font", true, true);

    default_font = graphics_font_default_get();
    default_alias = default_font;
    if(default_font.handle == FONT_HANDLE_INVALID ||
            !rohr_graphics_font_valid_check(default_font) ||
            !result_ok(rohr_graphics_font_retain(default_alias)) ||
            !result_ok(rohr_graphics_font_release(&default_font)) ||
            default_font.handle != FONT_HANDLE_INVALID ||
            !rohr_graphics_font_valid_check(default_alias) ||
            !stats_check(1, 0, 1, "built-in retain and release"))
        return fail("built-in font contract failed", true, true);

    if(!result_ok(rohr_graphics_font_release(&zero)) ||
            zero.handle != FONT_HANDLE_INVALID ||
            !rohr_error_check(rohr_graphics_font_release(NULL)) ||
            !stats_check(1, 0, 1, "zero and null release"))
        return fail("zero or null font release contract failed", true, true);

    if(!rohr_error_check(rohr_graphics_font_load(
                (FontDescriptor){.file = NULL, .point_size = 12.0f})) ||
            !rohr_error_check(rohr_graphics_font_load(
                (FontDescriptor){.file = ROHR_TEST_FONT_PATH,
                    .point_size = 0.0f})) ||
            !rohr_error_check(rohr_graphics_font_load(
                (FontDescriptor){.file = ROHR_TEST_FONT_PATH,
                    .point_size = NAN})) ||
            !rohr_error_check(rohr_graphics_font_load(
                (FontDescriptor){.file = "missing_font_asset.ttf",
                    .point_size = 12.0f})) ||
            !stats_check(1, 0, 1, "load failures"))
        return fail("font load failure changed registry ownership", true, true);

    first = graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_PATH, .point_size = 12.0f});
    shared = rohr_graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_ALIAS_PATH, .point_size = 12.0f});
    different_size = rohr_graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_PATH, .point_size = 24.0f});
    if(rohr_error_check(first) || rohr_error_check(shared) ||
            rohr_error_check(different_size) ||
            first.result.value.handle != shared.result.value.handle ||
            first.result.value.handle == different_size.result.value.handle ||
            !stats_check(3, 3, 1, "path and size identity"))
        return fail("font cache identity contract failed", true, true);

    retained = first.result.value;
    stale = first.result.value;
    if(!result_ok(graphics_font_retain(retained)) ||
            !stats_check(3, 4, 1, "custom retain"))
        return fail("custom font retain failed", true, true);
    failed_text = rohr_graphics_text_create(&first.result.value, NULL,
        (Color){255, 255, 255, 255});
    if(!rohr_error_check(failed_text) ||
            !stats_check(3, 4, 1, "text creation failure"))
        return fail("failed text creation changed font ownership", true, true);
    text = rohr_graphics_text_create(&first.result.value, "font owner",
        (Color){255, 255, 255, 255});
    if(rohr_error_check(text) || text.result.value.size.x <= 0.0f ||
            text.result.value.size.y <= 0.0f ||
            !stats_check(3, 5, 1, "text dependency"))
        return fail("text did not retain its custom font", true, true);
    if(!result_ok(rohr_graphics_font_release(&first.result.value)) ||
            !result_ok(rohr_graphics_font_release(&shared.result.value)) ||
            !result_ok(rohr_graphics_font_release(&retained)) ||
            !result_ok(rohr_graphics_font_release(
                &different_size.result.value)) ||
            !graphics_font_valid_check(stale) ||
            !stats_check(2, 1, 1, "caller releases with live text"))
        return fail("custom font reference release failed", true, true);
    if(!rohr_graphics_text_value_set(&text.result.value, "updated"))
        return fail("text could not use its retained font", true, true);
    rohr_graphics_text_destroy(&text.result.value);
    if(graphics_font_valid_check(stale) ||
            !stats_check(1, 0, 1, "text dependency release"))
        return fail("text destruction did not release its font", true, true);
    stale_release = stale;
    if(!rohr_error_check(graphics_font_retain(stale)) ||
            !rohr_error_check(rohr_graphics_font_release(&stale_release)) ||
            stale_release.handle != stale.handle ||
            !stats_check(1, 0, 1, "stale rejection"))
        return fail("stale font handle contract failed", true, true);
    expired_handle = stale.handle;

    text = rohr_graphics_text_create(&default_alias, "built in",
        (Color){255, 255, 255, 255});
    if(rohr_error_check(text) || !stats_check(1, 0, 1,
            "built-in text dependency"))
        return fail("built-in text changed owner counts", true, true);
    rohr_graphics_text_destroy(&text.result.value);
    if(!stats_check(1, 0, 1, "built-in text release"))
        return fail("built-in text cleanup changed its pin", true, true);

    first = rohr_graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_PATH, .point_size = 12.0f});
    if(rohr_error_check(first) || first.result.value.handle == expired_handle ||
            !stats_check(2, 1, 1, "generation reload"))
        return fail("reloaded font reused a stale handle", true, true);
    shutdown_stale = first.result.value;
    old_default = default_alias;
    rohr_graphics_stop();
    graphics_started = false;
    if(rohr_graphics_font_valid_check(shutdown_stale) ||
            rohr_graphics_font_valid_check(old_default) ||
            !stats_check(0, 0, 0, "graphics shutdown"))
        return fail("graphics shutdown did not clear fonts", false, true);

    if(!result_ok(rohr_graphics_start()))
        return fail("graphics restart failed", false, true);
    graphics_started = true;
    default_font = rohr_graphics_font_default_get();
    first = rohr_graphics_font_load((FontDescriptor){
        .file = ROHR_TEST_FONT_PATH, .point_size = 12.0f});
    if(default_font.handle == old_default.handle || rohr_error_check(first) ||
            first.result.value.handle == shutdown_stale.handle ||
            rohr_graphics_font_valid_check(old_default) ||
            rohr_graphics_font_valid_check(shutdown_stale) ||
            !stats_check(2, 1, 1, "graphics restart"))
        return fail("graphics restart did not advance font generations",
            true, true);
    if(!result_ok(rohr_graphics_font_release(&first.result.value)) ||
            !result_ok(rohr_graphics_font_release(&default_font)) ||
            !stats_check(1, 0, 1, "final releases"))
        return fail("final font releases failed", true, true);

    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
