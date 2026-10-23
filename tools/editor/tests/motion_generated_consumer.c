/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project_objects.h"
#include "motion_checks.h"
int main(void) {
    MOTION_OK(rohr_directory_working_set(rohr_directory_base_get()));
    for(unsigned path=0;path<3;++path) {
        MOTION_OK(rohr_engine_start());
        if(path==0) {
            Motion instance={0}; MOTION_OK(motion_create(&instance,(Position){0}));
            assert(instance.source_instance==ENTITY_INVALID);
            Entity handles[3][4]={{instance.body_0,instance.body_1,instance.body_2,instance.body_3},
                {instance.particle_0,instance.particle_1,instance.particle_2,instance.particle_3},
                {instance.node_0,instance.node_1,instance.node_2,instance.node_3}};
            for(unsigned kind=0;kind<3;++kind) for(unsigned mode=0;mode<4;++mode)
                motion_entity_check(handles[kind][mode],kind,mode);
            motion_entity_check(instance.driven_0,2,1);
            motion_entity_check(instance.driven_1,2,1);
            motion_destroy(&instance);
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
