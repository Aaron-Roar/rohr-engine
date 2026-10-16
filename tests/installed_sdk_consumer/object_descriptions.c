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
    assert(objects.mass_demo.automatic_body.mass > 0);
    EntityResult direct=rohr_entity_object_add(objects.mass_demo.automatic_body); OK(direct);
    EntityResult named=rohr_entity_object_by_name_add(objects.mass_demo,"automatic_body"); OK(named);
    assert(direct.result.value!=named.result.value);
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
