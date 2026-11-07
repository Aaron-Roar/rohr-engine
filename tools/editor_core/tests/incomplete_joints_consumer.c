/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project.h"
#include "incomplete_joint_checks.h"

int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    for(unsigned path = 0; path < 4; ++path) {
        OK(rohr_engine_start());
        if(path == 0) {
            /* Generated compatibility wrappers now use the shared constructor,
             * including incomplete joints and their soft-anchor dependencies. */
            Drafts instance = {0}; OK(drafts_create(&instance, (Position){0}));
            assert(rohr_entity_alive_check(instance.source_instance));
            DraftJoints handles = {.handles = {
                {instance.joint_pin_0, instance.joint_pin_1, instance.joint_pin_2, instance.joint_pin_3},
                {instance.joint_weld_0, instance.joint_weld_1, instance.joint_weld_2, instance.joint_weld_3},
                {instance.joint_spring_0, instance.joint_spring_1, instance.joint_spring_2, instance.joint_spring_3}}};
            draft_checks(handles, instance.source_instance); drafts_destroy(&instance);
        } else {
            ObjectDescriptionResult file = {0};
            const ObjectDescription *description = rohr_object_get(objects.drafts);
            if(path == 2) {
                file = rohr_object_load("objects/project.rohr.json"); OK(file);
                ObjectDescriptionResult found = rohr_object_description_by_name_get(file.result.value, "Drafts"); OK(found);
                description = found.result.value;
            }
            ObjectSpawnDescription options = rohr_object_spawn_description_default_get();
            if(path == 3) options.flags = ROHR_ENTITY_OBJECT_PHYSICS;
            EntityResult instance = rohr_entity_object_description_add(description, options); OK(instance);
            draft_checks(draft_members(instance.result.value), instance.result.value);
            OK(rohr_entity_delete(instance.result.value));
            if(path == 2) OK(rohr_object_destroy(file.result.value));
        }
        assert(rohr_entity_alive_count_get() == 0);
        rohr_engine_stop();
    }
    return 0;
}
