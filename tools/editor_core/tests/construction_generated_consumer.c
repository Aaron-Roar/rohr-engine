/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project_objects.h"
#include "construction_checks.h"

int main(void) {
    REQUIRE(rohr_directory_working_set(rohr_directory_base_get()));
    ConstructionSnapshot baseline,motion,actual,translated;
    REQUIRE(rohr_engine_start());
    ConstructionScene direct=construction_direct((Position){0});
    construction_capture(&direct,&baseline,false);
    ConstructionScene offset=construction_direct((Position){200,300});
    construction_capture(&offset,&translated,false); REQUIRE(rohr_entity_delete(offset.e[C_ROOT]));
    construction_simulate(direct,&motion);
    construction_lifecycle(direct); rohr_engine_stop();
    for(int path=0;path<3;++path) {
        REQUIRE(rohr_engine_start());
        Construction instance={0};
        ObjectDescriptionResult file={0};
        Entity root;
        if(path==0) root=construction_take(rohr_entity_object_add(objects.construction));
        else if(path==1) {
            REQUIRE(construction_create(&instance,(Position){20,30})); root=instance.source_instance;
        } else {
            file=rohr_object_load("objects/project.rohr.json"); REQUIRE(file);
            root=construction_take(rohr_entity_object_by_name_add(file.result.value,"Construction"));
        }
        ConstructionScene scene=construction_members(root);
        construction_capture(&scene,&actual,false); construction_compare(&baseline,&actual,"generated configuration",false);
        construction_simulate(scene,&actual); construction_compare(&motion,&actual,"generated simulation",true);
        /* A translated instance owns separate mutable elements and references. */
        Construction second={0}; REQUIRE(construction_create(&second,(Position){220,330}));
        ConstructionScene other=construction_members(second.source_instance);
        ConstructionSnapshot before,after; construction_capture(&other,&before,false);
        construction_compare(&translated,&before,"translated convenience instance",false);
        construction_mutate(scene); REQUIRE(rohr_entity_delete(scene.e[C_ROOT]));
        construction_capture(&other,&after,false);
        construction_compare(&before,&after,"independent instance",false);
        construction_destroy(&second);
        if(path==2) REQUIRE(rohr_object_destroy(file.result.value));
        rohr_engine_stop();
    }
    return 0;
}
