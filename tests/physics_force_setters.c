/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool allocation_forbidden;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { assert(!allocation_forbidden); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t size) { assert(!allocation_forbidden); return __real_calloc(n, size); }
void *__wrap_realloc(void *p, size_t n) { assert(!allocation_forbidden); return __real_realloc(p, n); }
#endif

static void success_at(EngineResult result, int line) {
    if(error_check(result)) fprintf(stderr, "line %d: %s\n", line, error_code_message_get(result.result.error));
    assert(!error_check(result) && result.result.value);
}
#define OK(r) success_at((r), __LINE__)
#define BAD(r) assert(error_check(r))
static Entity created(EntityResult result) { assert(!error_check(result)); return result.result.value; }
static bool near(float a, float b) { return fabsf(a-b) < 0.0003f; }
static Entity body(void) {
    Entity entity = created(rohr_entity_add());
    OK(rohr_physics_position_set(entity, (Position){0}));
    OK(rohr_physics_orientation_set(entity, 0));
    OK(rohr_physics_velocity_set(entity, (Velocity){0}));
    OK(rohr_physics_acceleration_set(entity, (Acceleration){0}));
    OK(rohr_physics_hitbox_set(entity, rohr_math_square_create(2,2)));
    OK(rohr_physics_mass_set(entity, 2));
    OK(rohr_physics_dynamic_set(entity));
    return entity;
}
static ForceDescription description(Entity source) {
    ForceDescriptionResult result = rohr_physics_force_description_get(source);
    assert(!error_check(result)); return result.result.value;
}
static void location_equal(ForceLocation a, ForceLocation b) {
    assert(a.offset.x==b.offset.x && a.offset.y==b.offset.y);
    assert(a.reference.kind==b.reference.kind);
    switch(a.reference.kind) {
    case ROHR_FORCE_WORLD_POSITION: break;
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION: assert(a.reference.entity==b.reference.entity); break;
    case ROHR_FORCE_BEAM_POSITION:
        assert(a.reference.beam.entity==b.reference.beam.entity);
        assert(a.reference.beam.anchor==b.reference.beam.anchor);
        assert(a.reference.beam.fraction==b.reference.beam.fraction); break;
    }
}
static void reference_equal(ForceOrientationReference a, ForceOrientationReference b) {
    assert(a.kind==b.kind);
    switch(a.kind) {
    case ROHR_FORCE_WORLD_ORIENTATION: break;
    case ROHR_FORCE_BODY_ORIENTATION:
    case ROHR_FORCE_BEAM_ORIENTATION: assert(a.entity==b.entity); break;
    case ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION:
        location_equal(a.between.from,b.between.from);
        location_equal(a.between.to,b.between.to); break;
    }
}
static void equal(ForceDescription a, ForceDescription b) {
    assert(a.owner==b.owner && a.target==b.target && a.magnitude==b.magnitude);
    location_equal(a.location,b.location);
    assert(a.orientation.offset==b.orientation.offset);
    reference_equal(a.orientation.reference,b.orientation.reference);
}
static Force vector(ForceDescription value) {
    double radians = remainder(value.orientation.offset,360.0)*(PI_F/180.0);
    return (Force){value.magnitude*sin(radians),value.magnitude*cos(radians)};
}
static void vector_near(Entity source, float x, float y) {
    Force value=vector(description(source)); assert(near(value.x,x) && near(value.y,y));
}
static void lifetime_and_torque(Entity source, Entity owner, Entity target) {
    EntityLifetimeDescriptionResult life=rohr_entity_lifetime_description_get(source);
    assert(!error_check(life));
    assert(life.result.value.tick_enabled && life.result.value.expiry_tick==100);
    TorqueDescriptionResult torque=rohr_physics_torque_description_get(source);
    assert(!error_check(torque));
    assert(torque.result.value.owner==owner && torque.result.value.target==target && torque.result.value.magnitude==-9);
}

typedef struct VectorApi {
    EngineResult (*x)(Entity,float), (*y)(Entity,float);
    EngineResult (*vector)(Entity,Force);
    EngineResult (*angle)(Entity,Orientation), (*radians)(Entity,Orientation);
    EngineResult (*reference)(Entity,ForceOrientationReference);
    EngineResult (*reference_entity)(Entity,Entity);
    ScalarResult (*x_get)(Entity), (*y_get)(Entity), (*angle_get)(Entity), (*radians_get)(Entity);
    ForceResult (*get)(Entity);
    ForceOrientationReferenceResult (*reference_get)(Entity);
    EntityResult (*reference_entity_get)(Entity);
} VectorApi;
#define VECTOR_API(p) {p##force_x_set,p##force_y_set,p##force_set, \
    p##force_orientation_offset_set,p##force_orientation_offset_radians_set, \
    p##force_orientation_reference_set,p##force_orientation_reference_entity_set, \
    p##force_x_get,p##force_y_get,p##force_orientation_offset_get,p##force_orientation_offset_radians_get, \
    p##force_get,p##force_orientation_reference_get,p##force_orientation_reference_entity_get}
static const VectorApi vector_apis[]={VECTOR_API(physics_),VECTOR_API(rohr_physics_)};

static void vectors(void) {
    for(size_t i=0;i<2;++i) {
        const VectorApi *api=&vector_apis[i];
        Entity owner=created(rohr_entity_add()), target=body(), reference=body();
        Entity source=created(rohr_physics_force_add((ForceDescription){.owner=owner,.target=target,
            .location={.offset={2,3},.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=reference}},
            .orientation={.offset=0,.reference={.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=reference}},.magnitude=-5}));
        OK(rohr_entity_lifetime_description_set(source,(EntityLifetimeDescription){.tick_enabled=true,.expiry_tick=100}));
        OK(rohr_physics_torque_description_set(source,(TorqueDescription){.owner=owner,.target=target,.magnitude=-9}));
        ForceDescription initial=description(source);
#ifdef ROHR_TEST_ALLOCATION_WRAP
        allocation_forbidden=true;
#endif
        OK(api->x(source,3)); vector_near(source,3,-5);
        assert(description(source).magnitude<0);
        OK(api->y(source,4)); vector_near(source,3,4);
        OK(api->vector(source,(Force){-6,8})); vector_near(source,-6,8);
        ForceResult fetched=api->get(source); assert(!error_check(fetched));
        assert(near(fetched.result.value.x,-6) && near(fetched.result.value.y,8));
        ScalarResult x=api->x_get(source), y=api->y_get(source);
        assert(!error_check(x) && !error_check(y) && near(x.result.value,-6) && near(y.result.value,8));
        assert(api->angle_get(source).result.value==description(source).orientation.offset);
        assert(near(api->radians_get(source).result.value,description(source).orientation.offset*(PI_F/180.0)));
        ForceOrientationReferenceResult ref=api->reference_get(source); assert(!error_check(ref));
        reference_equal(ref.result.value,initial.orientation.reference);
        assert(api->reference_entity_get(source).result.value==reference);
        float saved_angle=description(source).orientation.offset;
        OK(api->vector(source,(Force){0,0}));
        assert(description(source).orientation.offset==saved_angle);
        assert(signbit(description(source).magnitude));
        OK(api->y(source,-2)); vector_near(source,0,-2);
        ForceDescription now=description(source);
        assert(now.owner==owner && now.target==target);
        reference_equal(now.orientation.reference,initial.orientation.reference);
        location_equal(now.location,initial.location);
        lifetime_and_torque(source,owner,target);
        const float invalid[]={NAN,INFINITY,-INFINITY};
        for(size_t j=0;j<3;++j) {
            BAD(api->x(source,invalid[j])); BAD(api->y(source,invalid[j]));
            BAD(api->vector(source,(Force){0,invalid[j]}));
            BAD(api->angle(source,invalid[j])); BAD(api->radians(source,invalid[j]));
            equal(description(source),now);
        }
        BAD(api->vector(source,(Force){FLT_MAX,FLT_MAX})); equal(description(source),now);
        BAD(api->radians(source,FLT_MAX)); equal(description(source),now);
        BAD(api->reference(source,(ForceOrientationReference){.kind=(ForceOrientationReferenceKind)999}));
        equal(description(source),now);
        OK(api->angle(source,450)); now.orientation.offset=450; equal(description(source),now);
        OK(api->radians(source,PI_F)); assert(near(description(source).orientation.offset,180));
        now=description(source);
        OK(api->reference_entity(source,target)); now.orientation.reference.entity=target; equal(description(source),now);
        OK(api->reference(source,(ForceOrientationReference){0})); now.orientation.reference=(ForceOrientationReference){0}; equal(description(source),now);
        BAD(api->reference_entity(source,reference)); BAD(api->reference_entity_get(source)); equal(description(source),now);
        lifetime_and_torque(source,owner,target);
#ifdef ROHR_TEST_ALLOCATION_WRAP
        allocation_forbidden=false;
#endif
        /* Existing torque setters preserve the force and source ownership/expiry. */
        OK(rohr_physics_torque_set(source,12)); equal(description(source),now);
        ScalarResult magnitude = rohr_physics_torque_get(source);
        assert(!error_check(magnitude) && magnitude.result.value==12);
        OK(physics_torque_set(source,12)); assert(physics_torque_get(source).result.value==12);
        OK(physics_torque_target_set(source,reference)); equal(description(source),now);
        TorqueDescriptionResult torque=rohr_physics_torque_description_get(source);
        assert(!error_check(torque) && torque.result.value.magnitude==12 && torque.result.value.owner==owner);
        BAD(physics_torque_magnitude_set(source,NAN));
        assert(rohr_physics_torque_magnitude_get(source).result.value==12);
        OK(rohr_entity_delete(owner));
        assert(!rohr_entity_alive_check(source));
        Entity replacement=created(rohr_physics_force_add((ForceDescription){.magnitude=17}));
        BAD(api->x(source,1)); BAD(api->y(source,1)); BAD(api->vector(source,(Force){1,2}));
        BAD(api->angle(source,1)); BAD(api->radians(source,1));
        BAD(api->reference(source,(ForceOrientationReference){0})); BAD(api->reference_entity(source,target));
        BAD(api->get(source)); BAD(api->x_get(source)); BAD(api->y_get(source));
        BAD(api->angle_get(source)); BAD(api->radians_get(source)); BAD(api->reference_get(source)); BAD(api->reference_entity_get(source));
        assert(description(replacement).magnitude==17);
        BAD(api->x(target,1)); BAD(api->y(target,1)); BAD(api->vector(target,(Force){1,2}));
        BAD(api->angle(target,1)); BAD(api->radians(target,1));
        BAD(api->reference(target,(ForceOrientationReference){0})); BAD(api->reference_entity(target,reference));
        OK(rohr_entity_delete(replacement)); OK(rohr_entity_delete(target)); OK(rohr_entity_delete(reference));
    }
}

typedef struct LocationApi {
    unsigned selected;
    EngineResult (*set)(Entity,ForceLocation);
    EngineResult (*offset)(Entity,Vec2D), (*x)(Entity,float), (*y)(Entity,float);
    EngineResult (*reference)(Entity,ForcePositionReference);
    EngineResult (*entity)(Entity,Entity);
    EngineResult (*anchor)(Entity,ForceBeamAnchor);
    EngineResult (*fraction)(Entity,float);
    ForceLocationResult (*get)(Entity);
    PositionResult (*offset_get)(Entity);
    ScalarResult (*x_get)(Entity), (*y_get)(Entity);
    ForcePositionReferenceResult (*reference_get)(Entity);
    EntityResult (*entity_get)(Entity);
    ForceBeamAnchorResult (*anchor_get)(Entity);
    ScalarResult (*fraction_get)(Entity);
} LocationApi;
#define LOCATION_API(p,n,s) {s,p##n##_set,p##n##_offset_set,p##n##_offset_x_set,p##n##_offset_y_set, \
    p##n##_reference_set,p##n##_reference_entity_set,p##n##_reference_beam_anchor_set,p##n##_reference_beam_fraction_set, \
    p##n##_get,p##n##_offset_get,p##n##_offset_x_get,p##n##_offset_y_get,p##n##_reference_get, \
    p##n##_reference_entity_get,p##n##_reference_beam_anchor_get,p##n##_reference_beam_fraction_get}
static const LocationApi location_apis[]={
    LOCATION_API(physics_,force_location,0),LOCATION_API(rohr_physics_,force_location,0),
    LOCATION_API(physics_,force_orientation_between_from,1),LOCATION_API(rohr_physics_,force_orientation_between_from,1),
    LOCATION_API(physics_,force_orientation_between_to,2),LOCATION_API(rohr_physics_,force_orientation_between_to,2)};
static ForceLocation *selected(ForceDescription *value,unsigned which) {
    if(which==1) return &value->orientation.reference.between.from;
    if(which==2) return &value->orientation.reference.between.to;
    return &value->location;
}
static void locations(void) {
    Entity owner=created(rohr_entity_add()), target=body(), reference=body();
    Entity soft=created(rohr_physics_soft_body_add((SoftBodyDescription){0}));
    Entity a=created(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.owner=soft,.mass=1,.radius=1}));
    Entity b=created(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.owner=soft,.mass=1,.radius=1,.position={0,10}}));
    Entity beam=created(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=soft,.node_a=a,.node_b=b}));
    Entity beam2=created(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=soft,.node_a=b,.node_b=a}));
    ForceLocation loc={.offset={3,4},.reference={.kind=ROHR_FORCE_BEAM_POSITION,.beam={.entity=beam,.anchor=ROHR_FORCE_BEAM_CENTER,.fraction=.25f}}};
    for(size_t i=0;i<sizeof(location_apis)/sizeof(location_apis[0]);++i) {
        const LocationApi *api=&location_apis[i];
        ForceDescription expected={.owner=owner,.target=target,.location=loc,.magnitude=-7,
            .orientation={.offset=30,.reference={.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION,.between={.from=loc,.to=loc}}}};
        expected.orientation.reference.between.to.offset.x=8;
        Entity source=created(rohr_physics_force_add(expected));
        OK(rohr_physics_torque_description_set(source,(TorqueDescription){.owner=owner,.target=target,.magnitude=-9}));
        OK(rohr_entity_lifetime_description_set(source,(EntityLifetimeDescription){.tick_enabled=true,.expiry_tick=100}));
        ForceLocation *change=selected(&expected,api->selected);
#ifdef ROHR_TEST_ALLOCATION_WRAP
        allocation_forbidden=true;
#endif
        OK(api->x(source,-2)); change->offset.x=-2; equal(description(source),expected);
        OK(api->y(source,5)); change->offset.y=5; equal(description(source),expected);
        OK(api->offset(source,(Vec2D){6,7})); change->offset=(Vec2D){6,7}; equal(description(source),expected);
        OK(api->anchor(source,ROHR_FORCE_BEAM_END)); change->reference.beam.anchor=ROHR_FORCE_BEAM_END; equal(description(source),expected);
        OK(api->fraction(source,-2)); change->reference.beam.fraction=-2; equal(description(source),expected);
        OK(api->entity(source,beam2)); change->reference.beam.entity=beam2; equal(description(source),expected);
        ForceLocationResult fetched=api->get(source); assert(!error_check(fetched)); location_equal(fetched.result.value,*change);
        PositionResult offset=api->offset_get(source); assert(!error_check(offset));
        assert(offset.result.value.x==6 && offset.result.value.y==7);
        assert(api->x_get(source).result.value==6 && api->y_get(source).result.value==7);
        assert(api->entity_get(source).result.value==beam2);
        assert(api->anchor_get(source).result.value==ROHR_FORCE_BEAM_END);
        assert(api->fraction_get(source).result.value==-2);
        ForcePositionReferenceResult ref=api->reference_get(source); assert(!error_check(ref));
        assert(ref.result.value.kind==ROHR_FORCE_BEAM_POSITION && ref.result.value.beam.entity==beam2);
        BAD(api->anchor(source,(ForceBeamAnchor)99)); BAD(api->fraction(source,NAN));
        BAD(api->x(source,INFINITY)); BAD(api->y(source,NAN)); BAD(api->offset(source,(Vec2D){NAN,0}));
        BAD(api->reference(source,(ForcePositionReference){.kind=(ForcePositionReferenceKind)99}));
        BAD(api->entity(source,reference)); equal(description(source),expected);
        for(int kind=ROHR_FORCE_COM_POSITION;kind<=ROHR_FORCE_ORIGIN_POSITION;++kind) {
            ForcePositionReference ref={.kind=(ForcePositionReferenceKind)kind,.entity=reference};
            OK(api->reference(source,ref)); change->reference=ref; equal(description(source),expected);
            OK(api->entity(source,target)); change->reference.entity=target; equal(description(source),expected);
            BAD(api->anchor(source,ROHR_FORCE_BEAM_START)); BAD(api->fraction(source,0)); equal(description(source),expected);
        }
        OK(api->set(source,(ForceLocation){.offset={11,12}})); *change=(ForceLocation){.offset={11,12}}; equal(description(source),expected);
        BAD(api->entity(source,reference)); BAD(api->anchor(source,ROHR_FORCE_BEAM_START)); BAD(api->fraction(source,0));
        equal(description(source),expected); lifetime_and_torque(source,owner,target);
        BAD(api->entity_get(source)); BAD(api->anchor_get(source)); BAD(api->fraction_get(source));
        if(api->selected) {
            OK(rohr_physics_force_orientation_reference_set(source,(ForceOrientationReference){0}));
            ForceDescription before=description(source);
            BAD(api->set(source,loc)); BAD(api->offset(source,(Vec2D){1,2})); BAD(api->x(source,1)); BAD(api->y(source,1));
            BAD(api->reference(source,loc.reference)); BAD(api->entity(source,beam)); BAD(api->anchor(source,ROHR_FORCE_BEAM_START)); BAD(api->fraction(source,1));
            BAD(api->get(source)); BAD(api->offset_get(source)); BAD(api->x_get(source)); BAD(api->y_get(source));
            BAD(api->reference_get(source)); BAD(api->entity_get(source)); BAD(api->anchor_get(source)); BAD(api->fraction_get(source));
            equal(description(source),before);
        }
#ifdef ROHR_TEST_ALLOCATION_WRAP
        allocation_forbidden=false;
#endif
        OK(rohr_entity_delete(source));
        Entity replacement=created(rohr_physics_force_add((ForceDescription){0}));
        BAD(api->set(source,loc)); BAD(api->offset(source,(Vec2D){1,2})); BAD(api->x(source,1)); BAD(api->y(source,1));
        BAD(api->reference(source,loc.reference)); BAD(api->entity(source,beam)); BAD(api->anchor(source,ROHR_FORCE_BEAM_START)); BAD(api->fraction(source,1));
        BAD(api->get(source)); BAD(api->offset_get(source)); BAD(api->x_get(source)); BAD(api->y_get(source));
        BAD(api->reference_get(source)); BAD(api->entity_get(source)); BAD(api->anchor_get(source)); BAD(api->fraction_get(source));
        equal(description(replacement),(ForceDescription){0}); OK(rohr_entity_delete(replacement));
    }
    /* Vector edits don't require resolvable geometry: collapsed aiming is valid configuration. */
    Entity source=created(rohr_physics_force_add((ForceDescription){.target=target,.orientation.reference={.kind=ROHR_FORCE_BEAM_ORIENTATION,.entity=beam}}));
    OK(rohr_physics_force_orientation_reference_entity_set(source,beam2));
    OK(rohr_physics_position_set(b,(Position){0}));
    OK(rohr_physics_force_x_set(source,3)); OK(rohr_physics_force_y_set(source,4)); vector_near(source,3,4);
    OK(rohr_physics_force_orientation_reference_set(source,(ForceOrientationReference){.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION}));
    OK(rohr_physics_force_x_set(source,6)); vector_near(source,6,4);
    /* Deleted references reject unrelated edits until that reference is repaired. */
    Entity doomed=body();
    OK(rohr_physics_force_location_reference_set(source,(ForcePositionReference){.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=doomed}));
    OK(rohr_entity_delete(doomed)); Entity reused=body();
    ForceDescription before=description(source);
    BAD(rohr_physics_force_x_set(source,2)); equal(description(source),before);
    OK(rohr_physics_force_location_reference_entity_set(source,reused));
    assert(description(source).location.reference.entity==reused);
    OK(rohr_entity_delete(source)); OK(rohr_entity_delete(reused)); OK(rohr_entity_delete(soft));
    OK(rohr_entity_delete(owner)); OK(rohr_entity_delete(target)); OK(rohr_entity_delete(reference));
}

static void world_response(void) {
    /* A body-relative vector follows rotation; its untouched local axis stays fixed. */
    Entity target=body(), reference=body();
    OK(rohr_physics_static_set(reference));
    OK(rohr_physics_orientation_set(reference,90));
    Entity source=created(rohr_physics_force_add((ForceDescription){.target=target,
        .location.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=target},
        .orientation.reference={.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=reference}}));
    OK(rohr_physics_force_x_set(source,6)); OK(rohr_physics_force_y_set(source,8));
    OK(rohr_physics_substeps_set(4));
    Report report=rohr_system_physics_update(.1);
    assert(report.header.status!=ROHR_REPORT_FAILED);
    EntityInfoResult got=rohr_entity_info_get(target,ROHR_ENTITY_INFO_PHYSICS_VELOCITY);
    assert(!error_check(got)); EntityInfo info=got.result.value;
    assert(near(info.velocity.x,.4f) && near(info.velocity.y,-.3f));
    OK(rohr_entity_info_release(&info));
    OK(rohr_physics_orientation_set(reference,180));
    OK(rohr_physics_force_x_set(source,10)); vector_near(source,10,8);
    OK(rohr_physics_velocity_set(target,(Velocity){0}));
    report=rohr_system_physics_update(.1); assert(report.header.status!=ROHR_REPORT_FAILED);
    got=rohr_entity_info_get(target,ROHR_ENTITY_INFO_PHYSICS_VELOCITY); assert(!error_check(got)); info=got.result.value;
    assert(near(info.velocity.x,-.5f) && near(info.velocity.y,-.4f));
    OK(rohr_entity_info_release(&info));
    OK(rohr_entity_delete(source)); OK(rohr_entity_delete(target)); OK(rohr_entity_delete(reference));
}
int main(void) {
    OK(rohr_engine_start());
    vectors(); locations(); world_response();
    rohr_engine_stop();
    return 0;
}
