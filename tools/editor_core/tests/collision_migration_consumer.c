/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project_objects.h"
#include <assert.h>
#include <stdio.h>
#define OK(call) assert(!rohr_error_check(call))
static void check(Entity root) {
    Entity bodies[3];
    const uint64_t cats[] = {1,1,2}, masks[] = {2,0,1};
    for(unsigned i = 0; i < 3; ++i) {
        char name[32]; snprintf(name, sizeof(name), "body%u", i);
        EntityResult body = rohr_entity_object_member_by_name_get(root, name); OK(body);
        bodies[i] = body.result.value;
    }
    for(unsigned i = 0; i < 3; ++i) for(unsigned j = 0; j < 3; ++j) if(i != j)
        assert(rohr_physics_collision_response_to_check(bodies[i], bodies[j]) ==
            ((cats[i] & masks[j]) != 0 && (cats[j] & masks[i]) != 0));
}
int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start());
    EntityResult generated = rohr_entity_object_add(objects.migration); OK(generated);
    check(generated.result.value);
    OK(rohr_entity_delete(generated.result.value));
    ObjectDescriptionResult doc = rohr_object_load("objects/project.rohr.json"); OK(doc);
    EntityResult loaded = rohr_entity_object_by_name_add(doc.result.value, "Migration"); OK(loaded);
    check(loaded.result.value);
    OK(rohr_entity_delete(loaded.result.value)); OK(rohr_object_destroy(doc.result.value));
    rohr_engine_stop();
    return 0;
}
