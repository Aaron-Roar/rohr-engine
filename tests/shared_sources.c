/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_operation.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OK(x) assert(!error_check(x))
#define BAD(x) assert(error_check(x))
#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool deny_allocation;
static unsigned allocation_attempts;
static int fail_after = -1;
static bool allocation_reject_check(void) {
    if(deny_allocation) { ++allocation_attempts; return true; }
    if(fail_after >= 0 && fail_after-- == 0) return true;
    return false;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) {
    if(allocation_reject_check()) return NULL;
    return __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t size) {
    if(allocation_reject_check()) return NULL;
    return __real_calloc(n, size);
}
void *__wrap_realloc(void *p, size_t n) {
    if(allocation_reject_check()) return NULL;
    return __real_realloc(p, n);
}
#endif
static bool near(double a, double b) { return fabs(a-b) < 0.0001; }
static Entity make(void) { EntityResult r = entity_add(); OK(r); return r.result.value; }
static EntityIndex idx(Entity e) { EntityIndex i; assert(entity_index_get(e,&i)); return i; }
static Entity body(Position position) {
    Entity e = make();
    OK(physics_position_set(e, position)); OK(physics_orientation_set(e,0));
    OK(physics_hitbox_set(e, math_square_create(2,2)));
    OK(physics_mass_set(e,2)); OK(physics_dynamic_set(e));
    OK(entity_components_delete(e, ROHR_COLLISION));
    return e;
}
static ForceDescription at_com(Entity target, float angle, float magnitude) {
    return (ForceDescription){.target=target,
        .location.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=target},
        .orientation.offset=angle,.magnitude=magnitude};
}
static Entity add(ForceDescription d) { EntityResult r=physics_force_add(d); OK(r); return r.result.value; }
static PhysicsChange evaluate(Entity source, bool torque, bool expected) {
    PhysicsSourceChanges change={0}; bool active=false;
    OK(physics_source_change_get(source,torque,&change,&active));
    assert(active==expected); return change.values[0];
}
static uint64_t metric(Report r, ReportMetricId id) {
    uint64_t n=0;
    for(size_t i=0;i<r.statistic_count;++i) if(r.statistics[i].metric_id==id) n+=r.statistics[i].value;
    return n;
}
static EntityInfo inspect(Entity e) {
    EntityInfoResult r=entity_info_get(e,ROHR_ENTITY_INFO_PHYSICS_FORCE|ROHR_ENTITY_INFO_PHYSICS_TORQUE);
    OK(r); return r.result.value;
}
static void api_and_validation(void) {
    Entity target=body((Position){2,3});
    Entity incomplete_beam=make(); OK(entity_components_add(incomplete_beam,ROHR_SOFT_BODY_BEAM));
    Entity zero=add((ForceDescription){0});
    EntityResult t=rohr_physics_torque_add((TorqueDescription){0}); OK(t);
    evaluate(zero,false,false); evaluate(t.result.value,true,false);
    ForceDescription d=at_com(target,90,8); Entity source=add(d);
    d.magnitude=999;
    assert(physics_force_magnitude_get(source).result.value==8);
    OK(rohr_physics_force_target_set(zero,target));
    assert(rohr_physics_force_location_get(zero).result.value.reference.kind==ROHR_FORCE_WORLD_POSITION);
    OK(rohr_physics_force_description_set(zero,physics_force_description_get(source).result.value));
    OK(rohr_physics_force_magnitude_set(zero,-8));
    OK(rohr_physics_force_orientation_set(zero,(ForceOrientation){.offset=450}));
    OK(rohr_physics_force_location_set(zero,(ForceLocation){.offset={4,3}}));
    assert(physics_force_magnitude_get(zero).result.value==-8);
    assert(physics_force_orientation_get(zero).result.value.offset==450);
    assert(physics_force_target_get(zero).result.value==target);
    assert(physics_force_location_get(zero).result.value.offset.x==4);
    OK(rohr_physics_torque_target_set(t.result.value,target));
    OK(rohr_physics_torque_magnitude_set(t.result.value,-10));
    assert(physics_torque_target_get(t.result.value).result.value==target);
    assert(physics_torque_description_get(t.result.value).result.value.magnitude==-10);
    assert(rohr_physics_torque_magnitude_get(t.result.value).result.value==-10);
    OK(rohr_physics_torque_description_set(t.result.value,(TorqueDescription){.target = target, .magnitude = 7}));
    ForceDescription before=physics_force_description_get(source).result.value;
    uint32_t count=entity_alive_count_get();
    for(unsigned n=0;n<11;++n) {
        ForceDescription invalid=before;
        switch(n) {
        case 0: invalid.magnitude=NAN; break;
        case 1: invalid.magnitude=INFINITY; break;
        case 2: invalid.location.offset.x=NAN; break;
        case 3: invalid.orientation.offset=INFINITY; break;
        case 4: invalid.location.reference.kind=99; break;
        case 5: invalid.orientation.reference.kind=99; break;
        case 6: invalid.location.reference.entity=ENTITY_INVALID; break;
        case 7: invalid.location.reference.kind=ROHR_FORCE_BEAM_POSITION; invalid.location.reference.beam.entity=target; break;
        case 8: invalid.orientation.reference.kind=ROHR_FORCE_BODY_ORIENTATION; invalid.orientation.reference.entity=ENTITY_INVALID; break;
        case 9: invalid.orientation.reference.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION;
            invalid.orientation.reference.between.to.offset.y=INFINITY; break;
        case 10: invalid.location.reference=(ForcePositionReference){.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={.entity=incomplete_beam}}; break;
        }
        BAD(physics_force_add(invalid)); BAD(physics_force_description_set(source,invalid));
        ForceDescription after=physics_force_description_get(source).result.value;
        assert(memcmp(&before,&after,sizeof(before))==0 && count==entity_alive_count_get());
    }
    BAD(physics_force_magnitude_set(source,NAN));
    BAD(physics_torque_magnitude_set(t.result.value,INFINITY));
    assert(physics_torque_magnitude_get(t.result.value).result.value==7);
    /* Inactive union storage is not configuration. */
    ForceDescription unused={0}; memset(&unused.location.reference,0xff,sizeof(unused.location.reference));
    unused.location.reference.kind=ROHR_FORCE_WORLD_POSITION;
    memset(&unused.orientation.reference,0xff,sizeof(unused.orientation.reference));
    unused.orientation.reference.kind=ROHR_FORCE_WORLD_ORIENTATION;
    OK(physics_force_description_set(source,unused)); evaluate(source,false,false);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_attempts=0; deny_allocation=true;
    for(unsigned n=0;n<10;++n) {
        OK(physics_force_description_get(source)); OK(physics_force_magnitude_set(source,n));
        OK(physics_torque_description_get(t.result.value)); evaluate(source,false,false);
    }
    deny_allocation=false; assert(allocation_attempts==0);
#endif
    OK(entity_delete(source)); Entity replacement=make(); assert(replacement!=source);
    BAD(physics_force_description_get(source)); BAD(physics_force_description_set(source,(ForceDescription){0}));
    Entity plain=make(); BAD(physics_force_magnitude_set(plain,1));
    BAD(physics_force_description_get(plain));
}
static void force_geometry(void) {
    Entity target=body((Position){10,20});
    Entity source=add(at_com(target,90,6));
    PhysicsChange c=evaluate(source,false,true);
    assert(near(c.force_acceleration.x,3) && near(c.force_acceleration.y,0));
    assert(near(c.torque_acceleration,0));
    OK(physics_force_location_set(source,(ForceLocation){.offset={10,22}}));
    c=evaluate(source,false,true);
    /* Square inertia = m*(w*w+h*h)/12 = 4/3; moment = 2*6. */
    assert(near(math_degrees_to_radians(c.torque_acceleration),9));
    OK(physics_force_magnitude_set(source,-6)); c=evaluate(source,false,true);
    assert(near(c.force_acceleration.x,-3) && near(math_degrees_to_radians(c.torque_acceleration),-9));
    Entity reference=body((Position){30,40}); OK(physics_orientation_set(reference,90));
    OK(physics_force_description_set(source,(ForceDescription){.target=target,
        .location={.offset={2,0},.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=reference}},
        .orientation={.offset=0,.reference={.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=reference}},.magnitude=6}));
    c=evaluate(source,false,true); /* Origin offset (2,0) rotates to (0,-2). */
    assert(near(math_degrees_to_radians(c.torque_acceleration),81));
    OK(physics_force_location_set(source,(ForceLocation){.offset={2,0},
        .reference={.kind=ROHR_FORCE_COM_POSITION,.entity=reference}}));
    c=evaluate(source,false,true); assert(near(math_degrees_to_radians(c.torque_acceleration),90));
    OK(physics_position_set(reference,(Position){30,42}));
    c=evaluate(source,false,true); assert(near(math_degrees_to_radians(c.torque_acceleration),99));
    /* Location and orientation can follow unrelated references, including static entities. */
    OK(physics_static_set(reference));
    OK(physics_force_orientation_set(source,(ForceOrientation){.reference={
        .kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION,
        .between={.from={.offset={1,2}},.to={.offset={4,6}}}}}));
    c=evaluate(source,false,true); assert(near(c.force_acceleration.x,1.8) && near(c.force_acceleration.y,2.4));
    ForceDescription vector=physics_force_vector_description(target,(Force){3,4});
    OK(physics_force_description_set(source,vector)); c=evaluate(source,false,true);
    assert(near(c.force_acceleration.x,1.5) && near(c.force_acceleration.y,2));
    /* Overflow of the angular half must not apply the valid linear half. */
    OK(physics_force_location_set(source,(ForceLocation){.offset={0,FLT_MAX}}));
    OK(physics_force_orientation_set(source,(ForceOrientation){.offset=90}));
    OK(physics_force_magnitude_set(source,FLT_MAX));
    Report report=report_create(); report.header=(ReportHeader){ROHR_PHYSICS_INTEGRATION_REPORT_ID,ROHR_REPORT_COMPLETE,0};
    physics_sources_apply(&report,false);
    assert(force_accelerations[idx(target)].x==0 && torque_angular_accelerations[idx(target)]==0);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1);
}
static void beam_references(void) {
    EntityResult soft=physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
    EntityResult a=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = (Position){0,0},
        .mass = 1,
        .radius = 0.1f
    }); OK(a);
    EntityResult b=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = (Position){4,0},
        .mass = 1,
        .radius = 0.1f
    }); OK(b);
    EntityResult beam=physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = soft.result.value,
        .node_a = a.result.value,
        .node_b = b.result.value,
        .stiffness = 0,
        .damping = 0
    }); OK(beam);
    Entity target=body((Position){0,0});
    ForceDescription d={.target=target,.magnitude=2,
        .location={.offset={1,2},.reference={.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={beam.result.value,ROHR_FORCE_BEAM_CENTER,0.25f}}}};
    Entity source=add(d); PhysicsChange c=evaluate(source,false,true);
    assert(near(math_degrees_to_radians(c.torque_acceleration),-6)); /* P=(4,2), F=(0,2). */
    for(int anchor=0;anchor<3;++anchor) {
        d.location.reference.beam.anchor=anchor;
        OK(physics_force_description_set(source,d)); c=evaluate(source,false,true);
        assert(near(math_degrees_to_radians(c.torque_acceleration),-1.5*(2*anchor+2)));
    }
    d.orientation.reference=(ForceOrientationReference){.kind=ROHR_FORCE_BEAM_ORIENTATION,.entity=beam.result.value};
    OK(physics_force_description_set(source,d)); c=evaluate(source,false,true);
    assert(near(c.force_acceleration.x,1) && near(math_degrees_to_radians(c.torque_acceleration),3));
    OK(physics_position_set(b.result.value,(Position){0,8}));
    c=evaluate(source,false,true);
    assert(near(c.force_acceleration.y,1) && near(math_degrees_to_radians(c.torque_acceleration),3));
    /* Geometry failures are accepted at API time and observed at evaluation, even at zero/no target. */
    OK(physics_position_set(b.result.value,(Position){ROHR_PHYSICS_BEAM_MIN_LENGTH,0}));
    d.target=0; d.magnitude=0; OK(physics_force_description_set(source,d));
    PhysicsSourceChanges unused; bool active; BAD(physics_source_change_get(source,false,&unused,&active));
    OK(physics_position_set(b.result.value,(Position){2*ROHR_PHYSICS_BEAM_MIN_LENGTH,0}));
    evaluate(source,false,false);
    OK(entity_delete(a.result.value)); BAD(physics_source_change_get(source,false,&unused,&active));
    d.location.reference.beam.anchor=99; BAD(physics_force_description_set(source,d));
}
static void suspension_and_inspection(void) {
    Entity target=body((Position){0}); Entity reference=body((Position){3,4});
    ForceDescription d=at_com(target,0,2); d.location.reference.entity=reference;
    Entity source=add(d); Entity good=add(at_com(target,90,2));
    (void)good;
    EntityInfo before=inspect(source); assert(before.force_eligible && before.force.target==target);
    OK(entity_delete(reference)); Entity replacement=body((Position){8,9}); assert(reference!=replacement);
    EntityInfo suspended=inspect(source); assert(!suspended.force_eligible);
    assert(suspended.force.location.reference.entity==reference);
    assert(entity_info_issues_count_get(&suspended,ROHR_ENTITY_INFO_PHYSICS_FORCE).result.value==1);
    Report report=system_physics_update(0.1);
    assert(!report.has_fatal && metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1);
    assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==1);
    assert(near(velocities[idx(target)].x,0.1) && near(velocities[idx(target)].y,0));
    assert(report.diagnostics[0].source.id==source && report.diagnostics[0].target.id==target);
    ReportResult stored=report_get(ROHR_PHYSICS_REPORT); OK(stored);
    EntityInfo unchanged=inspect(source); OK(entity_info_release(&unchanged));
    ReportResult after=report_get(ROHR_PHYSICS_REPORT); OK(after);
    assert(memcmp(&stored.result.value,&after.result.value,sizeof(Report))==0);
    OK(physics_force_location_set(source,(ForceLocation){.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=target}}));
    report=system_physics_update(0.1); assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==2);
    assert(near(velocities[idx(target)].y,0.1));
    assert(before.force_eligible && !suspended.force_eligible);
    OK(physics_force_orientation_set(source,(ForceOrientation){.reference={.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION}}));
    OK(physics_force_magnitude_set(source,0)); OK(physics_force_target_set(source,0));
    report=system_physics_update(0.1); assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1);
    OK(physics_force_orientation_set(source,(ForceOrientation){0}));
    report=system_physics_update(0.1); assert(metric(report,ROHR_REPORT_SOURCE_INACTIVE_COUNT)==1);
    engine_stop(); OK(engine_start());
    assert(before.force.location.reference.entity==reference && suspended.force.target==target);
    OK(entity_info_release(&before)); OK(entity_info_release(&suspended));
}
static void particles(void) {
    ParticleDescription config=physics_particle_description_default_get(); config.mass_value=2;
    config.collision_enabled=false;
    EntityResult p=physics_particle_add(config); OK(p);
    Entity rigid=body((Position){10,0}); OK(physics_particle_radius_set(rigid,1));
    EntityResult soft=physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
    EntityResult node=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = (Position){20,0},
        .mass = 2,
        .radius = 0.2f
    }); OK(node);
    Entity entities[]={p.result.value,node.result.value,rigid};
    for(unsigned n=0;n<3;++n) {
        Entity source=add((ForceDescription){.target=entities[n],.location.offset={100,2},
            .orientation.offset=90,.magnitude=2});
        PhysicsChange c=evaluate(source,false,true); assert(near(c.force_acceleration.x,1));
        assert(n==2 ? c.torque_acceleration>0 : c.torque_acceleration==0);
        EntityResult torque=physics_torque_add((TorqueDescription){.target = entities[n], .magnitude = 2}); OK(torque);
        evaluate(torque.result.value,true,n==2);
    }
    Report r=system_physics_update(0.1); assert(!r.has_fatal);
    assert(angular_velocities[idx(p.result.value)]==0 && angular_velocities[idx(node.result.value)]==0);
    assert(angular_velocities[idx(rigid)]>0 && orientations[idx(rigid)]>0);
}
static void allocation_failure(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    unsigned failures=0;
    for(int ordinal=0;ordinal<128;++ordinal) {
        engine_stop(); OK(engine_start());
        Entity retained=add((ForceDescription){.magnitude=7});
        while(entity_alive_count_get()<positions_pool.capacity) make();
        uint32_t count=entity_alive_count_get(); fail_after=ordinal;
        EntityResult result=physics_force_add((ForceDescription){0});
        fail_after=-1;
        if(!error_check(result)) break;
        ++failures;
        assert(entity_alive_count_get()==count);
        assert(physics_force_magnitude_get(retained).result.value==7);
        EntityResult retry=physics_force_add((ForceDescription){0}); OK(retry);
        EntityIndex index=idx(retry.result.value);
        /* A partial table expansion must not leave the new live index outside
         * another pool; getters, lifecycle and subsequent additions need all. */
        assert(index<entity_mask_pool.capacity && index<life_times_pool.capacity);
        assert(index<forces_pool.capacity && index<torques_pool.capacity);
        assert(entity_components_check(retry.result.value,ROHR_FORCE));
        OK(physics_force_description_get(retry.result.value));
        OK(physics_force_apply((ForceDescription){0}));
    }
    assert(failures>40 && failures<128);
#endif
}

static void persistence_references(void) {
    /* Source precedes its dependencies in serialization order. */
    Entity source=add((ForceDescription){0}); OK(entity_name_set(source,"source"));
    Entity target=body((Position){5,7}), reference=body((Position){2,3});
    OK(entity_name_set(target,"target")); OK(entity_name_set(reference,"reference"));
    ForceDescription d={.target=target,.magnitude=-6,
        .location={.offset={3,4},.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=reference}},
        .orientation={.offset=720,.reference={.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION,
            .between={.from={.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=reference}},
                      .to={.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=target}}}}}};
    OK(physics_force_description_set(source,d));
    EntityResult torque=physics_torque_add((TorqueDescription){0}); OK(torque);
    OK(entity_name_set(torque.result.value,"zero_torque"));
    OK(game_state_file_save("shared_sources_state.json"));
    engine_stop(); OK(engine_start());
    OK(game_state_file_load("shared_sources_state.json"));
    EntityResult loaded=entity_by_name_get("source"); OK(loaded);
    EntityResult t=entity_by_name_get("target"), r=entity_by_name_get("reference"); OK(t); OK(r);
    ForceDescriptionResult got=rohr_physics_force_description_get(loaded.result.value); OK(got);
    assert(got.result.value.target==t.result.value && got.result.value.magnitude==-6);
    assert(got.result.value.location.offset.y==4 && got.result.value.location.reference.entity==r.result.value);
    assert(got.result.value.orientation.offset==720);
    assert(got.result.value.orientation.reference.between.from.reference.entity==r.result.value);
    assert(got.result.value.orientation.reference.between.to.reference.entity==t.result.value);
    evaluate(loaded.result.value,false,true);
    EntityResult z=entity_by_name_get("zero_torque"); OK(z); evaluate(z.result.value,true,false);
    assert(remove("shared_sources_state.json")==0);
    /* Malformed source input rolls back the new entity without changing existing state. */
    FILE *file=fopen("shared_sources_invalid.json","w"); assert(file);
    fprintf(file,"{\"version\":%u,\"entities\":[{\"name\":\"invalid_source\",\"components\":{\"force\":{\"target\":null,\"magnitude\":1,\"location\":{\"offset\":{\"x\":0,\"y\":0},\"kind\":99},\"orientation\":{\"kind\":0,\"offset\":0}}}}]}",GAME_STATE_VERSION);
    assert(fclose(file)==0);
    uint32_t before=entity_alive_count_get(); BAD(game_state_file_load("shared_sources_invalid.json"));
    assert(entity_alive_count_get()==before);
    got=physics_force_description_get(loaded.result.value); OK(got); assert(got.result.value.magnitude==-6);
    assert(remove("shared_sources_invalid.json")==0);
}
static void substeps_and_expiry(void) {
    Entity target=body((Position){0}); Entity reference=body((Position){20,0});
    OK(physics_angular_velocity_set(reference,90));
    ForceDescription d=at_com(target,0,2);
    d.orientation.reference=(ForceOrientationReference){.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=reference};
    Entity persistent=add(d); OK(physics_force_apply(d));
    OK(physics_substeps_set(2)); Report report=system_physics_update(1);
    assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==4);
    /* Per-substep source resolution: headings 0 and 45 degrees, two unit loads. */
    assert(near(velocities[idx(target)].x,sqrt(0.5)));
    assert(near(velocities[idx(target)].y,1+sqrt(0.5)));
    uint64_t tick=engine_tick_get();
    OK(physics_force_description_set(persistent,(ForceDescription){.orientation.reference={
        .kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION}}));
    uint32_t before=entity_alive_count_get();
    OK(physics_force_apply(physics_force_description_get(persistent).result.value));
    report=system_physics_update(0.1);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==4);
    assert(engine_tick_get()==tick && entity_alive_count_get()==before+1);
    OK(engine_time_per_tick_set(0.001)); SDL_Delay(3); report=engine_update();
    assert(!report.has_fatal && engine_tick_get()>tick);
    assert(entity_alive_count_get()==before-1 && entity_alive_check(persistent));
    /* Removing the source component suspends no work and retains the target. */
    OK(entity_components_delete(persistent,ROHR_FORCE));
    report=system_physics_update(0.1);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==0);
    assert(entity_alive_check(target));
}

static Report source_report(void) {
    Report report=report_create();
    report.header=(ReportHeader){ROHR_PHYSICS_INTEGRATION_REPORT_ID,ROHR_REPORT_COMPLETE,0};
    return report;
}
static void beam_loads(void) {
    EntityResult owner=physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    EntityResult a=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){0,0},
        .mass = 2,
        .radius = 0.1f
    });
    EntityResult b=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){10,0},
        .mass = 4,
        .radius = 0.1f
    });
    OK(a); OK(b);
    EntityResult beam=physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = owner.result.value,
        .node_a = a.result.value,
        .node_b = b.result.value,
        .stiffness = 0,
        .damping = 0
    });
    OK(beam);
    ForceDescription d={.target=beam.result.value,.magnitude=12,.orientation.offset=90,
        .location.reference={.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={.entity=beam.result.value,.fraction=0.5f}}};
    Entity source=add(d);
    uint32_t count=entity_alive_count_get();
    for(unsigned variant=0;variant<3;++variant) {
        if(variant==1) {
            d.location.reference.beam.fraction=0;
            d.location.offset.x=5;
        } else if(variant==2) d.location=(ForceLocation){.offset={5,0}};
        OK(physics_force_description_set(source,d));
        force_accelerations[idx(a.result.value)]=(Acceleration){0};
        force_accelerations[idx(b.result.value)]=(Acceleration){0};
        Report report=source_report(); physics_sources_apply(&report,false);
        assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==1);
        assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==0);
        assert(near(force_accelerations[idx(a.result.value)].x,3));
        assert(near(force_accelerations[idx(b.result.value)].x,1.5));
        assert(entity_alive_count_get()==count);
    }
}

typedef struct BeamFixture { Entity owner, a, b, beam, source; } BeamFixture;
static BeamFixture beam_fixture(float stiffness, float damping) {
    EntityResult owner=physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    EntityResult a=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){0,0},
        .mass = 2,
        .radius = 0.1f
    });
    EntityResult b=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){10,0},
        .mass = 4,
        .radius = 0.1f
    });
    OK(a); OK(b);
    OK(entity_components_delete(a.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    OK(entity_components_delete(b.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    EntityResult beam=physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = owner.result.value,
        .node_a = a.result.value,
        .node_b = b.result.value,
        .stiffness = stiffness,
        .damping = damping
    });
    OK(beam); OK(physics_soft_body_beam_collision_disable(beam.result.value));
    return (BeamFixture){owner.result.value,a.result.value,b.result.value,beam.result.value,
        add((ForceDescription){.target=beam.result.value,.magnitude=12,.location.offset={5,0}})};
}
static PhysicsSourceChanges beam_evaluate(BeamFixture f, bool torque) {
    PhysicsSourceChanges changes={0}; bool active=false;
    OK(physics_source_change_get(f.source,torque,&changes,&active));
    assert(active && changes.count==2);
    return changes;
}
static bool close_value(double a, double b) {
    return fabs(a-b)<0.0001*(1+fabs(a)+fabs(b));
}
static uint32_t beam_seed=0x4192c6d3u;
static double beam_random(void) {
    beam_seed=1664525u*beam_seed+1013904223u;
    return (double)(beam_seed>>8)/16777216.0;
}
/* Independent virtual attachment used only to differentiate endpoint work. */
static void virtual_point(double ax, double ay, double bx, double by,
        double fraction, double height, double *px, double *py) {
    double dx=bx-ax, dy=by-ay, length=hypot(dx,dy);
    *px=ax+fraction*dx-height*dy/length;
    *py=ay+fraction*dy+height*dx/length;
}
static void beam_equivalence_and_work(void) {
    BeamFixture f=beam_fixture(0,0);
    for(unsigned sample=0;sample<120;++sample) {
        Position a={(float)(10*beam_random()),(float)(10*beam_random())};
        double angle=6.28*beam_random(), length=2+20*beam_random();
        Position b={a.x+length*cos(angle),a.y+length*sin(angle)};
        OK(physics_position_set(f.a,a)); OK(physics_position_set(f.b,b));
        double dx=(double)b.x-a.x,dy=(double)b.y-a.y;
        length=hypot(dx,dy);
        double alpha=3*beam_random()-1, height=4*beam_random()-2;
        double px,py; virtual_point(a.x,a.y,b.x,b.y,alpha,height,&px,&py);
        Position point={px,py};
        ForceDescription d={.target=f.beam,.location.offset=point,
            .magnitude=sample%2 ? -12 : 12,.orientation.offset=(float)(360*beam_random())};
        OK(rohr_physics_force_description_set(f.source,d));
        PhysicsSourceChanges expected=beam_evaluate(f,false);
        double fax=expected.values[0].force_acceleration.x*2;
        double fay=expected.values[0].force_acceleration.y*2;
        double fbx=expected.values[1].force_acceleration.x*4;
        double fby=expected.values[1].force_acceleration.y*4;
        double radians=math_degrees_to_radians(d.orientation.offset);
        double fx=d.magnitude*sin(radians),fy=d.magnitude*cos(radians);
        assert(close_value(fax+fbx,fx) && close_value(fay+fby,fy));
        assert(close_value(a.y*fax-a.x*fay+b.y*fbx-b.x*fby,point.y*fx-point.x*fy));
        for(unsigned variant=0;variant<3;++variant) {
            /* All anchors and both pure-fraction and combined-distance representations. */
            float distance=variant==0 ? 0 : (float)(alpha*length*0.6);
            d.location=(ForceLocation){.offset={distance,height},
                .reference={.kind=ROHR_FORCE_BEAM_POSITION,
                    .beam={f.beam,(ForceBeamAnchor)variant,alpha-0.5*variant-distance/length}}};
            OK(physics_force_description_set(f.source,d));
            PhysicsSourceChanges actual=beam_evaluate(f,false);
            for(unsigned n=0;n<2;++n) {
                assert(close_value(actual.values[n].force_acceleration.x,expected.values[n].force_acceleration.x));
                assert(close_value(actual.values[n].force_acceleration.y,expected.values[n].force_acceleration.y));
            }
        }
        /* Central differences, with alpha/h frozen at the resolved world point. */
        double resolved_alpha=(((double)point.x-a.x)*dx+((double)point.y-a.y)*dy)/(length*length);
        double resolved_height=(-((double)point.x-a.x)*dy+((double)point.y-a.y)*dx)/length;
        const double epsilon=1e-5;
        double plus_x,plus_y,minus_x,minus_y;
        virtual_point(a.x+epsilon*0.3,a.y-epsilon*0.4,b.x-epsilon*0.7,b.y+epsilon*0.8,
            resolved_alpha,resolved_height,&plus_x,&plus_y);
        virtual_point(a.x-epsilon*0.3,a.y+epsilon*0.4,b.x+epsilon*0.7,b.y-epsilon*0.8,
            resolved_alpha,resolved_height,&minus_x,&minus_y);
        double attachment_work=(fx*(plus_x-minus_x)+fy*(plus_y-minus_y))/(2*epsilon);
        assert(close_value(attachment_work,0.3*fax-0.4*fay-0.7*fbx+0.8*fby));
    }
    /* Same point after stretching must still agree; fraction and distance can diverge. */
    OK(physics_position_set(f.a,(Position){0,0}));
    for(unsigned stretch=0;stretch<2;++stretch) {
        OK(physics_position_set(f.b,(Position){stretch ? 20 : 10,0}));
        for(unsigned fixed=0;fixed<2;++fixed) {
            ForceDescription d={.target=f.beam,.magnitude=12,.orientation.offset=90,
                .location={.offset={fixed ? 5 : 0,0},.reference={.kind=ROHR_FORCE_BEAM_POSITION,
                    .beam={f.beam,ROHR_FORCE_BEAM_START,fixed ? 0 : 0.5f}}}};
            OK(physics_force_description_set(f.source,d));
            PhysicsSourceChanges c=beam_evaluate(f,false);
            assert(close_value(c.values[1].force_acceleration.x,stretch && fixed ? 0.75 : 1.5));
            d.location=(ForceLocation){.offset={stretch && !fixed ? 10 : 5,0}};
            OK(physics_force_description_set(f.source,d));
            PhysicsSourceChanges world=beam_evaluate(f,false);
            assert(close_value(c.values[0].force_acceleration.x,world.values[0].force_acceleration.x));
            assert(close_value(c.values[1].force_acceleration.x,world.values[1].force_acceleration.x));
        }
    }
}

static void beam_failures_and_lifetime(void) {
    BeamFixture f=beam_fixture(0,0);
    EntityInfo original=inspect(f.source); assert(original.force_eligible);
    uint32_t count=entity_alive_count_get();
    force_accelerations[idx(f.a)]=(Acceleration){1,2};
    force_accelerations[idx(f.b)]=(Acceleration){3,4};
    /* A valid first endpoint cannot commit if the second would overflow. */
    OK(physics_mass_set(f.b,FLT_MIN));
    OK(physics_force_magnitude_set(f.source,100));
    Report report=source_report(); physics_sources_apply(&report,false);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1);
    assert(force_accelerations[idx(f.a)].y==2 && force_accelerations[idx(f.b)].y==4);
    assert(report.diagnostics[0].source.id==f.source && report.diagnostics[0].target.id==f.beam);
    EntityInfo failed=inspect(f.source); assert(!failed.force_eligible && original.force_eligible);
    OK(entity_info_release(&failed));
    OK(physics_mass_set(f.b,4)); OK(physics_force_magnitude_set(f.source,12));
    /* Fixed nodes do not donate their shares; held and kinematic nodes behave likewise. */
    OK(physics_static_set(f.a));
    report=source_report(); physics_sources_apply(&report,false);
    assert(force_accelerations[idx(f.a)].y==2 && near(force_accelerations[idx(f.b)].y,5.5));
    OK(physics_static_set(f.b));
    report=source_report(); physics_sources_apply(&report,false);
    assert(metric(report,ROHR_REPORT_SOURCE_INACTIVE_COUNT)==1);
    OK(physics_dynamic_set(f.a)); OK(physics_dynamic_set(f.b));
    OK(entity_components_add(f.a,ROHR_HOLD));
    PhysicsSourceChanges changes; bool active;
    OK(physics_source_change_get(f.source,false,&changes,&active)); assert(active && changes.count==1);
    OK(entity_components_delete(f.a,ROHR_HOLD));
    OK(physics_kinematic_driven_set(f.a));
    OK(physics_source_change_get(f.source,false,&changes,&active)); assert(active && changes.count==1);
    OK(physics_kinematic_driven_remove(f.a));
    for(unsigned n=0;n<3;++n) {
        float length=n==0 ? 0 : n==1 ? ROHR_PHYSICS_BEAM_MIN_LENGTH :
            nextafterf(ROHR_PHYSICS_BEAM_MIN_LENGTH,INFINITY);
        OK(physics_position_set(f.b,(Position){length,0}));
        EngineResult result=physics_source_change_get(f.source,false,&changes,&active);
        if(n<2) { BAD(result); assert(!active); } else { OK(result); assert(active); }
    }
    OK(physics_position_set(f.b,(Position){10,0}));
    force_accelerations[idx(f.a)]=(Acceleration){0}; force_accelerations[idx(f.b)]=(Acceleration){0};
    /* One source evaluation, two endpoint changes, no allocation during evaluation. */
    report=source_report();
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_attempts=0; deny_allocation=true;
#endif
    physics_sources_apply(&report,false);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    deny_allocation=false; assert(allocation_attempts==0);
#endif
    assert(entity_alive_count_get()==count && metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==1);
    assert(near(force_accelerations[idx(f.a)].y,3) && near(force_accelerations[idx(f.b)].y,1.5));
    /* Endpoint replacement is re-resolved, and stale generation handles never retarget. */
    EntityResult c=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = f.owner,
        .position = (Position){20,0},
        .mass = 8,
        .radius = 0.1f
    }); OK(c);
    soft_body_beams[idx(f.beam)].node_b=c.result.value;
    OK(physics_source_change_get(f.source,false,&changes,&active));
    assert(active && changes.values[1].index==idx(c.result.value));
    OK(entity_delete(c.result.value));
    Entity replacement=make(); assert(replacement!=c.result.value);
    BAD(physics_source_change_get(f.source,false,&changes,&active)); assert(!active);
    /* Endpoint deletion also deletes its connected beam; repair with a new beam. */
    if(entity_alive_check(f.beam)) OK(entity_delete(f.beam));
    EntityResult rebuilt=physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = f.owner,
        .node_a = f.a,
        .node_b = f.b,
        .stiffness = 0,
        .damping = 0
    }); OK(rebuilt);
    OK(physics_force_target_set(f.source,rebuilt.result.value));
    OK(physics_source_change_get(f.source,false,&changes,&active)); assert(active);
    assert(original.force.target==f.beam && original.force_eligible);
    OK(entity_info_release(&original));
    OK(entity_delete(rebuilt.result.value));
    BAD(physics_source_change_get(f.source,false,&changes,&active));
}

static void beam_torque_and_motion(void) {
    BeamFixture f=beam_fixture(0,0);
    OK(entity_components_delete(f.source,ROHR_FORCE));
    OK(physics_torque_description_set(f.source,(TorqueDescription){.target = f.beam, .magnitude = 20}));
    PhysicsSourceChanges c=beam_evaluate(f,true);
    assert(near(c.values[0].force_acceleration.y,1));
    assert(near(c.values[1].force_acceleration.y,-0.5));
    OK(rohr_physics_torque_magnitude_set(f.source,-20)); c=beam_evaluate(f,true);
    assert(near(c.values[0].force_acceleration.y,-1));
    assert(near(c.values[1].force_acceleration.y,0.5));
    OK(physics_torque_magnitude_set(f.source,0)); evaluate(f.source,true,false);
    OK(physics_torque_magnitude_set(f.source,20));
    OK(physics_substeps_set(4));
    Report report=system_physics_update(0.01);
    assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==4);
    assert(close_value(2*velocities[idx(f.a)].y+4*velocities[idx(f.b)].y,0));
    assert(angular_velocities[idx(f.a)]==0 && angular_velocities[idx(f.b)]==0);
    ReportResult stored=report_get(ROHR_PHYSICS_REPORT); OK(stored);
    EntityInfo info=inspect(f.source); assert(info.torque_eligible); OK(entity_info_release(&info));
    ReportResult after=report_get(ROHR_PHYSICS_REPORT); OK(after);
    assert(memcmp(&stored.result.value,&after.result.value,sizeof(Report))==0);
    OK(entity_delete(f.source));
    uint32_t before=entity_alive_count_get();
    OK(rohr_physics_torque_apply((TorqueDescription){.target = f.beam, .magnitude = 20}));
    uint64_t tick=engine_tick_get();
    report=system_physics_update(0.01);
    assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==4);
    assert(engine_tick_get()==tick && entity_alive_count_get()==before+1);
    OK(engine_time_per_tick_set(0.001)); SDL_Delay(3); report=engine_update();
    assert(!report.has_fatal && engine_tick_get()>tick && entity_alive_count_get()==before);
}

static double beam_energy_error(unsigned steps, bool fixed, float damping) {
    BeamFixture f=beam_fixture(8,damping);
    if(fixed) OK(physics_static_set(f.a));
    OK(physics_force_description_set(f.source,(ForceDescription){.target=f.beam,.magnitude=4,
        .orientation.offset=90,.location.reference={.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={f.beam,ROHR_FORCE_BEAM_END,0}}}));
    double dt=1.0/steps, work=0, dissipated=0;
    for(unsigned step=0;step<steps;++step) {
        Position a=positions[idx(f.a)], b=positions[idx(f.b)];
        double relative=velocities[idx(f.b)].x-velocities[idx(f.a)].x;
        dissipated+=damping*relative*relative*dt;
        Report report=system_physics_update(dt); assert(!report.has_fatal);
        work+=4*((double)positions[idx(f.b)].x-b.x);
        if(fixed) assert(positions[idx(f.a)].x==a.x && velocities[idx(f.a)].x==0);
    }
    double va=velocities[idx(f.a)].x, vb=velocities[idx(f.b)].x;
    double extension=(double)positions[idx(f.b)].x-positions[idx(f.a)].x-10;
    assert(extension>0.1); /* Actual deformation, not imposed rigid motion. */
    double energy=va*va+2*vb*vb+4*extension*extension;
    double error=fabs(work-energy-dissipated);
    OK(entity_delete(f.source)); OK(entity_delete(f.owner));
    return error;
}
static void beam_energy(void) {
    for(unsigned scenario=0;scenario<3;++scenario) {
        bool fixed=scenario==1; float damping=scenario==2 ? 0.7f : 0;
        double coarse=beam_energy_error(40,fixed,damping);
        double fine=beam_energy_error(160,fixed,damping);
        assert(fine<coarse*0.6 && fine<0.05);
    }
}

/* Whole-body analytics use independent net-force and moment sums. */
static PhysicsSourceChanges soft_evaluate(Entity source, bool torque, size_t count) {
    PhysicsSourceChanges changes={0}; bool active=false;
    OK(physics_source_change_get(source,torque,&changes,&active));
    assert(active==(count!=0) && changes.count==count);
    return changes;
}
static void soft_result_check(PhysicsSourceChanges c, double fx, double fy, double moment) {
    double x=0,y=0,t=0;
    for(size_t n=0;n<c.count;++n) {
        EntityIndex i=c.values[n].index;
        double nx=c.values[n].force_acceleration.x*mass[i];
        double ny=c.values[n].force_acceleration.y*mass[i];
        x+=nx; y+=ny; t+=positions[i].y*nx-positions[i].x*ny;
    }
    assert(close_value(x,fx) && close_value(y,fy) && close_value(t,moment));
}
static void soft_analytics(void) {
    BeamFixture f=beam_fixture(0,0);
    OK(physics_force_description_set(f.source,at_com(f.owner,0,12)));
    PhysicsSourceChanges c=soft_evaluate(f.source,false,2);
    assert(near(c.values[0].force_acceleration.y,2) && near(c.values[1].force_acceleration.y,2));
    soft_result_check(c,0,12,-80);
    /* Representable common point: world, COM offset, origin and beam references. */
    OK(physics_mass_set(f.b,6)); /* COM is (7.5,0). */
    ForceLocation locations[]={
        {.offset={10,2}},
        {.offset={2.5f,2},.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=f.owner}},
        {.offset={10,2},.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=f.owner}},
        {.offset={0,2},.reference={.kind=ROHR_FORCE_BEAM_POSITION,
            .beam={f.beam,ROHR_FORCE_BEAM_END,0}}}
    };
    for(unsigned n=0;n<4;++n) {
        OK(rohr_physics_force_location_set(f.source,locations[n]));
        c=soft_evaluate(f.source,false,2); soft_result_check(c,0,12,-120);
        assert(near(c.values[0].force_acceleration.y,0) && near(c.values[1].force_acceleration.y,2));
    }
    OK(physics_force_orientation_set(f.source,(ForceOrientation){.offset=90}));
    c=soft_evaluate(f.source,false,2); soft_result_check(c,12,0,24);
    OK(physics_force_magnitude_set(f.source,-12));
    c=soft_evaluate(f.source,false,2); soft_result_check(c,-12,0,-24);
    OK(entity_components_delete(f.source,ROHR_FORCE));
    for(int sign=-1;sign<=1;sign+=2) {
        OK(rohr_physics_torque_description_set(f.source,(TorqueDescription){.target = f.owner, .magnitude = sign*30}));
        c=soft_evaluate(f.source,true,2); soft_result_check(c,0,0,sign*30);
        assert(near(c.values[0].force_acceleration.y,sign*1.5));
        assert(near(c.values[1].force_acceleration.y,-sign*0.5));
    }
    /* Both current masses and geometry affect each subsequent evaluation. */
    OK(physics_position_set(f.b,(Position){0,20})); OK(physics_mass_set(f.b,4));
    c=soft_evaluate(f.source,true,2); soft_result_check(c,0,0,30);
    assert(near(c.values[0].force_acceleration.x,-0.75));
    assert(near(c.values[1].force_acceleration.x,0.375));
    EntityResult extra=physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = f.owner,
        .position = (Position){5,-4},
        .mass = 3,
        .radius = 0.1f
    }); OK(extra);
    c=soft_evaluate(f.source,true,3); soft_result_check(c,0,0,30);
    OK(entity_delete(extra.result.value));
    c=soft_evaluate(f.source,true,2); soft_result_check(c,0,0,30);
    /* No orientation-following or velocity overwrite. */
    OK(physics_orientation_set(f.owner,77));
    velocities[idx(f.a)]=(Velocity){3,4};
    c=soft_evaluate(f.source,true,2); soft_result_check(c,0,0,30);
    assert(velocities[idx(f.a)].x==3 && velocities[idx(f.a)].y==4);
}
static void soft_capacity_and_failures(void) {
    EntityResult owner=physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    Entity nodes[SOFT_BODY_MAX_NODES];
    for(unsigned n=0;n<SOFT_BODY_MAX_NODES;++n) {
        EntityResult node=physics_soft_body_node_add((SoftBodyNodeDescription){
            .restitution = 0.25f, .collision_enabled = true,
            .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
            .owner = owner.result.value,
            .position = (Position){n,n%3},
            .mass = n+1,
            .radius = 0.1f
        });
        OK(node); nodes[n]=node.result.value;
    }
    Entity source=add(at_com(owner.result.value,0,2080)); /* sum 1..64 */
    PhysicsSourceChanges c=soft_evaluate(source,false,SOFT_BODY_MAX_NODES);
    for(unsigned n=0;n<SOFT_BODY_MAX_NODES;++n) assert(near(c.values[n].force_acceleration.y,1));
    uint32_t alive=entity_alive_count_get();
    Report report=source_report();
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_attempts=0; deny_allocation=true;
#endif
    physics_sources_apply(&report,false);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    deny_allocation=false; assert(allocation_attempts==0);
#endif
    assert(entity_alive_count_get()==alive && metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==1);
    EntityInfo good=inspect(source); assert(good.force_eligible);
    for(unsigned n=0;n<SOFT_BODY_MAX_NODES;++n) force_accelerations[idx(nodes[n])]=(Acceleration){0};
    /* Last-node accumulator overflow must leave every earlier node untouched. */
    OK(physics_force_magnitude_set(source,FLT_MAX));
    force_accelerations[idx(nodes[SOFT_BODY_MAX_NODES-1])].y=FLT_MAX;
    report=source_report(); physics_sources_apply(&report,false);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1);
    assert(report.diagnostics[0].source.id==source && report.diagnostics[0].target.id==owner.result.value);
    for(unsigned n=0;n<SOFT_BODY_MAX_NODES-1;++n) assert(force_accelerations[idx(nodes[n])].y==0);
    EntityInfo bad=inspect(source); assert(!bad.force_eligible && good.force_eligible);
    OK(entity_info_release(&bad)); OK(entity_info_release(&good));
    OK(physics_force_description_set(source,(ForceDescription){.target=owner.result.value,.magnitude=12}));
    EntityIndex bi=idx(owner.result.value), ni=idx(nodes[SOFT_BODY_MAX_NODES-1]);
    SoftBody saved=soft_bodies[bi]; Position point=positions[ni]; Mass m=mass[ni];
    Entity parent=soft_body_nodes[ni].soft_body;
    for(unsigned scenario=0;scenario<10;++scenario) {
        switch(scenario) {
        case 0: soft_bodies[bi].node_count=0; break;
        case 1: soft_bodies[bi].node_count=SOFT_BODY_MAX_NODES+1; break;
        case 2: soft_bodies[bi].nodes[SOFT_BODY_MAX_NODES-1]=nodes[0]; break;
        case 3: soft_bodies[bi].nodes[SOFT_BODY_MAX_NODES-1]=ENTITY_INVALID; break;
        case 4: mass[ni]=0; break;
        case 5: mass[ni]=-1; break;
        case 6: mass[ni]=NAN; break;
        case 7: positions[ni].x=INFINITY; break;
        case 8: soft_body_nodes[ni].soft_body=ENTITY_INVALID; break;
        case 9: mass_pool.used[ni]=false; break;
        }
        bool active; BAD(physics_source_change_get(source,false,&c,&active)); assert(!active);
        /* Zero and held sources are inactive only after references are valid. */
        OK(physics_force_magnitude_set(source,0)); soft_evaluate(source,false,0);
        OK(physics_force_magnitude_set(source,12)); OK(entity_components_add(source,ROHR_HOLD));
        soft_evaluate(source,false,0); OK(entity_components_delete(source,ROHR_HOLD));
        soft_bodies[bi]=saved; positions[ni]=point; mass[ni]=m;
        soft_body_nodes[ni].soft_body=parent; mass_pool.used[ni]=true;
        force_accelerations[ni]=(Acceleration){0}; soft_evaluate(source,false,SOFT_BODY_MAX_NODES);
    }
    /* Stale generation handle cannot become a new member through slot reuse. */
    Entity stale=nodes[SOFT_BODY_MAX_NODES-1]; OK(entity_delete(stale));
    Entity replacement=make(); assert(replacement!=stale);
    SoftBody valid=soft_bodies[bi]; soft_bodies[bi]=saved;
    bool active; BAD(physics_source_change_get(source,false,&c,&active));
    soft_bodies[bi]=valid; soft_evaluate(source,false,SOFT_BODY_MAX_NODES-1);
}
static void soft_degenerate_and_constraints(void) {
    BeamFixture f=beam_fixture(0,0);
    OK(physics_force_description_set(f.source,at_com(f.owner,0,12)));
    OK(physics_static_set(f.a));
    PhysicsSourceChanges c=soft_evaluate(f.source,false,1);
    assert(c.values[0].index==idx(f.b) && near(c.values[0].force_acceleration.y,2));
    OK(entity_components_add(f.b,ROHR_HOLD)); soft_evaluate(f.source,false,0);
    OK(entity_components_delete(f.b,ROHR_HOLD)); OK(physics_dynamic_set(f.a));
    OK(entity_components_add(f.a,ROHR_HOLD)); c=soft_evaluate(f.source,false,1);
    assert(near(c.values[0].force_acceleration.y,2)); OK(entity_components_delete(f.a,ROHR_HOLD));
    OK(physics_position_set(f.b,(Position){0,0}));
    c=soft_evaluate(f.source,false,2); assert(near(c.values[0].force_acceleration.y,2));
    OK(physics_force_location_set(f.source,(ForceLocation){.offset={1,0}}));
    bool active; BAD(physics_source_change_get(f.source,false,&c,&active));
    Report report=source_report(); physics_sources_apply(&report,false);
    assert(metric(report,ROHR_REPORT_SOURCE_SUSPENDED_COUNT)==1 && force_accelerations[idx(f.a)].y==0);
    OK(physics_force_magnitude_set(f.source,0)); soft_evaluate(f.source,false,0);
    /* Collapsed explicit beam references warn even for held/zero loads. */
    OK(physics_force_location_set(f.source,(ForceLocation){.reference={.kind=ROHR_FORCE_BEAM_POSITION,
        .beam={f.beam,ROHR_FORCE_BEAM_START,0}}}));
    BAD(physics_source_change_get(f.source,false,&c,&active));
    OK(entity_components_add(f.source,ROHR_HOLD));
    BAD(physics_source_change_get(f.source,false,&c,&active));
    OK(entity_components_delete(f.source,ROHR_HOLD));
    OK(entity_components_delete(f.source,ROHR_FORCE));
    OK(physics_torque_description_set(f.source,(TorqueDescription){.target = f.owner, .magnitude = 1}));
    BAD(physics_source_change_get(f.source,true,&c,&active));
    /* Positive inertia far below the removed 0.0001 cutoff remains usable. */
    OK(physics_position_set(f.b,(Position){1e-10f,0}));
    c=soft_evaluate(f.source,true,2);
    assert(close_value(c.values[0].force_acceleration.y,5e9));
    assert(close_value(c.values[1].force_acceleration.y,-2.5e9));
    OK(physics_position_set(f.b,(Position){1e-30f,0}));
    OK(physics_torque_magnitude_set(f.source,FLT_MAX));
    OK(physics_static_set(f.a)); OK(physics_static_set(f.b));
    BAD(physics_source_change_get(f.source,true,&c,&active)); /* Invalid absorbed loads still reject. */
    OK(physics_dynamic_set(f.a)); OK(physics_dynamic_set(f.b));
    OK(physics_position_set(f.b,(Position){10,0})); OK(physics_torque_magnitude_set(f.source,20));
    soft_evaluate(f.source,true,2);
    OK(entity_delete(f.b)); /* Single node: torque fails, translation succeeds. */
    BAD(physics_source_change_get(f.source,true,&c,&active));
    OK(physics_torque_magnitude_set(f.source,0)); soft_evaluate(f.source,true,0);
    OK(entity_components_delete(f.source,ROHR_TORQUE));
    OK(physics_force_description_set(f.source,at_com(f.owner,0,12)));
    c=soft_evaluate(f.source,false,1); assert(near(c.values[0].force_acceleration.y,6));
    /* Tiny total mass causes an acceleration overflow without any partial write. */
    OK(physics_mass_set(f.a,FLT_MIN));
    BAD(physics_source_change_get(f.source,false,&c,&active));
    OK(physics_mass_set(f.a,2)); soft_evaluate(f.source,false,1);
}
static void soft_lifetime_and_parity(void) {
    BeamFixture a=beam_fixture(0,0), b=beam_fixture(0,0);
    OK(physics_force_description_set(a.source,at_com(a.owner,0,12)));
    OK(entity_delete(b.source));
    OK(rohr_physics_force_apply(at_com(b.owner,0,12)));
    EntityResult torque=physics_torque_add((TorqueDescription){.target = a.owner, .magnitude = -20}); OK(torque);
    OK(rohr_physics_torque_apply((TorqueDescription){.target = b.owner, .magnitude = -20}));
    uint32_t count=entity_alive_count_get(); uint64_t tick=engine_tick_get();
    OK(physics_substeps_set(4));
    for(unsigned step=0;step<2;++step) {
        Report report=rohr_system_physics_update(0.01);
        assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==16);
        assert(entity_alive_count_get()==count && engine_tick_get()==tick);
        Entity pairs[][2]={{a.a,b.a},{a.b,b.b}};
        for(unsigned n=0;n<2;++n) {
            EntityIndex i=idx(pairs[n][0]),j=idx(pairs[n][1]);
            assert(near(velocities[i].x,velocities[j].x) && near(velocities[i].y,velocities[j].y));
            assert(near(positions[i].x,positions[j].x) && near(positions[i].y,positions[j].y));
        }
    }
    ReportResult before=report_get(ROHR_PHYSICS_REPORT); OK(before);
    Velocity velocity=velocities[idx(a.a)];
    EntityInfo info=inspect(a.source); assert(info.force_eligible); OK(entity_info_release(&info));
    ReportResult after=report_get(ROHR_PHYSICS_REPORT); OK(after);
    assert(memcmp(&before.result.value,&after.result.value,sizeof(Report))==0);
    assert(memcmp(&velocity,&velocities[idx(a.a)],sizeof(velocity))==0);
    OK(engine_time_per_tick_set(0.001)); SDL_Delay(3); Report report=engine_update();
    assert(!report.has_fatal && engine_tick_get()>tick && entity_alive_count_get()==count-2);
    assert(entity_alive_check(a.source) && entity_alive_check(torque.result.value));
    OK(entity_delete(a.source)); OK(entity_delete(torque.result.value));
    report=system_physics_update(0.01); assert(metric(report,ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT)==0);
}
static double soft_energy_error(unsigned steps, bool held, float damping) {
    BeamFixture f=beam_fixture(8,damping);
    if(held) OK(entity_components_add(f.a,ROHR_HOLD));
    else OK(physics_static_set(f.a));
    OK(physics_force_description_set(f.source,at_com(f.owner,90,6)));
    double dt=1.0/steps, work=0,dissipated=0;
    for(unsigned step=0;step<steps;++step) {
        double x=positions[idx(f.b)].x, v=velocities[idx(f.b)].x;
        dissipated+=damping*v*v*dt;
        Report report=system_physics_update(dt); assert(!report.has_fatal);
        work+=4*((double)positions[idx(f.b)].x-x); /* Fixed share (2 N) does no work. */
        assert(positions[idx(f.a)].x==0 && velocities[idx(f.a)].x==0);
    }
    double v=velocities[idx(f.b)].x, extension=positions[idx(f.b)].x-10;
    assert(extension>0.1);
    double error=fabs(work-2*v*v-4*extension*extension-dissipated);
    OK(entity_delete(f.source)); OK(entity_delete(f.owner)); return error;
}
static double soft_free_energy_error(unsigned steps, float damping) {
    BeamFixture f=beam_fixture(8,damping);
    OK(entity_components_delete(f.source,ROHR_FORCE));
    OK(physics_torque_description_set(f.source,(TorqueDescription){.target = f.owner, .magnitude = 30}));
    double dt=1.0/steps, work=0,dissipated=0;
    for(unsigned step=0;step<steps;++step) {
        Position a=positions[idx(f.a)],b=positions[idx(f.b)];
        double dx=(double)b.x-a.x,dy=(double)b.y-a.y,length=hypot(dx,dy);
        double relative=((velocities[idx(f.b)].x-velocities[idx(f.a)].x)*dx+
            (velocities[idx(f.b)].y-velocities[idx(f.a)].y)*dy)/length;
        dissipated+=damping*relative*relative*dt;
        Report report=system_physics_update(dt); assert(!report.has_fatal);
        /* For two nodes, the pure-torque load is an equal/opposite couple.
         * Use that analytic force, independent of existing acceleration storage. */
        double dax=(double)positions[idx(f.a)].x-a.x, day=(double)positions[idx(f.a)].y-a.y;
        double dbx=(double)positions[idx(f.b)].x-b.x, dby=(double)positions[idx(f.b)].y-b.y;
        work+=30/(length*length)*(dy*(dbx-dax)-dx*(dby-day));
    }
    double dx=(double)positions[idx(f.b)].x-positions[idx(f.a)].x;
    double dy=(double)positions[idx(f.b)].y-positions[idx(f.a)].y;
    double extension=hypot(dx,dy)-10,energy=4*extension*extension;
    assert(extension>0.01); /* Free turning also deforms the axial spring. */
    Entity nodes[]={f.a,f.b};
    for(unsigned n=0;n<2;++n) {
        EntityIndex i=idx(nodes[n]); double vx=velocities[i].x,vy=velocities[i].y;
        energy+=0.5*mass[i]*(vx*vx+vy*vy);
    }
    OK(entity_delete(f.source)); OK(entity_delete(f.owner));
    return fabs(work-energy-dissipated);
}
static void soft_energy(void) {
    for(unsigned n=0;n<3;++n) {
        double coarse=soft_energy_error(40,n==1,n==2 ? 0.7f : 0);
        double fine=soft_energy_error(160,n==1,n==2 ? 0.7f : 0);
        assert(fine<coarse*0.6 && fine<0.05);
    }
    for(unsigned n=0;n<2;++n) {
        double coarse=soft_free_energy_error(40,n ? 0.7f : 0);
        double fine=soft_free_energy_error(160,n ? 0.7f : 0);

        assert(fine<coarse*0.6 && fine<0.05);
    }
}

static void soft_numeric_geometry(void) {
    BeamFixture f=beam_fixture(0,0);
    OK(physics_force_target_set(f.source,f.owner));
    for(unsigned trial=0;trial<100;++trial) {
        Position a={beam_random()*30-15,beam_random()*30-15};
        Position b={a.x+1+beam_random()*15,a.y+1+beam_random()*15};
        OK(physics_position_set(f.a,a)); OK(physics_position_set(f.b,b));
        OK(physics_mass_set(f.a,1+beam_random()*8)); OK(physics_mass_set(f.b,1+beam_random()*8));
        Position p={beam_random()*60-30,beam_random()*60-30};
        float magnitude=beam_random()*40-20;
        OK(physics_force_description_set(f.source,(ForceDescription){.target=f.owner,
            .location.offset=p,.magnitude=magnitude}));
        PhysicsSourceChanges c=soft_evaluate(f.source,false,2);
        soft_result_check(c,0,magnitude,-p.x*magnitude);
    }
    /* A nonrepresentable world COM still gives precisely uniform acceleration.
     * Translation of the geometry does not change pure-torque shares. */
    positions[idx(f.a)]=(Position){1e8f,1e8f}; positions[idx(f.b)]=(Position){1e8f+16,1e8f};
    OK(physics_mass_set(f.a,2)); OK(physics_mass_set(f.b,4));
    OK(physics_force_description_set(f.source,at_com(f.owner,0,12)));
    PhysicsSourceChanges c=soft_evaluate(f.source,false,2);
    assert(near(c.values[0].force_acceleration.y,2) && near(c.values[1].force_acceleration.y,2));
    OK(entity_components_delete(f.source,ROHR_FORCE));
    OK(physics_torque_description_set(f.source,(TorqueDescription){.target = f.owner, .magnitude = 32}));
    c=soft_evaluate(f.source,true,2);
    assert(near(c.values[0].force_acceleration.y,1) && near(c.values[1].force_acceleration.y,-0.5));
    /* A zero-mass member leaves a COM reference resolvable, but cannot receive
     * a whole-body load. Zero loads retain the existing reference semantics. */
    mass[idx(f.a)]=0;
    OK(entity_components_delete(f.source,ROHR_TORQUE));
    OK(physics_force_description_set(f.source,at_com(f.owner,0,0)));
    soft_evaluate(f.source,false,0);
    OK(physics_force_magnitude_set(f.source,12));
    bool active; BAD(physics_source_change_get(f.source,false,&c,&active));
    OK(entity_components_delete(f.source,ROHR_FORCE));
    OK(physics_torque_description_set(f.source,(TorqueDescription){.target = f.owner, .magnitude = 32}));
    /* Internal mass sums and inertia may exceed float without rejecting finite loads. */
    positions[idx(f.a)]=(Position){0,0}; positions[idx(f.b)]=(Position){10,0};
    mass[idx(f.a)]=FLT_MAX; mass[idx(f.b)]=FLT_MAX;
    OK(physics_torque_magnitude_set(f.source,FLT_MAX));
    c=soft_evaluate(f.source,true,2);
    assert(near(c.values[0].force_acceleration.y,0.1) && near(c.values[1].force_acceleration.y,-0.1));
    OK(entity_components_delete(f.source,ROHR_TORQUE));
    OK(physics_force_description_set(f.source,at_com(f.owner,0,FLT_MAX)));
    c=soft_evaluate(f.source,false,2);
    assert(near(c.values[0].force_acceleration.y,0.5) && near(c.values[1].force_acceleration.y,0.5));
}

int main(void) {
    void (*tests[])(void)={api_and_validation,force_geometry,beam_references,
        suspension_and_inspection,particles,allocation_failure,persistence_references,substeps_and_expiry,
        beam_loads,beam_equivalence_and_work,beam_failures_and_lifetime,beam_torque_and_motion,beam_energy,soft_analytics,soft_capacity_and_failures,
        soft_degenerate_and_constraints,soft_lifetime_and_parity,soft_energy,soft_numeric_geometry};
    for(size_t i=0;i<sizeof(tests)/sizeof(tests[0]);++i) {
        OK(engine_start()); tests[i](); engine_stop();
    }
    return 0;
}
