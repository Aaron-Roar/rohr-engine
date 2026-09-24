/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "graphics/texture_assets.h"
#include "test_png.h"

#include <stdio.h>

static const char *texture_path = "texture_assets_fixture.png";
static const char *second_texture_path = "texture_assets_fixture_second.png";

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "error %d: %s\n", (int)result.result.error,
        rohr_error_message_get(result));
    return false;
}

static bool stats_check(size_t live_resources, size_t owner_references,
        size_t command_references, const char *context) {
    GraphicsTextureAssetStats stats = graphics_texture_assets_stats_get();
    if(stats.live_resources == live_resources &&
            stats.owner_references == owner_references &&
            stats.command_references == command_references) return true;
    fprintf(stderr,
        "%s: texture stats were live=%zu owners=%zu commands=%zu; "
        "expected %zu/%zu/%zu\n",
        context, stats.live_resources, stats.owner_references,
        stats.command_references, live_resources, owner_references,
        command_references);
    return false;
}

static int fail(const char *message, bool graphics_started,
        bool engine_started) {
    fprintf(stderr, "%s\n", message);
    if(graphics_started) rohr_graphics_stop();
    if(engine_started) rohr_engine_stop();
    (void)SDL_RemovePath(texture_path);
    (void)SDL_RemovePath(second_texture_path);
    return 1;
}

int main(void) {
    TextureAssetResult first;
    TextureAssetResult second;
    TextureAsset retained;
    TextureAsset stale;
    TextureAsset stale_release;
    TextureAsset replacement;
    TextureAsset zero = {.size = {3.0f, 4.0f}};
    TextureHandle expired_handle;
    EntityResult entity;
    bool engine_started = false;
    bool graphics_started = false;

    if(!SDL_SaveFile(texture_path, test_png, sizeof(test_png)) ||
            !SDL_SaveFile(second_texture_path, test_png,
                sizeof(test_png)))
        return fail("could not write texture fixtures", false, false);
    if(!result_ok(rohr_engine_start()))
        return fail("engine start failed", false, false);
    engine_started = true;
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics start failed", false, true);
    graphics_started = true;
    if(!stats_check(0, 0, 0, "initial state"))
        return fail("texture registry did not start empty", true, true);

    if(!result_ok(rohr_graphics_texture_release(&zero)) ||
            zero.handle != TEXTURE_HANDLE_INVALID || zero.size.x != 0.0f ||
            zero.size.y != 0.0f ||
            !rohr_error_check(rohr_graphics_texture_release(NULL)) ||
            !stats_check(0, 0, 0, "zero and null release"))
        return fail("zero or null texture release contract failed", true, true);

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {10.0f, 20.0f}});
    second = rohr_graphics_texture_load(
        (TextureDescriptor){"./texture_assets_fixture.png", {30.0f, 40.0f}});
    if(rohr_error_check(first) || rohr_error_check(second) ||
            first.result.value.handle != second.result.value.handle ||
            first.result.value.size.x != 10.0f ||
            first.result.value.size.y != 20.0f ||
            second.result.value.size.x != 30.0f ||
            second.result.value.size.y != 40.0f ||
            !graphics_texture_valid_check(first.result.value) ||
            !stats_check(1, 2, 0, "shared load"))
        return fail("resolved-path sharing or logical sizes failed", true, true);

    retained = first.result.value;
    if(!result_ok(graphics_texture_retain(retained)) ||
            !stats_check(1, 3, 0, "direct retain"))
        return fail("texture retain failed", true, true);
    stale = first.result.value;
    entity = rohr_entity_add();
    if(rohr_error_check(entity) ||
            !result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(second.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !stats_check(1, 4, 0, "sprite addition"))
        return fail("sprite creation failed", true, true);
    if(!result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !result_ok(rohr_graphics_texture_release(&second.result.value)) ||
            !result_ok(graphics_texture_release(&retained)) ||
            !rohr_graphics_texture_valid_check(stale) ||
            !stats_check(1, 1, 0, "caller releases"))
        return fail("shared reference release failed", true, true);

    rohr_graphics_screen_texture_draw(stale, (Position){1.0f, 1.0f},
        (Scale){1.0f, 1.0f}, 0.0f);
    if(!stats_check(1, 1, 1, "queued draw"))
        return fail("queued draw did not retain texture command", true, true);
    if(!result_ok(rohr_entity_delete(entity.result.value)) ||
            rohr_graphics_texture_valid_check(stale) ||
            !stats_check(1, 0, 1, "entity deletion with queued draw"))
        return fail("entity deletion did not release its texture", true, true);
    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {2.0f, 2.0f}});
    if(rohr_error_check(first) || first.result.value.handle == stale.handle ||
            !stats_check(2, 1, 1, "reload while queued") ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !stats_check(1, 0, 1, "reload release while queued"))
        return fail("queued texture reference was incorrectly revived", true, true);
    rohr_graphics_show();
    if(!stats_check(0, 0, 0, "queued draw completion") ||
            !rohr_error_check(rohr_graphics_texture_retain(stale)))
        return fail("queued texture cleanup or stale retain failed", true, true);
    stale_release = stale;
    if(!rohr_error_check(rohr_graphics_texture_release(&stale_release)) ||
            stale_release.handle != stale.handle ||
            !stats_check(0, 0, 0, "stale release"))
        return fail("stale texture release changed ownership", true, true);
    expired_handle = stale.handle;

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {5.0f, 5.0f}});
    second = rohr_graphics_texture_load(
        (TextureDescriptor){second_texture_path, {6.0f, 6.0f}});
    entity = rohr_entity_add();
    if(rohr_error_check(first) || rohr_error_check(second) ||
            rohr_error_check(entity) ||
            first.result.value.handle == expired_handle ||
            !stats_check(2, 2, 0, "replacement setup"))
        return fail("texture generation or replacement setup failed", true, true);
    stale = first.result.value;
    replacement = second.result.value;
    if(!result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !stats_check(2, 2, 0, "first component ownership"))
        return fail("first component ownership failed", true, true);
    if(!result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(second.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            rohr_graphics_texture_valid_check(stale) ||
            !stats_check(1, 2, 0, "component replacement") ||
            !result_ok(rohr_graphics_texture_release(&second.result.value)) ||
            !stats_check(1, 1, 0, "replacement caller release"))
        return fail("sprite replacement ownership failed", true, true);
    stale = replacement;
    if(!result_ok(rohr_entity_components_delete(
                entity.result.value, ROHR_SPRITE)) ||
            rohr_graphics_texture_valid_check(stale) ||
            !stats_check(0, 0, 0, "component removal") ||
            !result_ok(rohr_entity_delete(entity.result.value)))
        return fail("replacement texture survived component removal", true, true);

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {1.0f, 1.0f}});
    if(rohr_error_check(first) ||
            !stats_check(1, 1, 0, "failed component setup") ||
            !rohr_error_check(rohr_graphics_sprite_add(ENTITY_INVALID,
                rohr_graphics_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !stats_check(1, 1, 0, "failed component addition") ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !stats_check(0, 0, 0, "failed component cleanup"))
        return fail("failed sprite addition changed ownership", true, true);

    first = rohr_graphics_texture_load((TextureDescriptor){
        "texture_assets_missing.png", {1.0f, 1.0f}});
    if(!rohr_error_check(first) ||
            !rohr_error_check(rohr_graphics_texture_load(
                (TextureDescriptor){NULL, {1.0f, 1.0f}})) ||
            !stats_check(0, 0, 0, "failed loads"))
        return fail("failed texture load changed ownership", true, true);

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {1.0f, 1.0f}});
    if(rohr_error_check(first) ||
            !stats_check(1, 1, 0, "shutdown load"))
        return fail("shutdown texture load failed", true, true);
    stale = first.result.value;
    rohr_graphics_stop();
    graphics_started = false;
    stale_release = stale;
    if(!stats_check(0, 0, 0, "graphics shutdown") ||
            rohr_graphics_texture_valid_check(stale) ||
            !rohr_error_check(rohr_graphics_texture_retain(stale)) ||
            !rohr_error_check(rohr_graphics_texture_release(&stale_release)) ||
            stale_release.handle != stale.handle)
        return fail("graphics shutdown left a texture handle alive", false, true);

    if(!result_ok(rohr_graphics_start()))
        return fail("graphics restart failed", false, true);
    graphics_started = true;
    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {1.0f, 1.0f}});
    if(rohr_error_check(first) || first.result.value.handle == stale.handle ||
            !stats_check(1, 1, 0, "graphics restart") ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !stats_check(0, 0, 0, "restart release"))
        return fail("graphics restart reused a stale texture handle", true, true);
    rohr_graphics_stop();
    graphics_started = false;
    if(!stats_check(0, 0, 0, "final graphics shutdown"))
        return fail("final texture shutdown leaked resources", false, true);

    rohr_engine_stop();
    engine_started = false;
    (void)SDL_RemovePath(texture_path);
    (void)SDL_RemovePath(second_texture_path);
    return 0;
}
