/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project.h"
#include "motion_checks.h"
int main(void) {
    MOTION_OK(rohr_directory_working_set(rohr_directory_base_get()));
    for(unsigned path=0;path<3;++path) {
        MOTION_OK(rohr_engine_start());
        if(path==0) {
            EntityResult instance = rohr_object_create(project.motion); MOTION_OK(instance);
            motion_instance_check(instance.result.value);
            MOTION_OK(rohr_entity_delete(instance.result.value));
        } else {
            ObjectDescriptionResult file={0};
            const ObjectDescription *description=rohr_object_get(objects.motion);
            if(path==2) { file=rohr_object_load("objects/project.rohr.json"); MOTION_OK(file);
                ObjectDescriptionResult found=rohr_object_description_by_name_get(file.result.value,"Motion");
                MOTION_OK(found); description=found.result.value; }
            EntityResult a=rohr_entity_object_add(description),b=rohr_entity_object_add(description); MOTION_OK(a); MOTION_OK(b);
            motion_instance_check(a.result.value); motion_instance_check(b.result.value);
            MOTION_OK(rohr_entity_delete(a.result.value)); MOTION_OK(rohr_entity_delete(b.result.value));
            if(path==2) MOTION_OK(rohr_object_destroy(file.result.value));
        }
        assert(rohr_entity_alive_count_get()==0); rohr_engine_stop();
    }
    return 0;
}
