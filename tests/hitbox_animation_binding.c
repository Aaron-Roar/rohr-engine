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
    animation_result = rohr_graphics_animation_load((AnimationDescriptor){
        .id = 41,
        .frame_files = {
            "hitbox_animation_binding.png",
            "hitbox_animation_binding.png",
            "hitbox_animation_binding.png",
        },
        .frame_ids = {101, 102, 103},
        .amount_of_descriptors = 3,
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

    sprite = rohr_graphics_animated_sprite_create(animation,
        (Scale){1.0f, 1.0f});
    if(rohr_error_check(rohr_graphics_animated_sprite_add(entity, sprite)) ||
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

    if(rohr_error_check(rohr_physics_hitbox_by_id_remove(
            entity, second_id.result.value))) goto fail;
    if(rohr_error_check(rohr_physics_hitbox_remove(entity)) ||
            rohr_entity_components_check(entity,
                ROHR_HITBOX_ANIMATION_BINDING)) goto fail;
    rohr_graphics_stop();
    rohr_engine_stop();
    (void)SDL_RemovePath("hitbox_animation_binding.png");
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
