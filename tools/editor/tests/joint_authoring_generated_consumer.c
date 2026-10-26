/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project_objects.h"
#include <assert.h>
#include <math.h>
#define OK(x) do {assert(!rohr_error_check(x));} while(0)
static Entity member(Entity owner,const char *name) {
    EntityResult e=rohr_entity_object_member_by_name_get(owner,name);OK(e);return e.result.value;
}
static void physics_check(Entity a,Entity b) {
    PositionResult pa=rohr_physics_position_get(a),pb=rohr_physics_position_get(b);OK(pa);OK(pb);
    assert(fabsf(hypotf(pa.result.value.x-pb.result.value.x,pa.result.value.y-pb.result.value.y)-80)<.001f);
    OK(rohr_physics_velocity_set(a,(Velocity){10,0}));
    Report report=rohr_system_physics_update(.1);assert(report.header.status!=ROHR_REPORT_FAILED);
    pa=rohr_physics_position_get(a);pb=rohr_physics_position_get(b);OK(pa);OK(pb);
    /* The editor-only distance lock must not turn the runtime spring into a pin. */
    assert(fabsf(hypotf(pa.result.value.x-pb.result.value.x,pa.result.value.y-pb.result.value.y)-80)>.1f);
}
int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    for(unsigned path=0;path<3;++path) {
        OK(rohr_engine_start());
        if(path==0) {
            JointLockedSpring object={0};OK(joint_locked_spring_create(&object,(Position){0}));
            physics_check(object.first,object.second);joint_locked_spring_destroy(&object);
        } else {
            ObjectDescriptionResult file={0};const ObjectDescription *description=rohr_object_get(objects.joint_locked_spring);
            if(path==2) {file=rohr_object_load("objects/project.rohr.json");OK(file);
                ObjectDescriptionResult found=rohr_object_description_by_name_get(file.result.value,"JointLockedSpring");OK(found);description=found.result.value;}
            for(unsigned copy=0;copy<2;++copy) {
                EntityResult root=rohr_entity_object_add(description);OK(root);
                physics_check(member(root.result.value,"first"),member(root.result.value,"second"));OK(rohr_entity_delete(root.result.value));
            }
            if(path==2) OK(rohr_object_destroy(file.result.value));
        }
        assert(rohr_entity_alive_count_get()==0);rohr_engine_stop();
    }
    return 0;
}
