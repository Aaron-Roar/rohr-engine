/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef MOTION_CHECKS_H
#define MOTION_CHECKS_H
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define MOTION_OK(value) assert((value).kind == ERROR_RESULT_VALUE)
static Entity motion_member(Entity owner, const char *name) {
    EntityResult e = rohr_entity_object_member_by_name_get(owner, name);
    MOTION_OK(e); return e.result.value;
}
static void motion_entity_check(Entity e, unsigned kind, unsigned mode) {
    bool explicit_mode = mode == 1 || mode == 2;
    bool static_mode = mode == 2;
    float mass_value = mode == 3 && kind != 2 ? 0 : 7;
    assert(rohr_entity_components_check(e, ROHR_STATIC) == static_mode);
    assert(rohr_physics_kinematic_driven_check(e) == (!static_mode && (explicit_mode || mass_value == 0)));
    assert(rohr_physics_mass_get(e).result.value == mass_value);
    EntityInfoResult info = rohr_entity_info_get(e, ROHR_ENTITY_INFO_PHYSICS_VELOCITY | ROHR_ENTITY_INFO_PHYSICS_ACCELERATION); MOTION_OK(info);
    assert(info.result.value.velocity.x == 3);
    assert(info.result.value.acceleration.y == 5);
    MOTION_OK(rohr_entity_info_release(&info.result.value));
    assert(rohr_entity_components_check(e, ROHR_GRAVITY));
    CollisionFilterDescriptionResult f = rohr_physics_collision_filter_description_get(e); MOTION_OK(f);
    assert(f.result.value.category == 1 && f.result.value.responds_to == 1);
    if(kind == 1) {
        ParticleDescriptionResult d = rohr_physics_particle_description_get(e); MOTION_OK(d);
        assert(d.result.value.kinematic_driven == explicit_mode);
        MOTION_OK(rohr_physics_particle_description_set(e, d.result.value));
    } else if(kind == 2) {
        SoftBodyNodeDescriptionResult d = rohr_physics_soft_body_node_description_get(e); MOTION_OK(d);
        assert(d.result.value.kinematic_driven == explicit_mode);
        MOTION_OK(rohr_physics_soft_body_node_description_set(e, d.result.value));
    }
    MOTION_OK(rohr_physics_mass_set(e, 9));
    MOTION_OK(rohr_physics_dynamic_set(e));
    assert(rohr_physics_kinematic_driven_check(e) == explicit_mode);
    if(kind == 1) {
        ParticleDescription d = rohr_physics_particle_description_get(e).result.value;
        d.kinematic_driven = false; d.mass_value = 0;
        MOTION_OK(rohr_physics_particle_description_set(e, d));
        assert(!rohr_physics_particle_description_get(e).result.value.kinematic_driven);
        assert(rohr_physics_kinematic_driven_check(e));
    } else if(kind == 2) {
        SoftBodyNodeDescription d = rohr_physics_soft_body_node_description_get(e).result.value;
        d.kinematic_driven = false;
        MOTION_OK(rohr_physics_soft_body_node_description_set(e, d));
        assert(!rohr_physics_soft_body_node_description_get(e).result.value.kinematic_driven);
    } else MOTION_OK(rohr_physics_kinematic_driven_remove(e));
    MOTION_OK(rohr_physics_mass_set(e, 11));
    assert(!rohr_physics_kinematic_driven_check(e));
    /* Authored numeric locks never restrict direct runtime setters. */
    PositionResult original = rohr_physics_position_get(e); MOTION_OK(original);
    Position moved = original.result.value; moved.x += 17;
    MOTION_OK(rohr_physics_position_set(e, moved));
    PositionResult after = rohr_physics_position_get(e); MOTION_OK(after);
    assert(fabsf(after.result.value.x - moved.x) < 0.001f);
    MOTION_OK(rohr_physics_position_set(e, original.result.value));
}
static void motion_instance_check(Entity root) {
    Entity driven = motion_member(root, "driven_soft");
    motion_entity_check(motion_member(driven, "driven_0"), 2, 1);
    motion_entity_check(motion_member(driven, "driven_1"), 2, 1);
    const char *kinds[] = {"body", "particle", "node"};
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned mode = 0; mode < 4; ++mode) {
        char name[32]; snprintf(name, sizeof(name), "%s_%u", kinds[kind], mode);
        Entity owner = kind == 2 ? motion_member(root, "soft") : root;
        motion_entity_check(motion_member(owner, name), kind, mode);
    }
}
#endif
