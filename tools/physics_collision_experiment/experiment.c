/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include "physics/physics_operation.h"
#include "physics/physics_step_internal.h"
#include "physics/broadphase/aabb_tree.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAIR_CAPACITY 32768
typedef struct Body {
    ExperimentPose start, end;
    AABB bounds;
    bool valid, particle, movable;
    unsigned impacts;
} Body;
typedef struct Pair {
    EntityIndex a, b;
    ExperimentContact contact;
    bool predictive;
} Pair;
/* Bounded diagnostic-owned storage; reset for each integration, no hidden
 * allocation, no persistent entity handles and no effect on engine defaults. */
static Body bodies[MAX_ENTITIES];
static EntityIndex indices[MAX_ENTITIES];
static Pair pairs[PAIR_CAPACITY];
static unsigned body_count, pair_count;
static AABBTreeNode tree_nodes[2*MAX_ENTITIES];
static uint32_t tree_stack[2*MAX_ENTITIES];
static AABBTree tree = {.nodes=tree_nodes, .query_stack=tree_stack, .capacity=2*MAX_ENTITIES};
static ExperimentCounts counts;
static int mode = -1;
static bool rows_active;

static void finish(void) {
    printf("experiment mode=%d integrations=%llu candidates=%llu casts=%llu cast_iterations=%llu impacts=%llu predictive_rows=%llu correction_rows=%llu initial_overlaps=%llu unsupported=%llu capacity_limits=%llu search_limits=%llu secondary_risks=%llu\n",
        mode, (unsigned long long)counts.integrations, (unsigned long long)counts.candidates,
        (unsigned long long)counts.casts, (unsigned long long)counts.cast_iterations,
        (unsigned long long)counts.impacts, (unsigned long long)counts.predictive_rows,
        (unsigned long long)counts.correction_rows, (unsigned long long)counts.initial_overlaps,
        (unsigned long long)counts.unsupported, (unsigned long long)counts.capacity_limits,
        (unsigned long long)counts.search_limits, (unsigned long long)counts.secondary_risks);
}
static int mode_get(void) {
    if(mode < 0) {
        const char *value = getenv("ROHR_COLLISION_EXPERIMENT");
        mode = value && !strcmp(value,"predict") ? 1 : value && !strcmp(value,"sweep") ? 2 : 0;
        atexit(finish);
    }
    return mode;
}
ExperimentCounts experiment_counts_get(void) { return counts; }
void experiment_counts_clear(void) { counts = (ExperimentCounts){0}; }
static float dot(Vec2D a, Vec2D b) { return a.x*b.x+a.y*b.y; }
static Vec2D subtract(Vec2D a, Vec2D b) { return (Vec2D){a.x-b.x,a.y-b.y}; }
static ExperimentPose pose_get(EntityIndex i) { return (ExperimentPose){positions[i],orientations[i]}; }
static Shape shape_get(EntityIndex i, ExperimentPose p) {
    return physics_shape_world_translate(hit_boxes[i],p.p,p.angle);
}
static ExperimentPose interpolate(ExperimentPose a, ExperimentPose b, double t) {
    return (ExperimentPose){{a.p.x+(b.p.x-a.p.x)*t,a.p.y+(b.p.y-a.p.y)*t},a.angle+(b.angle-a.angle)*t};
}
static AABB bounds_get(EntityIndex i) {
    Body *b=&bodies[i];
    Shape a=shape_get(i,b->start), end=shape_get(i,b->end);
    AABB x=math_aabb_create(a), y=math_aabb_create(end);
    x.min_x=fminf(x.min_x,y.min_x); x.max_x=fmaxf(x.max_x,y.max_x);
    x.min_y=fminf(x.min_y,y.min_y); x.max_y=fmaxf(x.max_y,y.max_y);
    if(b->start.angle!=b->end.angle) {
        float radius=0;
        for(unsigned v=0;v<hit_boxes[i].amount_of_vertices;++v)
            radius=fmaxf(radius,hypotf(hit_boxes[i].vertices[v].x,hit_boxes[i].vertices[v].y));
        x=(AABB){.min_x=fminf(b->start.p.x,b->end.p.x)-radius,
            .max_x=fmaxf(b->start.p.x,b->end.p.x)+radius,
            .min_y=fminf(b->start.p.y,b->end.p.y)-radius,
            .max_y=fmaxf(b->start.p.y,b->end.p.y)+radius};
    }
    x.min_x-=.001f; x.min_y-=.001f; x.max_x+=.001f; x.max_y+=.001f;
    return x;
}
static bool pair_visit(Entity entity, void *context) {
    EntityIndex a=*(EntityIndex *)context;
    EntityIndex b;
    if(!entity_index_get(entity,&b)) return true;
    if(a==b || (!bodies[a].particle && a>b) ||
            (!bodies[a].movable&&!bodies[b].movable) ||
            !physics_collision_response_between_by_index_check(a,b)) return true;
    if(pair_count==PAIR_CAPACITY) { ++counts.capacity_limits; return false; }
    pairs[pair_count++]=(Pair){.a=a,.b=b}; ++counts.candidates;
    return true;
}
static float inverse_mass(EntityIndex a, EntityIndex b) {
    return physics_entity_simulated_get(a) && physics_collision_response_to_by_index_check(a,b) ? 1/mass[a] : 0;
}
static Vec2D offset_get(EntityIndex i, ExperimentPose pose, Vec2D point) {
    Vec2D local=rohr_math_vector_rotate(physics_com_local_by_index_get(i),pose.angle);
    return (Vec2D){point.x-pose.p.x-local.x,point.y-pose.p.y-local.y};
}
static Vec2D point_velocity(EntityIndex i, Vec2D r) {
    Vec2D angular=physics_entity_nonrotating_check(i) ? (Vec2D){0} :
        math_angular_velocity_cross_vec(angular_velocities[i],r);
    return (Vec2D){velocities[i].x+angular.x,velocities[i].y+angular.y};
}
static bool impulse_apply(Pair *pair, ExperimentPose pa, ExperimentPose pb,
        float target, bool friction, double motion_remaining, Report *report) {
    EntityIndex a=pair->a,b=pair->b;
    Vec2D n=pair->contact.normal,point=pair->contact.point;
    Vec2D ra=offset_get(a,pa,point),rb=offset_get(b,pb,point);
    float ma=inverse_mass(a,b),mb=inverse_mass(b,a);
    float ia=ma&&!physics_entity_nonrotating_check(a) ? physics_inverse_inertia_by_index_get(a) : 0;
    float ib=mb&&!physics_entity_nonrotating_check(b) ? physics_inverse_inertia_by_index_get(b) : 0;
    float ca=math_cross_2d(ra,n),cb=math_cross_2d(rb,n);
    float denominator=ma+mb+ca*ca*ia+cb*cb*ib;
    float vn=dot(subtract(point_velocity(b,rb),point_velocity(a,ra)),n);
    if(!(denominator>0) || vn>=target) return false;
    float magnitude=(target-vn)/denominator;
    Vec2D impulse={n.x*magnitude,n.y*magnitude};
    PhysicsChange changes[2]={physics_change_get(a,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION|PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY),
        physics_change_get(b,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION|PHYSICS_CHANGE_VELOCITY|PHYSICS_CHANGE_ANGULAR_VELOCITY)};
    changes[0].velocity.x-=impulse.x*ma; changes[0].velocity.y-=impulse.y*ma;
    changes[1].velocity.x+=impulse.x*mb; changes[1].velocity.y+=impulse.y*mb;
    changes[0].angular_velocity+=math_radians_to_degrees(ca*magnitude*ia);
    changes[1].angular_velocity-=math_radians_to_degrees(cb*magnitude*ib);
    float masses[]={ma,mb};
    ExperimentPose poses[]={pa,pb}; EntityIndex ids[]={a,b};
    for(unsigned k=0;k<2;++k) if(masses[k]>0) {
        if(mode==2) {
            changes[k].position=(Position){poses[k].p.x+changes[k].velocity.x*motion_remaining,
                poses[k].p.y+changes[k].velocity.y*motion_remaining};
            changes[k].orientation=poses[k].angle+changes[k].angular_velocity*motion_remaining;
        } else {
            changes[k].position.x+=(changes[k].velocity.x-velocities[ids[k]].x)*motion_remaining;
            changes[k].position.y+=(changes[k].velocity.y-velocities[ids[k]].y)*motion_remaining;
            changes[k].orientation+=(changes[k].angular_velocity-angular_velocities[ids[k]])*motion_remaining;
        }
    }
    if(!physics_changes_commit(changes,2,report,ROHR_PHYSICS_NORMAL_IMPULSE,
            physics_operation_entity_get(a),physics_operation_entity_get(b))) return false;
    if(friction) {
        Velocity before[2]={velocities[a],velocities[b]};
        float angular_before[2]={angular_velocities[a],angular_velocities[b]};
        OverlapInfo overlap={.detected=true,.normal=n};
        (void)system_friction_impulse_apply(physics_operation_entity_get(a),physics_operation_entity_get(b),
            overlap,ra,rb,magnitude,ma,mb,ia,ib,(Vec2D){0},report);
        /* Friction acts at impact too, so its velocity change affects the
         * remaining travel rather than only the next integration. */
        for(unsigned k=0;k<2;++k) {
            EntityIndex i=ids[k];
            PhysicsChange change=physics_change_get(i,PHYSICS_CHANGE_POSITION|PHYSICS_CHANGE_ORIENTATION);
            change.position.x+=(velocities[i].x-before[k].x)*motion_remaining;
            change.position.y+=(velocities[i].y-before[k].y)*motion_remaining;
            change.orientation+=(angular_velocities[i]-angular_before[k])*motion_remaining;
            (void)physics_changes_commit(&change,1,report,ROHR_PHYSICS_POSITION,physics_operation_entity_get(i),0);
        }
    }
    ContactInfo contact={.detected=true,.normal=n,.point_count=1};
    contact.points[0].position=point;
    contact.points[0].normal_impulse=impulse;
    /* This is an applied predictive/swept response, not a geometric overlap. */
    physics_step_interaction_by_index_record(a,b,(OverlapInfo){0},contact,PHYSICS_INTERACTION_CONTACT,report);
    return true;
}

EngineResult __real_physics_rigid_integrate(double dt,Report *report);
EngineResult __wrap_physics_rigid_integrate(double dt,Report *report) {
    rows_active=false;
    if(!mode_get() || !(dt>0) || !physics_simple_baseline_active_check())
        return __real_physics_rigid_integrate(dt,report);
    body_count=pair_count=0; ++counts.integrations;
    uint64_t capacity_before=counts.capacity_limits;
    for(unsigned n=0;n<physics_step_collider_iteration_count_get();++n) {
        EntityIndex i;
        if(!physics_step_collider_index_at(n,&i)) continue;
        Body *b=&bodies[i]; *b=(Body){0};
        if(!(entity_mask[i]&ROHR_COLLISION)) continue;
        if(hit_boxes[i].amount_of_vertices<3 || hit_boxes[i].concave_piece_count ||
                (entity_mask[i]&(ROHR_AXIS_LOCK|ROHR_TRANSFORM_LOCK))) { ++counts.unsupported; continue; }
        b->valid=true; b->particle=(entity_mask[i]&ROHR_PARTICLE)!=0;
        b->movable=physics_entity_movable_get(i); b->start=pose_get(i);
        indices[body_count++]=i;
    }
    EngineResult result=__real_physics_rigid_integrate(dt,report);
    if(error_check(result) || (report&&report->has_fatal)) return result;
    aabb_tree_clear(&tree);
    for(unsigned n=0;n<body_count;++n) {
        EntityIndex i=indices[n]; bodies[i].end=pose_get(i); bodies[i].bounds=bounds_get(i);
        if(!bodies[i].particle && error_check(aabb_tree_insert(&tree,physics_operation_entity_get(i),bodies[i].bounds))) {
            ++counts.capacity_limits;
            physics_report_issue_add(report,ROHR_PHYSICS_BROADPHASE,ROHR_REPORT_SEVERITY_WARNING,
                ERROR_ENGINE_STATE_INVALID,ROHR_REPORT_CONTRIBUTION_SKIPPED,physics_operation_entity_get(i),0,
                "experimental obstacle bounds could not be inserted; protection incomplete");
            return result;
        }
    }
    for(unsigned n=0;n<body_count;++n) {
        EntityIndex i=indices[n];
        (void)aabb_tree_query(&tree,bodies[i].bounds,pair_visit,&i);
    }
    if(counts.capacity_limits!=capacity_before)
        physics_report_issue_add(report,ROHR_PHYSICS_BROADPHASE,ROHR_REPORT_SEVERITY_WARNING,
            ERROR_ENGINE_STATE_INVALID,ROHR_REPORT_CONTRIBUTION_SKIPPED,0,0,
            "experimental pair capacity exhausted; protection incomplete");
    for(unsigned k=0;k<pair_count;++k) {
        Pair *p=&pairs[k]; Body *a=&bodies[p->a],*b=&bodies[p->b];
        Shape sa=shape_get(p->a,a->start),sb=shape_get(p->b,b->start);
        p->contact=experiment_separation_get(&sa,&sb);
        if(p->contact.gap<-.0001f || (mode==2&&p->contact.gap<=.0001f)) { ++counts.initial_overlaps; continue; }
        if(mode==1) {
            Vec2D ra=offset_get(p->a,a->start,p->contact.point),rb=offset_get(p->b,b->start,p->contact.point);
            float vn=dot(subtract(point_velocity(p->b,rb),point_velocity(p->a,ra)),p->contact.normal);
            p->contact.gap=fmaxf(0,p->contact.gap);
            if(p->contact.gap+vn*dt>=0) continue;
            p->predictive=true; ++counts.predictive_rows;
            (void)impulse_apply(p,a->start,b->start,-p->contact.gap/dt,false,dt,report);
        } else {
            double fraction; unsigned iterations; bool limited; ++counts.casts;
            bool hit=experiment_cast_check(p->a,p->b,a->start,pose_get(p->a),b->start,pose_get(p->b),
                &fraction,&p->contact,&iterations,&limited);
            counts.cast_iterations+=iterations; counts.search_limits+=limited;
            if(limited) physics_report_issue_add(report,ROHR_PHYSICS_CONTACT_GATHER,ROHR_REPORT_SEVERITY_WARNING,
                ERROR_ENGINE_STATE_INVALID,ROHR_REPORT_CONTRIBUTION_SKIPPED,
                physics_operation_entity_get(p->a),physics_operation_entity_get(p->b),
                "experimental sweep did not converge; protection incomplete");
            if(!hit) continue;
            ExperimentPose pa=interpolate(a->start,pose_get(p->a),fraction),pb=interpolate(b->start,pose_get(p->b),fraction);
            Vec2D ra=offset_get(p->a,pa,p->contact.point),rb=offset_get(p->b,pb,p->contact.point);
            float vn=dot(subtract(point_velocity(p->b,rb),point_velocity(p->a,ra)),p->contact.normal);
            float target=-fminf(restitutions[p->a],restitutions[p->b])*fminf(vn,0);
            if(impulse_apply(p,pa,pb,target,true,dt*(1-fraction),report)) {
                ++counts.impacts;
                if((a->movable&&a->impacts) || (b->movable&&b->impacts)) ++counts.secondary_risks;
                if(a->movable) ++a->impacts;
                if(b->movable) ++b->impacts;
            }
        }
    }
    rows_active=true;
    return result;
}

void __real_physics_pipeline_joint_constraints_solve(void *context);
void __wrap_physics_pipeline_joint_constraints_solve(void *context) {
    __real_physics_pipeline_joint_constraints_solve(context);
    if(mode!=1 || !rows_active || !physics_simple_baseline_active_check()) return;
    Report *report=context;
    /* Couple active predictive planes to the existing contact/joint passes.
     * Newly created candidates are deliberately not rediscovered in this first
     * screen; correction-created misses are measured by the fixture oracle. */
    for(unsigned k=0;k<pair_count;++k) {
        Pair *p=&pairs[k]; if(!p->predictive) continue;
        Shape a=shape_get(p->a,pose_get(p->a)),b=shape_get(p->b,pose_get(p->b));
        float high=-INFINITY,low=INFINITY;
        for(unsigned v=0;v<a.amount_of_vertices;++v) high=fmaxf(high,dot(a.vertices[v],p->contact.normal));
        for(unsigned v=0;v<b.amount_of_vertices;++v) low=fminf(low,dot(b.vertices[v],p->contact.normal));
        float depth=high-low,ma=inverse_mass(p->a,p->b),mb=inverse_mass(p->b,p->a);
        if(depth<=0 || ma+mb<=0) continue;
        PhysicsChange c[2]={physics_change_get(p->a,PHYSICS_CHANGE_POSITION),physics_change_get(p->b,PHYSICS_CHANGE_POSITION)};
        float correction=depth/(ma+mb);
        c[0].position.x-=p->contact.normal.x*correction*ma; c[0].position.y-=p->contact.normal.y*correction*ma;
        c[1].position.x+=p->contact.normal.x*correction*mb; c[1].position.y+=p->contact.normal.y*correction*mb;
        (void)physics_changes_commit(c,2,report,ROHR_PHYSICS_POSITION,physics_operation_entity_get(p->a),physics_operation_entity_get(p->b));
        ++counts.correction_rows;
    }
}
