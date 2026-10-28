/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "core/engine_internal.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define OK(value) assert(!rohr_error_check(value))
static Velocity velocity_get(Entity entity) {
    EntityIndexResult index = rohr_entity_index_get(entity); OK(index);
    return velocities[index.result.value];
}
static void near(float actual, float expected) {
    if(fabsf(actual - expected) > .002f) {
        fprintf(stderr, "expected %g, got %g\n", expected, actual);
        assert(false);
    }
}
static Entity collider(int kind, Position position, Velocity velocity,
        RohrCollisionCategoryMask category, RohrCollisionCategoryMask response) {
    CollisionFilterDescription filter = {.category=category, .responds_to=response};
    EntityResult result;
    if(kind == 1) {
        result = rohr_physics_particle_add((ParticleDescription){
            .position=position, .velocity=velocity, .radius=1, .mass_value=2,
            .restitution=.5f, .friction=.8f, .collision_enabled=true,
            .collision_filter=filter});
    } else if(kind == 2) {
        result = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .position=position, .mass=2, .radius=1, .restitution=.5f,
            .collision_enabled=true, .collision_filter=filter});
        OK(result);
        OK(rohr_physics_velocity_set(result.result.value, velocity));
        OK(rohr_physics_gravity_disable(result.result.value));
    } else {
        result = rohr_entity_add(); OK(result);
        Entity e = result.result.value;
        OK(rohr_physics_position_set(e, position));
        OK(rohr_physics_hitbox_set(e, rohr_math_square_create(2,2)));
        OK(rohr_physics_mass_set(e, 2));
        OK(rohr_physics_dynamic_set(e));
        OK(rohr_physics_velocity_set(e, velocity));
        OK(rohr_physics_angular_velocity_set(e, 0));
        OK(rohr_physics_restitution_set(e, .5f));
        OK(rohr_physics_friction_set(e, .8f));
        OK(rohr_physics_collision_filter_description_set(e, filter));
    }
    OK(result); return result.result.value;
}
static void step(void) {
    Report report = rohr_system_physics_update(0);
    assert(report.header.status != ROHR_REPORT_FAILED);
}
static void pair(int first_kind, int second_kind, unsigned masks, bool reverse) {
    OK(rohr_engine_start());
    Entity a,b;
    if(reverse) {
        b=collider(second_kind,(Position){.75f,0},(Velocity){-2,0},2,masks&2?1:0);
        a=collider(first_kind,(Position){-.75f,0},(Velocity){10,0},1,masks&1?2:0);
    } else {
        a=collider(first_kind,(Position){-.75f,0},(Velocity){10,0},1,masks&1?2:0);
        b=collider(second_kind,(Position){.75f,0},(Velocity){-2,0},2,masks&2?1:0);
    }
    assert(rohr_physics_collision_response_to_check(a,b)==!!(masks&1));
    assert(rohr_physics_collision_response_to_check(b,a)==!!(masks&2));
    assert(rohr_physics_collision_response_between_check(a,b)==!!masks);
    assert(rohr_physics_collision_response_between_check(b,a)==!!masks);
    assert(!rohr_physics_collision_response_to_check(ENTITY_INVALID,b));
    step();
    assert(rohr_physics_overlap_check(a,b) && rohr_physics_overlap_check(b,a));
    OverlapInfo ab=rohr_physics_overlap_get(a,b),ba=rohr_physics_overlap_get(b,a);
    near(ab.normal.x,-ba.normal.x); near(ab.normal.y,-ba.normal.y);
    assert(rohr_physics_contact_check(a,b)==!!masks);
    assert(rohr_physics_contact_check(b,a)==!!masks);
    Velocity va=velocity_get(a);
    Velocity vb=velocity_get(b);
    Position pa=rohr_physics_position_get(a).result.value;
    Position pb=rohr_physics_position_get(b).result.value;
    if(!(masks&1)) {near(va.x,10);near(va.y,0);near(pa.x,-.75f);near(pa.y,0);near(rohr_physics_angular_velocity_get(a).result.value,0);}
    else {assert(va.x<9);assert(pa.x<-.75f);}
    if(!(masks&2)) {near(vb.x,-2);near(vb.y,0);near(pb.x,.75f);near(pb.y,0);near(rohr_physics_angular_velocity_get(b).result.value,0);}
    else {assert(vb.x>-1);assert(pb.x>.75f);}
    if(first_kind && second_kind) {
        if(masks==1) near(va.x,-8);
        if(masks==2) near(vb.x,16);
        if(masks==3) {near(va.x,1);near(vb.x,7);}
    }
    /* Mask changes end contact without ending a continuing geometric overlap. */
    OK(rohr_physics_collision_response_none_set(a));
    OK(rohr_physics_collision_response_none_set(b));
    OK(rohr_physics_position_set(a,(Position){-.75f,0}));
    OK(rohr_physics_position_set(b,(Position){.75f,0}));
    step();
    assert(rohr_physics_overlap_stayed_check(a,b));
    assert(!rohr_physics_contact_check(a,b));
    assert(rohr_physics_contact_exited_check(a,b)==!!masks);
    rohr_engine_stop();
}
static void beam_pair(unsigned masks, bool reverse) {
    OK(rohr_engine_start());
    Entity p=ENTITY_INVALID;
    if(reverse) p=collider(0,(Position){0,1.5f},(Velocity){3,-10},2,masks&2?1:0);
    Entity a=collider(2,(Position){-5,0},(Velocity){1,2},4,0);
    Entity b=collider(2,(Position){5,0},(Velocity){1,2},4,0);
    EntityResult beam=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .node_a=a,.node_b=b,.restitution=.5f,.friction=.8f,
        .collision={.enabled=true,.thickness=2,.filter={.category=1,.responds_to=masks&1?2:0}}});
    OK(beam);
    if(!reverse) p=collider(0,(Position){0,1.5f},(Velocity){3,-10},2,masks&2?1:0);
    OK(rohr_physics_collision_response_set(beam.result.value,masks&1?2:0));
    assert(rohr_physics_collision_response_to_check(beam.result.value,p)==!!(masks&1));
    step();
    assert(rohr_physics_overlap_check(beam.result.value,p));
    assert(rohr_physics_contact_check(beam.result.value,p)==!!masks);
    Velocity va=velocity_get(a);
    Velocity vb=velocity_get(b);
    Velocity vp=velocity_get(p);
    if(!(masks&1)) {
        near(va.x,1);near(va.y,2);near(vb.x,1);near(vb.y,2);
        near(rohr_physics_position_get(a).result.value.y,0);
        near(rohr_physics_position_get(b).result.value.y,0);
    } else assert(va.y<2 && vb.y<2);
    if(!(masks&2)) {
        near(vp.x,3);near(vp.y,-10);near(rohr_physics_position_get(p).result.value.y,1.5f);
        near(rohr_physics_angular_velocity_get(p).result.value,0);
    } else {assert(vp.y>-10);assert(vp.x<3);}
    rohr_engine_stop();
}
static void motion_and_material(bool kinematic, bool nonresponder) {
    OK(rohr_engine_start());
    Entity a=collider(1,(Position){-.75f,0},(Velocity){10,3},1,2);
    Entity b=collider(1,(Position){.75f,0},(Velocity){-2,-1},2,nonresponder?0:1);
    OK(rohr_physics_restitution_set(a,1));
    OK(rohr_physics_restitution_set(b,.25f));
    OK(rohr_physics_friction_set(b,0));
    if(kinematic) OK(rohr_physics_kinematic_driven_set(b));
    else OK(rohr_physics_static_set(b));
    step();
    Velocity va=velocity_get(a);
    near(va.x,kinematic?-5:-2.5f); near(va.y,3); // both materials, zero combined friction
    near(rohr_physics_position_get(b).result.value.x,.75f);
    near(velocity_get(b).x,-2); // stored velocity retained
    rohr_engine_stop();
}
static Velocity third_body(bool add_third) {
    OK(rohr_engine_start());
    Entity a=collider(1,(Position){0,0},(Velocity){0,0},1,2);
    Entity b=collider(1,(Position){1.5f,0},(Velocity){-10,0},2,0);
    Entity c=add_third?collider(1,(Position){-1.5f,0},(Velocity){10,0},4,1):ENTITY_INVALID;
    step();
    Velocity result=velocity_get(a);
    near(velocity_get(b).x,-10);
    if(add_third) {
        assert(!rohr_physics_collision_response_to_check(a,c));
        assert(rohr_physics_contact_check(a,c));
        assert(velocity_get(c).x<10);
    }
    rohr_engine_stop();return result;
}
static void off_center(bool kinematic, Velocity *linear, float *angular, Position *position) {
    OK(rohr_engine_start());
    Entity a=collider(0,(Position){-.75f,.65f},(Velocity){10,3},1,2);
    Entity b=collider(0,(Position){.75f,0},(Velocity){-2,-1},2,0);
    OK(rohr_physics_orientation_set(a,-10));
    OK(rohr_physics_orientation_set(b,20));
    OK(rohr_physics_angular_velocity_set(b,60));
    if(kinematic) OK(rohr_physics_kinematic_driven_set(b));
    step();
    near(velocity_get(b).x,-2); near(velocity_get(b).y,-1);
    near(rohr_physics_angular_velocity_get(b).result.value,60);
    near(rohr_physics_position_get(b).result.value.x,.75f);
    near(rohr_physics_position_get(b).result.value.y,0);
    *linear=velocity_get(a);
    *angular=rohr_physics_angular_velocity_get(a).result.value;
    *position=rohr_physics_position_get(a).result.value;
    assert(fabsf(*angular)>1); // off-center contact actually exercises rotational response
    rohr_engine_stop();
}
static void low_speed_rebound(void) {
    const float speeds[]={.5f,.01f,.001f},restitution[]={0,.1f,1};
    /* Reciprocal response selects the block solver; a nonresponding fixed
     * wall exercises the iterative path with the same physical result. */
    for(unsigned mode=0;mode<2;++mode)for(unsigned s=0;s<3;++s)for(unsigned r=0;r<3;++r) {
        OK(rohr_engine_start());
        OK(rohr_physics_substeps_set(1));
        OK(rohr_physics_solver_iterations_set(8));
        Entity body=collider(0,(Position){-1,0},(Velocity){speeds[s],0},1,2);
        Entity wall=collider(0,(Position){.99999f,0},(Velocity){0},2,mode?0:1);
        OK(rohr_physics_static_set(wall));
        OK(rohr_physics_friction_set(body,0));OK(rohr_physics_friction_set(wall,0));
        OK(rohr_physics_restitution_set(body,restitution[r]));
        OK(rohr_physics_restitution_set(wall,restitution[r]));
        step();
        float expected=-speeds[s]*restitution[r];
        if(fabsf(velocity_get(body).x-expected)>fmaxf(1e-6f,fabsf(expected)*.01f))
            fprintf(stderr,"low speed rebound mode=%u speed=%g restitution=%g expected=%g actual=%g\n",mode,speeds[s],restitution[r],expected,velocity_get(body).x);
        assert(fabsf(velocity_get(body).x-expected)<=fmaxf(1e-6f,fabsf(expected)*.01f));
        assert(fabsf(velocity_get(body).y)<1e-6f);
        assert(fabsf(rohr_physics_angular_velocity_get(body).result.value)<1e-4f);
        rohr_engine_stop();
    }
}
int main(void) {
    low_speed_rebound();
    for(int a=0;a<3;++a) for(int b=0;b<3;++b)
        for(unsigned masks=0;masks<4;++masks) for(int reversed=0;reversed<2;++reversed)
            pair(a,b,masks,reversed);
    for(unsigned masks=0;masks<4;++masks) for(int reversed=0;reversed<2;++reversed)
        beam_pair(masks,reversed);
    for(int k=0;k<2;++k) for(int n=0;n<2;++n) motion_and_material(k,n);
    Velocity isolated=third_body(false),with_third=third_body(true);
    near(isolated.x,with_third.x);near(isolated.y,with_third.y);
    Velocity va,vb; Position pa,pb; float wa,wb;
    off_center(false,&va,&wa,&pa); off_center(true,&vb,&wb,&pb);
    near(va.x,vb.x);near(va.y,vb.y);near(wa,wb);near(pa.x,pb.x);near(pa.y,pb.y);
    return 0;
}
