/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "test_png.h"

#include <stdio.h>

static Shape triangle(float x) {
    return (Shape){.amount_of_vertices = 3,
        .vertices = {{x, 0.0f}, {x + 1.0f, 0.0f}, {x, 1.0f}}};
}

int main(void) {
    EntityResult added;
    Entity entity;
    AnimatedSprite sprite = {0};
    AnimationAsset animation = {0};
    AnimationAssetResult animation_result;
    HitboxIdResult first_id, second_id;
    HitboxIndexResult active;

    if(!SDL_SaveFile("hitbox_animation_binding.png", test_png,
            sizeof(test_png))) return 1;
    if(rohr_error_check(rohr_engine_start())) goto fail_without_engine;
    if(rohr_error_check(rohr_graphics_start())) goto fail;
    animation_result = rohr_graphics_animation_load((AnimationDescription){
        .id = 41,
        .frame_files = {
            "hitbox_animation_binding.png",
            "hitbox_animation_binding.png",
            "hitbox_animation_binding.png",
        },
        .frame_ids = {101, 102, 103},
        .frame_count = 3,
    });
    if(rohr_error_check(animation_result)) goto fail;
    animation = animation_result.result.value;
    added = rohr_entity_add();
    if(rohr_error_check(added)) goto fail;
    entity = added.result.value;
    if(rohr_error_check(rohr_physics_hitbox_add(entity, triangle(0.0f))) ||
            rohr_error_check(rohr_physics_hitbox_add(entity, triangle(2.0f))))
        goto fail;
    first_id = rohr_physics_hitbox_id_at_get(entity, 0);
    second_id = rohr_physics_hitbox_id_at_get(entity, 1);
    if(rohr_error_check(first_id) || rohr_error_check(second_id)) goto fail;

    sprite = rohr_graphics_animated_sprite_create(rohr_graphics_animated_sprite_description_from_animation_get(animation).result.value).result.value;
    sprite.scale = (Scale){1.0f, 1.0f};
    if(rohr_error_check(rohr_graphics_animated_sprite_add(entity, rohr_graphics_animated_sprite_value_description_get(sprite))) ||
            rohr_error_check(rohr_physics_hitbox_animation_binding_set(
                entity, 41, 101, second_id.result.value)) ||
            rohr_error_check(rohr_physics_hitbox_animation_binding_set(
                entity, 41, 102, second_id.result.value)) ||
            rohr_error_check(rohr_physics_hitbox_animation_binding_set(
                entity, 41, 103, first_id.result.value))) goto fail;
    if(rohr_error_check(rohr_graphics_animation_release(&animation))) goto fail;
    {
        HitboxIdResult by_id = rohr_physics_hitbox_animation_binding_get(
            entity, 41, 102);
        HitboxIndexResult by_index =
            rohr_physics_hitbox_animation_binding_at_get(entity, 1);
        if(rohr_error_check(by_id) ||
                by_id.result.value != second_id.result.value ||
                rohr_error_check(by_index) || by_index.result.value != 1)
            goto fail;
    }

    {
        EntityIndexResult index = rohr_entity_index_get(entity);
        if(rohr_error_check(index)) goto fail;
        HitboxAnimationBinding *binding = &hitbox_animation_bindings_pool.objects[index.result.value].values[0];
        HitboxId valid_id = binding->hitbox_id;
        binding->hitbox_id = UINT32_MAX; /* Inject an unexpected runtime-stale binding. */
        Report report = rohr_system_physics_update(0);
        binding->hitbox_id = valid_id;
        active = rohr_physics_hitbox_active_index_get(entity);
        if(report.header.status != ROHR_REPORT_COMPLETE || report.diagnostic_count != 1 ||
                report.diagnostics[0].internal_stage != ROHR_PHYSICS_PREPARATION ||
                report.diagnostics[0].has_substep || report.diagnostics[0].source.id != entity ||
                report.diagnostics[0].severity != ROHR_REPORT_SEVERITY_WARNING ||
                rohr_error_check(active) || active.result.value != 0) goto fail;
    }

    rohr_physics_hitbox_animation_bindings_update();
    active = rohr_physics_hitbox_active_index_get(entity);
    if(rohr_error_check(active) || active.result.value != 1) goto fail;
    if(rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(entity, 1)))
        goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    active = rohr_physics_hitbox_active_index_get(entity);
    if(rohr_error_check(active) || active.result.value != 1) goto fail;
    if(rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(entity, 2)))
        goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    active = rohr_physics_hitbox_active_index_get(entity);
    if(rohr_error_check(active) || active.result.value != 0) goto fail;

    /* Two clocks can share an asset; only the selected Entity drives collision. */
    AnimatedSpriteDescriptionResult description = rohr_graphics_animated_sprite_description_get(entity);
    if(rohr_error_check(description)) goto fail;
    EntityResult left = rohr_graphics_animated_sprite_attachment_add(entity, "left", description.result.value);
    EntityResult right = rohr_graphics_animated_sprite_attachment_add(entity, "right", description.result.value);
    if(rohr_error_check(left) || rohr_error_check(right) ||
            rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(left.result.value, 0)) ||
            rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(right.result.value, 2)) ||
            rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, left.result.value))) goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    if(rohr_physics_hitbox_active_index_get(entity).result.value != 1 ||
            rohr_physics_hitbox_animation_driver_get(entity).result.value != left.result.value ||
            rohr_physics_hitbox_animation_binding_at_get(entity, 0).result.value != 1) goto fail;
    if(rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, right.result.value))) goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    if(rohr_physics_hitbox_active_index_get(entity).result.value != 0) goto fail;
    if(rohr_error_check(rohr_entity_name_set(entity, "driver_body")) ||
            rohr_error_check(rohr_game_state_file_save("hitbox_driver_state.json")) ||
            rohr_error_check(rohr_entity_delete(entity)) ||
            rohr_error_check(rohr_game_state_file_load("hitbox_driver_state.json"))) goto fail;
    EntityResult restored = rohr_entity_by_name_get("driver_body");
    left = rohr_entity_by_name_get("left"); right = rohr_entity_by_name_get("right");
    if(rohr_error_check(restored) || rohr_error_check(left) || rohr_error_check(right)) goto fail;
    entity = restored.result.value;
    if(rohr_physics_hitbox_animation_driver_get(entity).result.value != right.result.value ||
            rohr_physics_hitbox_count_get(entity).result.value != 2 ||
            rohr_physics_hitbox_animation_binding_get(entity, 41, 101).result.value != second_id.result.value)
        goto fail;
    if(rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, ENTITY_INVALID)) ||
            rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(right.result.value, 0))) goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    if(rohr_physics_hitbox_active_index_get(entity).result.value != 0 ||
            rohr_physics_hitbox_animation_driver_get(entity).result.value != ENTITY_INVALID) goto fail;
    if(rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, right.result.value)) ||
            rohr_error_check(rohr_entity_delete(right.result.value))) goto fail;
    EntityResult replacement = rohr_entity_add();
    if(rohr_error_check(replacement) ||
            !rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, replacement.result.value))) goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    if(rohr_physics_hitbox_active_index_get(entity).result.value != 0 ||
            rohr_physics_hitbox_animation_driver_get(entity).result.value != right.result.value) goto fail;
    if(rohr_error_check(rohr_entity_delete(left.result.value)) ||
            rohr_error_check(rohr_entity_delete(replacement.result.value)) ||
            rohr_error_check(rohr_physics_hitbox_animation_driver_set(entity, entity))) goto fail;

    /* Reordering configuration keeps the same stable frame and hitbox binding. */
    AnimatedSpriteDescriptionResult current = rohr_graphics_animated_sprite_description_get(entity);
    if(rohr_error_check(current)) goto fail;
    AnimationFrameDescription swap = current.result.value.frames[0];
    current.result.value.frames[0] = current.result.value.frames[2];
    current.result.value.frames[2] = swap;
    if(rohr_error_check(rohr_graphics_animated_sprite_description_set(entity, current.result.value)) ||
            rohr_graphics_animated_sprite_frame_index_get(entity).result.value != 0) goto fail;
    rohr_physics_hitbox_animation_bindings_update();
    active = rohr_physics_hitbox_active_index_get(entity);
    if(rohr_error_check(active) || active.result.value != 0) goto fail;

    if(rohr_error_check(rohr_physics_hitbox_by_id_remove(
            entity, second_id.result.value))) goto fail;
    if(rohr_error_check(rohr_physics_hitbox_remove(entity)) ||
            rohr_entity_components_check(entity,
                ROHR_HITBOX_ANIMATION_BINDING)) goto fail;
    rohr_graphics_stop();
    rohr_engine_stop();
    (void)SDL_RemovePath("hitbox_animation_binding.png");
    (void)SDL_RemovePath("hitbox_driver_state.json");
    return 0;
fail:
    fprintf(stderr, "hitbox animation binding test failed\n");
    (void)rohr_graphics_animation_release(&animation);
    rohr_graphics_stop();
    rohr_engine_stop();
fail_without_engine:
    (void)SDL_RemovePath("hitbox_animation_binding.png");
    return 1;
}
