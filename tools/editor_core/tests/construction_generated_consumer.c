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
        ObjectDescriptionResult file={0};
        Entity root;
        if(path==0) root=construction_take(rohr_entity_object_add(objects.construction));
        else if(path==1) {
            ObjectCreateOptions options = rohr_object_create_options_default_get();
            root=construction_take(rohr_object_create_with_options(objects.construction,options));
        } else {
            file=rohr_object_load("objects/project.rohr.json"); REQUIRE(file);
            root=construction_take(rohr_entity_object_by_name_add(file.result.value,"Construction"));
        }
        ConstructionScene scene=construction_members(root);
        construction_capture(&scene,&actual,false); construction_compare(&baseline,&actual,"generated configuration",false);
        construction_simulate(scene,&actual); construction_compare(&motion,&actual,"generated simulation",true);
        /* A translated instance owns separate mutable elements and references. */
        ObjectCreateOptions options = rohr_object_create_options_default_get();
        options.offset=(Position){200,300};
        Entity second=construction_take(rohr_object_create_with_options(objects.construction,options));
        ConstructionScene other=construction_members(second);
        ConstructionSnapshot before,after; construction_capture(&other,&before,false);
        construction_compare(&translated,&before,"translated convenience instance",false);
        construction_mutate(scene); REQUIRE(rohr_entity_delete(scene.e[C_ROOT]));
        construction_capture(&other,&after,false);
        construction_compare(&before,&after,"independent instance",false);
        REQUIRE(rohr_entity_delete(second));
        if(path==2) REQUIRE(rohr_object_destroy(file.result.value));
        rohr_engine_stop();
    }
    return 0;
}
