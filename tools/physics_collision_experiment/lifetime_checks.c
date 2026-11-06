/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include <assert.h>
#include <stdio.h>
#define OK(x) assert(!rohr_error_check(x))
static Entity particle_add(float x) {
    ParticleDescription d=rohr_physics_particle_description_default_get();
    d.position=(Position){x,0}; d.radius=.5f; d.velocity=(Velocity){1000,0};
    d.gravity_enabled=false;
    EntityResult p=rohr_physics_particle_add(d); OK(p); return p.result.value;
}
int main(void) {
    OK(rohr_engine_start());
    Entity particle=particle_add(-2);
    EntityIndexResult old_index=rohr_entity_index_get(particle); OK(old_index);
    EntityResult wall=rohr_entity_add(); OK(wall);
    OK(rohr_physics_hitbox_set(wall.result.value,rohr_math_square_create(.2f,10)));
    OK(rohr_physics_static_set(wall.result.value));
    OK(rohr_entity_components_add(wall.result.value,ROHR_COLLISION));
    Report r=rohr_physics_pipeline_substep(1.0/240); assert(!r.has_fatal);
    OK(rohr_entity_delete(particle)); OK(rohr_entity_delete(wall.result.value));
    /* Fresh handles may reuse the same slots. A standalone manual solve must
     * never consume predictive rows retained by a previous complete substep. */
    Entity replacement=particle_add(50);
    Entity other=particle_add(49);
    EntityIndexResult replacement_index=rohr_entity_index_get(replacement); OK(replacement_index);
    /* Arrange the reused slots opposite to the stale wall plane. The fresh
     * particles merely touch, so an ordinary manual solve must not move them. */
    if(replacement_index.result.value!=old_index.result.value) {
        OK(rohr_physics_position_set(replacement,(Position){49,0}));
        OK(rohr_physics_position_set(other,(Position){50,0}));
    }
    PositionResult before=rohr_physics_position_get(replacement); OK(before);
    rohr_physics_pipeline_step_begin(); rohr_physics_pipeline_substep_begin();
    r=rohr_physics_pipeline_contacts_gather(); assert(!r.has_fatal);
    r=rohr_physics_pipeline_joints_gather(); assert(!r.has_fatal);
    r=rohr_physics_pipeline_constraints_solve(4); assert(!r.has_fatal);
    PositionResult after=rohr_physics_position_get(replacement); OK(after);
    assert(before.result.value.x==after.result.value.x && before.result.value.y==after.result.value.y);
    rohr_engine_stop();
    OK(rohr_engine_start()); replacement=particle_add(70); (void)particle_add(69);
    r=rohr_physics_pipeline_constraints_solve(4); assert(!r.has_fatal);
    after=rohr_physics_position_get(replacement); OK(after); assert(after.result.value.x==70);
    rohr_engine_stop(); puts("PASS experiment manual-stage and entity lifetime isolation");
    return 0;
}
