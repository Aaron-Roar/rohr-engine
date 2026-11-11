/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "../../tools/editor_core/tests/runtime_fixture.h"
#include "project_objects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))
int main(void) {
    OK(rohr_engine_start());
    EntityResult metadata = rohr_entity_description_add((EntityDescription){.name = "sdk metadata"}); OK(metadata);
    EntityResult driver = rohr_entity_add(); OK(driver);
    OK(rohr_physics_axis_lock_description_set(metadata.result.value,
        (AxisLockDescription){.axis = {0, 2}, .point_on_axis = {3, 4}}));
    assert(rohr_physics_axis_lock_description_get(metadata.result.value).result.value.axis.y == 1);
    OK(rohr_physics_axis_lock_remove(metadata.result.value));
    OK(rohr_physics_transform_lock_description_set(metadata.result.value,
        (TransformLockDescription){.driver = driver.result.value,
            .local_angle = rohr_math_radians_to_degrees(PI_F), .lock_orientation = true}));
    TransformLockDescriptionResult lock = rohr_physics_transform_lock_description_get(metadata.result.value); OK(lock);
    assert(lock.result.value.local_angle == 180 && lock.result.value.driver == driver.result.value);
    assert(rohr_math_degrees_to_radians(lock.result.value.local_angle) == PI_F);
    OK(rohr_physics_transform_lock_remove(metadata.result.value));
    CollisionFilterDescription filter = rohr_physics_collision_filter_description_default_get();
    OK(rohr_physics_collision_filter_description_set(metadata.result.value, filter));
    assert(rohr_physics_collision_filter_description_get(metadata.result.value).result.value.responds_to == filter.responds_to);
    OK(rohr_entity_delete(driver.result.value));
    EntityDescriptionResult metadata_value = rohr_entity_description_get(metadata.result.value); OK(metadata_value);
    OK(rohr_entity_description_set(metadata.result.value, metadata_value.result.value));
    OK(rohr_entity_lifetime_description_set(metadata.result.value, (EntityLifetimeDescription){.tick_enabled = true}));
    assert(rohr_entity_lifetime_description_get(metadata.result.value).result.value.tick_enabled);
    assert(rohr_engine_lifecycle_update().header.status == ROHR_REPORT_COMPLETE);
    assert(!rohr_entity_alive_check(metadata.result.value));
    ObjectSpawnDescription spawn = rohr_object_spawn_description_default_get();
    assert(spawn.flags == ROHR_ENTITY_OBJECT_ALL);
    ObjectDescription source_object = {.id=1,.name="loads",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL};
    ObjectDescription receiving_body = {.id=2,.name="receiver",.kind=OBJECT_BODY,.mass=1,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS};
    ObjectDescription authored_force = {.id=3,.name="thrust",.kind=OBJECT_FORCE,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS,
        .force={.target={OBJECT_SOURCE_BODY,2,0},.magnitude=7}};
    ObjectDescription authored_torque = {.id=4,.name="turn",.kind=OBJECT_TORQUE,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_PHYSICS,
        .torque={.target={OBJECT_SOURCE_BODY,2,0},.mode=OBJECT_SOURCE_INITIAL}};
    ObjectDescription follower = {.id=5,.name="follower",.kind=OBJECT_PARTICLE,
        .mass=1,.radius=1,.parent=&source_object,.categories=ROHR_ENTITY_OBJECT_ALL,
        .position={5,6},.entity_settings={
            .gameplay_target={.kind=OBJECT_BODY,.id=2},
            .transform_driver={.kind=OBJECT_BODY,.id=2},.transform_position=true}};
    const ObjectDescription *source_children[] = {&receiving_body,&authored_force,&authored_torque,&follower};
    source_object.children=source_children;source_object.child_count=4;
    EntityResult standalone_follower=rohr_entity_create(follower); OK(standalone_follower);
    assert(!rohr_entity_components_check(standalone_follower.result.value,ROHR_TARGETABLE));
    assert(rohr_error_check(rohr_physics_transform_lock_description_get(standalone_follower.result.value)));
    assert(rohr_physics_position_get(standalone_follower.result.value).result.value.x==5);
    assert(rohr_error_check(rohr_entity_create(authored_force)));
    EntityResult complete_object=rohr_object_create(source_object); OK(complete_object);
    EntityResult created_follower=fixture_entity_get(complete_object.result.value,"follower"); OK(created_follower);
    EntityResult created_target=fixture_entity_get(complete_object.result.value,"receiver"); OK(created_target);
    assert(rohr_physics_transform_lock_description_get(created_follower.result.value).result.value.driver==created_target.result.value);
    OK(rohr_entity_delete(complete_object.result.value)); OK(rohr_entity_delete(standalone_follower.result.value));
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
    SoftBodyNodeDescription node_description = {
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = {1, 0}, .mass = 1, .radius = 1};
    EntityResult local = rohr_physics_soft_body_node_local_add(node_description); OK(local);
    EntityResult world = physics_soft_body_node_add(node_description); OK(world);
    assert(rohr_physics_position_get(local.result.value).result.value.x == 3);
    assert(rohr_physics_position_get(world.result.value).result.value.x == 1);
    EntityResult beam = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = soft.result.value, .node_a = local.result.value, .node_b = world.result.value}); OK(beam);
    EntityResult area = rohr_physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true, .owner = soft.result.value}); OK(area);
    SoftBodyDescriptionResult soft_config = rohr_physics_soft_body_description_get(soft.result.value); OK(soft_config);
    OK(rohr_physics_soft_body_description_set(soft.result.value, soft_config.result.value));
    SoftBodyNodeDescriptionResult node_config = rohr_physics_soft_body_node_description_get(local.result.value); OK(node_config);
    node_config.result.value.radius = 2;
    OK(rohr_physics_soft_body_node_description_set(local.result.value, node_config.result.value));
    SoftBodyBeamDescriptionResult beam_config = rohr_physics_soft_body_beam_description_get(beam.result.value); OK(beam_config);
    assert(!beam_config.result.value.collision.enabled && beam_config.result.value.rest_length_explicit);
    beam_config.result.value.collision = rohr_physics_soft_body_beam_collision_description_default_get();
    beam_config.result.value.collision.enabled = true;
    OK(rohr_physics_soft_body_beam_description_set(beam.result.value, beam_config.result.value));
    SoftBodyAreaDescriptionResult area_config = rohr_physics_soft_body_area_description_get(area.result.value); OK(area_config);
    area_config.result.value.visible = false;
    OK(rohr_physics_soft_body_area_description_set(area.result.value, area_config.result.value));
    ParticleDescription particle_description = rohr_physics_particle_description_default_get();
    particle_description.owner = soft.result.value;
    EntityResult particle = rohr_physics_particle_add(particle_description); OK(particle);
    ParticleDescriptionResult particle_value = rohr_physics_particle_description_get(particle.result.value); OK(particle_value);
    particle_value.result.value.static_body = true;
    particle_value.result.value.mass_value = 7;
    particle_value.result.value.velocity = (Velocity){2, 3};
    particle_value.result.value.gravity_enabled = true;
    OK(rohr_physics_particle_description_set(particle.result.value, particle_value.result.value));
    assert(rohr_physics_particle_description_get(particle.result.value).result.value.velocity.y == 3);
    OK(rohr_physics_particle_radius_set(particle.result.value, 4));
    assert(rohr_physics_hitbox_get(particle.result.value).result.value.vertices[0].x == 4);
    EntityResult particle_copy = physics_particle_add(particle_description); OK(particle_copy);
    assert(particle.result.value != particle_copy.result.value);
    JointAnchorIdResult local_anchor = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = local.result.value}); OK(local_anchor);
    JointAnchorIdResult particle_anchor = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = particle.result.value}); OK(particle_anchor);
    JointDescription joint_description = {.owner = soft.result.value, .anchor_a = local_anchor.result.value,
        .anchor_b = particle_anchor.result.value, .type = JOINT_PIN};
    EntityResult joint = rohr_physics_joint_add(joint_description); OK(joint);
    OK(physics_joint_add(joint_description));
    JointAnchorDescriptionResult anchor_config = rohr_physics_joint_anchor_description_get(local_anchor.result.value); OK(anchor_config);
    anchor_config.result.value.entity = world.result.value;
    OK(rohr_physics_joint_anchor_description_set(local_anchor.result.value, anchor_config.result.value));
    assert(rohr_physics_joint_description_get(joint.result.value).result.value.anchor_a == local_anchor.result.value);
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
    EntityResult generic = rohr_object_create(objects.mass_demo); OK(generic);
    EntityResult generic_body = fixture_entity_get(generic.result.value, "automatic_body"); OK(generic_body);
    EntityResult independent = rohr_entity_create(objects.mass_demo.automatic_body); OK(independent);
    assert(generic_body.result.value != independent.result.value);
    assert(rohr_physics_joint_anchors_get(generic_body.result.value).result.value.count == 1);
    assert(rohr_physics_joint_anchors_get(independent.result.value).result.value.count == 0);
    ObjectCreateBinding body_binding = {
        .description=rohr_object_get(objects.mass_demo.automatic_body), .entity=independent.result.value};
    EntityCreateOptions creation_options = rohr_entity_create_options_default_get();
    creation_options.bindings = &body_binding; creation_options.binding_count = 1;
    EntityResult independent_anchor = rohr_entity_create_with_options(
        objects.mass_demo.automatic_body.automatic_tip, creation_options); OK(independent_anchor);
    JointAnchorIdResult created_anchor = rohr_entity_object_anchor_get(independent_anchor.result.value); OK(created_anchor);
    assert(rohr_physics_joint_anchor_description_get(created_anchor.result.value).result.value.entity == independent.result.value);
    OK(rohr_entity_delete(independent_anchor.result.value));
    assert(rohr_physics_joint_anchors_get(independent.result.value).result.value.count == 0);
    assert(rohr_physics_mass_get(generic_body.result.value).result.value == objects.mass_demo.automatic_body.mass);
    OK(rohr_entity_delete(generic.result.value));
    assert(rohr_entity_alive_check(independent.result.value));
    OK(rohr_entity_delete(independent.result.value));
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
    OK(rohr_object_material_description_set(body.result.value, (ObjectMaterialDescription){.friction=0.4f,.restitution=0.6f}));
    ObjectMaterialDescriptionResult material = rohr_object_material_description_get(body.result.value); OK(material);
    assert(material.result.value.restitution == 0.6f);
    ObjectCollisionDescriptionResult collision = rohr_object_collision_description_get(body.result.value); OK(collision);
    OK(rohr_object_collision_description_set(body.result.value, collision.result.value));
    ObjectColorDescriptionResult color = rohr_object_color_description_get(body.result.value); OK(color);
    OK(rohr_object_color_description_set(body.result.value, color.result.value));
    EntityResult whole=rohr_entity_object_add(copy.result.value); OK(whole);
    OK(rohr_object_destroy(copy.result.value));
    EntityResult member=fixture_entity_get(whole.result.value,"automatic_body"); OK(member);
    assert(rohr_physics_mass_get(member.result.value).result.value==7);
    EntityResult camera=fixture_entity_get(whole.result.value,"main_camera"); OK(camera);
    OK(rohr_entity_object_camera_get(camera.result.value));
    EntityNameResult name=rohr_entity_name_get(whole.result.value); OK(name);
    OK(rohr_game_state_file_save("sdk_objects.json"));
    OK(rohr_entity_delete(whole.result.value)); OK(rohr_entity_delete(direct.result.value)); OK(rohr_entity_delete(named.result.value));
    OK(rohr_game_state_file_load("sdk_objects.json"));
    whole=rohr_entity_by_name_get(name.result.value.value); OK(whole);
    member=fixture_entity_get(whole.result.value,"automatic_body"); OK(member);
    assert(rohr_physics_mass_get(member.result.value).result.value==7);
    rohr_engine_stop();
    assert(remove("sdk_objects.json")==0);
    return 0;
}
