/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "test_png.h"

#include <stdio.h>

static const char *first_path = "animation_assets_first.png";
static const char *second_path = "animation_assets_second.png";

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
    (void)SDL_RemovePath(first_path);
    (void)SDL_RemovePath(second_path);
    return 1;
}

static AnimationDescriptor descriptor_get(void) {
    return (AnimationDescriptor){
        .id = 41,
        .texture_descriptors = {
            {"animation_assets_first.png", {10.0f, 20.0f}},
            {"animation_assets_second.png", {30.0f, 40.0f}},
        },
        .frame_ids = {101, 102},
        .amount_of_descriptors = 2,
        .ticks_per_frame = 2,
        .time_per_frame = 0.5,
    };
}

int main(void) {
    AnimationDescriptor descriptor = descriptor_get();
    AnimationDescriptor normalized = descriptor_get();
    AnimationAssetResult first;
    AnimationAssetResult second;
    AnimationAsset retained;
    AnimationAsset stale;
    AnimationInfoResult info;
    AnimationFrameResult frame;
    TextureAsset borrowed_texture;
    AnimationPlayer player_a;
    AnimationPlayer player_b;
    EntityResult entity_a;
    EntityResult entity_b;
    bool engine_started = false;
    bool graphics_started = false;

    normalized.texture_descriptors[0].file = "./animation_assets_first.png";
    normalized.texture_descriptors[1].file = "./animation_assets_second.png";
    if(!SDL_SaveFile(first_path, test_png, sizeof(test_png)) ||
            !SDL_SaveFile(second_path, test_png, sizeof(test_png)))
        return fail("could not write animation fixtures", false, false);
    if(!result_ok(rohr_engine_start()))
        return fail("engine start failed", false, false);
    engine_started = true;
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics start failed", false, true);
    graphics_started = true;

    first = rohr_graphics_animation_load(descriptor);
    second = rohr_graphics_animation_load(normalized);
    if(rohr_error_check(first) || rohr_error_check(second) ||
            first.result.value.handle != second.result.value.handle)
        return fail("identical animations were not shared", true, true);
    info = rohr_graphics_animation_info_get(first.result.value);
    frame = rohr_graphics_animation_frame_get(first.result.value, 1);
    if(rohr_error_check(info) || rohr_error_check(frame) ||
            info.result.value.id != 41 || info.result.value.frame_count != 2 ||
            info.result.value.ticks_per_frame != 2 ||
            info.result.value.time_per_frame != 0.5 ||
            frame.result.value.id != 102 ||
            frame.result.value.texture.size.x != 30.0f ||
            frame.result.value.texture.size.y != 40.0f)
        return fail("immutable animation metadata was incorrect", true, true);
    borrowed_texture = frame.result.value.texture;

    {
        AnimationDescriptor different = normalized;
        AnimationAssetResult result;
        AnimationFrameResult different_frame;
        different.texture_descriptors[1].size = (Scale){60.0f, 80.0f};
        result = rohr_graphics_animation_load(different);
        different_frame = rohr_error_check(result) ?
            (AnimationFrameResult){0} :
            rohr_graphics_animation_frame_get(result.result.value, 1);
        if(rohr_error_check(result) || rohr_error_check(different_frame) ||
                result.result.value.handle == first.result.value.handle ||
                different_frame.result.value.texture.handle !=
                    frame.result.value.texture.handle ||
                different_frame.result.value.texture.size.x != 60.0f ||
                !result_ok(rohr_graphics_animation_release(
                    &result.result.value)))
            return fail("logical frame size did not affect animation identity",
                true, true);
    }

    player_a = rohr_graphics_animation_player_create(first.result.value);
    player_b = rohr_graphics_animation_player_create(first.result.value);
    rohr_graphics_animation_player_update(&player_a, 2, 0.0);
    rohr_graphics_animation_player_update(&player_b, 1, 0.0);
    if(player_a.frame_index != 1 || player_b.frame_index != 0)
        return fail("shared animation players did not advance independently",
            true, true);
    player_b.ticks_per_frame = 0;
    player_b.time_per_frame = 0.25;
    rohr_graphics_animation_player_update(&player_b, 1, 0.25);
    info = rohr_graphics_animation_info_get(first.result.value);
    if(player_b.frame_index != 1 || rohr_error_check(info) ||
            info.result.value.time_per_frame != 0.5)
        return fail("player timing changed immutable animation data", true, true);

    retained = first.result.value;
    if(!result_ok(rohr_graphics_animation_retain(retained)))
        return fail("animation retain failed", true, true);
    stale = first.result.value;
    entity_a = rohr_entity_add();
    entity_b = rohr_entity_add();
    if(rohr_error_check(entity_a) || rohr_error_check(entity_b) ||
            !result_ok(rohr_graphics_animated_sprite_add(entity_a.result.value,
                rohr_graphics_animated_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !result_ok(rohr_graphics_animated_sprite_add(entity_b.result.value,
                rohr_graphics_animated_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))))
        return fail("animation component retain failed", true, true);
    if(!result_ok(rohr_graphics_animated_sprite_body_offset_set(
                entity_a.result.value, (Position){-2.0f, 6.0f})) ||
            !result_ok(rohr_graphics_animated_sprite_orientation_offset_set(
                entity_a.result.value, -0.25f)))
        return fail("animated-sprite transform set failed", true, true);
    {
        PositionResult offset = rohr_graphics_animated_sprite_body_offset_get(
            entity_a.result.value);
        SpriteOrientationResult orientation =
            rohr_graphics_animated_sprite_orientation_offset_get(
                entity_a.result.value);
        if(rohr_error_check(offset) || rohr_error_check(orientation) ||
                offset.result.value.x != -2.0f ||
                offset.result.value.y != 6.0f ||
                orientation.result.value != -0.25f)
            return fail("animated-sprite transform get failed", true, true);
    }
    if(!result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            !result_ok(rohr_graphics_animation_release(&second.result.value)) ||
            !result_ok(rohr_graphics_animation_release(&retained)) ||
            !result_ok(rohr_entity_delete(entity_a.result.value)) ||
            !rohr_graphics_animation_valid_check(stale) ||
            !result_ok(rohr_entity_components_delete(entity_b.result.value,
                ROHR_ANIMATED_SPRITE)) ||
            rohr_graphics_animation_valid_check(stale) ||
            rohr_graphics_texture_valid_check(borrowed_texture))
        return fail("component or entity animation lifetime failed", true, true);
    if(!rohr_error_check(rohr_graphics_animation_retain(stale)))
        return fail("stale animation handle was accepted", true, true);
    if(!result_ok(rohr_entity_delete(entity_b.result.value)))
        return fail("entity deletion after component removal failed", true, true);

    {
        AnimationAssetResult original = rohr_graphics_animation_load(descriptor);
        AnimationDescriptor replacement_descriptor = descriptor;
        AnimationAssetResult replacement;
        AnimationAsset old_stale;
        AnimationAsset replacement_stale;
        EntityResult entity = rohr_entity_add();
        replacement_descriptor.time_per_frame = 0.75;
        replacement = rohr_graphics_animation_load(replacement_descriptor);
        if(rohr_error_check(original) || rohr_error_check(replacement) ||
                rohr_error_check(entity))
            return fail("replacement setup failed", true, true);
        old_stale = original.result.value;
        replacement_stale = replacement.result.value;
        if(!result_ok(rohr_graphics_animated_sprite_add(entity.result.value,
                    rohr_graphics_animated_sprite_create(original.result.value,
                        (Scale){1.0f, 1.0f}))) ||
                !result_ok(rohr_graphics_animation_release(
                    &original.result.value)) ||
                !result_ok(rohr_graphics_animated_sprite_add(entity.result.value,
                    rohr_graphics_animated_sprite_create(replacement.result.value,
                        (Scale){1.0f, 1.0f}))) ||
                rohr_graphics_animation_valid_check(old_stale) ||
                !result_ok(rohr_graphics_animation_release(
                    &replacement.result.value)) ||
                !result_ok(rohr_entity_delete(entity.result.value)) ||
                rohr_graphics_animation_valid_check(replacement_stale))
            return fail("animated-sprite replacement ownership failed",
                true, true);
    }

    {
        TextureAssetResult probe = rohr_graphics_texture_load(
            (TextureDescriptor){first_path, {10.0f, 20.0f}});
        TextureAsset probe_stale;
        AnimationDescriptor failing = descriptor;
        AnimationAssetResult failed;
        if(rohr_error_check(probe))
            return fail("partial-failure probe load failed", true, true);
        probe_stale = probe.result.value;
        failing.texture_descriptors[1].file = "animation_assets_missing.png";
        failed = rohr_graphics_animation_load(failing);
        if(!rohr_error_check(failed) ||
                !result_ok(rohr_graphics_texture_release(&probe.result.value)) ||
                rohr_graphics_texture_valid_check(probe_stale))
            return fail("partial animation load retained frame textures",
                true, true);
    }

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) || first.result.value.handle == stale.handle)
        return fail("animation generation did not advance", true, true);
    frame = rohr_graphics_animation_frame_get(first.result.value, 0);
    if(rohr_error_check(frame))
        return fail("queued-frame lookup failed", true, true);
    borrowed_texture = frame.result.value.texture;
    rohr_graphics_screen_texture_draw(borrowed_texture,
        (Position){1.0f, 1.0f}, (Scale){1.0f, 1.0f}, 0.0f);
    if(!result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            rohr_graphics_texture_valid_check(borrowed_texture))
        return fail("queued frame kept a public owner reference", true, true);
    rohr_graphics_show();

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first))
        return fail("shutdown animation load failed", true, true);
    stale = first.result.value;
    frame = rohr_graphics_animation_frame_get(first.result.value, 0);
    if(rohr_error_check(frame))
        return fail("shutdown frame lookup failed", true, true);
    borrowed_texture = frame.result.value.texture;
    rohr_graphics_stop();
    graphics_started = false;
    if(rohr_graphics_animation_valid_check(stale) ||
            rohr_graphics_texture_valid_check(borrowed_texture) ||
            !rohr_error_check(rohr_graphics_animation_retain(stale)))
        return fail("graphics shutdown left an animation handle alive",
            false, true);

    rohr_engine_stop();
    engine_started = false;
    (void)SDL_RemovePath(first_path);
    (void)SDL_RemovePath(second_path);
    return 0;
}
