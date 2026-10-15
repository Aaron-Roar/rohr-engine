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
    PhysicsChange change={0}; bool active=false;
    OK(physics_source_change_get(source,torque,&change,&active));
    assert(active==expected); return change;
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
    OK(rohr_physics_torque_description_set(t.result.value,(TorqueDescription){target,7}));
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
    EntityResult soft=physics_soft_body_create(); OK(soft);
    EntityResult a=physics_soft_body_node_create(soft.result.value,(Position){0,0},1,0.1f); OK(a);
    EntityResult b=physics_soft_body_node_create(soft.result.value,(Position){4,0},1,0.1f); OK(b);
    EntityResult beam=physics_soft_body_beam_create(soft.result.value,a.result.value,b.result.value,0,0); OK(beam);
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
    PhysicsChange unused; bool active; BAD(physics_source_change_get(source,false,&unused,&active));
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
    ParticleConfig config=physics_particle_config_default_get(); config.mass_value=2;
    config.collision_enabled=false;
    EntityResult p=physics_particle_create(config); OK(p);
    Entity rigid=body((Position){10,0}); OK(physics_particle_radius_set(rigid,1));
    EntityResult soft=physics_soft_body_create(); OK(soft);
    EntityResult node=physics_soft_body_node_create(soft.result.value,(Position){20,0},2,0.2f); OK(node);
    Entity entities[]={p.result.value,node.result.value,rigid};
    for(unsigned n=0;n<3;++n) {
        Entity source=add((ForceDescription){.target=entities[n],.location.offset={100,2},
            .orientation.offset=90,.magnitude=2});
        PhysicsChange c=evaluate(source,false,true); assert(near(c.force_acceleration.x,1));
        assert(n==2 ? c.torque_acceleration>0 : c.torque_acceleration==0);
        EntityResult torque=physics_torque_add((TorqueDescription){entities[n],2}); OK(torque);
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

int main(void) {
    void (*tests[])(void)={api_and_validation,force_geometry,beam_references,
        suspension_and_inspection,particles,allocation_failure,persistence_references,substeps_and_expiry};
    for(size_t i=0;i<sizeof(tests)/sizeof(tests[0]);++i) {
        OK(engine_start()); tests[i](); engine_stop();
    }
    return 0;
}
