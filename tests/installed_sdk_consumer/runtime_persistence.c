/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include <rohr.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define OK(call) assert(!rohr_error_check(call))
static Entity take(EntityResult result) { OK(result); return result.result.value; }
static Entity named(const char *name) { return take(rohr_entity_by_name_get(name)); }
int main(void) {
    const char *path="installed_runtime_persistence.json";
    OK(rohr_engine_start());
    Entity body=take(rohr_physics_soft_body_add((SoftBodyDescription){0}));
    Entity a=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){0,0},
        .mass = 1,
        .radius = 1
    }));
    Entity b=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){10,0},
        .mass = 2,
        .radius = 1
    }));
    Entity c=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){0,10},
        .mass = 3,
        .radius = 1
    }));
    Entity beam=take(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = body,
        .node_a = a,
        .node_b = b,
        .stiffness = 10,
        .damping = 0.2f
    }));
    Entity area=take(rohr_physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = body,
        .geometry = (SoftBodyAreaGeometry){
        .outer={.nodes={a,b,c},.node_count=3}}
    }));
    OK(rohr_graphics_soft_body_area_visibility_set(area,false));
    OK(rohr_physics_position_set(b,(Position){13,1}));
    ForceDescription description={.target=body,.magnitude=8,
        .location={.reference={.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={.entity=beam,.anchor=ROHR_FORCE_BEAM_CENTER,.fraction=0.5f}}}};
    take(rohr_physics_force_add(description));
    JointAnchorIdResult anchor=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a, .local_offset = (Vec2D){0}}); OK(anchor);
    OK(rohr_physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){.node = c, .anchor = anchor.result.value}));
    OK(rohr_physics_angular_velocity_maximum_set(body,0));
    OK(rohr_physics_kinematic_driven_set(a));
    OK(rohr_graphics_layer_create((LayerDescription){.name="foreground",.value=12}));
    OK(rohr_graphics_layer_create((LayerDescription){.name="unused",.value=-4}));
    OK(rohr_graphics_layer_entity_name_set(body,"foreground"));
    OK(rohr_game_state_file_save(path));
    rohr_engine_stop(); OK(rohr_engine_start());
    OK(rohr_game_state_file_load(path));
    OK(rohr_physics_angular_velocity_maximum_get(named("soft_body")));
    assert(rohr_physics_angular_velocity_maximum_get(named("soft_body")).result.value==0);
    OK(rohr_physics_mass_set(named("node"),5));
    assert(rohr_physics_kinematic_driven_check(named("node")));
    assert(rohr_graphics_layer_name_get("unused").result.value==-4);
    OK(rohr_graphics_layer_name_set("foreground",25));
    assert(rohr_graphics_layer_entity_get(named("soft_body")).result.value==25);
    SoftBodyResult restored=rohr_physics_soft_body_get(named("soft_body")); OK(restored);
    assert(restored.result.value.node_count==3 && restored.result.value.area_count==1);
    assert(rohr_physics_position_get(named("node_1")).result.value.x==13);
    assert(rohr_physics_soft_body_beam_get(named("beam")).result.value.rest_length==10);
    assert(!rohr_physics_soft_body_area_get(named("area")).result.value.visible);
    assert(rohr_physics_force_description_get(named("force")).result.value.location.reference.beam.entity==named("beam"));
    assert(rohr_physics_joint_anchors_get(named("node")).result.value.count==1);
    rohr_engine_stop(); assert(remove(path)==0); return 0;
}
