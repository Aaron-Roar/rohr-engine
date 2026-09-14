/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>

static bool position_check(Position position, float x, float y) {
    return fabsf(position.x - x) < 0.001f &&
        fabsf(position.y - y) < 0.001f;
}

int main(void) {
    Shape shuffled = {.amount_of_vertices = 4,
        .vertices = {{1.0f, 1.0f}, {-1.0f, -1.0f},
            {-1.0f, 1.0f}, {1.0f, -1.0f}}};
    Shape valid = {.amount_of_vertices = 4,
        .vertices = {{-1.0f, -1.0f}, {1.0f, -1.0f},
            {1.0f, 1.0f}, {-1.0f, 1.0f}}};
    EntityResult entity;
    EntityResult soft_body;
    EntityResult nodes[4];
    ShapeResult hitbox;

    if(rohr_error_check(rohr_engine_init())) return 1;
    entity = rohr_entity_add();
    if(rohr_error_check(entity) ||
            rohr_error_check(rohr_physics_hitbox_set(entity.result.value, valid)) ||
            rohr_error_check(rohr_physics_hitbox_autoshape_rectangle_set(
                entity.result.value, 8.0f, 4.0f))) goto fail;
    hitbox = rohr_physics_hitbox_get(entity.result.value);
    if(rohr_error_check(hitbox) ||
            !position_check(hitbox.result.value.vertices[0], -4.0f, -2.0f))
        goto fail;
    soft_body = rohr_physics_soft_body_create();
    if(rohr_error_check(soft_body)) goto fail;
    for(size_t i = 0; i < 4; i += 1) {
        nodes[i] = rohr_physics_soft_body_node_create(soft_body.result.value,
            shuffled.vertices[i], 1.0f, 0.5f);
        if(rohr_error_check(nodes[i])) goto fail;
    }
    if(rohr_error_check(rohr_physics_soft_body_autoshape_rectangle_set(
            soft_body.result.value, 8.0f, 4.0f)) ||
            !rohr_error_check(rohr_physics_soft_body_autoshape_circle_set(
                soft_body.result.value, 0.0f))) goto fail;
    {
        PositionResult first = rohr_physics_position_get(nodes[1].result.value);
        PositionResult second = rohr_physics_position_get(nodes[3].result.value);
        if(rohr_error_check(first) || rohr_error_check(second) ||
                !position_check(first.result.value, -4.0f, -2.0f) ||
                !position_check(second.result.value, 4.0f, -2.0f)) goto fail;
    }
    rohr_engine_shutdown();
    return 0;
fail:
    rohr_engine_shutdown();
    return 1;
}
