/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STACK_COUNT 20
#define STACK_BODY_MAX 22

typedef struct StackBody {
    Entity entity;
    EntityIndex index;
    Position position;
    Velocity velocity;
    double angle, spin, hx, hy;
    double cosine, sine, extent_x, extent_y;
} StackBody;

typedef struct StackFixture {
    StackBody bodies[STACK_BODY_MAX];
    Position settled_positions[STACK_BODY_MAX];
    unsigned count;
    uint64_t substeps, pre_settling_samples, post_settling_samples;
    double time, depth, pre_linear, pre_angular, post_linear, post_angular;
    double pre_drift, pre_angle, settled_drift, energy_peak;
    bool struck, valid;
} StackFixture;

static bool stack_result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr,"%s\n",rohr_error_message_get(result));
    return false;
}

static bool stack_box_add(StackFixture *f, Position p, Velocity v, bool floor) {
    if(f->count==STACK_BODY_MAX) return false;
    EntityResult made=rohr_entity_add();
    if(rohr_error_check(made)) return false;
    Entity e=made.result.value;
    StackBody *b=&f->bodies[f->count++];
    *b=(StackBody){.entity=e,.index=rohr_entity_index_get(e).result.value,
        .position=p,.velocity=v,.hx=floor?500:5,.hy=5,
        .cosine=1,.extent_x=floor?500:5,.extent_y=5};
    if(!stack_result_check(rohr_physics_position_set(e,p)) ||
            !stack_result_check(rohr_physics_orientation_set(e,0)) ||
            !stack_result_check(rohr_physics_angular_velocity_set(e,0)) ||
            !stack_result_check(rohr_physics_hitbox_set(e,rohr_math_square_create(b->hx*2,10))) ||
            !stack_result_check(rohr_physics_mass_set(e,1)) ||
            !stack_result_check(rohr_physics_velocity_set(e,v)) ||
            !stack_result_check(rohr_physics_friction_set(e,.8)) ||
            !stack_result_check(rohr_physics_restitution_set(e,0)) ||
            !stack_result_check(rohr_entity_components_add(e,ROHR_COLLISION)) ||
            !stack_result_check(rohr_physics_collision_response_all_set(e)) ||
            !stack_result_check(floor?rohr_physics_static_set(e):rohr_physics_dynamic_set(e)) ||
            (!floor && !stack_result_check(rohr_physics_gravity_enable(e)))) return false;
    return true;
}

static bool stack_create_check(StackFixture *f) {
    *f=(StackFixture){.valid=true};
    if(!stack_result_check(rohr_physics_gravity_set((Acceleration){0,-9.8f})) ||
            !stack_result_check(rohr_physics_substeps_set(1)) ||
            !stack_result_check(rohr_physics_solver_iterations_set(8)) ||
            !stack_box_add(f,(Position){0,-5},(Velocity){0},true))return false;
    for(unsigned i=0;i<STACK_COUNT;++i)
        if(!stack_box_add(f,(Position){0,5+10*i},(Velocity){0},false))return false;
    return true;
}

static double stack_depth_get(const StackBody *a,const StackBody *b) {
    if(fabs((double)b->position.x-a->position.x)>a->extent_x+b->extent_x ||
            fabs((double)b->position.y-a->position.y)>a->extent_y+b->extent_y)return 0;
    double ac=a->cosine,as=a->sine,bc=b->cosine,bs=b->sine;
    double axes[4][2]={{ac,-as},{as,ac},{bc,-bs},{bs,bc}};
    double depth=DBL_MAX;
    for(unsigned i=0;i<4;++i) {
        double x=axes[i][0],y=axes[i][1];
        double ar=a->hx*fabs(x*ac-y*as)+a->hy*fabs(x*as+y*ac);
        double br=b->hx*fabs(x*bc-y*bs)+b->hy*fabs(x*bs+y*bc);
        double overlap=ar+br-fabs(((double)b->position.x-a->position.x)*x+
            ((double)b->position.y-a->position.y)*y);
        if(overlap<=0)return 0;
        depth=fmin(depth,overlap);
    }
    return depth;
}

static void stack_observe(double dt,void *context) {
    StackFixture *f=context;
    f->time+=dt;
    ++f->substeps;
    double energy=0;
    for(unsigned i=0;i<f->count;++i) {
        StackBody *b=&f->bodies[i];
        b->position=positions[b->index];b->velocity=velocities[b->index];
        b->angle=orientations[b->index];b->spin=angular_velocities[b->index];
        b->cosine=cos(b->angle*.017453292519943295);
        b->sine=sin(b->angle*.017453292519943295);
        b->extent_x=b->hx*fabs(b->cosine)+b->hy*fabs(b->sine);
        b->extent_y=b->hx*fabs(b->sine)+b->hy*fabs(b->cosine);
        if(!isfinite(b->position.x) || !isfinite(b->position.y) ||
                !isfinite(b->velocity.x) || !isfinite(b->velocity.y) ||
                !isfinite(b->angle) || !isfinite(b->spin))f->valid=false;
        if(i==0) {
            if(b->position.x!=0 || b->position.y!=-5 || b->angle!=0 ||
                    b->velocity.x!=0 || b->velocity.y!=0 || b->spin!=0)f->valid=false;
            continue;
        }
        if(fabs(b->position.x)+8>500 || b->position.y<4.9)f->valid=false;
        double speed=hypot(b->velocity.x,b->velocity.y),omega=b->spin*.017453292519943295;
        energy+=.5*speed*speed+100.0/12*omega*omega+9.8*b->position.y;
        if(!f->struck) {
            f->pre_drift=fmax(f->pre_drift,hypot(b->position.x,b->position.y-(5+10*(i-1))));
            f->pre_angle=fmax(f->pre_angle,fabs(b->angle));
            if(f->time>=2-1e-9) {
                if(!f->pre_settling_samples)f->settled_positions[i]=b->position;
                Position baseline=f->settled_positions[i];
                f->settled_drift=fmax(f->settled_drift,
                    hypot((double)b->position.x-baseline.x,(double)b->position.y-baseline.y));
                f->pre_linear=fmax(f->pre_linear,speed);
                f->pre_angular=fmax(f->pre_angular,fabs(b->spin));
            }
        } else if(f->time>=85-1e-9) {
            f->post_linear=fmax(f->post_linear,speed);
            f->post_angular=fmax(f->post_angular,fabs(b->spin));
        }
    }
    /* Check every completed substep in the user-approved settling windows.
     * Keep the settled pose as a fixed baseline so slow sinking cannot hide
     * behind individually small per-substep displacements. */
    if(!f->struck && f->time>=2-1e-9)++f->pre_settling_samples;
    if(f->struck && f->time>=85-1e-9)++f->post_settling_samples;
    if(f->pre_linear>.0001 || f->pre_angular>.0001 || f->settled_drift>.0001 ||
            f->post_linear>.05 || f->post_angular>.5)f->valid=false;
    f->energy_peak=fmax(f->energy_peak,energy);
    double budget=19600+(f->struck?1250+1029:0);
    if(!isfinite(energy) || energy>budget*1.001)f->valid=false;
    for(unsigned i=0;i<f->count;++i)
        for(unsigned j=i+1;j<f->count;++j)
            f->depth=fmax(f->depth,stack_depth_get(&f->bodies[i],&f->bodies[j]));
    if(f->depth>.1)f->valid=false;
}

static bool stack_update_check(StackFixture *f,double dt) {
    double before=f->time;
    uint64_t steps=f->substeps,reported=0;
    Report r=rohr_system_physics_update(dt);
    for(size_t i=0;i<r.statistic_count;++i)
        if(r.statistics[i].report_id==ROHR_PHYSICS_REPORT_ID &&
                r.statistics[i].metric_id==ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
            reported=r.statistics[i].value;
    return !r.has_fatal && r.header.status!=ROHR_REPORT_FAILED && reported>0 &&
        reported==f->substeps-steps && fabs(f->time-before-dt)<1e-9;
}

static int stack_time_compare(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;
    return (x>y)-(x<y);
}

static bool stack_run_check(unsigned hz,bool stalls,bool performance) {
    StackFixture f;
    if(!stack_result_check(rohr_engine_start()) || !stack_create_check(&f))return false;
    physics_substep_observer_set(stack_observe,&f);
    double requested=0,samples[12000],delayed_max=0;
    unsigned count=0;
    bool short_delay=false,long_delay=false;
    const double end=90;
    while(requested<end && f.valid) {
        if(!f.struck && requested>=60-1e-10) {
            f.struck=true;
            if(!stack_box_add(&f,(Position){-30,105},(Velocity){50,0},false)) {f.valid=false;break;}
        }
        double dt=fmin(1.0/hz,end-requested);
        bool delayed=false;
        if(stalls && !short_delay && requested>=59.8-1e-10) {
            dt=.1;short_delay=true;delayed=true;
        } else if(stalls && !long_delay && requested>=60.2-1e-10) {
            dt=1;long_delay=true;delayed=true;
        }
        if(!f.struck)dt=fmin(dt,60-requested);
        clock_t start=clock();
        bool updated=stack_update_check(&f,dt);
        double cpu=1000.0*(clock()-start)/CLOCKS_PER_SEC;
        requested+=dt;
        if(delayed)delayed_max=fmax(delayed_max,cpu);
        else if(count<12000)samples[count++]=cpu;
        else f.valid=false;
        if(!updated || fabs(f.time-requested)>1e-9)f.valid=false;
    }
    physics_substep_observer_set(NULL,NULL);
    qsort(samples,count,sizeof(double),stack_time_compare);
    double p95=count?samples[(count-1)*95/100]:0;
    if(!f.struck || !f.pre_settling_samples || !f.post_settling_samples ||
            fabs(f.time-end)>1e-9 || (stalls && (!short_delay || !long_delay)))f.valid=false;
    if(performance && (delayed_max>50 || (hz==60 && p95>2)))f.valid=false;
    printf("%s hz=%u stalls=%d time=%.9f substeps=%llu depth=%.9g pre_drift=%.9g pre_angle=%.9g pre_linear=%.9g pre_angular=%.9g settled_drift=%.9g post_linear=%.9g post_angular=%.9g p95_ms=%.6f stall_ms=%.6f energy=%.9g\n",
        f.valid?"PASS":"FAIL",hz,stalls,f.time,(unsigned long long)f.substeps,f.depth,
        f.pre_drift,f.pre_angle,f.pre_linear,f.pre_angular,f.settled_drift,
        f.post_linear,f.post_angular,p95,delayed_max,f.energy_peak);
    fflush(stdout);
    rohr_engine_stop();
    return f.valid;
}

int main(int argc,char **argv) {
    bool performance=argc==2 && strcmp(argv[1],"--performance")==0;
    bool valid=true;
    const unsigned rates[]={30,60,120,60};
    for(unsigned i=0;i<4;++i)valid=stack_run_check(rates[i],i==3,performance)&&valid;
    return valid?0:1;
}
