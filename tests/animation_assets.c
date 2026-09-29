/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "graphics/animation_assets.h"
#include "graphics/texture_assets.h"
#include "test_png.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

static const char *first_path = "animation_assets_first.png";
static const char *second_path = "animation_assets_second.png";

static bool result_ok(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "error %d: %s\n", (int)result.result.error,
        rohr_error_message_get(result));
    return false;
}

static bool stats_check(size_t animation_live, size_t animation_owners,
        size_t frame_texture_references, size_t texture_live,
        size_t texture_owners, size_t texture_commands,
        const char *context) {
    GraphicsAnimationAssetStats animation =
        graphics_animation_assets_stats_get();
    GraphicsTextureAssetStats texture = graphics_texture_assets_stats_get();
    if(animation.live_resources == animation_live &&
            animation.owner_references == animation_owners &&
            animation.frame_texture_references == frame_texture_references &&
            texture.live_resources == texture_live &&
            texture.owner_references == texture_owners &&
            texture.command_references == texture_commands) return true;
    fprintf(stderr,
        "%s: animation stats were live=%zu owners=%zu frames=%zu; "
        "texture stats were live=%zu owners=%zu commands=%zu; "
        "expected %zu/%zu/%zu and %zu/%zu/%zu\n",
        context, animation.live_resources, animation.owner_references,
        animation.frame_texture_references, texture.live_resources,
        texture.owner_references, texture.command_references,
        animation_live, animation_owners, frame_texture_references,
        texture_live, texture_owners, texture_commands);
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
        .frame_files = {"animation_assets_first.png", "animation_assets_second.png"},
        .frame_ids = {101, 102},
        .amount_of_descriptors = 2,
        .ticks_per_frame = 2,
        .time_per_frame = 0.5,
    };
}

static bool frame_values_check(AnimationAsset catalog) {
    AnimationFrame reusable = rohr_graphics_animation_frame_get(catalog, 0).result.value;
    reusable.scale = (Scale){-2, 3};
    reusable.offset = (Position){13, -9};
    reusable.rotation = 450;
    AnimatedSprite value = rohr_graphics_animated_sprite_create((AnimationAsset){0}, (Scale){1,1});
    if(!result_ok(rohr_graphics_animated_sprite_value_frame_add(&value, reusable))) return false;
    reusable.scale.x = 99;
    reusable.offset.x = 99;
    if(value.frames[0].scale.x != -2 || value.frames[0].offset.x != 13) return false;
    Entity a = rohr_entity_add().result.value, b = rohr_entity_add().result.value;
    value.player.last_update_tick = 17;
    value.player.last_update_time = 1.5;
    value.player.ticks_per_frame = 3;
    value.player.time_per_frame = 0.25;
    if(!result_ok(rohr_graphics_animated_sprite_add(a, value)) ||
            !result_ok(rohr_graphics_animated_sprite_add(b, value))) return false;
    value.frames[0].rotation = 99;
    if(!result_ok(rohr_graphics_animated_sprite_frame_scale_set(a, 0, (Scale){0,-4})) ||
            !result_ok(rohr_graphics_animated_sprite_frame_offset_set(a, 0, (Position){4,5})) ||
            !result_ok(rohr_graphics_animated_sprite_frame_rotation_radians_set(a, 0, PI_F * 3))) return false;
    AnimationFrame first = rohr_graphics_animated_sprite_frame_get(a, 0).result.value;
    AnimationFrame second = rohr_graphics_animated_sprite_frame_get(b, 0).result.value;
    if(first.scale.x != 0 || first.scale.y != -4 || first.offset.x != 4 ||
            fabsf(first.rotation - 540) > 0.001f || second.rotation != 450 ||
            second.offset.x != 13 || second.scale.x != -2 ||
            rohr_graphics_animation_frame_get(catalog, 0).result.value.rotation != 0) return false;
    if(fabsf(rohr_graphics_animated_sprite_frame_rotation_radians_get(a, 0).result.value -
            PI_F * 3) > 0.001f ||
            rohr_graphics_animated_sprite_frame_scale_get(a, 0).result.value.y != -4 ||
            rohr_graphics_animated_sprite_frame_offset_get(a, 0).result.value.y != 5) return false;
    EntityIndex index = rohr_entity_index_get(a).result.value;
    AnimationPlayer before = animated_sprites[index].player;
    AnimationFrame append = second;
    append.id = 0;
    if(!result_ok(rohr_graphics_animated_sprite_frame_add(a, append)) ||
            animated_sprites[index].frame_count != 2 ||
            animated_sprites[index].player.frame_count != 2) return false;
    if(!result_ok(rohr_graphics_animated_sprite_frame_index_set(a, 1))) return false;
    before.frame_index = 1;
    before.frame_count = 2;
    if(!result_ok(rohr_graphics_animated_sprite_frame_rotation_set(a, 0, -810))) return false;
    AnimationPlayer after = animated_sprites[index].player;
    if(after.frame_index != before.frame_index || after.last_update_tick != before.last_update_tick ||
            after.last_update_time != before.last_update_time || after.ticks_per_frame != before.ticks_per_frame ||
            after.time_per_frame != before.time_per_frame) return false;
    AnimationFrame preserved = rohr_graphics_animated_sprite_frame_get(a, 0).result.value;
    for(int failure = 0; failure < 6; failure += 1) {
        AnimationFrame invalid = preserved;
        if(failure == 0) invalid.scale.x = NAN;
        if(failure == 1) invalid.offset.y = INFINITY;
        if(failure == 2) invalid.rotation = INFINITY;
        if(failure == 3) invalid.texture = (TextureAsset){0};
        if(failure == 4) invalid.id = animated_sprites[index].frames[1].id;
        if(!rohr_error_check(rohr_graphics_animated_sprite_frame_set(a,
                failure == 5 ? MAX_ANIMATIONS_FRAMES : 0, invalid))) return false;
        AnimationFrame current = rohr_graphics_animated_sprite_frame_get(a, 0).result.value;
        if(current.scale.x != preserved.scale.x || current.rotation != preserved.rotation ||
                current.texture.handle != preserved.texture.handle) return false;
    }
    TextureAssetResult image = rohr_graphics_texture_load(
        (TextureDescriptor){second_path, {999,999}});
    if(rohr_error_check(image)) return false;
    AnimationFrame replacement = rohr_graphics_animation_frame_create(image.result.value);
    if(replacement.scale.x != 1 || replacement.texture.size.x != 1 ||
            !result_ok(rohr_graphics_animated_sprite_frame_set(a, 0, replacement)) ||
            !result_ok(rohr_graphics_texture_release(&image.result.value))) return false;
    if(rohr_graphics_animated_sprite_frame_get(b, 0).result.value.texture.handle !=
            second.texture.handle) return false;
    /* Replacing an entire component releases its independent image owners. */
    if(!result_ok(rohr_graphics_animated_sprite_add(a, value)) ||
            !result_ok(rohr_entity_delete(a)) || !result_ok(rohr_entity_delete(b))) return false;
    return stats_check(1, 2, 2, 2, 2, 0, "independent frame cleanup");
}

int main(void) {
    AnimationDescriptor descriptor = descriptor_get();
    AnimationDescriptor normalized = descriptor_get();
    AnimationAssetResult first;
    AnimationAssetResult second;
    AnimationAsset retained;
    AnimationAsset stale;
    AnimationAsset stale_release;
    AnimationAsset zero = {0};
    AnimationHandle expired_handle;
    AnimationInfoResult info;
    AnimationFrameResult frame;
    TextureAsset borrowed_texture;
    AnimationPlayer player_a;
    AnimationPlayer player_b;
    EntityResult entity_a;
    EntityResult entity_b;
    bool engine_started = false;
    bool graphics_started = false;

    normalized.frame_files[0] = "./animation_assets_first.png";
    normalized.frame_files[1] = "./animation_assets_second.png";
    if(!SDL_SaveFile(first_path, test_png, sizeof(test_png)) ||
            !SDL_SaveFile(second_path, test_png, sizeof(test_png)))
        return fail("could not write animation fixtures", false, false);
    if(!result_ok(rohr_engine_start()))
        return fail("engine start failed", false, false);
    engine_started = true;
    if(!result_ok(rohr_graphics_start()))
        return fail("graphics start failed", false, true);
    graphics_started = true;
    if(!stats_check(0, 0, 0, 0, 0, 0, "initial state"))
        return fail("asset registries did not start empty", true, true);

    if(!result_ok(rohr_graphics_animation_release(&zero)) ||
            zero.handle != ANIMATION_HANDLE_INVALID ||
            !rohr_error_check(rohr_graphics_animation_release(NULL)) ||
            !stats_check(0, 0, 0, 0, 0, 0, "zero and null release"))
        return fail("zero or null animation release contract failed", true, true);

    first = rohr_graphics_animation_load(descriptor);
    second = rohr_graphics_animation_load(normalized);
    if(rohr_error_check(first) || rohr_error_check(second) ||
            first.result.value.handle != second.result.value.handle ||
            !graphics_animation_valid_check(first.result.value) ||
            !stats_check(1, 2, 2, 2, 2, 0, "shared load"))
        return fail("identical animations were not shared", true, true);
    info = graphics_animation_info_get(first.result.value);
    frame = graphics_animation_frame_get(first.result.value, 1);
    if(rohr_error_check(info) || rohr_error_check(frame) ||
            info.result.value.id != 41 || info.result.value.frame_count != 2 ||
            info.result.value.ticks_per_frame != 2 ||
            info.result.value.time_per_frame != 0.5 ||
            frame.result.value.id != 102 ||
            frame.result.value.texture.size.x != 1.0f ||
            frame.result.value.texture.size.y != 1.0f ||
            !stats_check(1, 2, 2, 2, 2, 0, "metadata lookup"))
        return fail("immutable animation metadata was incorrect", true, true);
    borrowed_texture = frame.result.value.texture;

    if(!frame_values_check(first.result.value))
        return fail("independent frame values failed", true, true);

    player_a = rohr_graphics_animation_player_create(first.result.value);
    player_b = rohr_graphics_animation_player_create(first.result.value);
    rohr_graphics_animation_player_update(&player_a, 2, 0.0);
    rohr_graphics_animation_player_update(&player_b, 1, 0.0);
    if(player_a.frame_index != 1 || player_b.frame_index != 0 ||
            !stats_check(1, 2, 2, 2, 2, 0, "independent players"))
        return fail("shared animation players did not advance independently",
            true, true);
    player_b.ticks_per_frame = 0;
    player_b.time_per_frame = 0.25;
    rohr_graphics_animation_player_update(&player_b, 1, 0.25);
    info = rohr_graphics_animation_info_get(first.result.value);
    if(player_b.frame_index != 1 || rohr_error_check(info) ||
            info.result.value.time_per_frame != 0.5 ||
            !stats_check(1, 2, 2, 2, 2, 0, "player timing"))
        return fail("player timing changed immutable animation data", true, true);

    retained = first.result.value;
    if(!result_ok(graphics_animation_retain(retained)) ||
            !stats_check(1, 3, 2, 2, 2, 0, "direct retain"))
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
                    (Scale){1.0f, 1.0f}))) ||
            !stats_check(1, 5, 2, 2, 6, 0, "component addition"))
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
                orientation.result.value != -0.25f ||
                !stats_check(1, 5, 2, 2, 6, 0, "component transforms"))
            return fail("animated-sprite transform get failed", true, true);
    }
    if(!result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            !result_ok(rohr_graphics_animation_release(&second.result.value)) ||
            !result_ok(graphics_animation_release(&retained)) ||
            !stats_check(1, 2, 2, 2, 6, 0, "caller releases") ||
            !result_ok(rohr_entity_delete(entity_a.result.value)) ||
            !stats_check(1, 1, 2, 2, 4, 0, "entity deletion") ||
            !rohr_graphics_animation_valid_check(stale) ||
            !result_ok(rohr_entity_components_delete(entity_b.result.value,
                ROHR_ANIMATED_SPRITE)) ||
            !stats_check(0, 0, 0, 0, 0, 0, "component removal") ||
            rohr_graphics_animation_valid_check(stale) ||
            rohr_graphics_texture_valid_check(borrowed_texture))
        return fail("component or entity animation lifetime failed", true, true);
    if(!rohr_error_check(rohr_graphics_animation_retain(stale)))
        return fail("stale animation handle was accepted", true, true);
    stale_release = stale;
    if(!rohr_error_check(rohr_graphics_animation_release(&stale_release)) ||
            stale_release.handle != stale.handle ||
            !stats_check(0, 0, 0, 0, 0, 0, "stale release") ||
            !result_ok(rohr_entity_delete(entity_b.result.value)))
        return fail("stale release or entity cleanup failed", true, true);
    expired_handle = stale.handle;

    {
        AnimationAssetResult original = rohr_graphics_animation_load(descriptor);
        AnimationDescriptor replacement_descriptor = descriptor;
        AnimationAssetResult replacement;
        AnimationAsset original_stale;
        AnimationAsset replacement_stale;
        EntityResult entity = rohr_entity_add();
        replacement_descriptor.time_per_frame = 0.75;
        replacement = rohr_graphics_animation_load(replacement_descriptor);
        if(rohr_error_check(original) || rohr_error_check(replacement) ||
                rohr_error_check(entity) ||
                !stats_check(2, 2, 4, 2, 4, 0, "replacement setup"))
            return fail("replacement setup failed", true, true);
        original_stale = original.result.value;
        replacement_stale = replacement.result.value;
        if(!result_ok(rohr_graphics_animated_sprite_add(entity.result.value,
                    rohr_graphics_animated_sprite_create(original.result.value,
                        (Scale){1.0f, 1.0f}))) ||
                !result_ok(rohr_graphics_animation_release(
                    &original.result.value)) ||
                !stats_check(2, 2, 4, 2, 6, 0,
                    "first component ownership") ||
                !result_ok(rohr_graphics_animated_sprite_add(entity.result.value,
                    rohr_graphics_animated_sprite_create(replacement.result.value,
                        (Scale){1.0f, 1.0f}))) ||
                rohr_graphics_animation_valid_check(original_stale) ||
                !stats_check(1, 2, 2, 2, 4, 0, "component replacement") ||
                !result_ok(rohr_graphics_animation_release(
                    &replacement.result.value)) ||
                !stats_check(1, 1, 2, 2, 4, 0,
                    "replacement caller release") ||
                !result_ok(rohr_entity_delete(entity.result.value)) ||
                rohr_graphics_animation_valid_check(replacement_stale) ||
                !stats_check(0, 0, 0, 0, 0, 0, "replacement cleanup"))
            return fail("animated-sprite replacement ownership failed",
                true, true);
    }

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) ||
            !stats_check(1, 1, 2, 2, 2, 0, "failed component setup") ||
            !rohr_error_check(rohr_graphics_animated_sprite_add(ENTITY_INVALID,
                rohr_graphics_animated_sprite_create(first.result.value,
                    (Scale){1.0f, 1.0f}))) ||
            !stats_check(1, 1, 2, 2, 2, 0, "failed component addition") ||
            !result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, 0, "failed component cleanup"))
        return fail("failed animated-sprite addition changed ownership",
            true, true);

    {
        AnimationDescriptor invalid = descriptor;
        invalid.amount_of_descriptors = 0;
        if(!rohr_error_check(rohr_graphics_animation_load(invalid)))
            return fail("empty animation definition was accepted", true, true);
        invalid = descriptor;
        invalid.time_per_frame = -0.25;
        if(!rohr_error_check(rohr_graphics_animation_load(invalid)))
            return fail("negative frame time was accepted", true, true);
        invalid = descriptor;
        invalid.frame_ids[1] = invalid.frame_ids[0];
        if(!rohr_error_check(rohr_graphics_animation_load(invalid)) ||
                !stats_check(0, 0, 0, 0, 0, 0,
                    "invalid definition rollback"))
            return fail("invalid animation definition retained resources",
                true, true);
    }

    {
        TextureAssetResult probe = rohr_graphics_texture_load(
            (TextureDescriptor){first_path, {10.0f, 20.0f}});
        AnimationDescriptor failing = descriptor;
        AnimationAssetResult failed;
        if(rohr_error_check(probe) ||
                !stats_check(0, 0, 0, 1, 1, 0,
                    "partial-failure probe"))
            return fail("partial-failure probe load failed", true, true);
        failing.frame_files[1] = "animation_assets_missing.png";
        failed = rohr_graphics_animation_load(failing);
        if(!rohr_error_check(failed) ||
                !stats_check(0, 0, 0, 1, 1, 0,
                    "partial load rollback") ||
                !result_ok(rohr_graphics_texture_release(&probe.result.value)) ||
                !stats_check(0, 0, 0, 0, 0, 0,
                    "partial load cleanup"))
            return fail("partial animation load retained frame textures",
                true, true);
    }

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) ||
            !stats_check(1, 1, 2, 2, 2, 0, "borrowed frame setup"))
        return fail("borrowed-frame setup failed", true, true);
    frame = rohr_graphics_animation_frame_get(first.result.value, 0);
    if(rohr_error_check(frame) ||
            !stats_check(1, 1, 2, 2, 2, 0, "borrowed frame lookup"))
        return fail("borrowed frame lookup changed ownership", true, true);
    borrowed_texture = frame.result.value.texture;
    {
        TextureAsset owned_texture = borrowed_texture;
        if(!result_ok(rohr_graphics_texture_retain(owned_texture)) ||
                !stats_check(1, 1, 2, 2, 3, 0,
                    "borrowed frame retain") ||
                !result_ok(rohr_graphics_animation_release(
                    &first.result.value)) ||
                !rohr_graphics_texture_valid_check(owned_texture) ||
                !stats_check(0, 0, 0, 1, 1, 0,
                    "animation release after frame retain") ||
                !result_ok(rohr_graphics_texture_release(&owned_texture)) ||
                !stats_check(0, 0, 0, 0, 0, 0,
                    "owned frame release"))
            return fail("borrowed frame ownership contract failed", true, true);
    }

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) || first.result.value.handle == expired_handle)
        return fail("animation generation did not advance", true, true);
    frame = rohr_graphics_animation_frame_get(first.result.value, 0);
    if(rohr_error_check(frame))
        return fail("queued-frame lookup failed", true, true);
    borrowed_texture = frame.result.value.texture;
    rohr_graphics_screen_texture_draw(borrowed_texture,
        (Position){1.0f, 1.0f}, (Scale){1.0f, 1.0f}, 0.0f);
    if(!stats_check(1, 1, 2, 2, 2, 1, "queued frame") ||
            !result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            rohr_graphics_texture_valid_check(borrowed_texture) ||
            !stats_check(0, 0, 0, 1, 0, 1,
                "queued frame animation release"))
        return fail("queued frame kept a public owner reference", true, true);
    rohr_graphics_show();
    if(!stats_check(0, 0, 0, 0, 0, 0, "queued frame completion"))
        return fail("queued frame resource was not released", true, true);

    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) ||
            !stats_check(1, 1, 2, 2, 2, 0, "shutdown load"))
        return fail("shutdown animation load failed", true, true);
    stale = first.result.value;
    frame = rohr_graphics_animation_frame_get(first.result.value, 0);
    if(rohr_error_check(frame))
        return fail("shutdown frame lookup failed", true, true);
    borrowed_texture = frame.result.value.texture;
    rohr_graphics_stop();
    graphics_started = false;
    stale_release = stale;
    if(!stats_check(0, 0, 0, 0, 0, 0, "graphics shutdown") ||
            rohr_graphics_animation_valid_check(stale) ||
            rohr_graphics_texture_valid_check(borrowed_texture) ||
            !rohr_error_check(rohr_graphics_animation_retain(stale)) ||
            !rohr_error_check(rohr_graphics_animation_release(&stale_release)) ||
            stale_release.handle != stale.handle)
        return fail("graphics shutdown left an animation handle alive",
            false, true);

    if(!result_ok(rohr_graphics_start()))
        return fail("graphics restart failed", false, true);
    graphics_started = true;
    first = rohr_graphics_animation_load(descriptor);
    if(rohr_error_check(first) || first.result.value.handle == stale.handle ||
            !stats_check(1, 1, 2, 2, 2, 0, "graphics restart") ||
            !result_ok(rohr_graphics_animation_release(&first.result.value)) ||
            !stats_check(0, 0, 0, 0, 0, 0, "restart release"))
        return fail("graphics restart reused a stale animation handle",
            true, true);
    const char *state_path = "animation_alignment_state.json";
    const char *saved_path = "animation_alignment_saved.json";
    const char *template_path = "animation_alignment_template.json";
    const char *json = "{\"version\":4,\"assets\":{\"animations\":[{\"name\":\"aligned\",\"ticks_per_frame\":0,\"time_per_frame\":0,\"frames\":[{\"file\":\"animation_assets_first.png\"}]}]},\"entities\":[{\"name\":\"aligned_body\",\"components\":{\"animated_sprite\":{\"animation\":\"aligned\",\"scale\":{\"x\":2,\"y\":3},\"frames\":[{\"scale\":{\"x\":-32,\"y\":0},\"offset\":{\"x\":13,\"y\":-9},\"rotation\":450}]}}}]}";
    const char *legacy[] = {
        "{\"version\":3,\"entities\":[]}",
        "{\"version\":4,\"assets\":{\"animations\":[{\"name\":\"old\",\"frames\":["
        "{\"file\":\"animation_assets_first.png\",\"size\":{\"x\":32,\"y\":24}}]}]},\"entities\":[]}"
    };
    for(size_t old = 0; old < sizeof(legacy) / sizeof(legacy[0]); old += 1) {
        if(!SDL_SaveFile(state_path, legacy[old], strlen(legacy[old])) ||
                !rohr_error_check(rohr_game_state_file_load(state_path)) ||
                !stats_check(0, 0, 0, 0, 0, 0, "legacy state rollback"))
            return fail("legacy state definition was accepted", true, true);
    }
    if(!SDL_SaveFile(state_path, json, strlen(json)) ||
            !result_ok(rohr_game_state_file_load(state_path)) ||
            !result_ok(rohr_game_state_template_file_save(template_path)))
        return fail("aligned animation persistence failed", true, true);
    Entity saved_entity = rohr_entity_by_name_get("aligned_body").result.value;
    TextureAssetResult replacement_image = rohr_graphics_texture_load(
        (TextureDescriptor){.file = second_path});
    AnimationFrame saved_frame = rohr_graphics_animated_sprite_frame_get(saved_entity, 0).result.value;
    saved_frame.texture = replacement_image.result.value;
    if(rohr_error_check(replacement_image) ||
            !result_ok(rohr_graphics_animated_sprite_frame_set(saved_entity, 0, saved_frame)))
        return fail("persistent frame replacement failed", true, true);
    saved_frame.id = 0;
    if(!result_ok(rohr_graphics_animated_sprite_frame_add(saved_entity, saved_frame)) ||
            !result_ok(rohr_graphics_texture_release(&replacement_image.result.value)) ||
            !result_ok(rohr_game_state_file_save(saved_path)))
        return fail("persistent frame append failed", true, true);
    char *saved_json = SDL_LoadFile(saved_path, NULL);
    if(saved_json == NULL || strstr(saved_json, "\"file\": \"animation_assets_second.png\"") == NULL) {
        SDL_free(saved_json);
        return fail("saved frame image path was not portable", true, true);
    }
    SDL_free(saved_json);
    for(int pass = 0; pass < 2; pass += 1) {
        rohr_graphics_stop();
        rohr_engine_stop();
        if(!result_ok(rohr_engine_start()) || !result_ok(rohr_graphics_start()) ||
                !result_ok(rohr_game_state_file_load(pass == 0 ? saved_path : template_path)))
            return fail("aligned animation reload failed", true, true);
        EntityResult entity = rohr_entity_by_name_get("aligned_body");
        if(rohr_error_check(entity)) return fail("aligned entity missing", true, true);
        EntityIndexResult index = rohr_entity_index_get(entity.result.value);
        if(rohr_error_check(index)) return fail("aligned entity index missing", true, true);
        AnimationFrameResult loaded = rohr_graphics_animated_sprite_frame_get(entity.result.value, 0);
        if(rohr_error_check(loaded) || loaded.result.value.offset.x != 13 ||
                loaded.result.value.offset.y != -9 || loaded.result.value.rotation != 450 ||
                loaded.result.value.scale.x != -32 || loaded.result.value.scale.y != 0)
            return fail("aligned metadata did not survive reload", true, true);
        TextureAssetResult expected_image = rohr_graphics_texture_load(
            (TextureDescriptor){.file = pass == 0 ? second_path : first_path});
        if(rohr_error_check(expected_image) ||
                animated_sprites[index.result.value].frame_count != (pass == 0 ? 2u : 1u) ||
                loaded.result.value.texture.handle != expected_image.result.value.handle ||
                !result_ok(rohr_graphics_texture_release(&expected_image.result.value)))
            return fail("runtime frames or authored template changed", true, true);
    }
    (void)SDL_RemovePath(state_path);
    (void)SDL_RemovePath(saved_path);
    (void)SDL_RemovePath(template_path);
    rohr_graphics_stop();
    graphics_started = false;
    if(!stats_check(0, 0, 0, 0, 0, 0, "final graphics shutdown"))
        return fail("final graphics shutdown leaked resources", false, true);

    rohr_engine_stop();
    engine_started = false;
    (void)SDL_RemovePath(first_path);
    (void)SDL_RemovePath(second_path);
    return 0;
}
