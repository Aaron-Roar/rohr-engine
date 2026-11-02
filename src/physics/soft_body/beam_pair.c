/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include "physics/soft_body/soft_body.h"
#include <math.h>

static bool beam_nodes_get(EntityIndex index, EntityIndex nodes[2]) {
    SoftBodyBeam *b=&soft_body_beams[index];
    return b->collision_enabled && entity_index_alive_check(index) &&
        entity_index_get(b->node_a,&nodes[0]) && entity_index_get(b->node_b,&nodes[1]) &&
        entity_index_alive_check(nodes[0]) && entity_index_alive_check(nodes[1]);
}
static Shape beam_shape_get(const SystemBeamPair *p,unsigned side,bool response) {
    EntityIndex a=p->nodes[2*side],b=p->nodes[2*side+1];
    float ra=0,rb=0;
    if(response) {
        Entity other=p->beams[1-side];
        Entity na=physics_operation_entity_get(a),nb=physics_operation_entity_get(b);
        if(entity_index_components_check(a,ROHR_COLLISION)&&
            physics_collision_response_to_check(na,other)==physics_collision_response_to_check(p->beams[side],other)&&
            physics_collision_response_to_check(other,na)==physics_collision_response_to_check(other,p->beams[side]))
            ra=soft_body_nodes[a].radius;
        if(entity_index_components_check(b,ROHR_COLLISION)&&
            physics_collision_response_to_check(nb,other)==physics_collision_response_to_check(p->beams[side],other)&&
            physics_collision_response_to_check(other,nb)==physics_collision_response_to_check(other,p->beams[side]))
            rb=soft_body_nodes[b].radius;
    }
    return soft_body_boundary_shape_create(positions[a],positions[b],
        soft_body_beams[p->indices[side]].collision_thickness*.5f,ra,rb);
}
static bool beam_overlap_get(const SystemBeamPair *p,bool response,OverlapInfo *overlap,Position *point,float weights[4]) {
    Shape a=beam_shape_get(p,0,response),b=beam_shape_get(p,1,response);
    if(!a.amount_of_vertices||!b.amount_of_vertices)return false;
    if(!math_aabb_overlap(math_aabb_create(a),math_aabb_create(b)))return false;
    ContactManifoldSet set=contact_manifold_set_polygon_get(a,b);
    const ContactManifold *best=NULL;
    for(unsigned i=0;i<set.count;++i)if(set.values[i].count&&(!best||set.values[i].depth>best->depth))best=&set.values[i];
    if(!best)return false;
    *overlap=(OverlapInfo){.detected=true,.normal=best->normal,.depth=best->depth};
    *point=(Position){0};
    for(unsigned i=0;i<best->count;++i){point->x+=best->points[i].x/best->count;point->y+=best->points[i].y/best->count;}
    for(unsigned s=0;s<2;++s) {
        Position start=positions[p->nodes[2*s]],end=positions[p->nodes[2*s+1]];
        Vec2D d=math_vector_subtract(end,start);float length=math_dot_product(d,d);
        float t=length>0?fmaxf(0,fminf(1,math_dot_product(math_vector_subtract(*point,start),d)/length)):.5f;
        weights[2*s]=1-t;weights[2*s+1]=t;
    }
    return true;
}
static float beam_relative_speed_get(const SystemBeamPair *p,const float weights[4],Axis axis) {
    float speed=0;
    for(unsigned i=0;i<4;++i) {
        Velocity v=physics_point_velocity_by_index_get(p->nodes[i],positions[p->nodes[i]]);
        speed+=(i<2?-1:1)*weights[i]*math_dot_product(v,axis);
    }
    return speed;
}
static bool pair_gather_cached;
static Position pair_gather_starts[MAX_ENTITIES], pair_gather_ends[MAX_ENTITIES];
void physics_beam_pairs_gather_cache_reset(void) { pair_gather_cached = false; }

void physics_beam_pairs_gather(Report *report) {
    /* Rebuild bounded candidates from current endpoints. Reject disjoint bounds
     * before constructing cutout polygons or searching retained constraints. */
    typedef struct BeamCandidate { EntityIndex index,nodes[2]; AABB bounds; bool unchanged; } BeamCandidate;
    static BeamCandidate candidates[MAX_ENTITIES];
    bool cached=physics_step_contact_constraints.solving&&pair_gather_cached;
    unsigned count=0;
    for(EntityIndex i=0;i<soft_body_beams_pool.capacity;++i) {
        if(!soft_body_beams_pool.used[i])continue;
        EntityIndex nodes[2];if(!beam_nodes_get(i,nodes))continue;
        Position a=positions[nodes[0]],b=positions[nodes[1]];
        float r=.5f*soft_body_beams[i].collision_thickness;
        bool unchanged=cached&&pair_gather_starts[i].x==a.x&&pair_gather_starts[i].y==a.y&&
            pair_gather_ends[i].x==b.x&&pair_gather_ends[i].y==b.y;
        pair_gather_starts[i]=a;pair_gather_ends[i]=b;
        candidates[count++]=(BeamCandidate){.index=i,.nodes={nodes[0],nodes[1]},.unchanged=unchanged,
            .bounds={fminf(a.x,b.x)-r,fmaxf(a.x,b.x)+r,fminf(a.y,b.y)-r,fmaxf(a.y,b.y)+r}};
    }
    for(unsigned ai=0;ai<count;++ai) {
        EntityIndex a=candidates[ai].index,*an=candidates[ai].nodes;
        for(unsigned bi=ai+1;bi<count;++bi) {
            if(candidates[ai].unchanged&&candidates[bi].unchanged)continue;
            if(!math_aabb_overlap(candidates[ai].bounds,candidates[bi].bounds))continue;
            EntityIndex b=candidates[bi].index,*bn=candidates[bi].nodes;
            if(an[0]==bn[0]||an[0]==bn[1]||an[1]==bn[0]||an[1]==bn[1])continue;
            SystemContactConstraint *existing=NULL;
            for(size_t k=0;k<physics_step_contact_constraints.count;++k) {
                SystemContactConstraint *c=&physics_step_contact_constraints.values[k];
                if(c->type==SYSTEM_CONTACT_CONSTRAINT_BEAM_PAIR&&
                    c->value.beam_pair.indices[0]==a&&c->value.beam_pair.indices[1]==b){existing=c;break;}
            }
            if(existing&&(existing->value.beam_pair.responds||
                !physics_collision_response_between_check(existing->value.beam_pair.beams[0],
                    existing->value.beam_pair.beams[1])))continue;
            SystemBeamPair p={.beams={physics_operation_entity_get(a),physics_operation_entity_get(b)},
                .indices={a,b},.nodes={an[0],an[1],bn[0],bn[1]}};
            Position point;float weights[4];
            if(!beam_overlap_get(&p,false,&p.overlap,&point,weights))continue;
            OverlapInfo response;
            p.responds=physics_collision_response_between_check(p.beams[0],p.beams[1])&&
                beam_overlap_get(&p,true,&response,&point,weights);
            if(p.responds) {
                p.normal=response.normal;
                for(unsigned i=0;i<4;++i)p.weights[i]=weights[i];
                p.rebound_speed=-fminf(soft_body_beams[a].restitution,soft_body_beams[b].restitution)*
                    fminf(0,beam_relative_speed_get(&p,weights,p.normal));
                p.contact.points[0].position=point;
                p.contact.points[0].relative_velocity=(Velocity){
                    beam_relative_speed_get(&p,weights,(Axis){1,0}),
                    beam_relative_speed_get(&p,weights,(Axis){0,1})};
            }
            if(existing) {
                if(p.responds)existing->value.beam_pair=p;
                continue;
            }
            if(!contact_constraint_list_append(&physics_step_contact_constraints,(SystemContactConstraint){
                .type=SYSTEM_CONTACT_CONSTRAINT_BEAM_PAIR,.value.beam_pair=p})) {
                physics_report_issue_add(report,ROHR_PHYSICS_CONTACT_GATHER,ROHR_REPORT_SEVERITY_FATAL,
                    ERROR_MEMORY_POOL_ALLOCATION_FAILED,ROHR_REPORT_UPDATE_ABORTED,p.beams[0],p.beams[1],
                    "could not allocate beam pair constraint");return;
            }
        }
    }
    pair_gather_cached=physics_step_contact_constraints.solving;
}
void physics_beam_pair_solve(SystemBeamPair *p,float fraction,Report *report) {
    if(!p||!p->responds)return;
    fraction=1.0f;
    for(unsigned i=0;i<4;++i)if(!entity_index_alive_check(p->nodes[i]))return;
    float inverse[4],denominator=0;
    for(unsigned i=0;i<4;++i) {
        EntityIndex n=p->nodes[i];
        inverse[i]=physics_collision_response_to_check(p->beams[i/2],p->beams[1-i/2])&&
            physics_entity_simulated_get(n)&&mass[n]>0?1/mass[n]:0;
        denominator+=p->weights[i]*p->weights[i]*inverse[i];
    }
    if(denominator<=0)return;
    Axis axes[2]={p->normal,{-p->normal.y,p->normal.x}};
    for(unsigned axis=0;axis<2;++axis) {
        float previous=axis?p->tangent_impulse:p->normal_impulse;
        float next=previous+((axis?0:p->rebound_speed)-beam_relative_speed_get(p,p->weights,axes[axis]))/denominator;
        if(axis) {
            float limit=p->normal_impulse*sqrtf(soft_body_beams[p->indices[0]].friction*soft_body_beams[p->indices[1]].friction);
            next=fmaxf(-limit,fminf(limit,next));
        }else next=fmaxf(0,next);
        PhysicsChange changes[4];
        for(unsigned i=0;i<4;++i) {
            changes[i]=physics_change_get(p->nodes[i],PHYSICS_CHANGE_VELOCITY);
            float delta=(i<2?-1:1)*p->weights[i]*inverse[i]*(next-previous);
            changes[i].velocity.x+=delta*axes[axis].x;changes[i].velocity.y+=delta*axes[axis].y;
        }
        if(physics_changes_commit(changes,4,report,axis?ROHR_PHYSICS_FRICTION_IMPULSE:ROHR_PHYSICS_NORMAL_IMPULSE,p->beams[0],p->beams[1])) {
            if(axis)p->tangent_impulse=next;else p->normal_impulse=next;
        }
    }
    bool unchanged=p->geometry_valid;
    for(unsigned i=0;i<4&&unchanged;++i)
        unchanged=p->geometry_nodes[i].x==positions[p->nodes[i]].x&&
            p->geometry_nodes[i].y==positions[p->nodes[i]].y;
    if(!unchanged) {
        p->geometry_point=p->contact.points[0].position;
        p->geometry_overlapping=beam_overlap_get(p,true,&p->geometry_overlap,
            &p->geometry_point,p->geometry_weights);
        for(unsigned i=0;i<4;++i)p->geometry_nodes[i]=positions[p->nodes[i]];
        p->geometry_valid=true;
    }
    Position point=p->geometry_point;OverlapInfo overlap=p->geometry_overlap;
    float *weights=p->geometry_weights;
    bool hit=p->geometry_overlapping;
    if(hit) {
        float sum=0;for(unsigned i=0;i<4;++i)sum+=weights[i]*weights[i]*inverse[i];
        if(sum>0) {
            PhysicsChange changes[4];
            for(unsigned i=0;i<4;++i) {
                changes[i]=physics_change_get(p->nodes[i],PHYSICS_CHANGE_POSITION);
                float d=(i<2?-1:1)*weights[i]*inverse[i]*overlap.depth*fraction/sum;
                changes[i].position.x+=d*overlap.normal.x;changes[i].position.y+=d*overlap.normal.y;
            }
            (void)physics_changes_commit(changes,4,report,ROHR_PHYSICS_POSITION_CORRECTION,p->beams[0],p->beams[1]);
        }
    }
    if(!p->solved)physics_report_counter_add(report,ROHR_REPORT_CONTACT_COUNT,1);
    Velocity relative=p->contact.points[0].relative_velocity;
    p->contact=(ContactInfo){.detected=true,.normal=p->normal,.depth=hit?overlap.depth:0,.point_count=1,
        .points={{.position=point,.relative_velocity=relative,.normal_impulse={p->normal.x*p->normal_impulse,p->normal.y*p->normal_impulse},
            .friction_impulse={-p->normal.y*p->tangent_impulse,p->normal.x*p->tangent_impulse}}}};
    p->solved=true;
}
void physics_beam_pair_finalize(const SystemBeamPair *p,Report *report) {
    if(!p||!p->overlap.detected)return;
    physics_step_interaction_by_index_record(p->indices[0],p->indices[1],p->overlap,p->contact,
        PHYSICS_INTERACTION_OVERLAP|(p->contact.detected?PHYSICS_INTERACTION_CONTACT:0),report);
}
