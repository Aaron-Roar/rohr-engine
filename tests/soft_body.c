/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>

static bool beam_order_scene_create(
        float x, bool reversed, Entity *beam_out, Entity *target_out) {
    EntityResult body = rohr_physics_soft_body_create();
    EntityResult left;
    EntityResult right;
    EntityResult beam;
    EntityResult target = rohr_entity_add();

    if(beam_out == NULL || target_out == NULL || rohr_error_check(body) ||
            rohr_error_check(target))
        return false;
    left = rohr_physics_soft_body_node_create(body.result.value,
        (Position){x - 5.0f, 200.0f}, 1.0f, 1.0f);
    right = rohr_physics_soft_body_node_create(body.result.value,
        (Position){x + 5.0f, 200.0f}, 1.0f, 1.0f);
    if(rohr_error_check(left) || rohr_error_check(right) ||
            rohr_error_check(rohr_physics_restitution_set(
                left.result.value, 0.1f)) ||
            rohr_error_check(rohr_physics_restitution_set(
                right.result.value, 0.9f))) return false;
    beam = rohr_physics_soft_body_beam_create(body.result.value,
        reversed ? right.result.value : left.result.value,
        reversed ? left.result.value : right.result.value, 1.0f, 0.0f);
    if(rohr_error_check(beam) ||
            rohr_error_check(rohr_physics_position_set(
                target.result.value, (Position){x, 198.75f})) ||
            rohr_error_check(rohr_physics_hitbox_set(target.result.value,
                rohr_math_square_create(1.0f, 1.0f))) ||
            rohr_error_check(rohr_physics_mass_set(target.result.value, 1.0f)) ||
            rohr_error_check(rohr_physics_velocity_set(
                target.result.value, (Velocity){0.0f, 1.0f})) ||
            rohr_error_check(rohr_physics_dynamic_set(target.result.value)) ||
            rohr_error_check(rohr_physics_restitution_set(
                target.result.value, 1.0f))) return false;
    *beam_out = beam.result.value;
    *target_out = target.result.value;
    return true;
}

int main(void) {
    EntityResult body;
    EntityResult node_a;
    EntityResult node_b;
    EntityResult node_c;
    EntityResult beam;
    EntityResult triangle;
    SoftBodyNodeAnchorPinResult attachment;
    EntityResult rigid_body;
    JointAnchorIdResult rigid_anchor;
    EntityIndexResult index_a;
    EntityIndexResult index_b;

    {
        Color white = rohr_graphics_color_hex_create(UINT32_C(0xffffffff));
        Color black = rohr_graphics_color_hex_create(UINT32_C(0x000000ff));
        Color yellow = rohr_graphics_color_hex_create(UINT32_C(0xffff00ff));
        Color orange = rohr_graphics_color_hex_create(UINT32_C(0xff800080));
        if(white.red != 255 || white.green != 255 || white.blue != 255 ||
                white.alpha != 255 || black.red != 0 || black.green != 0 ||
                black.blue != 0 || black.alpha != 255 || yellow.red != 255 ||
                yellow.green != 255 || yellow.blue != 0 || orange.red != 255 ||
                orange.green != 128 || orange.blue != 0 || orange.alpha != 128) return 1;
    }
    if(rohr_error_check(rohr_engine_start())) return 1;
    body = rohr_physics_soft_body_create();
    if(rohr_error_check(body)) goto fail;
    node_a = rohr_physics_soft_body_node_create(body.result.value, (Position){-10.0f, 0.0f}, 1.0f, 2.0f);
    node_b = rohr_physics_soft_body_node_create(body.result.value, (Position){10.0f, 0.0f}, 1.0f, 2.0f);
    node_c = rohr_physics_soft_body_node_create(body.result.value, (Position){0.0f, 15.0f}, 1.0f, 2.0f);
    if(rohr_error_check(node_a) || rohr_error_check(node_b) || rohr_error_check(node_c)) goto fail;
    {
        SoftBodyResult topology = rohr_physics_soft_body_get(body.result.value);
        PositionResult position;
        EntityIndexResult body_index;
        if(rohr_error_check(topology) ||
                topology.result.value.origin != body.result.value ||
                rohr_error_check(rohr_physics_position_set(
                    body.result.value, (Position){5.0f, 5.0f})) ||
                rohr_error_check(rohr_physics_orientation_set(
                    body.result.value, -90.0f))) goto fail;
        position = rohr_physics_position_get(node_a.result.value);
        body_index = rohr_entity_index_get(body.result.value);
        if(rohr_error_check(position) || rohr_error_check(body_index) ||
                fabsf(position.result.value.x - 5.0f) > 0.0001f ||
                fabsf(position.result.value.y + 5.0f) > 0.0001f ||
                fabsf(orientations[body_index.result.value] -
                    -90.0f) > 0.0001f ||
                rohr_error_check(rohr_physics_orientation_set(
                    body.result.value, 0.0f)) ||
                rohr_error_check(rohr_physics_position_set(
                    body.result.value, (Position){0}))) goto fail;
    }
    {
        CollisionFilterConfigResult filter =
            rohr_physics_collision_filter_get(node_a.result.value);
        if(rohr_error_check(filter) ||
                !rohr_entity_components_check(node_a.result.value,
                    ROHR_SOFT_BODY_NODE | ROHR_PARTICLE | ROHR_HIT_BOX | ROHR_COLLISION) ||
                filter.result.value.category !=
                    ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE ||
                (filter.result.value.collides_with &
                    ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE) != 0) goto fail;
    }
    beam = rohr_physics_soft_body_beam_create(
        body.result.value, node_a.result.value, node_b.result.value, 10.0f, 1.0f);
    triangle = rohr_physics_soft_body_triangle_create(
        body.result.value, node_a.result.value, node_b.result.value, node_c.result.value);
    if(rohr_error_check(beam) || rohr_error_check(triangle)) goto fail;
    {
        SoftBodyBeamResult collision = rohr_physics_soft_body_beam_get(
            beam.result.value);
        if(rohr_error_check(collision) ||
                !collision.result.value.collision_enabled ||
                collision.result.value.collision_thickness != 4.0f ||
                collision.result.value.category !=
                    ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE ||
                (collision.result.value.collides_with &
                    ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE) != 0 ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_thickness_set(
                    beam.result.value, 100.0f))) goto fail;
        collision = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(collision) ||
                collision.result.value.collision_thickness != 4.0f ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_thickness_set(
                    beam.result.value, 0.001f))) goto fail;
        collision = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(collision) ||
                collision.result.value.collision_thickness !=
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_filter_set(
                    beam.result.value, UINT64_C(2), UINT64_C(4))) ||
                rohr_error_check(rohr_physics_particle_radius_set(
                    node_a.result.value, 0.001f)) ||
                !rohr_error_check(rohr_physics_soft_body_beam_collision_enable(
                    beam.result.value))) goto fail;
        if(!rohr_error_check(rohr_physics_soft_body_beam_collision_config_set(
                    beam.result.value, (SoftBodyBeamCollisionConfig){
                        .enabled = true,
                        .thickness = 0.02f,
                        .filter = {
                            .category = UINT64_C(32),
                            .collides_with = UINT64_C(64)
                        }
                    }))) goto fail;
        collision = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(collision) ||
                collision.result.value.collision_enabled ||
                collision.result.value.collision_thickness !=
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN ||
                collision.result.value.category != UINT64_C(2) ||
                collision.result.value.collides_with != UINT64_C(4) ||
                rohr_error_check(rohr_physics_particle_radius_set(
                    node_a.result.value, 2.0f)) ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_enable(
                    beam.result.value))) goto fail;
        collision = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(collision) || !collision.result.value.collision_enabled ||
                collision.result.value.collision_thickness !=
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_config_set(
                    beam.result.value, (SoftBodyBeamCollisionConfig){
                        .enabled = true,
                        .thickness = 3.0f,
                        .filter = {
                            .category = UINT64_C(8),
                            .collides_with = UINT64_C(16)
                        }
                    }))) goto fail;
        collision = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(collision) ||
                collision.result.value.collision_thickness != 3.0f ||
                collision.result.value.category != UINT64_C(8) ||
                collision.result.value.collides_with != UINT64_C(16)) goto fail;
    }
    {
        Color node_color = {10, 20, 30, 255};
        Color beam_color = {40, 50, 60, 255};
        SoftBodyNodeResult styled_node;
        SoftBodyBeamResult styled_beam;
        if(rohr_error_check(rohr_graphics_soft_body_node_color_set(
                    body.result.value, node_a.result.value, node_color)) ||
                rohr_error_check(rohr_graphics_soft_body_beam_color_set(
                    body.result.value, node_b.result.value, node_a.result.value,
                    beam_color))) goto fail;
        styled_node = rohr_physics_soft_body_node_get(node_a.result.value);
        styled_beam = rohr_physics_soft_body_beam_get(beam.result.value);
        if(rohr_error_check(styled_node) || rohr_error_check(styled_beam) ||
                !styled_node.result.value.draw_color_overridden ||
                styled_node.result.value.draw_color.red != node_color.red ||
                !styled_beam.result.value.draw_color_overridden ||
                styled_beam.result.value.draw_color.green != beam_color.green) goto fail;
    }
    {
        SoftBodyResult topology = rohr_physics_soft_body_get(body.result.value);
        if(rohr_error_check(topology) || topology.result.value.node_count != 3 ||
                topology.result.value.beam_count != 1 || topology.result.value.triangle_count != 1) goto fail;
    }
    if(rohr_error_check(rohr_physics_position_set(node_b.result.value, (Position){20.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_angular_velocity_set(
                node_a.result.value, 10.0f))) goto fail;
    rohr_system_physics_update(0.1);
    index_a = rohr_entity_index_get(node_a.result.value);
    index_b = rohr_entity_index_get(node_b.result.value);
    if(rohr_error_check(index_a) || rohr_error_check(index_b) ||
            velocities[index_a.result.value].x <= 0.0f ||
            velocities[index_b.result.value].x >= 0.0f ||
            angular_velocities[index_a.result.value] != 0.0f) goto fail;
    if(rohr_error_check(rohr_physics_soft_body_node_impulse_apply(
                node_c.result.value, (Vec2D){0.0f, 2.0f})) ||
            rohr_error_check(rohr_physics_soft_body_node_force_apply(
                node_c.result.value, (Force){0.0f, 1.0f})) ||
            rohr_error_check(rohr_physics_soft_body_force_apply(
                body.result.value, (Force){3.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_soft_body_torque_apply(
                body.result.value, 2.0f))) goto fail;
    rigid_body = rohr_entity_add();
    if(rohr_error_check(rigid_body) ||
            rohr_error_check(rohr_physics_position_set(rigid_body.result.value, (Position){0.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_mass_set(rigid_body.result.value, 4.0f)) ||
            rohr_error_check(rohr_physics_velocity_set(rigid_body.result.value, (Velocity){0})) ||
            rohr_error_check(rohr_physics_angular_velocity_set(rigid_body.result.value, 10.0f)) ||
            rohr_error_check(rohr_physics_dynamic_set(rigid_body.result.value))) goto fail;
    {
        AngularVelocityResult angular_velocity =
            rohr_physics_angular_velocity_get(rigid_body.result.value);
        if(rohr_error_check(angular_velocity) || angular_velocity.result.value != 10.0f) goto fail;
        if(rohr_error_check(rohr_physics_angular_velocity_maximum_set(
                    rigid_body.result.value, 1.0f))) goto fail;
        rohr_system_physics_update(0.1);
        angular_velocity = rohr_physics_angular_velocity_get(rigid_body.result.value);
        if(rohr_error_check(angular_velocity) ||
                angular_velocity.result.value > 1.0f ||
                angular_velocity.result.value < -1.0f) goto fail;
    }
    rigid_anchor = rohr_physics_joint_anchor_create(
        rigid_body.result.value, (Vec2D){0.0f, 15.0f});
    if(rohr_error_check(rigid_anchor)) goto fail;
    attachment = rohr_physics_soft_body_node_to_anchor_pin_create(
        node_c.result.value, rigid_anchor.result.value);
    if(rohr_error_check(attachment) ||
            !rohr_entity_alive_check(attachment.result.value.joint)) goto fail;
    if(rohr_error_check(rohr_entity_delete(body.result.value)) ||
            rohr_entity_alive_check(node_a.result.value) || rohr_entity_alive_check(node_b.result.value) ||
            rohr_entity_alive_check(node_c.result.value) || rohr_entity_alive_check(beam.result.value) ||
            rohr_entity_alive_check(triangle.result.value) ||
            rohr_entity_alive_check(attachment.result.value.joint)) goto fail;
    {
        EntityResult collision_body = rohr_physics_soft_body_create();
        EntityResult collision_node;
        EntityResult wall = rohr_entity_add();
        EntityIndexResult collision_node_index;
        PositionResult wall_position;
        if(rohr_error_check(collision_body) || rohr_error_check(wall)) goto fail;
        collision_node = rohr_physics_soft_body_node_create(
            collision_body.result.value, (Position){11.0f, 0.0f}, 1.0f, 3.0f);
        if(rohr_error_check(collision_node)) goto fail;
        if(rohr_error_check(rohr_physics_friction_set(collision_node.result.value, 0.5f)) ||
                rohr_error_check(rohr_physics_restitution_set(collision_node.result.value, 0.3f)) ||
                rohr_error_check(rohr_physics_friction_set(wall.result.value, 0.5f)) ||
                rohr_error_check(rohr_physics_restitution_set(wall.result.value, 0.3f))) goto fail;
        if(rohr_error_check(rohr_physics_position_set(wall.result.value, (Position){0.0f, 0.0f}))) goto fail;
        if(rohr_error_check(rohr_physics_hitbox_set(
                    wall.result.value, rohr_math_square_create(20.0f, 20.0f)))) goto fail;
        if(rohr_error_check(rohr_physics_dynamic_set(wall.result.value))) goto fail;
        rohr_system_physics_update(0.0);
        collision_node_index = rohr_entity_index_get(collision_node.result.value);
        wall_position = rohr_physics_position_get(wall.result.value);
        if(rohr_error_check(collision_node_index) ||
                rohr_error_check(wall_position) ||
                positions[collision_node_index.result.value].x <= 11.0f ||
                wall_position.result.value.x != 0.0f ||
                wall_position.result.value.y != 0.0f ||
                frictions[collision_node_index.result.value] != 0.5f ||
                restitutions[collision_node_index.result.value] != 0.3f) goto fail;
    }
    {
        EntityResult boundary_body = rohr_physics_soft_body_create();
        EntityResult boundary_a;
        EntityResult boundary_b;
        EntityResult boundary_c;
        EntityResult boundary_beam;
        EntityResult boundary_triangle;
        EntityResult object = rohr_entity_add();
        EntityIndexResult object_index;
        PositionResult object_position;

        if(rohr_error_check(boundary_body) || rohr_error_check(object)) goto fail;
        boundary_a = rohr_physics_soft_body_node_create(
            boundary_body.result.value, (Position){-10.0f, 0.0f}, 1.0f, 1.0f);
        boundary_b = rohr_physics_soft_body_node_create(
            boundary_body.result.value, (Position){10.0f, 0.0f}, 1.0f, 1.0f);
        boundary_c = rohr_physics_soft_body_node_create(
            boundary_body.result.value, (Position){0.0f, 10.0f}, 1.0f, 1.0f);
        if(rohr_error_check(boundary_a) || rohr_error_check(boundary_b) ||
                rohr_error_check(boundary_c)) goto fail;
        if(rohr_error_check(rohr_physics_friction_set(
                    boundary_a.result.value, 1.0f)) ||
                rohr_error_check(rohr_physics_friction_set(
                    boundary_b.result.value, 1.0f))) goto fail;
        boundary_triangle = rohr_physics_soft_body_triangle_create(
            boundary_body.result.value, boundary_a.result.value,
            boundary_b.result.value, boundary_c.result.value);
        boundary_beam = rohr_physics_soft_body_beam_create(
            boundary_body.result.value, boundary_a.result.value,
            boundary_b.result.value, 10.0f, 1.0f);
        if(rohr_error_check(boundary_triangle) || rohr_error_check(boundary_beam) ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_disable(
                    boundary_beam.result.value)) ||
                rohr_error_check(rohr_physics_position_set(
                    object.result.value, (Position){0.0f, -1.25f})) ||
                rohr_error_check(rohr_physics_hitbox_set(
                    object.result.value, rohr_math_square_create(1.0f, 1.0f))) ||
                rohr_error_check(rohr_physics_mass_set(object.result.value, 1.0f)) ||
                rohr_error_check(rohr_physics_velocity_set(
                    object.result.value, (Velocity){0.0f, 1.0f})) ||
                rohr_error_check(rohr_physics_angular_velocity_radians_set(object.result.value, -4.0f)) ||
                rohr_error_check(rohr_physics_dynamic_set(object.result.value)) ||
                rohr_error_check(rohr_physics_restitution_set(
                    object.result.value, 0.0f)) ||
                rohr_error_check(rohr_physics_friction_set(
                    object.result.value, 1.0f))) goto fail;
        rohr_system_physics_update(0.0);
        object_position = rohr_physics_position_get(object.result.value);
        if(rohr_error_check(object_position) ||
                object_position.result.value.y != -1.25f ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_enable(
                    boundary_beam.result.value)) ||
                rohr_error_check(rohr_physics_soft_body_beam_collision_filter_set(
                    boundary_beam.result.value, UINT64_C(2), UINT64_C(2))))
            goto fail;
        rohr_system_physics_update(0.0);
        object_position = rohr_physics_position_get(object.result.value);
        if(rohr_error_check(object_position) ||
                object_position.result.value.y != -1.25f ||
                rohr_physics_contact_check(
                    boundary_beam.result.value, object.result.value) ||
                rohr_error_check(rohr_physics_collision_filter_set(
                    object.result.value, (CollisionFilterConfig){
                        .category = UINT64_C(2),
                        .collides_with = UINT64_C(2)
                    }))) goto fail;
        rohr_system_physics_update(0.0);
        object_index = rohr_entity_index_get(object.result.value);
        object_position = rohr_physics_position_get(object.result.value);
        if(rohr_error_check(object_index) || rohr_error_check(object_position) ||
                object_position.result.value.y >= -1.25f ||
                velocities[object_index.result.value].x <= 0.0f ||
                -rohr_math_degrees_to_radians(angular_velocities[object_index.result.value]) >= 4.0f ||
                !rohr_physics_contact_check(
                    boundary_beam.result.value, object.result.value) ||
                rohr_physics_contact_check(
                    boundary_a.result.value, object.result.value) ||
                rohr_physics_contact_check(
                    boundary_b.result.value, object.result.value)) goto fail;
    }
    {
        EntityResult seam_body = rohr_physics_soft_body_create();
        EntityResult seam_a;
        EntityResult seam_b;
        EntityResult seam_beam;
        EntityResult target = rohr_entity_add();

        if(rohr_error_check(seam_body) || rohr_error_check(target)) goto fail;
        seam_a = rohr_physics_soft_body_node_create(seam_body.result.value,
            (Position){95.0f, 100.0f}, 1.0f, 2.0f);
        seam_b = rohr_physics_soft_body_node_create(seam_body.result.value,
            (Position){105.0f, 100.0f}, 1.0f, 2.0f);
        if(rohr_error_check(seam_a) || rohr_error_check(seam_b)) goto fail;
        seam_beam = rohr_physics_soft_body_beam_create(seam_body.result.value,
            seam_a.result.value, seam_b.result.value, 10.0f, 1.0f);
        if(rohr_error_check(seam_beam) ||
                rohr_error_check(rohr_physics_position_set(
                    target.result.value, (Position){95.0f, 99.0f})) ||
                rohr_error_check(rohr_physics_hitbox_set(target.result.value,
                    rohr_math_square_create(0.5f, 0.5f))) ||
                rohr_error_check(rohr_physics_mass_set(target.result.value, 1.0f)) ||
                rohr_error_check(rohr_physics_velocity_set(
                    target.result.value, (Velocity){0})) ||
                rohr_error_check(rohr_physics_dynamic_set(target.result.value)) ||
                rohr_error_check(rohr_physics_restitution_set(
                    target.result.value, 0.0f))) goto fail;
        rohr_system_physics_update(0.0);
        if(!rohr_physics_contact_check(seam_a.result.value, target.result.value) ||
                rohr_physics_contact_check(
                    seam_beam.result.value, target.result.value) ||
                rohr_error_check(rohr_physics_soft_body_node_collision_filter_set(
                    seam_a.result.value, ROHR_COLLISION_CATEGORY_NONE,
                    ROHR_COLLISION_CATEGORY_NONE)) ||
                rohr_error_check(rohr_physics_position_set(
                    seam_a.result.value, (Position){95.0f, 100.0f})) ||
                rohr_error_check(rohr_physics_position_set(
                    seam_b.result.value, (Position){105.0f, 100.0f})) ||
                rohr_error_check(rohr_physics_position_set(
                    target.result.value, (Position){95.0f, 99.0f})) ||
                rohr_error_check(rohr_physics_velocity_set(
                    seam_a.result.value, (Velocity){0})) ||
                rohr_error_check(rohr_physics_velocity_set(
                    seam_b.result.value, (Velocity){0})) ||
                rohr_error_check(rohr_physics_velocity_set(
                    target.result.value, (Velocity){0}))) goto fail;
        rohr_system_physics_update(0.0);
        if(rohr_physics_contact_check(seam_a.result.value, target.result.value) ||
                !rohr_physics_contact_check(
                    seam_beam.result.value, target.result.value)) goto fail;
    }
    {
        EntityResult local_body = rohr_physics_soft_body_create();
        EntityResult local_node;
        PositionResult world_position;
        PositionResult local_position;
        if(rohr_error_check(local_body) ||
                rohr_error_check(rohr_physics_position_set(
                    local_body.result.value, (Position){10.0f, 20.0f})) ||
                rohr_error_check(rohr_physics_orientation_set(
                    local_body.result.value, -90.0f))) goto fail;
        local_node = rohr_physics_soft_body_node_local_create(
            local_body.result.value, (Position){5.0f, 0.0f}, 1.0f, 2.0f);
        if(rohr_error_check(local_node)) goto fail;
        world_position = rohr_physics_position_get(local_node.result.value);
        local_position = rohr_physics_soft_body_node_local_position_get(
            local_node.result.value);
        if(rohr_error_check(world_position) || rohr_error_check(local_position) ||
                fabsf(world_position.result.value.x - 10.0f) > 0.0001f ||
                fabsf(world_position.result.value.y - 25.0f) > 0.0001f ||
                fabsf(local_position.result.value.x - 5.0f) > 0.0001f ||
                fabsf(local_position.result.value.y) > 0.0001f ||
                rohr_error_check(rohr_physics_soft_body_node_local_position_set(
                    local_node.result.value, (Position){0.0f, 5.0f}))) goto fail;
        world_position = rohr_physics_position_get(local_node.result.value);
        if(rohr_error_check(world_position) ||
                fabsf(world_position.result.value.x - 5.0f) > 0.0001f ||
                fabsf(world_position.result.value.y - 20.0f) > 0.0001f) goto fail;
    }
    {
        EntityResult defaults_body = rohr_physics_soft_body_create();
        EntityResult defaults_a;
        EntityResult defaults_b;
        EntityResult defaults_beam;
        EntityResult disabled_defaults_beam;
        SoftBodyBeamResult defaults;

        if(rohr_error_check(defaults_body)) goto fail;
        defaults_a = rohr_physics_soft_body_node_create(defaults_body.result.value,
            (Position){0.0f, 0.0f}, 1.0f, 1.0f);
        defaults_b = rohr_physics_soft_body_node_create(defaults_body.result.value,
            (Position){5.0f, 0.0f}, 1.0f, 1.0f);
        if(rohr_error_check(defaults_a) || rohr_error_check(defaults_b) ||
                rohr_error_check(rohr_physics_soft_body_node_collision_filter_set(
                    defaults_a.result.value, UINT64_C(2), UINT64_C(8))) ||
                rohr_error_check(rohr_physics_soft_body_node_collision_filter_set(
                    defaults_b.result.value, UINT64_C(4), UINT64_C(16))))
            goto fail;
        defaults_beam = rohr_physics_soft_body_beam_create(
            defaults_body.result.value, defaults_a.result.value,
            defaults_b.result.value, 1.0f, 0.0f);
        defaults = rohr_error_check(defaults_beam) ?
            ERROR_RESULT_MAKE_ERROR(SoftBodyBeamResult,
                defaults_beam.result.error) :
            rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || !defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness != 2.0f ||
                defaults.result.value.category != UINT64_C(6) ||
                defaults.result.value.collides_with != UINT64_C(24) ||
                rohr_error_check(rohr_physics_soft_body_node_collision_filter_set(
                    defaults_a.result.value, ROHR_COLLISION_CATEGORY_NONE,
                    ROHR_COLLISION_CATEGORY_NONE))) goto fail;
        defaults = rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || !defaults.result.value.collision_enabled ||
                defaults.result.value.category != UINT64_C(6) ||
                defaults.result.value.collides_with != UINT64_C(24)) goto fail;
        disabled_defaults_beam = rohr_physics_soft_body_beam_create(
            defaults_body.result.value, defaults_a.result.value,
            defaults_b.result.value, 1.0f, 0.0f);
        defaults = rohr_error_check(disabled_defaults_beam) ?
            ERROR_RESULT_MAKE_ERROR(SoftBodyBeamResult,
                disabled_defaults_beam.result.error) :
            rohr_physics_soft_body_beam_get(
                disabled_defaults_beam.result.value);
        if(rohr_error_check(defaults) || defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness != 2.0f ||
                defaults.result.value.category != UINT64_C(4) ||
                defaults.result.value.collides_with != UINT64_C(16) ||
                rohr_error_check(rohr_physics_particle_radius_set(
                    defaults_a.result.value, 0.25f))) goto fail;
        defaults = rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || !defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness != 0.5f ||
                rohr_error_check(rohr_physics_particle_radius_set(
                    defaults_a.result.value, 1.0f))) goto fail;
        defaults = rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || !defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness != 0.5f ||
                rohr_error_check(rohr_physics_particle_radius_set(
                    defaults_a.result.value, 0.001f))) goto fail;
        defaults = rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness !=
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN)
            goto fail;
        defaults_beam = rohr_physics_soft_body_beam_create(
            defaults_body.result.value, defaults_a.result.value,
            defaults_b.result.value, 1.0f, 0.0f);
        defaults = rohr_error_check(defaults_beam) ?
            ERROR_RESULT_MAKE_ERROR(SoftBodyBeamResult,
                defaults_beam.result.error) :
            rohr_physics_soft_body_beam_get(defaults_beam.result.value);
        if(rohr_error_check(defaults) || defaults.result.value.collision_enabled ||
                defaults.result.value.collision_thickness !=
                    ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN) goto fail;
    }
    {
        Entity forward_beam = ENTITY_INVALID;
        Entity reverse_beam = ENTITY_INVALID;
        Entity forward_target = ENTITY_INVALID;
        Entity reverse_target = ENTITY_INVALID;
        ContactInfo forward_contact;
        ContactInfo reverse_contact;

        if(!beam_order_scene_create(200.0f, false, &forward_beam,
                    &forward_target) ||
                !beam_order_scene_create(300.0f, true, &reverse_beam,
                    &reverse_target))
            goto fail;
        rohr_system_physics_update(0.0);
        forward_contact = rohr_physics_contact_get(
            forward_beam, forward_target);
        reverse_contact = rohr_physics_contact_get(
            reverse_beam, reverse_target);
        if(!forward_contact.detected || !reverse_contact.detected ||
                forward_contact.point_count != reverse_contact.point_count ||
                forward_contact.point_count == 0 ||
                fabsf(forward_contact.points[0].normal_impulse.x -
                    reverse_contact.points[0].normal_impulse.x) > 0.0001f ||
                fabsf(forward_contact.points[0].normal_impulse.y -
                    reverse_contact.points[0].normal_impulse.y) > 0.0001f)
            goto fail;
    }
    rohr_engine_stop();
    return 0;

fail:
    rohr_engine_stop();
    return 1;
}
