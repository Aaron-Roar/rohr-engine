/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
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
    TextureAsset replacement;
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

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {10.0f, 20.0f}});
    second = rohr_graphics_texture_load(
        (TextureDescriptor){"./texture_assets_fixture.png", {30.0f, 40.0f}});
    if(rohr_error_check(first) || rohr_error_check(second) ||
            first.result.value.handle != second.result.value.handle ||
            first.result.value.size.x != 10.0f ||
            first.result.value.size.y != 20.0f ||
            second.result.value.size.x != 30.0f ||
            second.result.value.size.y != 40.0f)
        return fail("resolved-path sharing or logical sizes failed", true, true);

    retained = first.result.value;
    if(!result_ok(rohr_graphics_texture_retain(retained)))
        return fail("texture retain failed", true, true);
    stale = first.result.value;
    entity = rohr_entity_add();
    if(rohr_error_check(entity) ||
            !result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(second.result.value,
                    (Scale){1.0f, 1.0f}))))
        return fail("sprite creation failed", true, true);
    if(!result_ok(rohr_graphics_texture_release(&first.result.value)) ||
            !result_ok(rohr_graphics_texture_release(&second.result.value)) ||
            !result_ok(rohr_graphics_texture_release(&retained)) ||
            !rohr_graphics_texture_valid_check(stale))
        return fail("shared reference release failed", true, true);

    rohr_graphics_screen_texture_draw(stale, (Position){1.0f, 1.0f},
        (Scale){1.0f, 1.0f}, 0.0f);
    if(!result_ok(rohr_entity_delete(entity.result.value)) ||
            rohr_graphics_texture_valid_check(stale))
        return fail("entity deletion did not release its texture", true, true);
    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {2.0f, 2.0f}});
    if(rohr_error_check(first) || first.result.value.handle == stale.handle ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)))
        return fail("queued texture reference was incorrectly revived", true, true);
    rohr_graphics_show();
    if(!rohr_error_check(rohr_graphics_texture_retain(stale)))
        return fail("stale texture handle was accepted", true, true);
    expired_handle = stale.handle;

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {5.0f, 5.0f}});
    second = rohr_graphics_texture_load(
        (TextureDescriptor){second_texture_path, {6.0f, 6.0f}});
    entity = rohr_entity_add();
    if(rohr_error_check(first) || rohr_error_check(second) ||
            rohr_error_check(entity) ||
            first.result.value.handle == expired_handle)
        return fail("texture generation did not advance", true, true);
    stale = first.result.value;
    replacement = second.result.value;
    if(
            !result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !result_ok(rohr_graphics_texture_release(&first.result.value)))
        return fail("texture generation or first component add failed", true, true);
    if(!result_ok(rohr_graphics_sprite_add(entity.result.value,
                rohr_graphics_sprite_create(second.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            rohr_graphics_texture_valid_check(stale) ||
            !result_ok(rohr_graphics_texture_release(&second.result.value)))
        return fail("sprite replacement ownership failed", true, true);
    stale = replacement;
    if(!result_ok(rohr_entity_components_delete(
                entity.result.value, ROHR_SPRITE)) ||
            rohr_graphics_texture_valid_check(stale))
        return fail("replacement texture survived component removal", true, true);
    if(!result_ok(rohr_entity_delete(entity.result.value)))
        return fail("entity deletion after component removal failed", true, true);

    first = rohr_graphics_texture_load((TextureDescriptor){
        "texture_assets_missing.png", {1.0f, 1.0f}});
    if(!rohr_error_check(first))
        return fail("missing texture unexpectedly loaded", true, true);

    first = rohr_graphics_texture_load(
        (TextureDescriptor){texture_path, {1.0f, 1.0f}});
    if(rohr_error_check(first))
        return fail("shutdown texture load failed", true, true);
    stale = first.result.value;
    rohr_graphics_stop();
    graphics_started = false;
    if(rohr_graphics_texture_valid_check(stale) ||
            !rohr_error_check(rohr_graphics_texture_retain(stale)))
        return fail("graphics shutdown left a texture handle alive", false, true);

    rohr_engine_stop();
    engine_started = false;
    (void)SDL_RemovePath(texture_path);
    (void)SDL_RemovePath(second_texture_path);
    return 0;
}
