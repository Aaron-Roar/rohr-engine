/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include <rohr.h>
#include <assert.h>
#include <math.h>

#define OK(call) assert(!rohr_error_check(call))
int main(void) {
    OK(rohr_engine_start());
    EntityResult body = rohr_entity_add(); OK(body);
    OK(rohr_physics_position_set(body.result.value, (Position){20,30}));
    AnimatedSpriteDescription description = rohr_graphics_animated_sprite_description_default_get();
    description.body_offset = (Position){4,5};
    EntityResult a = rohr_graphics_animated_sprite_attachment_add(body.result.value, "left", description); OK(a);
    description.body_offset.x = -4;
    EntityResult b = graphics_animated_sprite_attachment_add(body.result.value, "right", description); OK(b);
    Entity storage[2]; EntityList list = {storage,2,0};
    OK(rohr_graphics_animated_sprite_attachment_list_get(body.result.value, &list));
    assert(list.count == 2 && a.result.value != b.result.value);
    AnimatedSpritePoseResult pose = rohr_graphics_animated_sprite_pose_get(a.result.value); OK(pose);
    assert(fabsf(pose.result.value.position.x - 24) < .001f);
    pose = graphics_animated_sprite_pose_get(b.result.value); OK(pose);
    assert(fabsf(pose.result.value.position.x - 16) < .001f);
    OK(rohr_graphics_animated_sprite_attachment_set(a.result.value, body.result.value));
    assert(graphics_animated_sprite_attachment_get(a.result.value).result.value == body.result.value);
    OK(rohr_entity_delete(body.result.value));
    assert(!rohr_entity_alive_check(a.result.value) && !rohr_entity_alive_check(b.result.value));
    rohr_engine_stop();
    return 0;
}
