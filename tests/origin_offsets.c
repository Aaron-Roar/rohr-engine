/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>
#include <stdio.h>

#define CHECK(condition) do { \
    if(!(condition)) { \
        fprintf(stderr, "origin offsets: line %d: %s\n", __LINE__, #condition); \
        return false; \
    } \
} while(0)

static bool position_equal(Position a, Position b) {
    return fabsf(a.x - b.x) < 0.001f && fabsf(a.y - b.y) < 0.001f;
}

static bool origin_offsets_test(void) {
    const float quarter_turn = -90.0f;
    Shape local = {.amount_of_vertices = 4,
        .vertices = {{2, 3}, {6, 3}, {6, 5}, {2, 5}}};
    const Position expected[] = {{7, 22}, {7, 26}, {5, 26}, {5, 22}};
    Shape world = rohr_physics_shape_world_translate(local,
        (Position){10, 20}, quarter_turn);
    EntityResult body = rohr_entity_add();
    EntityResult target = rohr_entity_add();
    EntityResult at_origin = rohr_entity_add();
    ShapeResult queried;
    JointAnchorIdResult anchor;
    JointAnchorPositionResult point;
    EntityIndexResult body_index;

    for(uint16_t i = 0; i < 4; i += 1)
        CHECK(position_equal(world.vertices[i], expected[i]));
    CHECK(!rohr_error_check(body) && !rohr_error_check(target) &&
        !rohr_error_check(at_origin));
    CHECK(!rohr_error_check(rohr_physics_hitbox_set(body.result.value, local)));
    CHECK(!rohr_error_check(rohr_physics_position_set(body.result.value,
        (Position){10, 20})));
    CHECK(!rohr_error_check(rohr_physics_orientation_set(body.result.value,
        quarter_turn)));
    anchor = rohr_physics_joint_anchor_create(body.result.value, (Position){2, 3});
    CHECK(!rohr_error_check(anchor));
    /* Rendering uses this getter: it must be current even before a tick. */
    queried = rohr_physics_global_hit_box_get(body.result.value);
    CHECK(!rohr_error_check(queried));
    for(uint16_t i = 0; i < 4; i += 1)
        CHECK(position_equal(queried.result.value.vertices[i], expected[i]));
    point = rohr_physics_joint_anchor_world_position_get(anchor.result.value);
    CHECK(!rohr_error_check(point) && position_equal(point.result.value, expected[0]));

    CHECK(!rohr_error_check(rohr_physics_hitbox_set(target.result.value,
        rohr_math_square_create(1, 1))));
    CHECK(!rohr_error_check(rohr_physics_hitbox_set(at_origin.result.value,
        rohr_math_square_create(1, 1))));
    CHECK(!rohr_error_check(rohr_physics_position_set(target.result.value,
        (Position){6, 24})));
    CHECK(!rohr_error_check(rohr_physics_position_set(at_origin.result.value,
        (Position){10, 20})));
    CHECK(!rohr_error_check(rohr_physics_static_set(body.result.value)));
    CHECK(!rohr_error_check(rohr_physics_static_set(target.result.value)));
    CHECK(!rohr_error_check(rohr_physics_static_set(at_origin.result.value)));
    rohr_system_physics_update(0);
    CHECK(rohr_physics_overlap_check(body.result.value, target.result.value));
    CHECK(!rohr_physics_overlap_check(body.result.value, at_origin.result.value));

    CHECK(!rohr_error_check(rohr_physics_velocity_set(body.result.value,
        (Velocity){3, -4})));
    CHECK(!rohr_error_check(rohr_physics_angular_velocity_set(body.result.value, 2)));
    CHECK(!rohr_error_check(rohr_physics_position_set(body.result.value,
        (Position){30, 40})));
    queried = rohr_physics_global_hit_box_get(body.result.value);
    CHECK(!rohr_error_check(queried));
    CHECK(position_equal(queried.result.value.vertices[0], (Position){27, 42}));
    point = rohr_physics_joint_anchor_local_position_get(anchor.result.value);
    CHECK(!rohr_error_check(point) && position_equal(point.result.value, (Position){2, 3}));
    point = rohr_physics_joint_anchor_world_position_get(anchor.result.value);
    CHECK(!rohr_error_check(point) && position_equal(point.result.value, (Position){27, 42}));
    CHECK(!rohr_error_check(rohr_physics_orientation_set(body.result.value, 0)));
    queried = rohr_physics_global_hit_box_get(body.result.value);
    CHECK(!rohr_error_check(queried));
    CHECK(position_equal(queried.result.value.vertices[0], (Position){32, 43}));
    body_index = rohr_entity_index_get(body.result.value);
    CHECK(!rohr_error_check(body_index));
    CHECK(position_equal(velocities[body_index.result.value], (Velocity){3, -4}));
    CHECK(angular_velocities[body_index.result.value] == 2);
    queried = rohr_physics_hitbox_get(body.result.value);
    CHECK(!rohr_error_check(queried));
    for(uint16_t i = 0; i < 4; i += 1)
        CHECK(position_equal(queried.result.value.vertices[i], local.vertices[i]));

    /* Concave variants retain their offsets and topology when selected. */
    local = (Shape){.amount_of_vertices = 6,
        .vertices = {{2, 3}, {6, 3}, {6, 4}, {3, 4}, {3, 7}, {2, 7}}};
    CHECK(!rohr_error_check(rohr_physics_hitbox_add(body.result.value, local)));
    CHECK(!rohr_error_check(rohr_physics_hitbox_active_index_set(body.result.value, 1)));
    queried = rohr_physics_global_hit_box_get(body.result.value);
    CHECK(!rohr_error_check(queried) && queried.result.value.concave_piece_count == 4);
    for(uint16_t i = 0; i < 6; i += 1)
        CHECK(position_equal(queried.result.value.vertices[i],
            ((Position){30 + local.vertices[i].x, 40 + local.vertices[i].y})));
    return true;
}

static bool particle_offsets_test(void) {
    ParticleConfig config = rohr_physics_particle_config_default_get();
    EntityResult particle;
    ShapeResult world;
    PositionResult offset;
    EntityIndexResult index;

    config.position = (Position){-10, 8};
    config.local_origin = (Position){3, 4};
    config.radius = 2;
    particle = rohr_physics_particle_create(config);
    CHECK(!rohr_error_check(particle));
    world = rohr_physics_global_hit_box_get(particle.result.value);
    CHECK(!rohr_error_check(world));
    CHECK(position_equal(world.result.value.vertices[0], (Position){-5, 12}));
    CHECK(!rohr_error_check(rohr_physics_position_set(particle.result.value,
        (Position){-20, 9})));
    CHECK(!rohr_error_check(rohr_physics_angular_velocity_set(particle.result.value, 4)));
    rohr_system_physics_update(0.01);
    world = rohr_physics_global_hit_box_get(particle.result.value);
    offset = rohr_physics_particle_origin_get(particle.result.value);
    index = rohr_entity_index_get(particle.result.value);
    CHECK(!rohr_error_check(world) && !rohr_error_check(offset) && !rohr_error_check(index));
    CHECK(position_equal(world.result.value.vertices[0], (Position){-15, 13}));
    CHECK(position_equal(offset.result.value, config.local_origin));
    CHECK(orientations[index.result.value] == 0);
    return true;
}

static bool camera_offsets_test(void) {
    EntityResult body = rohr_entity_add();
    CameraResult camera;
    CameraAttachmentResult attachment;
    CHECK(!rohr_error_check(body));
    CHECK(!rohr_error_check(rohr_physics_position_set(body.result.value,
        (Position){10, 20})));
    CHECK(!rohr_error_check(rohr_physics_orientation_set(body.result.value,
        -90.0f)));
    CHECK(!rohr_error_check(rohr_graphics_camera_attach(body.result.value,
        (Position){2, 3}, -15.0f)));
    camera = rohr_camera_get(rohr_camera_active_get());
    CHECK(!rohr_error_check(camera) &&
        position_equal(camera.result.value.position, (Position){7, 22}));
    CHECK(!rohr_error_check(rohr_physics_position_set(body.result.value,
        (Position){30, 40})));
    camera = rohr_camera_get(rohr_camera_active_get());
    attachment = rohr_graphics_camera_attachment_get();
    CHECK(!rohr_error_check(camera) &&
        position_equal(camera.result.value.position, (Position){27, 42}));
    CHECK(!rohr_error_check(attachment) &&
        position_equal(attachment.result.value.position_offset, (Position){2, 3}));
    CHECK(fabsf(camera.result.value.orientation + 105.0f) < 0.001f);
    rohr_graphics_camera_detach();
    return true;
}

int main(void) {
    bool passed;
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(rohr_error_check(rohr_graphics_start())) {
        rohr_engine_stop();
        return 1;
    }
    passed = origin_offsets_test() && particle_offsets_test() && camera_offsets_test();
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
