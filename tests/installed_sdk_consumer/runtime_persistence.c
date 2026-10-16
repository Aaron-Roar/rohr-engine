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
        .owner = body,
        .position = (Position){0,0},
        .mass = 1,
        .radius = 1
    }));
    Entity b=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = body,
        .position = (Position){10,0},
        .mass = 2,
        .radius = 1
    }));
    Entity c=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = body,
        .position = (Position){0,10},
        .mass = 3,
        .radius = 1
    }));
    Entity beam=take(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = body,
        .node_a = a,
        .node_b = b,
        .stiffness = 10,
        .damping = 0.2f
    }));
    Entity area=take(rohr_physics_soft_body_area_add((SoftBodyAreaDescription){
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
    OK(rohr_game_state_file_save(path));
    rohr_engine_stop(); OK(rohr_engine_start());
    OK(rohr_game_state_file_load(path));
    SoftBodyResult restored=rohr_physics_soft_body_get(named("soft_body_1")); OK(restored);
    assert(restored.result.value.node_count==3 && restored.result.value.area_count==1);
    assert(rohr_physics_position_get(named("soft_body_1_node_2")).result.value.x==13);
    assert(rohr_physics_soft_body_beam_get(named("soft_body_1_beam_1")).result.value.rest_length==10);
    assert(!rohr_physics_soft_body_area_get(named("soft_body_1_area_1")).result.value.visible);
    assert(rohr_physics_force_description_get(named("force_1")).result.value.location.reference.beam.entity==named("soft_body_1_beam_1"));
    assert(rohr_physics_joint_anchors_get(named("soft_body_1_node_1")).result.value.count==1);
    rohr_engine_stop(); assert(remove(path)==0); return 0;
}
