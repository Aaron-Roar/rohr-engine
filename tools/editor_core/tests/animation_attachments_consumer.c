/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project.h"
#include <assert.h>
#include <math.h>

int main(void) {
    assert(!rohr_error_check(rohr_directory_working_set(rohr_directory_base_get())));
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    assert(project.actor.body.hitbox_animation_driver == project.actor.body.animation_39.id);
    for(unsigned path = 0; path < 3; ++path) {
        ProjectId project_id = ROHR_PROJECT_INVALID;
        ProjectId second_project_id = ROHR_PROJECT_INVALID;
        Entity second_driver = ENTITY_INVALID;
        Entity root = ENTITY_INVALID;
        if(path == 0) {
            ProjectIdResult result = rohr_project_create(project);
            assert(!rohr_error_check(result)); project_id = result.result.value;
        } else if(path == 1) {
            EntityResult result = rohr_object_create(project.actor);
            assert(!rohr_error_check(result)); root = result.result.value;
        } else {
            EntityResult result = rohr_entity_create(project.actor.body);
            assert(!rohr_error_check(result)); root = result.result.value;
            ObjectCreateBinding binding = {&project.actor.body._description, root};
            EntityCreateOptions options = {.owner = root, .bindings = &binding, .binding_count = 1};
            for(size_t i = 0; i < project.actor.body.child_count; ++i) {
                result = rohr_internal_entity_create(project.actor.body.children[i], options);
                assert(!rohr_error_check(result));
            }
            EntityResult driver = rohr_entity_by_name_get("animation_39"); assert(!rohr_error_check(driver));
            assert(!rohr_error_check(rohr_physics_hitbox_animation_driver_set(root, driver.result.value)));
            for(size_t b = 0; b < project.actor.body.hitbox_binding_count; ++b) {
                HitboxAnimationBinding binding_value = project.actor.body.hitbox_bindings[b];
                assert(!rohr_error_check(rohr_physics_hitbox_animation_binding_set(root,
                    binding_value.animation_id, binding_value.frame_id, binding_value.hitbox_id)));
            }
        }
        EntityResult body = rohr_entity_by_name_get("body"); assert(!rohr_error_check(body));
        EntityResult a = rohr_entity_by_name_get("animation_0"), b = rohr_entity_by_name_get("animation_39");
        assert(!rohr_error_check(a) && !rohr_error_check(b));
        assert(a.result.value != body.result.value && a.result.value != b.result.value);
        Entity storage[40]; EntityList list = {.data = storage, .capacity = 40};
        assert(!rohr_error_check(rohr_graphics_animated_sprite_attachment_list_get(body.result.value, &list)));
        assert(list.count == 40);
        assert(rohr_graphics_animated_sprite_description_get(a.result.value).result.value.player.ticks_per_frame == 1);
        assert(rohr_graphics_animated_sprite_description_get(b.result.value).result.value.player.ticks_per_frame == 40);
        EntityResult hidden = rohr_entity_by_name_get("animation_1"); assert(!rohr_error_check(hidden));
        assert(!rohr_graphics_animated_sprite_description_get(hidden.result.value).result.value.visible);
        EntityInfoResult snapshot = rohr_entity_info_get(body.result.value, ROHR_ENTITY_INFO_PHYSICS_HITBOX_BINDINGS);
        assert(!rohr_error_check(snapshot));
        assert(snapshot.result.value.hitbox_binding_count == 40);
        assert(rohr_entity_info_check(&snapshot.result.value, ROHR_ENTITY_INFO_PHYSICS_HITBOX_BINDINGS));
        assert(!rohr_error_check(rohr_entity_info_release(&snapshot.result.value)));
        assert(rohr_physics_hitbox_animation_driver_get(body.result.value).result.value == b.result.value);
        if(path == 0) {
            ProjectIdResult second = rohr_project_create(project); assert(!rohr_error_check(second));
            second_project_id = second.result.value;
            EntityResult second_body = rohr_entity_by_name_get("body_1");
            EntityResult other = rohr_entity_by_name_get("animation_39_1");
            assert(!rohr_error_check(second_body) && !rohr_error_check(other)); second_driver = other.result.value;
            assert(rohr_physics_hitbox_animation_driver_get(second_body.result.value).result.value == second_driver);
            assert(rohr_graphics_animated_sprite_attachment_get(second_driver).result.value == second_body.result.value);
        }
        rohr_physics_hitbox_animation_bindings_update();
        assert(rohr_physics_hitbox_active_index_get(body.result.value).result.value == 1);
        assert(!rohr_error_check(rohr_physics_hitbox_animation_driver_set(body.result.value, a.result.value)));
        rohr_physics_hitbox_animation_bindings_update();
        assert(rohr_physics_hitbox_active_index_get(body.result.value).result.value == 0);
        assert(!rohr_error_check(rohr_graphics_animated_sprite_frame_index_set(a.result.value, 1)));
        assert(rohr_graphics_animated_sprite_frame_index_get(b.result.value).result.value == 0);
        assert(rohr_graphics_layer_animation_get(b.result.value).result.value == 39);
        AnimatedSpritePoseResult pose = rohr_graphics_animated_sprite_pose_get(b.result.value);
        assert(!rohr_error_check(pose));
        assert(fabsf(pose.result.value.position.x - 2) < .001f && fabsf(pose.result.value.position.y + 39) < .001f);
        assert(!rohr_entity_components_check(b.result.value, ROHR_TRANSFORM_LOCK));
        assert(!rohr_error_check(rohr_entity_delete(a.result.value)));
        assert(rohr_entity_alive_check(b.result.value));
        if(path == 0) assert(!rohr_error_check(rohr_project_destroy(project_id)));
        else assert(!rohr_error_check(rohr_entity_delete(root)));
        assert(!rohr_entity_alive_check(b.result.value));
        if(path == 0) {
            assert(rohr_entity_alive_check(second_driver));
            assert(rohr_graphics_animated_sprite_frame_index_get(second_driver).result.value == 0);
            assert(!rohr_error_check(rohr_project_destroy(second_project_id)));
        }
    }
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
