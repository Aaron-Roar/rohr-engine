/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project_objects.h"
#include "soft_physics_checks.h"
int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    for(unsigned path=0;path<3;++path) {
        OK(rohr_engine_start());
        if(path==0) {
            SoftPhysics instance={0}; OK(soft_physics_create(&instance,(Position){0}));
            Entity nodes[]={instance.node_0,instance.node_1,instance.node_2,instance.node_3};
            for(unsigned i=0;i<4;++i) soft_node_check(nodes[i],i);
            soft_physics_destroy(&instance);
        } else {
            ObjectDescriptionResult file={0};
            const ObjectDescription *description=rohr_object_get(objects.soft_physics);
            if(path==2) { file=rohr_object_load("objects/project.rohr.json"); OK(file);
                ObjectDescriptionResult found=rohr_object_description_by_name_get(file.result.value,"SoftPhysics");
                OK(found); description=found.result.value; }
            EntityResult a=rohr_entity_object_add(description),b=rohr_entity_object_add(description); OK(a); OK(b);
            soft_instance_check(a.result.value); soft_instance_check(b.result.value);
            OK(rohr_entity_delete(a.result.value)); OK(rohr_entity_delete(b.result.value));
            if(path==2) OK(rohr_object_destroy(file.result.value));
        }
        assert(rohr_entity_alive_count_get()==0); rohr_engine_stop();
    }
    return 0;
}
