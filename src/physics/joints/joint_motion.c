/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include "systems.h"
#include "math2d.h"
#include <math.h>
#include <float.h>
#include <string.h>

#define MOTION_RAD (3.14159265358979323846 / 180.0)
/* Scratch belongs to the current integration call, like the existing global
 * physics step. No entity handles or predictions survive motion_end. */
static unsigned short connections[MAX_ENTITIES];
static bool integrated[MAX_ENTITIES];
static bool motion_enabled;

void physics_joint_motion_enabled_set(bool enabled) { motion_enabled=enabled; }

bool physics_joint_motion_integrated_check(EntityIndex i) {
    return i < MAX_ENTITIES && integrated[i];
}
void physics_joint_motion_end(void) { memset(integrated, 0, sizeof(integrated)); }

static bool motion_body_check(EntityIndex i) {
    return entity_index_components_check(i, ROHR_MASS | ROHR_HIT_BOX) &&
        !entity_index_components_check(i, ROHR_PARTICLE) &&
        !entity_index_components_check(i, ROHR_SOFT_BODY) &&
        !entity_index_components_check(i, ROHR_AXIS_LOCK) &&
        !entity_index_components_check(i, ROHR_ANGLE_LOCK) &&
        !entity_index_components_check(i, ROHR_TRANSFORM_LOCK) &&
        !(i < angular_velocity_maximums_pool.capacity && angular_velocity_maximums_pool.used[i]) &&
        isfinite(mass[i]) && mass[i] > 0;
}
static bool motion_endpoints_check(Joint joint, EntityIndex *a, EntityIndex *b) {
    return entity_index_get(joint.a,a) && entity_index_get(joint.b,b) && *a != *b &&
        entity_index_alive_check(*a) && entity_index_alive_check(*b) &&
        !error_check(physics_joint_anchor_world_position_get(joint.anchor_a)) &&
        !error_check(physics_joint_anchor_world_position_get(joint.anchor_b));
}
static Vec2D motion_rotate(Vec2D r,double angle) {
    double c=cos(angle),s=sin(angle);
    return (Vec2D){r.x*c+r.y*s,-r.x*s+r.y*c};
}
static void motion_pose_set(PhysicsChange *c,Position com,double angle) {
    physics_change_orientation_set(c,angle);
    Vec2D offset=math_vector_rotate(physics_com_local_by_index_get(c->index),c->orientation);
    c->position=(Position){com.x-offset.x,com.y-offset.y};
}
static Acceleration motion_acceleration_get(EntityIndex i) {
    return (Acceleration){accelerations[i].x+force_accelerations[i].x,
        accelerations[i].y+force_accelerations[i].y};
}

static double motion_pose_tolerance_get(Position a, Position b) {
    return 8 * FLT_EPSILON * fmax(1, fmax(hypot(a.x,a.y),hypot(b.x,b.y)));
}

static bool motion_pin_check(Joint joint,EntityIndex a,EntityIndex b,double dt,Report *report,Entity source) {
    bool sa=physics_entity_simulated_get(a),sb=physics_entity_simulated_get(b);
    if(sa==sb || (sa?physics_entity_movable_get(b):physics_entity_movable_get(a)))return false;
    EntityIndex body=sa?a:b;
    if(!motion_body_check(body)||physics_entity_nonrotating_check(body))return false;
    JointAnchorId moving=sa?joint.anchor_a:joint.anchor_b,fixed=sa?joint.anchor_b:joint.anchor_a;
    Position pivot=physics_joint_anchor_world_position_get(fixed).result.value;
    Position anchor=physics_joint_anchor_world_position_get(moving).result.value;
    Position com=physics_com_world_by_index_get(body);
    /* Existing inconsistent poses must first go through ordinary correction. */
    if(hypot((double)anchor.x-pivot.x,(double)anchor.y-pivot.y)>
            motion_pose_tolerance_get(com,pivot))return false;
    double inverse=physics_inverse_inertia_by_index_get(body);if(!(inverse>0))return false;
    double inertia=1/inverse,m=mass[body];
    Vec2D r=math_vector_subtract(com,pivot);
    double effective=inertia+m*((double)r.x*r.x+(double)r.y*r.y);
    double angular_momentum=inertia*angular_velocities[body]*MOTION_RAD+
        m*((double)r.y*velocities[body].x-(double)r.x*velocities[body].y);
    Acceleration force=motion_acceleration_get(body);
    double torque=inertia*(angular_accelerations[body]+torque_angular_accelerations[body])*MOTION_RAD;
    double alpha=(torque+m*((double)r.y*force.x-(double)r.x*force.y))/effective;
    double half=angular_momentum/effective+.5*dt*alpha;
    float angle=orientations[body]+half*dt/MOTION_RAD;
    Vec2D local=math_vector_subtract(physics_com_local_by_index_get(body),
        physics_joint_anchor_local_position_get(moving).result.value);
    Vec2D next=math_vector_rotate(local,angle);
    double omega=half+.5*dt*(torque+m*((double)next.y*force.x-(double)next.x*force.y))/effective;
    PhysicsChange change=physics_change_get(body,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION|
        PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY);
    motion_pose_set(&change,(Position){pivot.x+next.x,pivot.y+next.y},angle);
    change.velocity=(Velocity){omega*next.y,-omega*next.x};change.angular_velocity=omega/MOTION_RAD;
    return physics_changes_commit(&change,1,report,ROHR_PHYSICS_INTEGRATION,source,joint.b);
}

static bool motion_weld_check(Joint joint,EntityIndex a,EntityIndex b,double dt,Report *report,Entity source) {
    if(!physics_entity_simulated_get(a)||!physics_entity_simulated_get(b)||
            !motion_body_check(a)||!motion_body_check(b)||physics_entity_nonrotating_check(a)||
            physics_entity_nonrotating_check(b))return false;
    Position pa=physics_joint_anchor_world_position_get(joint.anchor_a).result.value;
    Position pb=physics_joint_anchor_world_position_get(joint.anchor_b).result.value;
    if(hypot((double)pa.x-pb.x,(double)pa.y-pb.y)>motion_pose_tolerance_get(pa,pb) ||
            fabs(remainder((double)orientations[b]-orientations[a]-joint.rest_angle,360))>
            8*FLT_EPSILON*fmax(1,fmax(fabs(orientations[a]),fabs(orientations[b]))))return false;
    EntityIndex bodies[]={a,b};double m[2]={mass[a],mass[b]},total=m[0]+m[1];
    Position p[2]={physics_com_world_by_index_get(a),physics_com_world_by_index_get(b)};
    double cx=(m[0]*p[0].x+m[1]*p[1].x)/total,cy=(m[0]*p[0].y+m[1]*p[1].y)/total;
    double vx=(m[0]*velocities[a].x+m[1]*velocities[b].x)/total;
    double vy=(m[0]*velocities[a].y+m[1]*velocities[b].y)/total;
    Vec2D r[2]={{p[0].x-cx,p[0].y-cy},{p[1].x-cx,p[1].y-cy}};
    double effective=0,l=0,fx=0,fy=0,torque=0;
    for(unsigned i=0;i<2;++i) {
        EntityIndex body=bodies[i];double inverse=physics_inverse_inertia_by_index_get(body);
        if(!(inverse>0))return false;
        double inertia=1/inverse;
        Acceleration acc=motion_acceleration_get(body);
        effective+=inertia+m[i]*((double)r[i].x*r[i].x+(double)r[i].y*r[i].y);
        l+=inertia*angular_velocities[body]*MOTION_RAD+m[i]*(r[i].y*(velocities[body].x-vx)-r[i].x*(velocities[body].y-vy));
        fx+=m[i]*acc.x;fy+=m[i]*acc.y;
        torque+=inertia*(angular_accelerations[body]+torque_angular_accelerations[body])*MOTION_RAD+
            m[i]*((double)r[i].y*acc.x-(double)r[i].x*acc.y);
    }
    double half=l/effective+.5*dt*torque/effective,omega=l/effective+dt*torque/effective;
    cx+=dt*vx+.5*dt*dt*fx/total;cy+=dt*vy+.5*dt*dt*fy/total;
    vx+=dt*fx/total;vy+=dt*fy/total;
    PhysicsChange changes[2];
    for(unsigned i=0;i<2;++i) {
        EntityIndex body=bodies[i];Vec2D next=motion_rotate(r[i],half*dt);
        changes[i]=physics_change_get(body,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION|
            PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY);
        motion_pose_set(&changes[i],(Position){cx+next.x,cy+next.y},orientations[body]+half*dt/MOTION_RAD);
        changes[i].velocity=(Velocity){vx+omega*next.y,vy-omega*next.x};
        changes[i].angular_velocity=omega/MOTION_RAD;
    }
    return physics_changes_commit(changes,2,report,ROHR_PHYSICS_INTEGRATION,source,joint.b);
}

static bool motion_spring_check(Joint joint,EntityIndex a,EntityIndex b,double dt,Report *report,Entity source) {
    if(!physics_entity_simulated_get(a)||!physics_entity_simulated_get(b)||!motion_body_check(a)||!motion_body_check(b))return false;
    EntityIndex bodies[]={a,b};
    Position p[2]={physics_com_world_by_index_get(a),physics_com_world_by_index_get(b)};
    Position anchors[2]={physics_joint_anchor_world_position_get(joint.anchor_a).result.value,
        physics_joint_anchor_world_position_get(joint.anchor_b).result.value};
    for(unsigned i=0;i<2;++i)if(hypot((double)p[i].x-anchors[i].x,(double)p[i].y-anchors[i].y)>1e-6)return false;
    double dx=(double)p[1].x-p[0].x,dy=(double)p[1].y-p[0].y;
    double dvx=(double)velocities[b].x-velocities[a].x,dvy=(double)velocities[b].y-velocities[a].y;
    double length=hypot(dx,dy);if(!(length>0))return false;
    double initial_force=joint.stiffness*(length-joint.rest_length)+joint.damping*(dx*dvx+dy*dvy)/length;
    double initial_x=initial_force*dx/length,initial_y=initial_force*dy/length;
    double ext[2][2];
    for(unsigned i=0;i<2;++i) {
        Acceleration acc=motion_acceleration_get(bodies[i]);double sign=i?-1:1;
        ext[i][0]=acc.x-sign*initial_x/mass[bodies[i]];
        ext[i][1]=acc.y-sign*initial_y/mass[bodies[i]];
    }
    double fx=initial_x,fy=initial_y,inv=1.0/mass[a]+1.0/mass[b];
    bool converged=false;
    for(unsigned iteration=0;iteration<32;++iteration) {
        double avx=ext[1][0]-ext[0][0]-inv*fx,avy=ext[1][1]-ext[0][1]-inv*fy;
        double mx=dx+.5*dt*dvx+.25*dt*dt*avx,my=dy+.5*dt*dvy+.25*dt*dt*avy;
        double mvx=dvx+.5*dt*avx,mvy=dvy+.5*dt*avy,r=hypot(mx,my);
        if(!(r>0))return false;
        double force=joint.stiffness*(r-joint.rest_length)+joint.damping*(mx*mvx+my*mvy)/r;
        double nx=force*mx/r,ny=force*my/r;
        if(hypot(nx-fx,ny-fy)<=1e-10*fmax(1,hypot(nx,ny)))converged=true;
        fx=nx;fy=ny;if(converged)break;
    }
    if(!converged)return false;
    PhysicsChange changes[2];
    for(unsigned i=0;i<2;++i) {
        EntityIndex body=bodies[i];double sign=i?-1:1;
        double ax=ext[i][0]+sign*fx/mass[body],ay=ext[i][1]+sign*fy/mass[body];
        double alpha=physics_entity_nonrotating_check(body)?0:angular_accelerations[body]+torque_angular_accelerations[body];
        double spin=physics_entity_nonrotating_check(body)?0:angular_velocities[body];
        changes[i]=physics_change_get(body,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION|
            PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY);
        motion_pose_set(&changes[i],(Position){p[i].x+dt*velocities[body].x+.5*dt*dt*ax,
            p[i].y+dt*velocities[body].y+.5*dt*dt*ay},orientations[body]+dt*spin+.5*dt*dt*alpha);
        changes[i].velocity=(Velocity){velocities[body].x+dt*ax,velocities[body].y+dt*ay};
        changes[i].angular_velocity=spin+dt*alpha;
    }
    return physics_changes_commit(changes,2,report,ROHR_PHYSICS_INTEGRATION,source,joint.b);
}

void physics_joint_motion_begin(double dt,Report *report) {
    memset(connections,0,sizeof(connections));memset(integrated,0,sizeof(integrated));
    if(!motion_enabled||!(dt>0))return;
    for(size_t i=0;i<joints_pool.capacity;++i) {
        EntityIndex a,b;
        if(!joints_pool.used[i]||!entity_index_alive_check(i)||!motion_endpoints_check(joints[i],&a,&b))continue;
        if(connections[a]<2)++connections[a];
        if(connections[b]<2)++connections[b];
    }
    for(size_t i=0;i<joints_pool.capacity;++i) {
        EntityIndex a,b;
        if(!joints_pool.used[i]||!entity_index_alive_check(i)||!motion_endpoints_check(joints[i],&a,&b)||
                connections[a]!=1||connections[b]!=1)continue;
        Entity source=physics_operation_entity_get(i);Joint joint=joints[i];bool handled=false;
        if(joint.type==JOINT_PIN)handled=motion_pin_check(joint,a,b,dt,report,source);
        else if(joint.type==JOINT_WELD)handled=motion_weld_check(joint,a,b,dt,report,source);
        else if(joint.type==JOINT_SPRING)handled=motion_spring_check(joint,a,b,dt,report,source);
        if(handled) {
            integrated[a]=true;integrated[b]=true;
            physics_report_counter_add(report,ROHR_REPORT_PROCESSED_COUNT,
                (unsigned)physics_entity_movable_get(a)+(unsigned)physics_entity_movable_get(b));
        }
    }
}

/* Bound angular travel and spring stiffness before prediction. This controls
 * integration error, not velocity or elapsed time; every remainder is consumed. */
double physics_joint_duration_get(double dt) {
    for(size_t i=0;i<joints_pool.capacity;++i) {
        EntityIndex a,b;
        if(!joints_pool.used[i]||!entity_index_alive_check(i)||!motion_endpoints_check(joints[i],&a,&b))continue;
        dt=fmin(dt,1.0/120);
        EntityIndex bodies[]={a,b};
        for(unsigned side=0;side<2;++side) {
            EntityIndex body=bodies[side];
            double spin=fabs(angular_velocities[body]);
            if(physics_entity_movable_get(body)&&spin>0)dt=fmin(dt,10/spin);
        }
        if(joints[i].type==JOINT_SPRING) {
            double inverse=(physics_entity_simulated_get(a)?1.0/mass[a]:0)+
                (physics_entity_simulated_get(b)?1.0/mass[b]:0);
            double rate=sqrt(joints[i].stiffness*inverse);
            if(isfinite(rate)&&rate>0)dt=fmin(dt,.1/rate);
        }
    }
    return dt;
}
