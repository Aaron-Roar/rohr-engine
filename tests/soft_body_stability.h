/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Goal 5 fixture shared with the sandbox preview. No test mutates from an observer.
 */
#ifndef ROHR_TEST_SOFT_BODY_STABILITY_H
#define ROHR_TEST_SOFT_BODY_STABILITY_H
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

static const Position sb_points[8]={{-50,-50},{50,-50},{50,50},{-50,50},{0,-75},{75,0},{0,75},{-75,0}};
static const unsigned sb_edges[14][2]={{0,1},{1,2},{2,3},{3,0},{0,2},{1,3},{0,4},{4,1},{1,5},{5,2},{2,6},{6,3},{3,7},{7,0}};
typedef struct SbFixture {
    Entity owner,nodes[8],beams[14],terrain,load,particle;
    EntityIndex ni[8];
    unsigned kind; /* 0 drop, 1 box, 2 node, 3 beam, 4 combined, 5 fold, 6 control */
    bool ramp,valid,loaded,launched;
    const char *stop_reason;
    double time,depth,energy_base,energy_added,energy_error,peak_speed,final_speed;
    double observer_cpu;
    uint64_t substeps,self_contacts,particle_contacts,load_contacts;
    Position previous[11];
    bool previous_valid[11],tunneled;
    unsigned depth_a,depth_b;
} SbFixture;
static bool sb_ok(EngineResult r) {
    if(!rohr_error_check(r))return true;
    fprintf(stderr,"%s\n",rohr_error_message_get(r));return false;
}
static EntityIndex sb_index(Entity e) {return rohr_entity_index_get(e).result.value;}
static bool sb_body(Entity *e,Position p,double w,double h,double m,bool fixed,double angle) {
    EntityResult r=rohr_entity_add();if(rohr_error_check(r))return false;*e=r.result.value;
    return sb_ok(rohr_physics_position_set(*e,p))&&sb_ok(rohr_physics_orientation_set(*e,angle))&&
        sb_ok(rohr_physics_hitbox_set(*e,rohr_math_square_create(w,h)))&&sb_ok(rohr_physics_mass_set(*e,m))&&
        sb_ok(rohr_physics_velocity_set(*e,(Velocity){0}))&&sb_ok(rohr_physics_friction_set(*e,.5))&&
        sb_ok(rohr_physics_restitution_set(*e,0))&&sb_ok(rohr_entity_components_add(*e,ROHR_COLLISION))&&
        sb_ok(fixed?rohr_physics_static_set(*e):rohr_physics_dynamic_set(*e))&&
        (fixed||sb_ok(rohr_physics_gravity_enable(*e)));
}
static double sb_energy(const SbFixture *f) {
    double value=0;
    for(unsigned i=0;i<10;++i) {
        Entity e=i<8?f->nodes[i]:i==8?f->load:f->particle;if(!e)continue;
        EntityIndex n=sb_index(e);double vx=velocities[n].x,vy=velocities[n].y;
        value+=.5*mass[n]*(vx*vx+vy*vy);
        if(i==8) {double w=angular_velocities[n]*.017453292519943295;value+=.5*80*(120*120+20*20)/12*w*w;}
        if(f->kind<5)value+=mass[n]*9.8*positions[n].y;
    }
    for(unsigned i=0;i<14;++i) {
        Position a=positions[f->ni[sb_edges[i][0]]],b=positions[f->ni[sb_edges[i][1]]];
        Position p=sb_points[sb_edges[i][0]],q=sb_points[sb_edges[i][1]];
        double dl=hypot(b.x-a.x,b.y-a.y)-hypot(q.x-p.x,q.y-p.y);value+=50*dl*dl;
    }
    return value;
}
static bool sb_create(SbFixture *f,unsigned kind,bool ramp) {
    *f=(SbFixture){.kind=kind,.ramp=ramp,.valid=true};
    if(!sb_ok(rohr_physics_gravity_set((Acceleration){0,-9.8}))||
        !sb_ok(rohr_physics_substeps_set(1))||!sb_ok(rohr_physics_solver_iterations_set(16)))return false;
    double angle=ramp?15:0,rad=angle*.017453292519943295;
    double cy=kind<5?75+105/cos(rad):0;
    /* Lowest collider measured along the terrain normal; owner remains upright. */
    if(kind<5&&!sb_body(&f->terrain,(Position){-10*sin(rad),-10*cos(rad)},4000,20,1,true,angle))return false;
    EntityResult owner=rohr_physics_soft_body_add((SoftBodyDescription){.position={0,cy}});
    if(rohr_error_check(owner))return false;
    f->owner=owner.result.value;
    for(unsigned i=0;i<8;++i) {
        EntityResult n=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.owner=f->owner,
            .position={sb_points[i].x,sb_points[i].y+cy},.mass=1,.radius=5,.friction=.5,.restitution=0,
            .collision_enabled=kind!=6,.collision_filter={.category=1,.responds_to=1}});
        if(rohr_error_check(n))return false;
        f->nodes[i]=n.result.value;f->ni[i]=sb_index(n.result.value);
        if(kind<5&&!sb_ok(rohr_physics_gravity_enable(n.result.value)))return false;
    }
    for(unsigned i=0;i<14;++i) {
        EntityResult b=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=f->owner,
            .node_a=f->nodes[sb_edges[i][0]],.node_b=f->nodes[sb_edges[i][1]],.stiffness=100,.damping=0,
            .friction=.5,.restitution=0,.collision={.enabled=i>=6&&kind!=6,.thickness=2,
                .filter={.category=1,.responds_to=1}}});
        if(rohr_error_check(b))return false;
        f->beams[i]=b.result.value;
    }
    if(kind==5) {
        if(!sb_ok(rohr_physics_velocity_set(f->nodes[5],(Velocity){-200,0}))||
            !sb_ok(rohr_physics_velocity_set(f->nodes[7],(Velocity){200,0})))return false;
    }
    if(kind==6&&!sb_ok(rohr_physics_position_set(f->nodes[6],(Position){0,80})))return false;
    f->energy_base=sb_energy(f);return true;
}
static bool sb_events(SbFixture *f,double time) {
    if((f->kind==1||f->kind==4)&&!f->loaded&&time>=8-1e-8) {
        double lo=DBL_MAX,hi=-DBL_MAX,top=-DBL_MAX;
        for(unsigned i=0;i<8;++i){Position p=positions[f->ni[i]];lo=fmin(lo,p.x-5);hi=fmax(hi,p.x+5);top=fmax(top,p.y+5);}
        double before=sb_energy(f);
        if(!sb_body(&f->load,(Position){(lo+hi)/2,top+30},120,20,80,false,0))return false;
        f->energy_added+=sb_energy(f)-before;f->loaded=true;
    }
    if(f->kind>=2&&f->kind<=4&&!f->launched&&time>=(f->kind==4?12:8)-1e-8) {
        Position target={-FLT_MAX,0};
        if(f->kind==2)for(unsigned i=0;i<8;++i){Position p=positions[f->ni[i]];if(p.x>target.x)target=p;}
        else for(unsigned i=6;i<14;++i) {
            Position a=positions[f->ni[sb_edges[i][0]]],b=positions[f->ni[sb_edges[i][1]]];
            Position p={(a.x+b.x)/2,(a.y+b.y)/2};if(p.x>target.x)target=p;
        }
        double before=sb_energy(f);
        ParticleDescription d=rohr_physics_particle_description_default_get();
        d.position=(Position){target.x+200,target.y};d.radius=5;d.mass_value=1;d.restitution=0;
        EntityResult p=rohr_physics_particle_add(d);if(rohr_error_check(p))return false;f->particle=p.result.value;
        if(!sb_ok(rohr_physics_velocity_set(f->particle,(Velocity){-10000,0}))||
            !sb_ok(rohr_physics_gravity_enable(f->particle)))return false;
        f->energy_added+=sb_energy(f)-before;f->launched=true;
    }
    return true;
}
/* Independent convex SAT, with double projection arithmetic. */
static double sb_depth(Shape a,Shape b) {
    if(!a.amount_of_vertices||!b.amount_of_vertices)return 0;
    double depth=DBL_MAX;Shape shapes[2]={a,b};
    for(unsigned k=0;k<2;++k)for(unsigned i=0;i<shapes[k].amount_of_vertices;++i) {
        Position p=shapes[k].vertices[i],q=shapes[k].vertices[(i+1)%shapes[k].amount_of_vertices];
        double nx=q.y-p.y,ny=p.x-q.x,length=hypot(nx,ny);if(length<1e-12)continue;nx/=length;ny/=length;
        double al=DBL_MAX,ah=-DBL_MAX,bl=DBL_MAX,bh=-DBL_MAX;
        for(unsigned j=0;j<a.amount_of_vertices;++j){double t=nx*a.vertices[j].x+ny*a.vertices[j].y;al=fmin(al,t);ah=fmax(ah,t);}
        for(unsigned j=0;j<b.amount_of_vertices;++j){double t=nx*b.vertices[j].x+ny*b.vertices[j].y;bl=fmin(bl,t);bh=fmax(bh,t);}
        double d=fmin(ah-bl,bh-al);if(d<=0)return 0;depth=fmin(depth,d);
    }
    return depth==DBL_MAX?0:depth;
}
static Shape sb_shape(Entity e) {
    EntityIndex i=sb_index(e);Shape s=hit_boxes[i];double a=orientations[i]*.017453292519943295,c=cos(a),sn=sin(a);
    for(unsigned j=0;j<s.amount_of_vertices;++j){Position p=s.vertices[j];s.vertices[j]=(Position){positions[i].x+c*p.x+sn*p.y,positions[i].y+c*p.y-sn*p.x};}
    return s;
}
static Shape sb_beam_shape(const SbFixture *f,unsigned edge) {
    Position a=positions[f->ni[sb_edges[edge][0]]],b=positions[f->ni[sb_edges[edge][1]]];
    double dx=b.x-a.x,dy=b.y-a.y,l=hypot(dx,dy);if(l<=10)return (Shape){0};dx/=l;dy/=l;
    /* Endpoint disks own the first/last five units of the surface. */
    a.x+=5*dx;a.y+=5*dy;b.x-=5*dx;b.y-=5*dy;
    return (Shape){.amount_of_vertices=4,.vertices={{a.x-dy,a.y+dx},{b.x-dy,b.y+dx},{b.x+dy,b.y-dx},{a.x+dy,a.y-dx}}};
}
/* Detect a center crossing a moving finite beam between completed substeps.
 * This checks trajectory topology independently of the engine's contact flags. */
static double sb_side(Position p,Position a,Position b) {
    return ((double)p.x-a.x)*(b.y-a.y)-((double)p.y-a.y)*(b.x-a.x);
}
static Position sb_lerp(Position a,Position b,double t) {return (Position){a.x+t*(b.x-a.x),a.y+t*(b.y-a.y)};}
static bool sb_crossed(Position p0,Position p1,Position a0,Position a1,Position b0,Position b1) {
    double before=sb_side(p0,a0,b0),after=sb_side(p1,a1,b1);
    if(before*after>=0)return false;
    double lo=0,hi=1;
    for(unsigned i=0;i<24;++i) {
        double t=(lo+hi)/2;
        double side=sb_side(sb_lerp(p0,p1,t),sb_lerp(a0,a1,t),sb_lerp(b0,b1,t));
        if(side*before>0)lo=t;else hi=t;
    }
    double t=(lo+hi)/2;Position p=sb_lerp(p0,p1,t),a=sb_lerp(a0,a1,t),b=sb_lerp(b0,b1,t);
    double dx=b.x-a.x,dy=b.y-a.y,length=hypot(dx,dy);
    if(length<=10)return false;
    double along=((p.x-a.x)*dx+(p.y-a.y)*dy)/length;
    return along>5&&along<length-5;
}
static void sb_observe(double dt,void *context) {
    clock_t start=clock();SbFixture *f=context;f->time+=dt;++f->substeps;
    Entity objects[11];for(unsigned i=0;i<8;++i)objects[i]=f->nodes[i];objects[8]=f->terrain;objects[9]=f->load;objects[10]=f->particle;
    Shape shape[19];for(unsigned i=0;i<11;++i)shape[i]=objects[i]?sb_shape(objects[i]):(Shape){0};
    for(unsigned i=0;i<8;++i)shape[11+i]=sb_beam_shape(f,i+6);
    if(f->kind!=6)for(unsigned a=0;a<19;++a)for(unsigned b=a+1;b<19;++b) {
        if(a>=8&&a<=10&&b>=8&&b<=10&&(!objects[a]||!objects[b]))continue;
        if(b>=11) {
            const unsigned *eb=sb_edges[b-11+6];
            if(a<8&&(a==eb[0]||a==eb[1]))continue;
            if(a>=11){const unsigned *ea=sb_edges[a-11+6];if(ea[0]==eb[0]||ea[0]==eb[1]||ea[1]==eb[0]||ea[1]==eb[1])continue;}
        }
        double d=sb_depth(shape[a],shape[b]);
        if(d>f->depth){f->depth=d;f->depth_a=a;f->depth_b=b;}
        if(d>0&&(a<8||a>=11)&&(b<8||b>=11))++f->self_contacts;
    }
    for(unsigned n=0;n<11;++n)if(objects[n]&&n!=8) {
        Position now=positions[sb_index(objects[n])];
        for(unsigned edge=6;edge<14;++edge) {
            unsigned a=sb_edges[edge][0],b=sb_edges[edge][1];
            if(n==a||n==b)continue;
            if(f->previous_valid[n]&&f->kind!=6&&sb_crossed(f->previous[n],now,
                f->previous[a],positions[f->ni[a]],f->previous[b],positions[f->ni[b]]))f->tunneled=true;
        }
    }
    for(unsigned i=0;i<8;++i) {
        if(f->particle&&rohr_physics_contact_check(f->particle,f->nodes[i]))++f->particle_contacts;
        if(f->load&&rohr_physics_contact_check(f->load,f->nodes[i]))++f->load_contacts;
    }
    for(unsigned i=6;i<14;++i) {
        if(f->particle&&rohr_physics_contact_check(f->particle,f->beams[i]))++f->particle_contacts;
        if(f->load&&rohr_physics_contact_check(f->load,f->beams[i]))++f->load_contacts;
    }
    for(unsigned i=0;i<11;++i)if(objects[i]) {f->previous[i]=positions[sb_index(objects[i])];f->previous_valid[i]=true;}
    double e=sb_energy(f),budget=f->energy_base+f->energy_added;
    double error=(e-budget)/fmax(1,fabs(budget));if(f->kind==6)error=fabs(error);
    f->energy_error=fmax(f->energy_error,error);f->final_speed=0;
    for(unsigned i=0;i<11;++i)if(objects[i]) {
        EntityIndex n=sb_index(objects[i]);double speed=hypot(velocities[n].x,velocities[n].y);
        f->peak_speed=fmax(f->peak_speed,speed);if(i<8)f->final_speed=fmax(f->final_speed,speed);
        if(!isfinite(positions[n].x)||!isfinite(positions[n].y)||!isfinite(speed)) {
            f->valid=false;f->stop_reason="invalid_state";
        }
    }
    if(!isfinite(e))f->stop_reason="invalid_energy";
    if(f->tunneled||!isfinite(e)||f->depth>.02||f->energy_error>.01)f->valid=false;
    f->observer_cpu+=1000.0*(clock()-start)/CLOCKS_PER_SEC;
}
#endif
