/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define OK(value) do { if(rohr_error_check(value)) { fprintf(stderr,"setup error line %d\n",__LINE__); exit(2); } } while(0)
typedef struct Case {
    const char *name;
    bool rigid, moving, slide, ghost, correction, particle_pair, rotating, embedded, filtered;
    float speed, angle, thickness, restitution;
} Case;
static Entity body_add(bool particle, Position p, Velocity v, float width, float height, float mass_value, bool stationary, float angle, float restitution) {
    Entity entity;
    if(particle) {
        ParticleDescription d=rohr_physics_particle_description_default_get();
        d.position=p; d.velocity=v; d.radius=width*.5f; d.rigid_vertices=32;
        d.mass_value=mass_value; d.static_body=stationary; d.gravity_enabled=false;
        d.friction=0; d.restitution=restitution;
        EntityResult r=rohr_physics_particle_add(d); OK(r); entity=r.result.value;
    } else {
        EntityResult r=rohr_entity_add(); OK(r); entity=r.result.value;
        OK(rohr_physics_position_set(entity,p)); OK(rohr_physics_velocity_set(entity,v));
        OK(rohr_physics_hitbox_set(entity,rohr_math_square_create(width,height)));
        OK(rohr_physics_mass_set(entity,mass_value));
        if(stationary) OK(rohr_physics_static_set(entity)); else OK(rohr_physics_dynamic_set(entity));
        OK(rohr_entity_components_add(entity,ROHR_COLLISION));
        OK(rohr_physics_orientation_set(entity,angle));
        OK(rohr_physics_friction_set(entity,0)); OK(rohr_physics_restitution_set(entity,restitution));
    }
    return entity;
}
static EntityIndex index_get(Entity e) { EntityIndexResult r=rohr_entity_index_get(e); OK(r); return r.result.value; }
static float dot(Vec2D a,Vec2D b) { return a.x*b.x+a.y*b.y; }
static float near_face_get(EntityIndex i,Vec2D n,bool high) {
    Shape s=rohr_physics_shape_world_translate(hit_boxes[i],positions[i],orientations[i]);
    float result=high ? -INFINITY : INFINITY;
    for(unsigned k=0;k<s.amount_of_vertices;++k) result=high ? fmaxf(result,dot(s.vertices[k],n)) : fminf(result,dot(s.vertices[k],n));
    return result;
}
static unsigned case_run(Case c) {
    OK(rohr_engine_start()); OK(rohr_physics_substeps_set(4)); OK(rohr_physics_solver_iterations_set(4));
    experiment_counts_clear();
    Vec2D normal=rohr_math_vector_rotate((Vec2D){1,0},c.angle),tangent={-normal.y,normal.x};
    Position start=rohr_math_vector_rotate((Position){-c.thickness*.5f-2,0},c.angle);
    Velocity velocity={normal.x*c.speed,normal.y*c.speed};
    if(c.slide) { velocity.x+=tangent.x*100; velocity.y+=tangent.y*100; }
    if(c.ghost) { start=(Position){-3,.6f}; velocity=(Velocity){1000,1000}; }
    if(c.correction) { start=(Position){-1,0}; velocity=(Velocity){0}; }
    if(c.embedded) { start=(Position){-.1f,0}; velocity=(Velocity){0}; }
    if(c.rotating) { start=(Position){2,2}; velocity=(Velocity){0}; }
    Entity first=body_add(!c.rigid,start,velocity,1,1,1,c.rotating,c.rigid?c.angle:0,c.restitution);
    Velocity obstacle_velocity=c.moving ? (Velocity){-normal.x*c.speed,-normal.y*c.speed} : (Velocity){0};
    Entity second=body_add(c.particle_pair,(Position){0},obstacle_velocity,
        c.rotating?8:c.particle_pair?1:c.thickness,c.rotating?.2f:c.ghost?2:100,
        10,!c.moving&&!c.rotating, c.rotating?0:c.angle,c.restitution);
    if(c.rotating) OK(rohr_physics_angular_velocity_set(second,-21600));
    if(c.correction) (void)body_add(false,(Position){-2,0},(Velocity){0},6,10,1000,true,0,0);
    if(c.filtered) OK(rohr_physics_collision_response_set(first,ROHR_COLLISION_CATEGORY_NONE));
    if(c.filtered) OK(rohr_physics_collision_response_set(second,ROHR_COLLISION_CATEGORY_NONE));
    EntityIndex a=index_get(first),b=index_get(second);
    double initial_gap=near_face_get(b,normal,false)-near_face_get(a,normal,true);
    double max_depth=0,slide_error=0,bounce_error=0,travel_error=0; unsigned crossings=0,diagnostics=0;
    bool contact=false,complete=true;
    unsigned completed_steps=0;
    /* Exactly twelve configured substeps (0.05 s); keep every numerical
     * failure until the endpoint. Check geometry after each substep. */
    for(unsigned step=0;step<12;++step) {
        Report r=rohr_physics_pipeline_substep(1.0/240.0);
        if(r.has_fatal || r.header.status==ROHR_REPORT_FAILED) { complete=false; break; }
        diagnostics+=r.diagnostic_count;
        for(unsigned i=0;i<rohr_entity_alive_count_get();++i) {
            EntityResult e=rohr_entity_alive_at_get(i); OK(e); EntityIndex n=index_get(e.result.value);
            if(!isfinite(positions[n].x)||!isfinite(positions[n].y)||!isfinite(velocities[n].x)||!isfinite(velocities[n].y)||!isfinite(orientations[n])) complete=false;
        }
        if(!complete) break;
        ++completed_steps;
        contact|=rohr_physics_contact_check(first,second);
        if(!c.ghost&&!c.rotating&&!c.filtered&&!c.embedded) {
            crossings+=near_face_get(a,normal,false)>near_face_get(b,normal,true)+.001f;
            Shape sa=rohr_physics_shape_world_translate(hit_boxes[a],positions[a],orientations[a]);
            Shape sb=rohr_physics_shape_world_translate(hit_boxes[b],positions[b],orientations[b]);
            OverlapInfo overlap=rohr_physics_sat_overlap_get(sa,sb);
            if(overlap.detected) max_depth=fmax(max_depth,overlap.depth);
        }
        if(c.rotating) {
            Vec2D n=rohr_math_vector_rotate((Vec2D){0,1},orientations[b]);
            Vec2D relative={positions[a].x-positions[b].x,positions[a].y-positions[b].y};
            Vec2D along={n.y,-n.x};
            if(dot(relative,n)<-.6f && fabsf(dot(relative,along))<3.5f) ++crossings;
        }
    }
    Velocity relative={velocities[a].x-velocities[b].x,velocities[a].y-velocities[b].y};
    if(!c.ghost&&!c.correction&&!c.rotating&&!c.embedded&&!c.filtered)
        bounce_error=fabs(dot(relative,normal)+c.speed*(c.moving?2:1)*c.restitution);
    if(c.slide) slide_error=fabs(dot(positions[a],tangent)-5);
    if(!c.moving&&!c.ghost&&!c.correction&&!c.rotating&&!c.embedded&&!c.filtered) {
        double impact_time=initial_gap/c.speed;
        double expected=dot(start,normal)+initial_gap-c.restitution*c.speed*(.05-impact_time);
        travel_error=fabs(dot(positions[a],normal)-expected);
    }
    if(c.ghost||c.filtered) bounce_error=hypot(velocities[a].x-velocity.x,velocities[a].y-velocity.y);
    unsigned failures=(!complete)+(crossings>0)+(max_depth>.1)+(bounce_error>.1)+(slide_error>.1)+(travel_error>.1)+diagnostics;
    ExperimentCounts x=experiment_counts_get();
    printf("case name=%s complete=%d steps=%u target_steps=12 failures=%u crossing_samples=%u depth=%.9g bounce_error=%.9g slide_error=%.9g travel_error=%.9g contact=%d initial_gap=%.9g final_x=%.9g final_y=%.9g final_vx=%.9g final_vy=%.9g casts=%llu limits=%llu unsupported=%llu secondary=%llu\n",
        c.name,complete,completed_steps,failures,crossings,max_depth,bounce_error,slide_error,travel_error,contact,initial_gap,positions[a].x,positions[a].y,velocities[a].x,velocities[a].y,
        (unsigned long long)x.casts,(unsigned long long)(x.search_limits+x.capacity_limits),(unsigned long long)x.unsupported,(unsigned long long)x.secondary_risks);
    rohr_engine_stop();
    if(!complete) exit(2);
    return failures;
}
int main(void) {
    Case cases[]={
        {.name="particle_thin_bounce",.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="rigid_thin_bounce",.rigid=true,.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="particle_thick_bounce",.speed=10000,.thickness=10,.restitution=.8f},
        {.name="rigid_thick_bounce",.rigid=true,.speed=10000,.thickness=10,.restitution=.8f},
        {.name="particle_angled_bounce",.angle=37,.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="rigid_angled_bounce",.rigid=true,.angle=37,.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="particle_slide",.slide=true,.speed=1000,.thickness=.2f},
        {.name="rigid_slide",.rigid=true,.slide=true,.speed=1000,.thickness=.2f},
        {.name="particle_moving_wall",.moving=true,.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="rigid_moving_wall",.rigid=true,.moving=true,.speed=1000,.thickness=.2f,.restitution=.8f},
        {.name="particle_corner_miss",.ghost=true,.speed=1000,.thickness=.2f},
        {.name="rigid_corner_miss",.rigid=true,.ghost=true,.speed=1000,.thickness=.2f},
        {.name="particle_correction_crossing",.correction=true,.thickness=.2f},
        {.name="rigid_correction_crossing",.rigid=true,.correction=true,.thickness=.2f},
        {.name="rotating_obstacle",.rotating=true,.thickness=.2f},
        {.name="initial_embedding",.embedded=true,.thickness=.2f},
        {.name="filtered_crossing",.filtered=true,.speed=1000,.thickness=.2f},
        {.name="particle_pair_inventory",.particle_pair=true,.moving=true,.speed=1000,.thickness=1,.restitution=.8f}
    };
    unsigned failed=0;
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i) failed+=case_run(cases[i])!=0;
    printf("matrix complete=1 cases=%zu failed=%u\n",sizeof(cases)/sizeof(cases[0]),failed);
    return failed?1:0;
}
