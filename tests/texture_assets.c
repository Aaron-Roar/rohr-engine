/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <stdio.h>

static const unsigned char texture_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x04, 0x00, 0x00, 0x00, 0xb5, 0x1c, 0x0c,
    0x02, 0x00, 0x00, 0x00, 0x0b, 0x49, 0x44, 0x41,
    0x54, 0x78, 0xda, 0x63, 0x64, 0xf8, 0x0f, 0x00,
    0x01, 0x05, 0x01, 0x01, 0x27, 0x18, 0xe3, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
    0xae, 0x42, 0x60, 0x82,
};

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

    if(!SDL_SaveFile(texture_path, texture_png, sizeof(texture_png)) ||
            !SDL_SaveFile(second_texture_path, texture_png,
                sizeof(texture_png)))
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

    {
        AnimationDescriptor descriptor = {
            .texture_descriptors = {{texture_path, {7.0f, 8.0f}}},
            .amount_of_descriptors = 1,
            .ticks_per_frame = 1,
        };
        AnimationAssetResult animation =
            rohr_graphics_animation_load(descriptor);
        TextureAsset animation_texture;
        if(rohr_error_check(animation))
            return fail("compatibility animation load failed", true, true);
        animation_texture = animation.result.value.texture_list.textures[0];
        entity = rohr_entity_add();
        if(rohr_error_check(entity) ||
                !result_ok(rohr_graphics_animated_sprite_add(
                    entity.result.value,
                    rohr_graphics_animated_sprite_create(
                        animation.result.value, (Scale){1.0f, 1.0f}))))
            return fail("animation component retain failed", true, true);
        rohr_graphics_animation_destroy(&animation.result.value);
        if(!rohr_graphics_texture_valid_check(animation_texture) ||
                !result_ok(rohr_entity_delete(entity.result.value)) ||
                rohr_graphics_texture_valid_check(animation_texture))
            return fail("animation frame ownership failed", true, true);
    }

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
