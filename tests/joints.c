/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))
static Entity named_entity(const char *name) {
    EntityResult result = rohr_entity_add(); OK(result);
    OK(rohr_entity_name_set(result.result.value, name));
    return result.result.value;
}
static JointDescription description_get(Entity joint) {
    JointDescriptionResult result = rohr_physics_joint_description_get(joint); OK(result);
    return result.result.value;
}
static void incomplete_tests(void) {
    const JointType kinds[] = {JOINT_PIN, JOINT_WELD, JOINT_SPRING};
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned endpoints = 0; endpoints < 3; ++endpoints) {
        Entity a = named_entity("draft_a"), b = named_entity("draft_b");
        OK(rohr_physics_position_set(a, (Position){0}));
        OK(rohr_physics_position_set(b, (Position){10, 0}));
        OK(rohr_physics_dynamic_set(a)); OK(rohr_physics_dynamic_set(b));
        OK(rohr_physics_mass_set(a, 1)); OK(rohr_physics_mass_set(b, 1));
        OK(rohr_physics_velocity_set(a, (Velocity){1, 0}));
        OK(rohr_physics_velocity_set(b, (Velocity){0}));
        JointAnchorIdResult aa = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a}); OK(aa);
        JointAnchorIdResult bb = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = b}); OK(bb);
        JointDescription description = {.type = kinds[kind], .stiffness = 10,
            .rest_length_explicit = true, .rest_angle_explicit = true,
            .anchor_a = endpoints & 1 ? aa.result.value : 0,
            .anchor_b = endpoints & 2 ? bb.result.value : 0};
        EntityResult joint = rohr_physics_joint_add(description); OK(joint);
        Report report = rohr_system_physics_update(.1);
        assert(!report.has_fatal && report.diagnostic_count == 1);
        assert(report.diagnostics[0].source.id == joint.result.value &&
            report.diagnostics[0].severity == ROHR_REPORT_SEVERITY_WARNING &&
            report.diagnostics[0].recovery == ROHR_REPORT_CONTRIBUTION_SKIPPED);
        assert(fabsf(rohr_physics_position_get(a).result.value.x - .1f) < .0001f);
        assert(rohr_physics_position_get(b).result.value.x == 10);
        assert(description_get(joint.result.value).anchor_a == description.anchor_a);
        description.anchor_a = aa.result.value; description.anchor_b = bb.result.value;
        OK(rohr_physics_joint_description_set(joint.result.value, description));
        report = rohr_system_physics_update(.1);
        assert(!report.has_fatal && report.diagnostic_count == 0);
        assert(rohr_entity_alive_check(joint.result.value));
        assert(rohr_physics_position_get(b).result.value.x < 10);
        OK(rohr_entity_delete(joint.result.value)); OK(rohr_entity_delete(a)); OK(rohr_entity_delete(b));
    }
}
static void description_tests(void) {
    Entity root = named_entity("joint_root"), other = named_entity("joint_other");
    Entity a = named_entity("joint_a"), b = named_entity("joint_b");
    OK(rohr_entity_owner_set(a, root)); OK(rohr_entity_owner_set(b, root));
    OK(rohr_physics_position_set(a, (Position){0, 0}));
    OK(rohr_physics_position_set(b, (Position){3, 4}));
    OK(rohr_physics_orientation_set(a, 30)); OK(rohr_physics_orientation_set(b, 90));
    JointAnchorIdResult aa = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a}); OK(aa);
    JointAnchorIdResult bb = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = b}); OK(bb);
    JointDescription automatic = {.owner = root, .type = JOINT_SPRING,
        .anchor_a = aa.result.value, .anchor_b = bb.result.value, .stiffness = 10, .damping = 2};
    EntityResult created = rohr_physics_joint_add(automatic); OK(created);
    Entity joint = created.result.value;
    OK(rohr_entity_name_set(joint, "joint_subject"));
    JointDescription stored = description_get(joint);
    assert(stored.owner == root && stored.rest_length == 5 && stored.rest_angle == 60);
    assert(stored.rest_length_explicit && stored.rest_angle_explicit);
    assert(rohr_physics_joint_anchors_get(a).result.value.count == 1);
    EntityResult copy = physics_joint_add(automatic); OK(copy);
    OK(rohr_entity_delete(copy.result.value));

    /* Get-modify-set freezes measured values; an automatic replacement remeasures. */
    OK(rohr_physics_position_set(b, (Position){6, 8})); OK(rohr_physics_orientation_set(b, 150));
    stored.stiffness = 20;
    OK(physics_joint_description_set(joint, stored));
    assert(description_get(joint).rest_length == 5 && description_get(joint).rest_angle == 60);
    OK(rohr_physics_joint_description_set(joint, automatic));
    assert(description_get(joint).rest_length == 10 && description_get(joint).rest_angle == 120);
    stored = description_get(joint); stored.rest_length = 0; stored.rest_angle = 0;
    OK(rohr_physics_joint_description_set(joint, stored));
    assert(description_get(joint).rest_length == 0 && description_get(joint).rest_angle == 0);
    OK(rohr_entity_owner_set(joint, other)); assert(description_get(joint).owner == other);
    stored.owner = root; OK(rohr_physics_joint_description_set(joint, stored));

    /* Invalid replacements cannot transfer ownership, change type, or replace endpoints. */
    Entity stale_owner = named_entity("stale_owner"); OK(rohr_entity_delete(stale_owner));
    JointAnchorIdResult stale = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a}); OK(stale);
    OK(rohr_physics_joint_anchor_remove(stale.result.value));
    for(int bad = 0; bad < 9; ++bad) {
        JointDescription invalid = stored; invalid.owner = other;
        switch(bad) {
            case 0: invalid.anchor_a = stale.result.value; break;
            case 1: invalid.stiffness = -1; break;
            case 2: invalid.damping = NAN; break;
            case 3: invalid.rest_length = -1; break;
            case 4: invalid.rest_angle = INFINITY; break;
            case 5: invalid.type = (JointType)99; break;
            case 6: invalid.owner = stale_owner; break;
            case 7: invalid.owner = joint; break;
            default: invalid.damping = -1; break;
        }
        size_t count = rohr_entity_alive_count_get();
        assert(rohr_error_check(rohr_physics_joint_description_set(joint, invalid)));
        JointDescription after = description_get(joint);
        assert(after.owner == root && after.type == stored.type && after.anchor_a == stored.anchor_a &&
            after.anchor_b == stored.anchor_b && after.stiffness == stored.stiffness && after.rest_length == 0);
        assert(rohr_entity_alive_count_get() == count);
    }
    assert(rohr_error_check(rohr_physics_joint_description_get(a)));
    assert(rohr_error_check(rohr_physics_joint_description_set(stale_owner, stored)));

    /* Anchor saves round-trip; live body endpoints without anchors are explicitly rejected. */
    OK(rohr_game_state_file_save("joint_descriptions.json"));
    OK(rohr_entity_delete(root)); OK(rohr_entity_delete(other));
    size_t empty = rohr_entity_alive_count_get();
    size_t size = 0; char *text = SDL_LoadFile("joint_descriptions.json", &size); assert(text);
    char *field = strstr(text, "\"anchor_a\":"); assert(field);
    char *value = field + strlen("\"anchor_a\":");
    while(*value == ' ') ++value;
    assert(*value == '0');
    FILE *file = fopen("joint_legacy.json", "wb"); assert(file);
    assert(fwrite(text, 1, (size_t)(value - text), file) == (size_t)(value - text));
    assert(fputs("null", file) >= 0);
    assert(fwrite(value + 1, 1, size - (size_t)(value + 1 - text), file) == size - (size_t)(value + 1 - text));
    assert(fclose(file) == 0); SDL_free(text);
    EngineResult rejected = rohr_game_state_file_load("joint_legacy.json");
    assert(rohr_error_check(rejected) && rejected.result.error == ERROR_ENGINE_STATE_INVALID);
    assert(strstr(rohr_error_message_get(rejected), "anchor"));
    assert(rohr_entity_alive_count_get() == empty);
    OK(rohr_game_state_file_load("joint_descriptions.json"));
    EntityResult loaded = rohr_entity_by_name_get("joint_subject"); OK(loaded); joint = loaded.result.value;
    EntityResult loaded_root = rohr_entity_by_name_get("joint_root"); OK(loaded_root); root = loaded_root.result.value;
    stored = description_get(joint); assert(stored.owner == root && stored.rest_length == 0 && stored.rest_angle == 0);
    assert(stored.anchor_a != JOINT_ANCHOR_INVALID && stored.anchor_b != JOINT_ANCHOR_INVALID);
    OK(rohr_physics_joint_anchor_remove(stored.anchor_a));
    assert(description_get(joint).anchor_a == JOINT_ANCHOR_INVALID && rohr_entity_alive_check(joint));
    stored = description_get(joint); stored.rest_length_explicit = stored.rest_angle_explicit = false;
    OK(rohr_physics_joint_description_set(joint, stored));
    assert(description_get(joint).rest_length == 0 && description_get(joint).rest_angle == 0);
    OK(rohr_game_state_file_save("joint_disconnected.json"));
    OK(rohr_entity_delete(root));
    loaded = rohr_entity_by_name_get("joint_other"); OK(loaded); OK(rohr_entity_delete(loaded.result.value));
    OK(rohr_game_state_file_load("joint_disconnected.json"));
    loaded = rohr_entity_by_name_get("joint_subject"); OK(loaded);
    assert(description_get(loaded.result.value).anchor_a == JOINT_ANCHOR_INVALID);
    loaded_root = rohr_entity_by_name_get("joint_root"); OK(loaded_root); OK(rohr_entity_delete(loaded_root.result.value));
    loaded = rohr_entity_by_name_get("joint_other"); OK(loaded); OK(rohr_entity_delete(loaded.result.value));
    assert(remove("joint_descriptions.json") == 0 && remove("joint_legacy.json") == 0 && remove("joint_disconnected.json") == 0);
}

static Entity placement_body(Entity owner, Position position) {
    EntityResult created = rohr_entity_add(); OK(created);
    Entity entity = created.result.value;
    OK(rohr_entity_owner_set(entity, owner));
    OK(rohr_physics_position_set(entity, position));
    OK(rohr_physics_orientation_set(entity, 0));
    return entity;
}
static Entity placement_joint(Entity owner, Entity a, Entity b, JointType type,
        float length, float angle) {
    JointAnchorIdResult aa = rohr_physics_joint_anchor_add(
        (JointAnchorDescription){.entity = a}); OK(aa);
    JointAnchorIdResult bb = rohr_physics_joint_anchor_add(
        (JointAnchorDescription){.entity = b}); OK(bb);
    EntityResult joint = rohr_physics_joint_add((JointDescription){
        .owner = owner, .type = type, .anchor_a = aa.result.value,
        .anchor_b = bb.result.value, .rest_length = length, .rest_angle = angle,
        .rest_length_explicit = true, .rest_angle_explicit = true});
    OK(joint); return joint.result.value;
}
static void placement_position_expect(Entity entity, Position expected) {
    PositionResult result = rohr_physics_position_get(entity); OK(result);
    assert(fabsf(result.result.value.x - expected.x) < 0.0001f);
    assert(fabsf(result.result.value.y - expected.y) < 0.0001f);
}
static void placement_tests(void) {
    for(JointType type = JOINT_SPRING; type <= JOINT_PIN; ++type) {
        Entity root = named_entity("placement_root");
        Entity a = placement_body(root, (Position){0, 0});
        Entity b = placement_body(root, (Position){3, 4});
        Entity joint = placement_joint(root, a, b, type, 2, 30);
        OK(rohr_physics_velocity_set(a, (Velocity){1, 2}));
        OK(rohr_physics_orientation_set(b, 90));
        placement_position_expect(a, (Position){0, 0}); /* Creation does not place. */
        OK(rohr_physics_joint_place(joint));
        placement_position_expect(a, type == JOINT_SPRING ?
            (Position){1.8f, 2.4f} : (Position){3, 4});
        placement_position_expect(b, (Position){3, 4});
        EntityIndex index;
        assert(entity_index_get(a, &index));
        assert(orientations[index] == (type == JOINT_WELD ? 60 : 0));
        assert(velocities[index].x == 1);
        assert(rohr_entity_owner_get(a).result.value == root);
        assert(description_get(joint).rest_angle == 30);
        OK(physics_joint_place(joint)); /* Idempotent. */
        JointDescription swapped = description_get(joint);
        JointAnchorId old = swapped.anchor_a;
        swapped.anchor_a = swapped.anchor_b; swapped.anchor_b = old;
        swapped.rest_angle = -swapped.rest_angle;
        OK(rohr_physics_joint_description_set(joint, swapped));
        OK(rohr_physics_position_set(b, (Position){8, 4}));
        Position before = rohr_physics_position_get(a).result.value;
        OK(physics_joint_place(joint)); /* Reversed arguments move B. */
        placement_position_expect(a, before);
        OK(rohr_entity_delete(root));
    }
    Entity root = named_entity("placement_group");
    Entity a = placement_body(root, (Position){0, 0});
    Entity b = placement_body(root, (Position){10, 0});
    Entity c = placement_body(root, (Position){0, 0});
    Entity connection = placement_joint(root, a, c, JOINT_PIN, 0, 0);
    Entity joint = placement_joint(root, a, b, JOINT_PIN, 0, 0);
    OK(rohr_physics_static_set(c));
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    placement_position_expect(a, (Position){0, 0});
    placement_position_expect(c, (Position){0, 0});
    OK(rohr_physics_dynamic_set(c));
    OK(rohr_physics_joint_place(joint));
    placement_position_expect(a, (Position){10, 0});
    placement_position_expect(c, (Position){10, 0});
    OK(rohr_physics_position_set(b, (Position){20, 0}));
    OK(rohr_physics_axis_lock_description_set(a,
        (AxisLockDescription){.axis = {0, 1}, .point_on_axis = {10, 0}}));
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    placement_position_expect(a, (Position){10, 0});
    placement_position_expect(c, (Position){10, 0});
    OK(rohr_physics_axis_lock_remove(a));
    Entity follower = placement_body(root, (Position){10, 0});
    OK(rohr_physics_transform_lock_description_set(follower,
        (TransformLockDescription){.driver = a, .lock_position = true}));
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    OK(rohr_physics_transform_lock_remove(follower));
    Entity shared = placement_joint(root, c, b, JOINT_WELD, 0, 0);
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    placement_position_expect(a, (Position){10, 0});
    OK(rohr_entity_delete(shared));
    OK(rohr_entity_delete(connection));
    OK(rohr_physics_joint_place(joint));
    /* Coincident spring placement chooses world +Y. */
    JointDescription spring = description_get(joint);
    spring.type = JOINT_SPRING; spring.rest_length = 4;
    OK(rohr_physics_joint_description_set(joint, spring));
    OK(rohr_physics_joint_place(joint));
    placement_position_expect(a, (Position){20, 4});
    OK(rohr_entity_delete(root));

    root = named_entity("placement_soft");
    EntityResult soft = rohr_physics_soft_body_add((SoftBodyDescription){.owner = root}); OK(soft);
    EntityResult n1 = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = soft.result.value, .position = {0, 0}, .mass = 1, .radius = 0.5f}); OK(n1);
    EntityResult n2 = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = soft.result.value, .position = {2, 0}, .mass = 1, .radius = 0.5f}); OK(n2);
    b = placement_body(root, (Position){10, 3});
    joint = placement_joint(root, n1.result.value, b, JOINT_PIN, 0, 0);
    OK(rohr_physics_joint_place(joint));
    placement_position_expect(n1.result.value, (Position){10, 3});
    placement_position_expect(n2.result.value, (Position){12, 3});
    placement_position_expect(soft.result.value, (Position){10, 3});
    assert(rohr_error_check(rohr_physics_joint_place(b)));
    OK(rohr_entity_delete(root));
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    root = named_entity("placement_particle");
    EntityResult particle = rohr_physics_particle_add((ParticleDescription){
        .owner = root, .radius = 1, .mass_value = 1}); OK(particle);
    b = placement_body(root, (Position){10, 0});
    OK(rohr_physics_orientation_set(b, 60));
    joint = placement_joint(root, particle.result.value, b, JOINT_WELD, 0, 0);
    assert(rohr_error_check(rohr_physics_joint_place(joint)));
    placement_position_expect(particle.result.value, (Position){0});
    JointDescription pin = description_get(joint); pin.type = JOINT_PIN;
    OK(rohr_physics_joint_description_set(joint, pin));
    OK(rohr_physics_joint_place(joint));
    placement_position_expect(particle.result.value, (Position){10, 0});
    OK(rohr_entity_delete(root));
}

int main(void) {
    EntityResult body_a;
    EntityResult body_b;
    EntityResult joint;
    JointAnchorIdResult anchor_a;
    JointAnchorIdResult anchor_b;
    JointAnchorListResult anchors;
    EntityIndexResult index_a;
    EntityIndexResult index_b;

    if(rohr_error_check(rohr_engine_start())) return 1;
    description_tests();
    incomplete_tests();
    placement_tests();

    /* Anchor placement is body-origin-relative and must not depend on hitbox geometry. */
    {
        EntityResult origin_body;
        JointAnchorIdResult origin_anchor;
        JointAnchorPositionResult world_position;
        Shape shifted_hitbox = rohr_math_square_create(2.0f, 2.0f);

        origin_body = rohr_entity_add();
        if(rohr_error_check(origin_body) ||
                rohr_error_check(rohr_physics_position_set(
                    origin_body.result.value, (Position){10.0f, 20.0f})) ||
                rohr_error_check(rohr_physics_orientation_set(
                    origin_body.result.value, (Orientation){-90.0f}))) goto fail;
        origin_anchor = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = origin_body.result.value, .local_offset = (Vec2D){2.0f, 0.0f}});
        if(rohr_error_check(origin_anchor)) goto fail;
        world_position = rohr_physics_joint_anchor_world_position_get(origin_anchor.result.value);
        if(rohr_error_check(world_position) ||
                fabsf(world_position.result.value.x - 10.0f) > 0.0001f ||
                fabsf(world_position.result.value.y - 22.0f) > 0.0001f) goto fail;
        for(uint16_t vertex = 0; vertex < shifted_hitbox.amount_of_vertices; vertex++) {
            shifted_hitbox.vertices[vertex].x += 100.0f;
            shifted_hitbox.vertices[vertex].y -= 50.0f;
        }
        if(rohr_error_check(rohr_physics_hitbox_set(origin_body.result.value, shifted_hitbox))) goto fail;
        world_position = rohr_physics_joint_anchor_world_position_get(origin_anchor.result.value);
        if(rohr_error_check(world_position) ||
                fabsf(world_position.result.value.x - 10.0f) > 0.0001f ||
                fabsf(world_position.result.value.y - 22.0f) > 0.0001f ||
                rohr_error_check(rohr_entity_delete(origin_body.result.value))) goto fail;
    }

    body_a = rohr_entity_add();
    body_b = rohr_entity_add();
    joint = rohr_entity_add();
    if(rohr_error_check(body_a) || rohr_error_check(body_b) || rohr_error_check(joint)) goto fail;
    if(rohr_error_check(rohr_physics_position_set(body_a.result.value, (Position){0.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_position_set(body_b.result.value, (Position){4.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_mass_set(body_a.result.value, 1.0f)) ||
            rohr_error_check(rohr_physics_mass_set(body_b.result.value, 1.0f)) ||
            rohr_error_check(rohr_physics_dynamic_set(body_a.result.value)) ||
            rohr_error_check(rohr_physics_dynamic_set(body_b.result.value))) goto fail;

    anchor_a = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body_a.result.value, .local_offset = (Vec2D){0.0f, 0.0f}});
    anchor_b = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body_b.result.value, .local_offset = (Vec2D){0.0f, 0.0f}});
    if(rohr_error_check(anchor_a) || rohr_error_check(anchor_b)) goto fail;
    anchors = rohr_physics_joint_anchors_get(body_a.result.value);
    if(rohr_error_check(anchors) || anchors.result.value.count != 1 ||
            anchors.result.value.values[0] != anchor_a.result.value) goto fail;
    if(rohr_error_check(rohr_physics_joint_description_set(joint.result.value, (JointDescription){.type = JOINT_PIN, .anchor_a = anchor_a.result.value, .anchor_b = anchor_b.result.value}))) goto fail;

    rohr_physics_pipeline_substep_begin();
    rohr_physics_pipeline_joints_gather();
    rohr_physics_pipeline_constraints_solve(
        rohr_physics_solver_iterations_get());
    index_a = rohr_entity_index_get(body_a.result.value);
    index_b = rohr_entity_index_get(body_b.result.value);
    if(rohr_error_check(index_a) || rohr_error_check(index_b) ||
            fabsf(positions[index_a.result.value].x - positions[index_b.result.value].x) > 0.0001f) goto fail;
    if(rohr_error_check(rohr_physics_acceleration_set(
                body_a.result.value, (Acceleration){-10.0f, 0.0f})) ||
            rohr_error_check(rohr_physics_acceleration_set(
                body_b.result.value, (Acceleration){10.0f, 0.0f}))) goto fail;
    rohr_system_physics_update(0.1);
    if(fabsf(positions[index_a.result.value].x -
            positions[index_b.result.value].x) > 0.0001f) goto fail;

    if(rohr_error_check(rohr_physics_joint_anchor_remove(anchor_a.result.value)) ||
            !rohr_entity_alive_check(joint.result.value)) goto fail;
    if(joints[rohr_entity_index_get(joint.result.value).result.value].a != ENTITY_INVALID) goto fail;
    if(rohr_error_check(rohr_entity_delete(joint.result.value))) goto fail;

    anchor_a = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body_a.result.value, .local_offset = (Vec2D){1.0f, 0.0f}});
    anchor_b = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body_b.result.value, .local_offset = (Vec2D){-1.0f, 0.0f}});
    joint = rohr_entity_add();
    if(rohr_error_check(anchor_a) || rohr_error_check(anchor_b) || rohr_error_check(joint) ||
            rohr_error_check(rohr_physics_joint_description_set(joint.result.value, (JointDescription){.type = JOINT_WELD, .anchor_a = anchor_a.result.value, .anchor_b = anchor_b.result.value}))) goto fail;
    {
        EntityIndexResult joint_index = rohr_entity_index_get(joint.result.value);
        if(rohr_error_check(joint_index) || joints[joint_index.result.value].type != JOINT_WELD ||
                rohr_error_check(rohr_physics_joint_description_set(joint.result.value, (JointDescription){.type = JOINT_SPRING, .anchor_a = anchor_a.result.value, .anchor_b = anchor_b.result.value, .rest_length = 2.0f, .rest_length_explicit = true, .stiffness = 8.0f, .damping = 0.5f})) || joints[joint_index.result.value].type != JOINT_SPRING ||
                joints[joint_index.result.value].rest_length != 2.0f) goto fail;
    }
    if(rohr_error_check(rohr_entity_delete(body_a.result.value)) ||
            !rohr_entity_alive_check(joint.result.value) ||
            !rohr_error_check(rohr_physics_joint_anchor_local_position_get(anchor_a.result.value))) goto fail;
    rohr_engine_stop();
    return 0;

fail:
    rohr_engine_stop();
    return 1;
}
