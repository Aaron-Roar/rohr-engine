/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Independent individual-API construction shared by native and installed-SDK tests.
 */
#include "runtime_fixture.h"
#ifndef ROHR_CONSTRUCTION_CHECKS_H
#define ROHR_CONSTRUCTION_CHECKS_H
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define REQUIRE(call) do { if(rohr_error_check(call)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#call); abort(); } } while(0)
enum { C_ROOT, C_BODY, C_PARTICLE, C_SOFT, C_OTHER, C_NODE, C_BEAM=C_NODE+7,
    C_AREA=C_BEAM+4, C_WORLD, C_JOINT, C_FORCE=C_JOINT+3, C_TORQUE=C_FORCE+16,
    C_INITIAL, C_COUNT };
typedef struct ConstructionScene { Entity e[C_COUNT]; } ConstructionScene;
typedef struct ConstructionSnapshot { double values[4096]; const char *labels[4096]; bool exact[4096]; size_t count; } ConstructionSnapshot;
static Entity construction_take(EntityResult r) { REQUIRE(r); return r.result.value; }
static Entity construction_member(Entity owner,const char *name) {
    return construction_take(fixture_entity_get(owner,name));
}
static ConstructionScene construction_members(Entity root) {
    ConstructionScene s={0}; s.e[C_ROOT]=root;
    s.e[C_BODY]=construction_member(root,"body"); s.e[C_PARTICLE]=construction_member(root,"particle");
    s.e[C_SOFT]=construction_member(root,"soft"); s.e[C_OTHER]=construction_member(root,"other");
    s.e[C_WORLD]=construction_member(root,"world");
    s.e[C_AREA]=construction_member(s.e[C_SOFT],"fill");
    const struct { int first,count; const char *prefix; bool soft; } groups[]={
        {C_NODE,7,"node",true},{C_BEAM,4,"beam",true},{C_JOINT,3,"joint",false},
        {C_FORCE,16,"force",false}};
    for(size_t g=0;g<sizeof(groups)/sizeof(groups[0]);++g)
        for(int i=0;i<groups[g].count;++i) {
            char name[32]; snprintf(name,sizeof(name),"%s%d",groups[g].prefix,i);
            s.e[groups[g].first+i]=construction_member(groups[g].soft?s.e[C_SOFT]:root,name);
        }
    s.e[C_TORQUE]=construction_member(root,"spin"); s.e[C_INITIAL]=construction_member(root,"initial");
    return s;
}
static Entity construction_entity(Entity owner,Position p) {
    Entity e=construction_take(rohr_entity_add()); REQUIRE(rohr_entity_owner_set(e,owner));
    REQUIRE(rohr_physics_position_set(e,p)); REQUIRE(rohr_physics_orientation_set(e,0)); return e;
}
static ConstructionScene construction_direct(Position offset) {
    ConstructionScene s={0};
    s.e[C_ROOT]=construction_entity(ENTITY_INVALID,(Position){20+offset.x,30+offset.y});
    Entity root=s.e[C_ROOT];
    Entity body=s.e[C_BODY]=construction_entity(root,(Position){23+offset.x,34+offset.y});
    REQUIRE(rohr_physics_hitbox_set(body,rohr_math_square_create(6,4)));
    REQUIRE(rohr_physics_orientation_set(body,-25)); REQUIRE(rohr_physics_mass_set(body,5));
    REQUIRE(rohr_physics_velocity_set(body,(Velocity){1,2}));
    REQUIRE(rohr_physics_acceleration_set(body,(Acceleration){.2f,.3f}));
    REQUIRE(rohr_physics_angular_velocity_set(body,12)); REQUIRE(rohr_physics_dynamic_set(body));
    REQUIRE(rohr_physics_friction_set(body,.4f)); REQUIRE(rohr_physics_restitution_set(body,.2f));
    REQUIRE(rohr_physics_collision_filter_description_set(body,(CollisionFilterDescription){4,8}));
    REQUIRE(rohr_entity_components_add(body,ROHR_COLLISION));
    REQUIRE(rohr_physics_center_of_mass_local_position_set(body,(Position){1,-.5f}));
    REQUIRE(rohr_physics_angular_acceleration_set(body,2)); REQUIRE(rohr_physics_angular_velocity_maximum_set(body,50));
    REQUIRE(rohr_entity_lifetime_description_set(body,(EntityLifetimeDescription){
        .expiry_time=rohr_engine_time_get()+100,.expiry_tick=rohr_engine_tick_get()+100000,
        .time_enabled=true,.tick_enabled=true}));
    s.e[C_PARTICLE]=construction_take(rohr_physics_particle_add((ParticleDescription){.owner=root,
        .position={70+offset.x,30+offset.y},.local_origin={1,0},.radius=2,.rigid_vertices=12,
        .velocity={-1,0},.mass_value=2,.friction=.3f,.restitution=.6f,
        .collision_filter={8,4},.collision_enabled=true}));
    s.e[C_SOFT]=construction_take(rohr_physics_soft_body_add((SoftBodyDescription){.owner=root,
        .position={100+offset.x,50+offset.y},.orientation=30}));
    s.e[C_OTHER]=construction_take(rohr_physics_soft_body_add((SoftBodyDescription){.owner=root,
        .position={-30+offset.x,50+offset.y}}));
    const Position points[]={{0,0},{20,0},{20,20},{0,20},{5,5},{10,5},{7,10}};
    for(int i=0;i<7;++i) s.e[C_NODE+i]=construction_take(rohr_physics_soft_body_node_local_add(
        (SoftBodyNodeDescription){.owner=s.e[C_SOFT],.position=points[i],.mass=(float)i+1,.radius=.5f,
            .velocity={.2f,.1f},.friction=.25f,.restitution=.1f,.static_body=i==0,
            .collision_enabled=true,.collision_filter={32,0},
            .draw_color={10,20,30,255},.draw_color_overridden=true}));
    for(int i=0;i<7;++i) REQUIRE(rohr_physics_orientation_set(s.e[C_NODE+i],30));
    for(int i=0;i<4;++i) s.e[C_BEAM+i]=construction_take(rohr_physics_soft_body_beam_add(
        (SoftBodyBeamDescription){.owner=s.e[C_SOFT],.node_a=s.e[C_NODE+i],.node_b=s.e[C_NODE+(i+1)%4],
            .stiffness=40,.damping=2,.rest_length=20,.rest_length_explicit=i==0,
            .friction=.2f,.restitution=.3f,.collision={.enabled=i==1,.thickness=.2f,.filter={64,0}},
            .draw_color={40,50,60,255},.draw_color_overridden=true}));
    s.e[C_AREA]=construction_take(rohr_physics_soft_body_area_add((SoftBodyAreaDescription){
        .owner=s.e[C_SOFT],.geometry={.outer={.nodes={s.e[C_NODE],s.e[C_NODE+1],s.e[C_NODE+2],s.e[C_NODE+3]},.node_count=4},
            .hole_count=1,.holes={{.nodes={s.e[C_NODE+4],s.e[C_NODE+5],s.e[C_NODE+6]},.node_count=3}}},
        .visible=true,.draw_color={70,80,90,255},.draw_color_overridden=true}));
    s.e[C_WORLD]=construction_entity(root,(Position){100+offset.x,50+offset.y}); REQUIRE(rohr_physics_static_set(s.e[C_WORLD]));
    JointAnchorIdResult a=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=s.e[C_NODE]}); REQUIRE(a);
    JointAnchorIdResult b=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=s.e[C_WORLD]}); REQUIRE(b);
    const JointType kinds[]={JOINT_PIN,JOINT_WELD,JOINT_SPRING};
    for(int i=0;i<3;++i) s.e[C_JOINT+i]=construction_take(rohr_physics_joint_add((JointDescription){
        .owner=root,.type=kinds[i],.anchor_a=a.result.value,.anchor_b=b.result.value,
        .stiffness=30,.damping=2,.rest_length=2,.rest_length_explicit=true,.rest_angle=15,.rest_angle_explicit=true}));
    for(int location=0;location<4;++location) for(int angle=0;angle<4;++angle) {
        ForceLocation at={.offset={1,2},.reference.kind=location};
        if(location==ROHR_FORCE_COM_POSITION || location==ROHR_FORCE_ORIGIN_POSITION) at.reference.entity=body;
        if(location==ROHR_FORCE_BEAM_POSITION) {
            at.reference.beam.entity=s.e[C_BEAM]; at.reference.beam.anchor=ROHR_FORCE_BEAM_CENTER;
            at.reference.beam.fraction=.3f; }
        ForceOrientation direction={.offset=13,.reference.kind=angle};
        if(angle==ROHR_FORCE_BODY_ORIENTATION) direction.reference.entity=body;
        if(angle==ROHR_FORCE_BEAM_ORIENTATION) direction.reference.entity=s.e[C_BEAM];
        if(angle==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            direction.reference.between.from=at; direction.reference.between.to=(ForceLocation){.offset={80,90}};
        }
        s.e[C_FORCE+location*4+angle]=construction_take(rohr_physics_force_add((ForceDescription){
            .owner=root,.target=body,.magnitude=.25f,.location=at,.orientation=direction}));
        REQUIRE(rohr_graphics_layer_entity_set(s.e[C_FORCE+location*4+angle], 100+location*4+angle));
    }
    s.e[C_TORQUE]=construction_take(rohr_physics_torque_add((TorqueDescription){.owner=root,.target=body,.magnitude=-.5f}));
    REQUIRE(rohr_graphics_layer_entity_set(s.e[C_TORQUE], -55));
    s.e[C_INITIAL]=construction_take(rohr_physics_torque_add((TorqueDescription){.owner=root}));
    REQUIRE(rohr_entity_lifetime_description_set(s.e[C_INITIAL],(EntityLifetimeDescription){
        .tick_enabled=true,.expiry_tick=rohr_engine_tick_get()+1}));
    return s;
}
static int construction_role(const ConstructionScene *s,Entity e) {
    if(e==ENTITY_INVALID) return -1;
    for(int i=0;i<C_COUNT;++i) if(s->e[i]==e) return i;
    fprintf(stderr,"Unmapped construction reference\n"); abort();
}
static void construction_value(ConstructionSnapshot *s,double value,const char *label) {
    assert(s->count<4096 && isfinite(value)); s->labels[s->count]=label; s->values[s->count++]=value;
}
#define V(value) construction_value(out,(double)(value),#value)
#define XY(value) do { Vec2D v_=(value); V(v_.x); V(v_.y); } while(0)
#define EXACT(value) do { V(value); out->exact[out->count-1]=true; } while(0)
#define REF(value) EXACT(construction_role(scene,(value)))
static void construction_location(const ConstructionScene *scene,ConstructionSnapshot *out,ForceLocation l) {
    EXACT(l.reference.kind); XY(l.offset);
    if(l.reference.kind==ROHR_FORCE_COM_POSITION || l.reference.kind==ROHR_FORCE_ORIGIN_POSITION) REF(l.reference.entity);
    if(l.reference.kind==ROHR_FORCE_BEAM_POSITION) { REF(l.reference.beam.entity); EXACT(l.reference.beam.anchor); V(l.reference.beam.fraction); }
}
static void construction_capture(const ConstructionScene *scene,ConstructionSnapshot *out,bool motion) {
    *out=(ConstructionSnapshot){0};
    const int physical[]={C_BODY,C_PARTICLE,C_SOFT,C_OTHER,C_NODE,C_NODE+1,C_NODE+2,C_NODE+3,C_NODE+4,C_NODE+5,C_NODE+6};
    for(size_t n=0;n<sizeof(physical)/sizeof(physical[0]);++n) {
        Entity e=scene->e[physical[n]]; REF(rohr_entity_owner_get(e).result.value);
        PositionResult p=rohr_physics_position_get(e); REQUIRE(p); XY(p.result.value);
        EntityInfoResult info=rohr_entity_info_get(e,ROHR_ENTITY_INFO_PHYSICS_ORIENTATION | ROHR_ENTITY_INFO_PHYSICS_VELOCITY | ROHR_ENTITY_INFO_PHYSICS_FRICTION | ROHR_ENTITY_INFO_PHYSICS_RESTITUTION | ROHR_ENTITY_INFO_PHYSICS_ACCELERATION | ROHR_ENTITY_INFO_PHYSICS_ANGULAR_VELOCITY); REQUIRE(info);
        EntityInfo value=info.result.value; rohr_entity_info_release(&info.result.value);
        V(value.orientation);
        if(physical[n]==C_SOFT || physical[n]==C_OTHER) continue;
        XY(value.velocity); V(value.angular_velocity);
        if(motion) continue;
        XY(value.acceleration); EXACT(rohr_physics_gravity_check(e));
        V(rohr_physics_mass_get(e).result.value); V(value.friction);
        V(value.restitution);
        EXACT(rohr_entity_components_check(e,ROHR_STATIC)); EXACT(rohr_entity_components_check(e,ROHR_DYNAMIC));
        EXACT(rohr_entity_components_check(e,ROHR_COLLISION));
        CollisionFilterDescriptionResult f=rohr_physics_collision_filter_description_get(e); REQUIRE(f);
        EXACT(f.result.value.category); EXACT(f.result.value.responds_to);
    }
    if(motion) return;
    Entity body=scene->e[C_BODY];
    XY(rohr_physics_center_of_mass_local_position_get(body).result.value);
    V(rohr_physics_moment_of_inertia_get(body).result.value);
    V(rohr_physics_angular_velocity_get(body).result.value); V(rohr_physics_angular_velocity_maximum_get(body).result.value);
    EntityInfoResult acceleration=rohr_entity_info_get(body,ROHR_ENTITY_INFO_PHYSICS_ANGULAR_ACCELERATION); REQUIRE(acceleration);
    V(acceleration.result.value.angular_acceleration); rohr_entity_info_release(&acceleration.result.value);
    EntityLifetimeDescription lifetime=rohr_entity_lifetime_description_get(body).result.value;
    V(lifetime.expiry_time-rohr_engine_time_get()); EXACT(lifetime.expiry_tick-rohr_engine_tick_get());
    Shape shape=rohr_physics_hitbox_get(body).result.value; EXACT(shape.amount_of_vertices);
    for(unsigned i=0;i<shape.amount_of_vertices;++i) XY(shape.vertices[i]);
    ParticleDescription particle=rohr_physics_particle_description_get(scene->e[C_PARTICLE]).result.value;
    XY(particle.local_origin); V(particle.radius); EXACT(particle.rigid_vertices);
    for(int i=0;i<7;++i) {
        SoftBodyNodeDescriptionResult result=rohr_physics_soft_body_node_description_get(scene->e[C_NODE+i]); REQUIRE(result);
        SoftBodyNodeDescription n=result.result.value;
        V(n.radius); EXACT(n.draw_color.red); EXACT(n.draw_color.green); EXACT(n.draw_color.blue);
        EXACT(n.draw_color.alpha); EXACT(n.draw_color_overridden);
    }
    for(int i=0;i<4;++i) {
        SoftBodyBeamDescription b=rohr_physics_soft_body_beam_description_get(scene->e[C_BEAM+i]).result.value;
        REF(b.owner); REF(b.node_a); REF(b.node_b); V(b.rest_length); V(b.stiffness); V(b.damping);
        V(b.friction); V(b.restitution); EXACT(b.collision.enabled); V(b.collision.thickness);
        EXACT(b.collision.filter.category); EXACT(b.collision.filter.responds_to);
        EXACT(b.draw_color.red); EXACT(b.draw_color.green); EXACT(b.draw_color.blue); EXACT(b.draw_color_overridden);
    }
    SoftBodyAreaDescription area=rohr_physics_soft_body_area_description_get(scene->e[C_AREA]).result.value;
    REF(area.owner); EXACT(area.visible); EXACT(area.geometry.hole_count); EXACT(area.draw_color.red);
    EXACT(area.draw_color.green); EXACT(area.draw_color.blue); EXACT(area.draw_color.alpha); EXACT(area.draw_color_overridden);
    for(unsigned l=0;l<=area.geometry.hole_count;++l) {
        SoftBodyAreaLoop loop=l?area.geometry.holes[l-1]:area.geometry.outer; EXACT(loop.node_count);
        for(unsigned i=0;i<loop.node_count;++i) REF(loop.nodes[i]);
    }
    for(int i=0;i<3;++i) {
        JointDescription j=rohr_physics_joint_description_get(scene->e[C_JOINT+i]).result.value;
        REF(j.owner); EXACT(j.type); V(j.rest_length); V(j.rest_angle); V(j.stiffness); V(j.damping);
        JointAnchorDescription a=rohr_physics_joint_anchor_description_get(j.anchor_a).result.value,b=rohr_physics_joint_anchor_description_get(j.anchor_b).result.value;
        REF(a.entity); REF(b.entity); XY(a.local_offset); XY(b.local_offset);
    }
    for(int i=0;i<16;++i) {
        ForceDescription f=rohr_physics_force_description_get(scene->e[C_FORCE+i]).result.value;
        EXACT(rohr_graphics_layer_entity_get(scene->e[C_FORCE+i]).result.value);
        REF(f.owner); REF(f.target); V(f.magnitude); construction_location(scene,out,f.location);
        V(f.orientation.offset); EXACT(f.orientation.reference.kind);
        if(f.orientation.reference.kind==ROHR_FORCE_BODY_ORIENTATION || f.orientation.reference.kind==ROHR_FORCE_BEAM_ORIENTATION)
            REF(f.orientation.reference.entity);
        if(f.orientation.reference.kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            construction_location(scene,out,f.orientation.reference.between.from);
            construction_location(scene,out,f.orientation.reference.between.to);
        }
    }
    TorqueDescription t=rohr_physics_torque_description_get(scene->e[C_TORQUE]).result.value;
    REF(t.owner); REF(t.target); V(t.magnitude);
    EXACT(rohr_graphics_layer_entity_get(scene->e[C_TORQUE]).result.value);
    lifetime=rohr_entity_lifetime_description_get(scene->e[C_INITIAL]).result.value;
    EXACT(lifetime.tick_enabled); EXACT(lifetime.expiry_tick-rohr_engine_tick_get());
}
#undef EXACT
#undef REF
#undef XY
#undef V
static void construction_compare(const ConstructionSnapshot *a,const ConstructionSnapshot *b,const char *path,bool motion) {
    assert(a->count==b->count);
    for(size_t i=0;i<a->count;++i) {
        assert(a->exact[i]==b->exact[i]);
        double tolerance=a->exact[i] ? 0 : motion ? .002 : 1e-4+1e-5*fmax(fabs(a->values[i]),fabs(b->values[i]));
        if(fabs(a->values[i]-b->values[i])>tolerance) {
            fprintf(stderr,"%s value %zu (%s): direct %.9g, authored %.9g\n",path,i,a->labels[i],a->values[i],b->values[i]); abort();
        }
    }
}
static void construction_simulate(ConstructionScene s,ConstructionSnapshot *out) {
    REQUIRE(rohr_physics_substeps_set(2)); REQUIRE(rohr_physics_solver_iterations_set(8));
    for(int i=0;i<120;++i) assert(rohr_physics_dt_update(1.0/120).header.status!=ROHR_REPORT_FAILED);
    construction_capture(&s,out,true);
}
static void construction_mutate(ConstructionScene s) {
    REQUIRE(rohr_physics_soft_body_beam_endpoints_set(s.e[C_BEAM],s.e[C_NODE],ENTITY_INVALID));
    REQUIRE(rohr_entity_owner_set(s.e[C_NODE+1],s.e[C_OTHER]));
    Entity bridge=construction_take(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner=s.e[C_OTHER],.node_a=s.e[C_NODE],.node_b=s.e[C_NODE+1],.stiffness=20}));
    assert(rohr_entity_owner_get(s.e[C_NODE]).result.value==s.e[C_SOFT]);
    REQUIRE(rohr_entity_delete(s.e[C_SOFT]));
    assert(rohr_entity_alive_check(bridge));
    SoftBodyBeamDescription b=rohr_physics_soft_body_beam_description_get(bridge).result.value;
    assert(b.node_a==ENTITY_INVALID && b.node_b==s.e[C_NODE+1]);
    assert(rohr_entity_alive_check(s.e[C_NODE+1]));
    assert(rohr_entity_alive_check(s.e[C_JOINT]));
    assert(rohr_physics_joint_description_get(s.e[C_JOINT]).result.value.anchor_a==JOINT_ANCHOR_INVALID);
}
static void construction_lifecycle(ConstructionScene s) {
    construction_mutate(s);
    REQUIRE(rohr_engine_time_per_tick_set(.001)); SDL_Delay(3); (void)rohr_engine_tick_update(); rohr_engine_lifecycle_update();
    assert(!rohr_entity_alive_check(s.e[C_INITIAL]) && rohr_entity_alive_check(s.e[C_TORQUE]));
    REQUIRE(rohr_entity_delete(s.e[C_ROOT]));
}
#endif
