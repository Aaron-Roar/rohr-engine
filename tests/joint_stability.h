/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Shared fixture/independent measurements for three standalone joint tests.
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define JS_PI 3.14159265358979323846
#define JS_RAD (JS_PI / 180.0)
#define JS_OMEGA (20 * JS_PI)
#define JS_G 9.8

typedef struct JsBody {
    Entity entity;
    EntityIndex index;
    Position p;
    Velocity v;
    double angle, spin, hx, hy, inertia;
    bool touched, bounced;
    double rebound_time;
} JsBody;
typedef struct JsReference { double x, y, vx, vy; } JsReference;
typedef struct JsFixture {
    JsBody bodies[2], floor, pivot;
    unsigned count;
    Entity joint;
    bool impact, calibration, kicked, valid;
    double time, energy_ref, energy_scale, energy_error, anchor_error, angle_error;
    double depth, sweep_depth, ref_position_error, ref_velocity_error;
    double reference_difference, peak_length, peak_speed, momentum_error;
    double initial_px, initial_py, initial_l, impulse_x, impulse_y, impulse_l;
    double quiet_since, settling_time;
    JsReference reference, fine_reference;
    uint64_t substeps;
} JsFixture;
static volatile sig_atomic_t js_interrupted;
static void js_interrupt(int signal_number) { (void)signal_number; js_interrupted=1; }
static bool js_ok(EngineResult r) {
    if(!rohr_error_check(r))return true;
    fprintf(stderr,"%s\n",rohr_error_message_get(r));return false;
}
static bool js_body_create(JsBody *b,Position p,Velocity v,double spin,double hx,double hy,bool fixed,bool collision,unsigned category,unsigned response) {
    EntityResult made=rohr_entity_add();if(rohr_error_check(made))return false;
    EntityIndexResult index=rohr_entity_index_get(made.result.value);if(rohr_error_check(index))return false;
    *b=(JsBody){.entity=made.result.value,.index=index.result.value,.p=p,.v=v,
        .spin=spin,.hx=hx,.hy=hy,.inertia=(hx*hx+hy*hy)/3,.rebound_time=-1};
    Entity e=b->entity;
    return js_ok(rohr_physics_position_set(e,p)) && js_ok(rohr_physics_orientation_set(e,0)) &&
        js_ok(rohr_physics_velocity_set(e,v)) && js_ok(rohr_physics_angular_velocity_set(e,spin)) &&
        js_ok(rohr_physics_hitbox_set(e,rohr_math_square_create(2*hx,2*hy))) &&
        js_ok(rohr_physics_mass_set(e,1)) && js_ok(rohr_physics_friction_set(e,1)) &&
        js_ok(rohr_physics_restitution_set(e,1)) &&
        (collision ? (js_ok(rohr_entity_components_add(e,ROHR_COLLISION)) &&
          js_ok(rohr_physics_collision_filter_description_set(e,(CollisionFilterDescription){.category=category,.responds_to=response}))) :
          js_ok(rohr_physics_collision_filter_description_set(e,(CollisionFilterDescription){0}))) &&
        js_ok(fixed?rohr_physics_static_set(e):rohr_physics_dynamic_set(e)) &&
        (fixed || js_ok(rohr_physics_gravity_enable(e)));
}
static void js_read(JsBody *b) {
    b->p=positions[b->index];b->v=velocities[b->index];
    b->angle=orientations[b->index];b->spin=angular_velocities[b->index];
}
static double js_energy(const JsFixture *f) {
    double e=0;
    for(unsigned i=0;i<f->count;++i) {
        const JsBody *b=&f->bodies[i];
        e+=.5*((double)b->v.x*b->v.x+(double)b->v.y*b->v.y)+
            .5*b->inertia*b->spin*b->spin*JS_RAD*JS_RAD+JS_G*b->p.y;
    }
    if(JS_KIND==JOINT_SPRING) {
        double length=hypot((double)f->bodies[1].p.x-f->bodies[0].p.x,
            (double)f->bodies[1].p.y-f->bodies[0].p.y);
        e+=10*(length-100)*(length-100);
    }
    return e;
}
static double js_angular_momentum(const JsFixture *f) {
    double cx=0,cy=60,vx=0,vy=0;
    if(JS_KIND!=JOINT_PIN) {
        cx=((double)f->bodies[0].p.x+f->bodies[1].p.x)/2;
        cy=((double)f->bodies[0].p.y+f->bodies[1].p.y)/2;
        vx=((double)f->bodies[0].v.x+f->bodies[1].v.x)/2;
        vy=((double)f->bodies[0].v.y+f->bodies[1].v.y)/2;
    }
    double l=0;
    for(unsigned i=0;i<f->count;++i) {
        const JsBody *b=&f->bodies[i];
        l+=b->inertia*b->spin*JS_RAD-(b->p.x-cx)*(b->v.y-vy)+(b->p.y-cy)*(b->v.x-vx);
    }
    return l;
}
static JsReference js_derivative(JsReference q) {
    double r=hypot(q.x,q.y),k=r>0?-40*(r-100)/r:0;
    return (JsReference){q.vx,q.vy,k*q.x,k*q.y};
}
static JsReference js_offset(JsReference q,JsReference d,double h) {
    return (JsReference){q.x+h*d.x,q.y+h*d.y,q.vx+h*d.vx,q.vy+h*d.vy};
}
static void js_reference_advance(JsReference *q,double dt,double maximum) {
    unsigned n=(unsigned)ceil(dt/maximum);if(!n)return;
    double h=dt/n;
    for(unsigned i=0;i<n;++i) {
        JsReference a=js_derivative(*q),b=js_derivative(js_offset(*q,a,h/2)),
            c=js_derivative(js_offset(*q,b,h/2)),d=js_derivative(js_offset(*q,c,h));
        q->x+=h*(a.x+2*b.x+2*c.x+d.x)/6;q->y+=h*(a.y+2*b.y+2*c.y+d.y)/6;
        q->vx+=h*(a.vx+2*b.vx+2*c.vx+d.vx)/6;q->vy+=h*(a.vy+2*b.vy+2*c.vy+d.vy)/6;
    }
}
static bool js_create(JsFixture *f,bool impact,bool calibration) {
    *f=(JsFixture){.impact=impact,.calibration=calibration,.valid=true,.quiet_since=-1,.settling_time=-1};
    if(!js_ok(rohr_physics_gravity_set((Acceleration){0,-JS_G})) ||
            !js_ok(rohr_physics_substeps_set(1)) || !js_ok(rohr_physics_solver_iterations_set(8)))return false;
    if(impact && !js_body_create(&f->floor,(Position){0,-5},(Velocity){0},0,500000,5,true,true,1,2))return false;
    if(JS_KIND==JOINT_PIN) {
        f->count=1;
        if(!js_body_create(&f->pivot,(Position){0,60},(Velocity){0},0,1,1,true,false,0,0) ||
            !js_body_create(&f->bodies[0],(Position){50,60},(Velocity){0,-50*JS_OMEGA},3600,50,5,false,impact,2,1))return false;
        if(rohr_physics_collision_response_between_check(f->pivot.entity,f->bodies[0].entity))return false;
    } else {
        f->count=2;
        double radius=JS_KIND==JOINT_WELD?5:50;
        for(unsigned i=0;i<2;++i) {
            double x=i?radius:-radius;
            Velocity v=calibration?(Velocity){i?1:-1,0}:(Velocity){600,-800-JS_OMEGA*x};
            if(!js_body_create(&f->bodies[i],(Position){x,200},v,calibration?0:3600,5,5,false,impact,2,
                    JS_KIND==JOINT_WELD?1:3))return false;
        }
    }
    Entity a=JS_KIND==JOINT_PIN?f->pivot.entity:f->bodies[0].entity;
    Entity b=JS_KIND==JOINT_PIN?f->bodies[0].entity:f->bodies[1].entity;
    JointAnchorIdResult aa=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=a,
        .local_offset={JS_KIND==JOINT_WELD?5:0,0}});
    JointAnchorIdResult bb=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=b,
        .local_offset={JS_KIND==JOINT_PIN?-50:JS_KIND==JOINT_WELD?-5:0,0}});
    if(rohr_error_check(aa)||rohr_error_check(bb))return false;
    EntityResult joint=rohr_physics_joint_add((JointDescription){.type=JS_KIND,
        .anchor_a=aa.result.value,.anchor_b=bb.result.value,.stiffness=20,.damping=0,
        .rest_length=JS_KIND==JOINT_SPRING?100:0,.rest_length_explicit=true,.rest_angle_explicit=true});
    if(rohr_error_check(joint))return false;
    f->joint=joint.result.value;
    f->energy_ref=js_energy(f);f->energy_scale=fmax(1,fabs(f->energy_ref));
    for(unsigned i=0;i<f->count;++i) {f->initial_px+=f->bodies[i].v.x;f->initial_py+=f->bodies[i].v.y;}
    f->initial_l=js_angular_momentum(f);
    f->reference=(JsReference){100,0,calibration?2:0,calibration?0:-100*JS_OMEGA};
    f->fine_reference=f->reference;f->peak_length=100;f->peak_speed=1000;
    return true;
}
static bool js_kick(JsFixture *f) {
    unsigned i=JS_KIND==JOINT_SPRING?1:0;JsBody *b=&f->bodies[i];
    Vec2D j={1000,0};
    if(JS_KIND==JOINT_PIN) {
        double rx=b->p.x,ry=b->p.y-60,r=hypot(rx,ry);
        j=(Vec2D){1000*ry/r,-1000*rx/r};
    }
    double work;
    if(!f->impact && JS_KIND!=JOINT_SPRING) {
        double inertia=JS_KIND==JOINT_PIN?b->inertia+2500:2*(b->inertia+25);
        double rx=JS_KIND==JOINT_PIN?b->p.x:(b->p.x-f->bodies[1].p.x)/2;
        double ry=JS_KIND==JOINT_PIN?b->p.y-60:(b->p.y-f->bodies[1].p.y)/2;
        double torque=ry*j.x-rx*j.y,omega=js_angular_momentum(f)/inertia;
        work=torque*omega+torque*torque/(2*inertia);
        if(JS_KIND==JOINT_WELD)work+=j.x*(b->v.x+f->bodies[1].v.x)/2+
            j.y*(b->v.y+f->bodies[1].v.y)/2+((double)j.x*j.x+(double)j.y*j.y)/4;
        f->impulse_l+=torque;
    } else work=(double)j.x*b->v.x+(double)j.y*b->v.y+.5*((double)j.x*j.x+(double)j.y*j.y);
    f->energy_ref+=work;f->energy_scale=fmax(f->energy_scale,fabs(f->energy_ref));
    f->impulse_x+=j.x;f->impulse_y+=j.y;
    if(JS_KIND==JOINT_SPRING) {
        f->impulse_l+=(b->p.y-f->bodies[0].p.y)*j.x/2-(b->p.x-f->bodies[0].p.x)*j.y/2;
        f->reference.vx+=j.x;f->reference.vy+=j.y;
        f->fine_reference.vx+=j.x;f->fine_reference.vy+=j.y;
    }
    f->kicked=true;return js_ok(rohr_physics_impulse_apply(b->entity,j));
}
static double js_bottom(JsBody b) {
    return b.p.y-b.hx*fabs(sin(b.angle*JS_RAD))-b.hy*fabs(cos(b.angle*JS_RAD));
}
static double js_pair_depth(JsBody a,JsBody b) {
    double depth=INFINITY;
    JsBody bodies[]={a,b};
    for(unsigned i=0;i<2;++i)for(unsigned axis=0;axis<2;++axis) {
        double t=bodies[i].angle*JS_RAD+axis*JS_PI/2,x=cos(t),y=-sin(t);
        double aa=a.angle*JS_RAD,ba=b.angle*JS_RAD;
        double ar=a.hx*fabs(x*cos(aa)-y*sin(aa))+a.hy*fabs(x*sin(aa)+y*cos(aa));
        double br=b.hx*fabs(x*cos(ba)-y*sin(ba))+b.hy*fabs(x*sin(ba)+y*cos(ba));
        depth=fmin(depth,ar+br-fabs(((double)b.p.x-a.p.x)*x+((double)b.p.y-a.p.y)*y));
    }
    return fmax(0,depth);
}
static bool js_floor_contact_check(Entity body,Entity floor) {
    for(size_t i=0;i<physics_step_contact_constraints.count;++i) {
        const SystemContactConstraint *c=&physics_step_contact_constraints.values[i];
        if(c->type!=SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR||!c->value.rigid.responds||!c->value.rigid.solved)continue;
        if((c->value.rigid.first==body&&c->value.rigid.second==floor)||
           (c->value.rigid.second==body&&c->value.rigid.first==floor))return true;
    }
    return false;
}
static double js_sweep_depth(JsBody old,JsBody now,double dt) {
    /* Check a continuous linear-pose sweep; unwrapping with the incoming spin
     * also catches a complete revolution hidden by normalized end angles. */
    double da=remainder(now.angle-old.angle,360);
    da+=360*nearbyint((old.spin*dt-da)/360);
    unsigned n=(unsigned)ceil(fabs(da)*JS_RAD/.01);if(!n)n=1;
    if(n>100000)return INFINITY;
    double worst=0;
    for(unsigned i=0;i<=n;++i) {
        double t=(double)i/n;JsBody q=old;
        q.p.y=old.p.y+((double)now.p.y-old.p.y)*t;q.angle=old.angle+da*t;
        worst=fmax(worst,-js_bottom(q));
    }
    /* Bound sag between samples rather than missing a vertex's interior low. */
    double step=da*JS_RAD/n;
    return worst+hypot(old.hx,old.hy)*step*step/8;
}
static void js_observe(double dt,void *context) {
    JsFixture *f=context;f->time+=dt;++f->substeps;
    double speed=0,spin=0;
    for(unsigned i=0;i<f->count;++i) {
        JsBody *b=&f->bodies[i],old=*b;js_read(b);
        if(!isfinite(b->p.x)||!isfinite(b->p.y)||!isfinite(b->v.x)||!isfinite(b->v.y)||
                !isfinite(b->angle)||!isfinite(b->spin)) {f->valid=false;return;}
        speed=fmax(speed,hypot(b->v.x,b->v.y));spin=fmax(spin,fabs(b->spin));
        if(f->impact) {
            double bottom=js_bottom(*b);
            f->depth=fmax(f->depth,-bottom);
            f->sweep_depth=fmax(f->sweep_depth,js_sweep_depth(old,*b,dt));
            if(js_floor_contact_check(b->entity,f->floor.entity))b->touched=true;
            if(b->touched&&!b->bounced&&bottom>.01&&bottom>js_bottom(old)) {
                b->bounced=true;b->rebound_time=f->time;
                if(!js_ok(rohr_physics_restitution_set(b->entity,.1f)))f->valid=false;
            }
            if(fabs(b->p.x)+hypot(b->hx,b->hy)>500000)f->valid=false;
        }
    }
    if(JS_KIND==JOINT_PIN) {
        JsBody b=f->bodies[0];double a=b.angle*JS_RAD;
        f->anchor_error=fmax(f->anchor_error,hypot(b.p.x-50*cos(a),b.p.y+50*sin(a)-60));
        js_read(&f->pivot);
        if(f->pivot.p.x!=0||f->pivot.p.y!=60||f->pivot.angle!=0||f->pivot.spin!=0||f->pivot.v.x!=0||f->pivot.v.y!=0)f->valid=false;
    } else if(JS_KIND==JOINT_WELD) {
        JsBody a=f->bodies[0],b=f->bodies[1];double aa=a.angle*JS_RAD,ba=b.angle*JS_RAD;
        f->anchor_error=fmax(f->anchor_error,hypot(a.p.x+5*cos(aa)-b.p.x+5*cos(ba),
            a.p.y-5*sin(aa)-b.p.y-5*sin(ba)));
        f->angle_error=fmax(f->angle_error,fabs(remainder(b.angle-a.angle,360)));
    } else if(!f->impact) {
        js_reference_advance(&f->reference,dt,.0001);
        js_reference_advance(&f->fine_reference,dt,.00005);
        JsReference ref=f->fine_reference;
        if(f->calibration) {
            double w=sqrt(40.0);ref=(JsReference){100+2*sin(w*f->time)/w,0,2*cos(w*f->time),0};
        }
        f->peak_length=fmax(f->peak_length,hypot(ref.x,ref.y));
        f->peak_speed=fmax(f->peak_speed,hypot(ref.vx,ref.vy));
        f->ref_position_error=fmax(f->ref_position_error,hypot(
            (double)f->bodies[1].p.x-f->bodies[0].p.x-ref.x,(double)f->bodies[1].p.y-f->bodies[0].p.y-ref.y));
        f->ref_velocity_error=fmax(f->ref_velocity_error,hypot(
            (double)f->bodies[1].v.x-f->bodies[0].v.x-ref.vx,(double)f->bodies[1].v.y-f->bodies[0].v.y-ref.vy));
        f->reference_difference=fmax(f->reference_difference,fmax(
            hypot(f->reference.x-ref.x,f->reference.y-ref.y),hypot(f->reference.vx-ref.vx,f->reference.vy-ref.vy)));
    }
    double e=js_energy(f),error=(e-f->energy_ref)/f->energy_scale;
    f->energy_error=fmax(f->energy_error,f->impact?error:fabs(error));
    if(f->impact) {
        if(JS_KIND==JOINT_SPRING)f->depth=fmax(f->depth,js_pair_depth(f->bodies[0],f->bodies[1]));
        js_read(&f->floor);
        if(f->floor.p.x!=0||f->floor.p.y!=-5||f->floor.angle!=0||f->floor.spin!=0||f->floor.v.x!=0||f->floor.v.y!=0)f->valid=false;
    } else if(JS_KIND!=JOINT_PIN) {
        double px=f->bodies[0].v.x+f->bodies[1].v.x,py=f->bodies[0].v.y+f->bodies[1].v.y;
        f->momentum_error=fmax(f->momentum_error,hypot(px-f->initial_px-f->impulse_x,
            py-f->initial_py-f->impulse_y+2*JS_G*f->time)/fmax(1,hypot(f->initial_px,f->initial_py)+hypot(f->impulse_x,f->impulse_y)));
        f->momentum_error=fmax(f->momentum_error,
            fabs(js_angular_momentum(f)-f->initial_l-f->impulse_l)/fmax(1,fabs(f->initial_l)+fabs(f->impulse_l)));
    }
    if(f->time>=5 && speed<=.0001 && spin<=.0001) {
        if(f->quiet_since<0)f->quiet_since=f->time;
        if(f->time-f->quiet_since>=2 && f->settling_time<0)f->settling_time=f->quiet_since;
    } else f->quiet_since=-1;
    if(f->anchor_error>(JS_KIND==JOINT_PIN?.1:.01) || f->angle_error>.1 ||
            f->depth>.1||f->sweep_depth>.1||f->energy_error>.01||f->momentum_error>.01)f->valid=false;
}
static int js_compare(const void *a,const void *b) {double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
static bool js_run(unsigned hz,bool delayed,bool impact,bool calibration,bool performance) {
    JsFixture f;bool started=js_ok(rohr_engine_start());if(!started)return false;
    if(!js_create(&f,impact,calibration)) {rohr_engine_stop();return false;}
    physics_substep_observer_set(js_observe,&f);
    double requested=0,samples[4000],stall=0,end=calibration?2:30;unsigned count=0;
    bool short_delay=false,long_delay=false;
    while(requested<end && f.valid && !js_interrupted) {
        if(!calibration&&!f.kicked&&requested>=5-1e-10)if(!js_kick(&f)) {f.valid=false;break;}
        double dt=fmin(1.0/hz,end-requested);bool is_delay=false;
        if(delayed&&!short_delay&&requested>=4.8-1e-10) {dt=.1;short_delay=true;is_delay=true;}
        else if(delayed&&!long_delay&&requested>=5.2-1e-10) {dt=1;long_delay=true;is_delay=true;}
        if(!calibration&&!f.kicked)dt=fmin(dt,5-requested);
        uint64_t before=f.substeps,reported=0;clock_t begin=clock();
        Report r=rohr_system_physics_update(dt);
        double cpu=1000.0*(clock()-begin)/CLOCKS_PER_SEC;
        requested+=dt;
        if(is_delay)stall=fmax(stall,cpu);else if(count<4000)samples[count++]=cpu;else f.valid=false;
        for(size_t i=0;i<r.statistic_count;++i)
            if(r.statistics[i].report_id==ROHR_PHYSICS_REPORT_ID&&r.statistics[i].metric_id==ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)reported=r.statistics[i].value;
        if(r.has_fatal||r.header.status==ROHR_REPORT_FAILED||reported!=f.substeps-before||!reported||fabs(f.time-requested)>1e-9)f.valid=false;
    }
    physics_substep_observer_set(NULL,NULL);
    qsort(samples,count,sizeof(*samples),js_compare);double p95=count?samples[(count-1)*95/100]:0;
    if(fabs(f.time-end)>1e-9||js_interrupted||(!calibration&&!f.kicked)||
            (delayed&&(!short_delay||!long_delay)))f.valid=false;
    if(JS_KIND==JOINT_SPRING&&!impact) {
        double pe=calibration?.01:.01*f.peak_length,ve=calibration?.05:.01*f.peak_speed;
        if(f.ref_position_error>pe||f.ref_velocity_error>ve||f.reference_difference>.01*fmin(pe,ve))f.valid=false;
    }
    if(performance&&((hz==60&&p95>2)||stall>50))f.valid=false;
    if(impact) {
        bool rebound=false;
        for(unsigned i=0;i<f.count;++i)rebound|=f.bodies[i].bounced;
        if(!rebound)f.valid=false;
    }
    printf("%s kind=%d impact=%d calibration=%d hz=%u delayed=%d time=%.9f steps=%llu anchor=%.9g angle=%.9g depth=%.9g swept=%.9g energy_error=%.9g momentum_error=%.9g ref_position=%.9g ref_velocity=%.9g ref_convergence=%.9g p95_ms=%.6f stall_ms=%.6f settled_at=%.9g rebound_a=%.9g rebound_b=%.9g\n",
        f.valid?"PASS":"FAIL",JS_KIND,impact,calibration,hz,delayed,f.time,(unsigned long long)f.substeps,
        f.anchor_error,f.angle_error,f.depth,f.sweep_depth,f.energy_error,f.momentum_error,
        f.ref_position_error,f.ref_velocity_error,f.reference_difference,p95,stall,f.settling_time,
        f.bodies[0].rebound_time,f.count>1?f.bodies[1].rebound_time:-1);
    fflush(stdout);rohr_engine_stop();return f.valid;
}
int main(int argc,char **argv) {
    signal(SIGINT,js_interrupt);signal(SIGTERM,js_interrupt);
    bool performance=argc==2&&!strcmp(argv[1],"--performance"),valid=true;
    const unsigned rates[]={30,60,120,60};
    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<4&&!js_interrupted;++r)
        valid=js_run(rates[r],r==3,c,false,performance)&&valid;
    if(JS_KIND==JOINT_SPRING)for(unsigned r=0;r<3&&!js_interrupted;++r)
        valid=js_run(rates[r],false,false,true,performance)&&valid;
    return valid&&!js_interrupted?0:1;
}
