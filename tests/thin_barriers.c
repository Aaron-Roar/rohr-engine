/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Observation {
    Entity body, wall;
    EntityIndex index, wall_index;
    double time, impact, depth, vx, vy, restitution, speed;
    uint64_t substeps;
    bool valid, particle;
} Observation;

static bool result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
    return false;
}

static Shape rectangle_get(float x, float y) {
    return (Shape){.amount_of_vertices = 4,
        .vertices = {{-x,-y},{x,-y},{x,y},{-x,y}}};
}

static Entity box_create(Position p, Velocity v, Shape shape, float restitution, bool fixed) {
    EntityResult r = rohr_entity_add();
    if(rohr_error_check(r)) return 0;
    Entity e = r.result.value;
    if(!result_check(rohr_physics_position_set(e,p)) || !result_check(rohr_physics_hitbox_set(e,shape)) ||
            !result_check(rohr_physics_mass_set(e,1)) || !result_check(rohr_physics_velocity_set(e,v)) ||
            !result_check(rohr_physics_friction_set(e,0)) || !result_check(rohr_physics_restitution_set(e,restitution)) ||
            !result_check(rohr_entity_components_add(e,ROHR_COLLISION)) ||
            !result_check(rohr_physics_collision_response_all_set(e)) ||
            !result_check(fixed ? rohr_physics_static_set(e) : rohr_physics_dynamic_set(e))) return 0;
    return e;
}

static void observe(double dt, void *context) {
    Observation *o = context;
    o->time += dt;
    ++o->substeps;
    Position p = positions[o->index];
    Velocity v = velocities[o->index];
    double angle = orientations[o->index], spin = angular_velocities[o->index];
    double radians = angle * 3.14159265358979323846 / 180;
    double extent = o->particle ? .5 : .5 * (fabs(cos(radians)) + fabs(sin(radians)));
    double depth = fmax(0, (double)p.x + extent + (double)(float).05);
    o->depth = fmax(o->depth, depth);
    if(!isfinite(p.x) || !isfinite(p.y) || !isfinite(v.x) || !isfinite(v.y) ||
            !isfinite(angle) || !isfinite(spin) || depth > .001 ||
            fabs(p.y) + extent >= 20000 ||
            positions[o->wall_index].x != 0 || positions[o->wall_index].y != 0 ||
            orientations[o->wall_index] != 0 ||
            (double)v.x*v.x + (double)v.y*v.y > 1.001 * o->speed * o->speed ||
            fabs(spin) > .0001) o->valid = false;
    if(o->impact == 0 && v.x < o->vx * .5) o->impact = o->time;
    if(o->impact > 0) {
        double expected = -o->restitution * o->vx;
        double tolerance = o->restitution == 0 ? .0001 : fabs(expected) * .01;
        if(fabs(v.x - expected) > tolerance || fabs(v.y - o->vy) > fmax(.0001, fabs(o->vy) * .0001))
            o->valid = false;
    }
}

static int compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Circle/circle contacts have no polygon feature ID. They must retain the
 * same rebound target as mixed particle/wall contacts across iterations. */
static bool particle_rebound_check(void) {
    for(unsigned iterations = 1; iterations <= 8; iterations *= 2) {
        if(!result_check(rohr_engine_start()) || !result_check(rohr_physics_solver_iterations_set(iterations))) return false;
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.radius = .5f; d.friction = 0; d.restitution = .8f;
        d.position = (Position){-.49f,0}; d.velocity = (Velocity){100,0};
        EntityResult a = rohr_physics_particle_add(d);
        d.position.x = .5f; d.velocity.x = 0; d.static_body = true;
        EntityResult b = rohr_physics_particle_add(d);
        bool valid = !rohr_error_check(a) && !rohr_error_check(b);
        if(valid) {
            Report report = rohr_system_physics_update(0);
            EntityIndex index = rohr_entity_index_get(a.result.value).result.value;
            valid = !report.has_fatal && fabs(velocities[index].x + 80) < .001 &&
                fabs(velocities[index].y) < .0001 && angular_velocities[index] == 0;
        }
        rohr_engine_stop();
        if(!valid) return false;
    }
    puts("PASS particle_pair_rebound iterations=1,2,4,8");
    return true;
}

static bool run_check(unsigned kind, unsigned speed, unsigned oblique, unsigned bounce,
        unsigned hz, unsigned delays, bool performance) {
    Observation o = {.valid=true, .particle=kind != 0, .speed=speed,
        .restitution=bounce ? .8f : 0};
    o.vx = (float)(oblique ? speed / sqrt(2) : speed);
    o.vy = oblique ? o.vx : 0;
    Position p = {(float)(-.55 - o.vx * .25), (float)(-o.vy * .25)};
    Velocity v = {o.vx,o.vy};
    if(!result_check(rohr_engine_start()) || !result_check(rohr_physics_gravity_set((Acceleration){0})) ||
            !result_check(rohr_physics_substeps_set(1)) || !result_check(rohr_physics_solver_iterations_set(8))) return false;
    o.wall = box_create((Position){0}, (Velocity){0}, rectangle_get(.05,20000), o.restitution,true);
    if(kind) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.position=p; d.velocity=v; d.radius=.5; d.rigid_vertices=32;
        d.mass_value=1; d.friction=0; d.restitution=o.restitution;
        EntityResult r=rohr_physics_particle_add(d);
        if(!rohr_error_check(r)) o.body=r.result.value;
    } else o.body=box_create(p,v,rectangle_get(.5,.5),o.restitution,false);
    if(!o.body || !o.wall) { rohr_engine_stop(); return false; }
    o.index=rohr_entity_index_get(o.body).result.value;
    o.wall_index=rohr_entity_index_get(o.wall).result.value;
    physics_substep_observer_set(observe,&o);
    double requested=0, samples[512], stall=0;
    size_t count=0;
    bool first=false, second=false;
    while(requested < 2.5 && o.valid) {
        double dt=fmin(1.0/hz,2.5-requested);
        bool delayed=false;
        if(delays && !first && requested >= .2 - 1e-12) {
            dt=delays==1 ? .1 : 1; first=true; delayed=true;
        } else if(delays && !second && requested >= 1.3 - 1e-12) {
            dt=delays==1 ? 1 : .1; second=true; delayed=true;
        }
        uint64_t before=o.substeps;
        clock_t start=clock();
        Report r=rohr_system_physics_update(dt);
        double cpu=1000.0*(clock()-start)/CLOCKS_PER_SEC;
        requested+=dt;
        uint64_t reported=0;
        for(size_t i=0;i<r.statistic_count;++i)
            if(r.statistics[i].report_id==ROHR_PHYSICS_REPORT_ID &&
                    r.statistics[i].metric_id==ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
                reported=r.statistics[i].value;
        if(r.has_fatal || r.header.status==ROHR_REPORT_FAILED || reported != o.substeps-before ||
                reported==0 || fabs(o.time-requested)>1e-9) o.valid=false;
        if(delayed) stall=fmax(stall,cpu);
        else if(count < 512) samples[count++]=cpu;
        else o.valid=false;
    }
    physics_substep_observer_set(NULL,NULL);
    qsort(samples,count,sizeof(double),compare);
    double p95=count ? samples[(count-1)*95/100] : 0;
    if(o.impact < .249 || o.impact > .251 || (delays && (!first || !second)) ||
            (performance && (stall > 50 || (hz==60 && p95 > 2)))) o.valid=false;
    printf("%s kind=%u speed=%u oblique=%u bounce=%u hz=%u delays=%u time=%.9f impact=%.9f substeps=%llu depth=%.9g vx=%.9g vy=%.9g p95_ms=%.6f stall_ms=%.6f\n",
        o.valid?"PASS":"FAIL",kind,speed,oblique,bounce,hz,delays,o.time,o.impact,
        (unsigned long long)o.substeps,o.depth,velocities[o.index].x,velocities[o.index].y,p95,stall);
    fflush(stdout);
    rohr_engine_stop();
    return o.valid;
}

int main(int argc,char **argv) {
    bool performance=argc==2 && strcmp(argv[1],"--performance")==0;
    bool valid=particle_rebound_check();
    const unsigned speeds[]={100,1000,10000}, rates[]={30,60,120,60,60};
    for(unsigned kind=0;kind<2;++kind)
        for(unsigned s=0;s<3;++s)
            for(unsigned angle=0;angle<2;++angle)
                for(unsigned bounce=0;bounce<2;++bounce)
                    for(unsigned schedule=0;schedule<5;++schedule)
                        valid=run_check(kind,speeds[s],angle,bounce,rates[schedule],
                            schedule<3?0:schedule-2,performance) && valid;
    return valid?0:1;
}
