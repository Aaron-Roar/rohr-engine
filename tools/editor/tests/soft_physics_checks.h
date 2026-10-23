/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef SOFT_PHYSICS_CHECKS_H
#define SOFT_PHYSICS_CHECKS_H
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define OK(call) assert((call).kind == ERROR_RESULT_VALUE)
static Entity soft_member(Entity parent, const char *name) {
    EntityResult e=rohr_entity_object_member_by_name_get(parent,name); OK(e); return e.result.value;
}
static void soft_node_check(Entity entity, unsigned i) {
    const uint64_t categories[]={5,6,4,5}, responses[]={0,2,4,0};
    SoftBodyNodeDescriptionResult result=rohr_physics_soft_body_node_description_get(entity); OK(result);
    SoftBodyNodeDescription d=result.result.value;
    assert(d.gravity_enabled==(i!=0) && d.collision_enabled==(i!=1));
    assert(fabsf(d.friction-(i==0?.2f:.6f))<.0001f && fabsf(d.restitution-.8f)<.0001f);
    assert(d.collision_filter.category==categories[i] && d.collision_filter.responds_to==responses[i]);
    assert(d.mass==(i==3?1:3) && d.velocity.x==2 && d.acceleration.y==5);
    assert(!d.static_body && !d.kinematic_driven);
}
static void soft_instance_check(Entity root) {
    Entity soft=soft_member(root,"cloth");
    for(unsigned i=0;i<4;++i) {
        char name[32]; snprintf(name,sizeof(name),"node_%u",i);
        soft_node_check(soft_member(soft,name),i);
    }
    SoftBodyBeamDescriptionResult beam=rohr_physics_soft_body_beam_description_get(soft_member(soft,"seam")); OK(beam);
    assert(!beam.result.value.collision.enabled && beam.result.value.friction==.35f);
}
#endif
