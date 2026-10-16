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
    OK(rohr_entity_delete(soft.result.value));
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
