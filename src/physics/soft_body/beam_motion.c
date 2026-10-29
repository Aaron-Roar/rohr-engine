/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include <math.h>
#include <string.h>

/* Bounded scratch for one synchronous integration pass; no state survives end. */
static bool enabled,integrated[MAX_ENTITIES],elastic[MAX_ENTITIES];
static Acceleration old_spring[MAX_ENTITIES],new_spring[MAX_ENTITIES];
static double rates[MAX_ENTITIES];
void physics_soft_motion_enabled_set(bool value){enabled=value;}
bool physics_soft_motion_integrated_check(EntityIndex i){return i<MAX_ENTITIES&&integrated[i];}
static void spring_accelerations_get(Acceleration *out) {
    memset(out,0,sizeof(old_spring));
    for(EntityIndex i=0;i<soft_body_beams_pool.capacity;++i) {
        if(!soft_body_beams_pool.used[i]||!entity_index_alive_check(i))continue;
        SoftBodyBeam b=soft_body_beams[i];EntityIndex a,c;
        if(!entity_index_get(b.node_a,&a)||!entity_index_get(b.node_b,&c)||
            !entity_index_alive_check(a)||!entity_index_alive_check(c)||mass[a]<=0||mass[c]<=0)continue;
        if(b.stiffness>0){elastic[a]=true;elastic[c]=true;}
        double dx=positions[c].x-positions[a].x,dy=positions[c].y-positions[a].y,l=hypot(dx,dy);
        if(l<=.0001)continue;
        double force=b.stiffness*(l-b.rest_length)/l;
        out[a].x+=force*dx/mass[a];out[a].y+=force*dy/mass[a];
        out[c].x-=force*dx/mass[c];out[c].y-=force*dy/mass[c];
    }
}
void physics_soft_motion_begin(double dt,Report *report) {
    if(!enabled||!soft_body_nodes_pool.count)return;
    memset(elastic,0,sizeof(elastic));
    spring_accelerations_get(old_spring);
    for(EntityIndex i=0;i<soft_body_nodes_pool.capacity;++i) {
        if(!elastic[i]||!soft_body_nodes_pool.used[i]||!entity_index_alive_check(i)||!physics_entity_simulated_get(i)||
            entity_index_components_check(i,ROHR_AXIS_LOCK)||entity_index_components_check(i,ROHR_TRANSFORM_LOCK))continue;
        double ax=accelerations[i].x+force_accelerations[i].x,ay=accelerations[i].y+force_accelerations[i].y;
        PhysicsChange c=physics_change_get(i,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY);
        c.angular_velocity=0;
        c.position.x+=velocities[i].x*dt+.5*ax*dt*dt;c.position.y+=velocities[i].y*dt+.5*ay*dt*dt;
        c.velocity.x+=ax*dt;c.velocity.y+=ay*dt;
        integrated[i]=physics_changes_commit(&c,1,report,ROHR_PHYSICS_INTEGRATION,physics_operation_entity_get(i),0);
    }
}
void physics_soft_motion_end(double dt,Report *report) {
    if(!enabled||!soft_body_nodes_pool.count)return;
    spring_accelerations_get(new_spring);
    for(EntityIndex i=0;i<soft_body_nodes_pool.capacity;++i)if(integrated[i]) {
        PhysicsChange c=physics_change_get(i,PHYSICS_CHANGE_VELOCITY);
        c.velocity.x+=.5*dt*(new_spring[i].x-old_spring[i].x);
        c.velocity.y+=.5*dt*(new_spring[i].y-old_spring[i].y);
        (void)physics_changes_commit(&c,1,report,ROHR_PHYSICS_INTEGRATION,physics_operation_entity_get(i),0);
        integrated[i]=false;
    }
}
double physics_soft_duration_get(double dt) {
    if(!enabled||dt<=0||!soft_body_beams_pool.count)return dt;
    memset(rates,0,sizeof(rates));
    double rate=0;
    for(EntityIndex i=0;i<soft_body_beams_pool.capacity;++i) {
        if(!soft_body_beams_pool.used[i]||!entity_index_alive_check(i))continue;
        SoftBodyBeam b=soft_body_beams[i];EntityIndex a,c;
        if(!entity_index_get(b.node_a,&a)||!entity_index_get(b.node_b,&c)||
            !entity_index_alive_check(a)||!entity_index_alive_check(c))continue;
        if(mass[a]>0){rates[a]+=2*b.stiffness/mass[a];rate=fmax(rate,rates[a]);}
        if(mass[c]>0){rates[c]+=2*b.stiffness/mass[c];rate=fmax(rate,rates[c]);}
    }
    if(rate>0&&isfinite(rate))dt=fmin(dt,.2/sqrt(rate));
    return dt;
}

/* Convex bounds use endpoint velocities, so common translation cancels and
 * tangent motion does not force steps proportional to beam thickness. */
typedef struct SoftSweep {
    Shape shape;
    EntityIndex nodes[2];
    unsigned count;
    double feature,angular_speed,low_fraction,high_fraction;
} SoftSweep;
static SoftSweep soft_sweep_beam_get(EntityIndex beam,EntityIndex a,EntityIndex b) {
    double dx=positions[b].x-positions[a].x,dy=positions[b].y-positions[a].y,l=hypot(dx,dy);
    if(l<=.0001)return (SoftSweep){0};
    double radius=.5*soft_body_beams[beam].collision_thickness,nx=-dy/l*radius,ny=dx/l*radius;
    return (SoftSweep){.nodes={a,b},.count=2,.feature=2*radius,.high_fraction=1,
        .angular_speed=radius*hypot(velocities[b].x-velocities[a].x,velocities[b].y-velocities[a].y)/l,
        .shape={.amount_of_vertices=4,.vertices={{positions[a].x+nx,positions[a].y+ny},
            {positions[b].x+nx,positions[b].y+ny},{positions[b].x-nx,positions[b].y-ny},
            {positions[a].x-nx,positions[a].y-ny}}}};
}
static void soft_projection_get(const SoftSweep *s,Axis axis,double dt,double *lo,double *hi,double *vl,double *vh) {
    *lo=INFINITY;*hi=-INFINITY;*vl=INFINITY;*vh=-INFINITY;
    for(unsigned i=0;i<s->shape.amount_of_vertices;++i) {
        double p=(double)s->shape.vertices[i].x*axis.x+(double)s->shape.vertices[i].y*axis.y;
        *lo=fmin(*lo,p);*hi=fmax(*hi,p);
    }
    for(unsigned i=0;i<s->count;++i) {
        EntityIndex n=s->nodes[i];double v=0,a=0;
        if(physics_entity_movable_get(n)) {
            v=(double)velocities[n].x*axis.x+(double)velocities[n].y*axis.y;
            a=fabs((accelerations[n].x+force_accelerations[n].x)*axis.x+(accelerations[n].y+force_accelerations[n].y)*axis.y)*dt;
        }
        *vl=fmin(*vl,v-a-s->angular_speed);*vh=fmax(*vh,v+a+s->angular_speed);
    }
    if(s->count==2) {
        double v[2],acceleration=0;
        for(unsigned i=0;i<2;++i) {
            EntityIndex n=s->nodes[i];v[i]=physics_entity_movable_get(n)?
                (double)velocities[n].x*axis.x+(double)velocities[n].y*axis.y:0;
            if(physics_entity_movable_get(n))acceleration=fmax(acceleration,
                fabs((accelerations[n].x+force_accelerations[n].x)*axis.x+
                    (accelerations[n].y+force_accelerations[n].y)*axis.y)*dt);
        }
        double lo=v[0]+s->low_fraction*(v[1]-v[0]),hi=v[0]+s->high_fraction*(v[1]-v[0]);
        *vl=fmin(lo,hi)-acceleration-s->angular_speed;
        *vh=fmax(lo,hi)+acceleration+s->angular_speed;
    }
}
static SoftSweep soft_sweep_local_get(const SoftSweep *beam,const SoftSweep *target,double dt) {
    SoftSweep result=*beam;
    if(beam->count!=2||!target->shape.amount_of_vertices)return result;
    Position start=positions[beam->nodes[0]],end=positions[beam->nodes[1]];
    double dx=end.x-start.x,dy=end.y-start.y,length=hypot(dx,dy);
    if(length<=.0001)return result;
    double low=INFINITY,high=-INFINITY,travel=0;
    for(unsigned i=0;i<target->shape.amount_of_vertices;++i) {
        Position p=target->shape.vertices[i];double t=((p.x-start.x)*dx+(p.y-start.y)*dy)/length;
        low=fmin(low,t);high=fmax(high,t);
    }
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<target->count;++j) {
        EntityIndex a=beam->nodes[i],b=target->nodes[j];
        double vx=0,vy=0,acceleration=0;
        if(physics_entity_movable_get(a)){vx=velocities[a].x;vy=velocities[a].y;
            acceleration+=hypot(accelerations[a].x+force_accelerations[a].x,accelerations[a].y+force_accelerations[a].y);}
        if(physics_entity_movable_get(b)){vx-=velocities[b].x;vy-=velocities[b].y;
            acceleration+=hypot(accelerations[b].x+force_accelerations[b].x,accelerations[b].y+force_accelerations[b].y);}
        travel=fmax(travel,(hypot(vx,vy)+beam->angular_speed+target->angular_speed+acceleration*dt)*dt);
    }
    /* Only this swept interval can reach the other collider in this step.
     * A distant endpoint's velocity must not constrain a resting local contact. */
    result.low_fraction=fmax(0,fmin(1,(low-travel-beam->feature)/length));
    result.high_fraction=fmax(0,fmin(1,(high+travel+beam->feature)/length));
    return result;
}
static double soft_travel_time_get(double distance,double speed,double acceleration) {
    speed=fmax(0,speed);
    if(acceleration>0)return 2*distance/(speed+sqrt(speed*speed+4*acceleration*distance));
    return speed>0?distance/speed:INFINITY;
}
static double soft_pair_duration_get(const SoftSweep *original_a,const SoftSweep *original_b,double dt) {
    SoftSweep local_a=soft_sweep_local_get(original_a,original_b,dt),local_b=soft_sweep_local_get(original_b,original_a,dt);
    const SoftSweep *a=&local_a,*b=&local_b;
    if(!a->shape.amount_of_vertices||!b->shape.amount_of_vertices)return dt;
    const Shape *shapes[2]={&a->shape,&b->shape};
    double entry=0,min_depth=INFINITY,closing=0,closing_acceleration=0,feature=fmin(a->feature,b->feature);
    for(unsigned s=0;s<2;++s)for(unsigned i=0;i<shapes[s]->amount_of_vertices;++i) {
        Position p=shapes[s]->vertices[i],q=shapes[s]->vertices[(i+1)%shapes[s]->amount_of_vertices];
        double nx=q.y-p.y,ny=p.x-q.x,l=hypot(nx,ny);if(l<.0001)continue;Axis axis={nx/l,ny/l};
        double al,ah,avl,avh,bl,bh,bvl,bvh;
        soft_projection_get(a,axis,dt,&al,&ah,&avl,&avh);soft_projection_get(b,axis,dt,&bl,&bh,&bvl,&bvh);
        double d=fmin(ah-bl,bh-al),speed=(bl+bh>al+ah)?avh-bvl:bvh-avl;
        soft_projection_get(a,axis,0,&al,&ah,&avl,&avh);soft_projection_get(b,axis,0,&bl,&bh,&bvl,&bvh);
        double initial_speed=(bl+bh>al+ah)?avh-bvl:bvh-avl;
        double acceleration=fmax(0,(speed-initial_speed)/dt);
        if(d<0) {
            if(speed<=0||-d>speed*dt)return dt;
            entry=fmax(entry,soft_travel_time_get(-d+.005*feature,initial_speed,acceleration));
        }
        if(d<min_depth){min_depth=d;closing=fmax(0,initial_speed);closing_acceleration=acceleration;}
    }
    if(entry>0)return fmin(dt,entry);
    return fmin(dt,soft_travel_time_get(.005*feature,closing,closing_acceleration));
}
double physics_soft_sweep_duration_get(double dt,Report *report) {
    if(dt<=0||!soft_body_beams_pool.count)return dt;
    double next=dt;
    for(EntityIndex i=0;i<soft_body_beams_pool.capacity;++i) {
        if(!soft_body_beams_pool.used[i]||!entity_index_alive_check(i))continue;
        SoftBodyBeam beam=soft_body_beams[i];EntityIndex a,b;
        if(!beam.collision_enabled||!entity_index_get(beam.node_a,&a)||!entity_index_get(beam.node_b,&b)||
            !entity_index_alive_check(a)||!entity_index_alive_check(b))continue;
        Entity be=physics_operation_entity_get(i);SoftSweep first=soft_sweep_beam_get(i,a,b);
        for(uint32_t n=0;n<entity_alive_count_get();++n) {
            EntityIndex j;if(!physics_step_alive_index_at(n,&j)||j==a||j==b)continue;
            Entity other=physics_operation_entity_get(j);
            if(!physics_collision_response_between_check(be,other))continue;
            SoftSweep second={0};
            if(j>i&&entity_index_components_check(j,ROHR_SOFT_BODY_BEAM)) {
                SoftBodyBeam target=soft_body_beams[j];EntityIndex c,d;
                if(!target.collision_enabled||!entity_index_get(target.node_a,&c)||!entity_index_get(target.node_b,&d)||
                    !entity_index_alive_check(c)||!entity_index_alive_check(d)||a==c||a==d||b==c||b==d)continue;
                second=soft_sweep_beam_get(j,c,d);
            } else if(entity_index_components_check(j,ROHR_COLLISION|ROHR_HIT_BOX)) {
                if(!physics_step_hitbox_update(j,report))continue;
                second=(SoftSweep){.shape=world_hit_boxes[j],.nodes={j},.count=1,.feature=first.feature};
                if(physics_entity_movable_get(j)&&!entity_index_components_check(j,ROHR_PARTICLE)) {
                    double radius=0;for(unsigned v=0;v<hit_boxes[j].amount_of_vertices;++v)
                        radius=fmax(radius,hypot(hit_boxes[j].vertices[v].x,hit_boxes[j].vertices[v].y));
                    second.angular_speed=radius*fabs(angular_velocities[j])*.017453292519943295;
                }
            }
            next=fmin(next,soft_pair_duration_get(&first,&second,dt));
        }
    }
    return next;
}
