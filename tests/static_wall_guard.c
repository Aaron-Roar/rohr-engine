/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include "physics/rigid_body/static_wall_guard.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define OK(x) assert(!rohr_error_check(x))
static unsigned failures, cases;
static void verdict(const char *name, bool pass, double error) {
    ++cases; failures += !pass;
    printf("%s wall_guard case=%s error=%.9g\n", pass ? "PASS" : "FAIL", name, error);
    printf("completion condition=state target=%s reached=1 reason=endpoint_reached\n", name);
}
static void start(void) {
    OK(rohr_engine_start()); OK(rohr_physics_gravity_set((Acceleration){0}));
    OK(rohr_physics_substeps_set(4)); OK(rohr_physics_solver_iterations_set(4));
}
static Entity particle(bool node, Position p, Velocity v, float radius) {
    EntityResult e;
    if(node) {
        SoftBodyNodeDescription d = rohr_physics_soft_body_node_description_default_get();
        d.position = p; d.velocity = v; d.radius = radius; d.mass = 1;
        d.gravity_enabled = false; d.collision_enabled = true;
        e = rohr_physics_soft_body_node_add(d);
    } else {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.position = p; d.velocity = v; d.radius = radius; d.mass_value = 1;
        d.gravity_enabled = false;
        e = rohr_physics_particle_add(d);
    }
    OK(e); return e.result.value;
}
static Entity wall(Position p, float angle, float half_width, float height) {
    EntityResult e = rohr_entity_add(); OK(e);
    Shape shape = {.amount_of_vertices = 4,
        .vertices = {{-half_width,-height},{half_width,-height},{half_width,height},{-half_width,height}}};
    OK(rohr_physics_position_set(e.result.value, p));
    OK(rohr_physics_hitbox_set(e.result.value, shape));
    OK(rohr_physics_orientation_set(e.result.value, angle));
    OK(rohr_physics_mass_set(e.result.value, 1000));
    OK(rohr_physics_static_set(e.result.value));
    OK(rohr_entity_components_add(e.result.value, ROHR_COLLISION));
    return e.result.value;
}
static EntityIndex index_get(Entity e) {
    EntityIndexResult i = rohr_entity_index_get(e); OK(i); return i.result.value;
}
static bool step(bool manual, double dt) {
    if(!manual) return !rohr_physics_pipeline_substep(dt).has_fatal;
    rohr_physics_pipeline_substep_begin();
    rohr_physics_pipeline_accelerations_clear();
    Report r = rohr_physics_pipeline_forces_apply(); if(r.has_fatal) return false;
    r = rohr_physics_pipeline_integrate(dt); if(r.has_fatal) return false;
    r = rohr_physics_pipeline_contacts_gather(); if(r.has_fatal) return false;
    r = rohr_physics_pipeline_joints_gather(); if(r.has_fatal) return false;
    return !rohr_physics_pipeline_constraints_solve(4).has_fatal;
}
static void crossing(bool node, bool manual, float angle, float radius, float half,
        float speed, float sign, float offset, double dt) {
    start();
    Position p = rohr_math_vector_rotate((Position){-sign * 30, -3}, angle);
    p.x += offset; p.y += offset;
    Velocity v = rohr_math_vector_rotate((Velocity){sign * speed, speed * .1f}, angle);
    Entity e = particle(node, p, v, radius);
    Entity w = wall((Position){offset,offset}, angle, half, 1000);
    EntityIndex i = index_get(e);
    rohr_physics_pipeline_step_begin();
    bool valid = true, contact = false; double worst = 0;
    for(unsigned n = 0; n < 8; ++n) {
        valid &= step(manual, dt);
        Position relative = {positions[i].x-offset, positions[i].y-offset};
        Position local = rohr_math_vector_rotate(relative, -angle);
        worst = fmax(worst, sign * local.x + half + radius);
        valid &= isfinite(local.x) && isfinite(local.y);
        contact |= rohr_physics_contact_check(e,w);
    }
    char label[200];
    snprintf(label,sizeof(label),"cross_node%d_manual%d_angle%.0f_r%.2f_wall%.2f_v%.0f_sign%.0f_at%.0f_dt%.4f",
        node,manual,angle,radius,half,speed,sign,offset,dt);
    verdict(label,valid && contact && worst <= .1,worst);
    rohr_engine_stop();
}
static void correction(bool node, bool manual) {
    start();
    Entity e = particle(node,(Position){-.6f,0},(Velocity){0},.5f);
    (void)particle(false,(Position){-1,0},(Velocity){0},3);
    Entity w = wall((Position){0},0,.01f,10);
    rohr_physics_pipeline_step_begin();
    bool valid = step(manual,0);
    double depth = positions[index_get(e)].x + .51;
    verdict(node ? "node_correction" : "particle_correction",
        valid && depth <= .1 && rohr_physics_contact_check(e,w),depth);
    rohr_engine_stop();
}
static void boundary_cases(void) {
    for(unsigned mode = 0; mode < 7; ++mode) {
        start();
        Entity e = particle(false,(Position){-3,0},(Velocity){10000,0},.5f);
        Entity w = wall((Position){0},0,.1f,10);
        if(mode == 0) OK(rohr_physics_collision_response_none_set(e));
        if(mode == 1) OK(rohr_entity_components_delete(w,ROHR_COLLISION));
        if(mode == 2) OK(rohr_physics_position_set(e,(Position){-.6f,0}));
        if(mode == 3) {
            OK(rohr_physics_position_set(e,(Position){-.4f,0}));
            OK(rohr_physics_velocity_set(e,(Velocity){-10,0}));
        }
        if(mode == 4) {
            OK(rohr_physics_position_set(e,(Position){-.6f,0}));
            OK(rohr_physics_velocity_set(e,(Velocity){0,10}));
        }
        if(mode == 5) {
            OK(rohr_physics_position_set(e,(Position){-3,10.6f}));
        }
        if(mode == 6) {
            OK(rohr_physics_position_set(e,(Position){-3,10.25f}));
        }
        bool valid = step(true,.01);
        Position p = positions[index_get(e)];
        bool pass = (mode < 2 || mode == 5) ? p.x > 1 : mode == 4 ?
            fabs(p.x + .6) < .001 && p.y > 0 : p.x <= -.09;
        char label[40];snprintf(label,sizeof(label),"boundary_%u",mode);
        verdict(label,valid && pass,p.x);
        rohr_engine_stop();
    }
}
static void lifecycle(void) {
    start();
    Entity e = particle(false,(Position){-3,0},(Velocity){10000,0},.5f);
    Entity w = wall((Position){0},0,.1f,10);
    bool valid = step(true,.01);
    valid &= positions[index_get(e)].x < 0;
    OK(rohr_entity_delete(w));
    Entity replacement = wall((Position){50,50},0,.1f,1);
    (void)replacement;
    OK(rohr_physics_position_set(e,(Position){-3,0}));
    OK(rohr_physics_velocity_set(e,(Velocity){10000,0}));
    valid &= step(true,.01); valid &= positions[index_get(e)].x > 1;
    OK(rohr_physics_position_set(replacement,(Position){0}));
    OK(rohr_physics_position_set(e,(Position){-3,0}));
    OK(rohr_physics_velocity_set(e,(Velocity){10000,0}));
    valid &= step(true,.01);valid &= positions[index_get(e)].x < 0;
    verdict("delete_reuse_edit",valid,positions[index_get(e)].x);
    rohr_engine_stop();
}
static void corner_and_locks(void) {
    start();
    Entity e = particle(true,(Position){-3,-3},(Velocity){10000,10000},.5f);
    (void)wall((Position){0},0,.1f,10);
    (void)wall((Position){0},90,.1f,10);
    bool valid = step(true,.01);
    Position p = positions[index_get(e)];
    verdict("corner",valid && p.x <= -.49 && p.y <= -.49,fmax(p.x,p.y));
    rohr_engine_stop();
    start();
    e = particle(false,(Position){-3,-3},(Velocity){10000,10000},.5f);
    (void)wall((Position){0},0,.1f,10);
    OK(rohr_physics_axis_lock_description_set(e,(AxisLockDescription){.axis={1,1},.point_on_axis={0}}));
    valid = step(true,.01);p=positions[index_get(e)];Velocity v=velocities[index_get(e)];
    verdict("axis_lock",valid && p.x<0 && fabs(p.x-p.y)<.001 && fabs(v.x-v.y)<.001,p.x-p.y);
    rohr_engine_stop();
    start();
    Entity driver=particle(false,(Position){5,0},(Velocity){0},.5f);
    e=particle(false,(Position){-5,0},(Velocity){0},.5f);
    (void)wall((Position){0},0,.1f,10);
    OK(rohr_physics_transform_lock_description_set(e,(TransformLockDescription){.driver=driver,.lock_position=true}));
    valid=step(true,0);
    verdict("prescribed_transform_ownership",valid && positions[index_get(e)].x==positions[index_get(driver)].x,
        positions[index_get(e)].x-positions[index_get(driver)].x);
    rohr_engine_stop();
}
static void exact_edge_and_origin(void) {
    for(unsigned reverse=0;reverse<2;++reverse)for(unsigned escape=0;escape<2;++escape) {
        start();
        float sign=reverse?-1:1;
        Entity e=particle(false,(Position){sign*5,0},(Velocity){sign*(escape?1000:-1000),0},.2f);
        (void)wall((Position){0},0,5,10);
        Report r=rohr_physics_pipeline_integrate(.01);
        float x=positions[index_get(e)].x*sign;
        verdict("exact_edge",!r.has_fatal && x>=4.999f && (escape?x>5:true),x-5);
        rohr_engine_stop();
    }
    start();
    Entity e=particle(false,(Position){-30,0},(Velocity){10000,0},.5f);
    OK(rohr_physics_particle_origin_set(e,(Position){3,2}));
    OK(rohr_physics_orientation_set(e,37));
    Entity w=wall((Position){0},0,.01f,10);
    bool valid=step(true,.01);
    Position center=physics_particle_world_origin_by_index_get(index_get(e));
    verdict("offset_circle",valid && center.x<=-.51f+.001f && rohr_physics_contact_check(e,w),center.x+.51);
    rohr_engine_stop();
    start();
    e=particle(false,(Position){-3,0},(Velocity){10000,0},.5f);
    w=wall((Position){0},0,.01f,10);
    Shape concave={.amount_of_vertices=6,.vertices={{0,-5},{2,-5},{2,-3},{1,-3},{1,5},{0,5}}};
    OK(rohr_physics_hitbox_set(w,concave));
    valid=step(true,.01);
    verdict("concave_boundary",valid && positions[index_get(e)].x<=-.499f,positions[index_get(e)].x+.5);
    rohr_engine_stop();
}
int main(void) {
    for(unsigned n=0;n<2;++n) for(unsigned m=0;m<2;++m)
      for(unsigned a=0;a<2;++a) for(unsigned r=0;r<2;++r)
       for(unsigned fast=0;fast<2;++fast) for(unsigned reverse=0;reverse<2;++reverse)
        crossing(n,m,a?37:0,r?2.5f:.2f,r?5:.01f,fast?10000:1000,reverse?-1:1,
            a?10000:0,fast?.025:1.0/240);
    for(unsigned n=0;n<2;++n)for(unsigned m=0;m<2;++m)correction(n,m);
    boundary_cases();lifecycle();corner_and_locks();exact_edge_and_origin();
    printf("wall_guard_summary cases=%u failures=%u\n",cases,failures);
    return failures?1:0;
}
