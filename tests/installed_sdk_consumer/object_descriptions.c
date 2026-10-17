/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_objects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))
int main(void) {
    OK(rohr_engine_start());
    ObjectDescription source_object = {.id=1,.name="loads",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL};
    ObjectDescription receiving_body = {.id=2,.name="receiver",.kind=OBJECT_BODY,.mass=1,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS};
    ObjectDescription authored_force = {.id=3,.name="thrust",.kind=OBJECT_FORCE,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS,
        .force={.target={OBJECT_SOURCE_BODY,2,0},.magnitude=7}};
    ObjectDescription authored_torque = {.id=4,.name="turn",.kind=OBJECT_TORQUE,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS,
        .torque={.target={OBJECT_SOURCE_BODY,2,0},.mode=OBJECT_SOURCE_INITIAL}};
    const ObjectDescription *source_children[] = {&receiving_body,&authored_force,&authored_torque};
    source_object.children=source_children;source_object.child_count=3;
    ObjectDescriptionResult source_copy=rohr_object_copy(source_object);OK(source_copy);
    ObjectDescriptionResult force_copy=rohr_object_description_by_name_get(source_copy.result.value,"thrust");OK(force_copy);
    ObjectDescriptionResult torque_copy=rohr_object_description_by_name_get(source_copy.result.value,"turn");OK(torque_copy);
    ObjectForceDescription force_value=force_copy.result.value->force;force_value.magnitude=9;
    OK(rohr_object_force_description_set(force_copy.result.value,force_value));
    ObjectTorqueDescription torque_value=torque_copy.result.value->torque;torque_value.magnitude=-5;
    OK(rohr_object_torque_description_set(torque_copy.result.value,torque_value));
    assert(authored_force.force.magnitude==7 && authored_torque.torque.magnitude==0);
    EntityResult pending=rohr_entity_object_add(source_copy.result.value);
    OK(pending);
    OK(rohr_entity_delete(pending.result.value));
    assert(ENTITY_NONE==ENTITY_INVALID);
    OK(rohr_object_destroy(source_copy.result.value));

    SoftBodyDescription soft_description = {.position = {2, 3}};
    EntityResult soft = rohr_physics_soft_body_add(soft_description); OK(soft);
    SoftBodyNodeDescription node_description = {.owner = soft.result.value,
        .position = {1, 0}, .mass = 1, .radius = 1};
    EntityResult local = rohr_physics_soft_body_node_local_add(node_description); OK(local);
    EntityResult world = physics_soft_body_node_add(node_description); OK(world);
    assert(rohr_physics_position_get(local.result.value).result.value.x == 3);
    assert(rohr_physics_position_get(world.result.value).result.value.x == 1);
    EntityResult beam = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = soft.result.value, .node_a = local.result.value, .node_b = world.result.value}); OK(beam);
    EntityResult area = rohr_physics_soft_body_area_add((SoftBodyAreaDescription){.owner = soft.result.value}); OK(area);
    ParticleDescription particle_description = rohr_physics_particle_description_default_get();
    particle_description.owner = soft.result.value;
    EntityResult particle = rohr_physics_particle_add(particle_description); OK(particle);
    EntityResult particle_copy = physics_particle_add(particle_description); OK(particle_copy);
    assert(particle.result.value != particle_copy.result.value);
    JointDescription joint_description = {.owner = soft.result.value, .a = local.result.value,
        .b = particle.result.value, .type = JOINT_PIN};
    EntityResult joint = rohr_physics_joint_add(joint_description); OK(joint);
    OK(physics_joint_add(joint_description));
    JointAnchorDescription anchor_description = {.entity = particle.result.value, .local_offset = {1, 0}};
    JointAnchorIdResult anchor = rohr_physics_joint_anchor_add(anchor_description); OK(anchor);
    OK(physics_joint_anchor_add(anchor_description));
    SoftBodyNodeAnchorPinDescription pin_description = {.owner = soft.result.value,
        .node = local.result.value, .anchor = anchor.result.value};
    SoftBodyNodeAnchorPinResult pin = rohr_physics_soft_body_node_to_anchor_pin_add(pin_description); OK(pin);
    OK(physics_soft_body_node_to_anchor_pin_add(pin_description));
    EntityResult force = rohr_physics_force_add((ForceDescription){.owner = soft.result.value,
        .target = particle.result.value}); OK(force);
    OK(physics_torque_description_set(force.result.value, (TorqueDescription){.owner = soft.result.value,
        .target = particle.result.value}));
    assert(rohr_physics_force_description_get(force.result.value).result.value.owner == soft.result.value);
    assert(rohr_physics_torque_description_get(force.result.value).result.value.owner == soft.result.value);
    OK(rohr_entity_delete(soft.result.value));
    assert(!rohr_entity_alive_check(particle.result.value) && !rohr_entity_alive_check(joint.result.value));
    assert(!rohr_entity_alive_check(pin.result.value.joint) && !rohr_entity_alive_check(force.result.value));
    assert(!rohr_entity_alive_check(local.result.value) && !rohr_entity_alive_check(beam.result.value));
    assert(objects.mass_demo.automatic_body.mass > 0);
    EntityResult direct=rohr_entity_object_add(objects.mass_demo.automatic_body); OK(direct);
    EntityResult named=rohr_entity_object_by_name_add(objects.mass_demo,"automatic_body"); OK(named);
    assert(direct.result.value!=named.result.value);
    OK(rohr_entity_owner_set(direct.result.value, named.result.value));
    assert(entity_owner_get(direct.result.value).result.value == named.result.value);
    OK(entity_owner_set(direct.result.value, ENTITY_INVALID));
    assert(rohr_entity_owner_get(direct.result.value).result.value == ENTITY_INVALID);
    assert(rohr_physics_mass_get(direct.result.value).result.value==objects.mass_demo.automatic_body.mass);
    ObjectDescriptionResult copy=rohr_object_copy(objects.mass_demo); OK(copy);
    ObjectDescriptionResult body=rohr_object_description_by_name_get(copy.result.value,"automatic_body"); OK(body);
    OK(rohr_object_mass_set(body.result.value,7));
    OK(rohr_object_material_set(body.result.value,0.4f,0.6f));
    EntityResult whole=rohr_entity_object_add(copy.result.value); OK(whole);
    OK(rohr_object_destroy(copy.result.value));
    EntityResult member=rohr_entity_object_member_by_name_get(whole.result.value,"automatic_body"); OK(member);
    assert(rohr_physics_mass_get(member.result.value).result.value==7);
    EntityResult camera=rohr_entity_object_member_by_name_get(whole.result.value,"main_camera"); OK(camera);
    OK(rohr_entity_object_camera_get(camera.result.value));
    EntityNameResult name=rohr_entity_name_get(whole.result.value); OK(name);
    OK(rohr_game_state_file_save("sdk_objects.json"));
    OK(rohr_entity_delete(whole.result.value)); OK(rohr_entity_delete(direct.result.value)); OK(rohr_entity_delete(named.result.value));
    OK(rohr_game_state_file_load("sdk_objects.json"));
    whole=rohr_entity_by_name_get(name.result.value.value); OK(whole);
    member=rohr_entity_object_member_by_name_get(whole.result.value,"automatic_body"); OK(member);
    assert(rohr_physics_mass_get(member.result.value).result.value==7);
    rohr_engine_stop();
    assert(remove("sdk_objects.json")==0);
    return 0;
}
