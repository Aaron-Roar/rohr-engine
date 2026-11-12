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
void *__wrap_calloc(size_t n, size_t s) { assert(!allocation_forbidden); return __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { assert(!allocation_forbidden); return __real_realloc(p,n); }
#endif
static void success(EngineResult r, int line) {
    if(error_check(r)) fprintf(stderr,"line %d: %s\n",line,error_code_message_get(r.result.error));
    assert(!error_check(r) && r.result.value);
}
#define OK(r) success((r),__LINE__)
#define BAD(r) assert(error_check(r))
#define CALL(name, ...) (facade ? rohr_physics_##name(__VA_ARGS__) : physics_##name(__VA_ARGS__))
static Entity made(EntityResult r) { assert(!error_check(r)); return r.result.value; }
static bool near(float a,float b) { return fabsf(a-b)<0.0005f; }
static void point(Position a,Position b) { assert(near(a.x,b.x) && near(a.y,b.y)); }
static Entity body(Position p, float angle) {
    Entity e=made(rohr_entity_add());
    OK(rohr_physics_position_set(e,p)); OK(rohr_physics_orientation_set(e,angle));
    return e;
}
static JointAnchorId anchor(Entity e,Position p) {
    JointAnchorIdResult r=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=e,.local_offset=p});
    assert(!error_check(r)); return r.result.value;
}
static JointDescription joint_read(Entity e) {
    JointDescriptionResult r=rohr_physics_joint_description_get(e); assert(!error_check(r)); return r.result.value;
}
static void joint_equal(JointDescription a,JointDescription b) {
    assert(a.owner==b.owner && a.type==b.type && a.anchor_a==b.anchor_a && a.anchor_b==b.anchor_b);
    assert(near(a.rest_length,b.rest_length) && near(a.rest_angle,b.rest_angle));
    assert(a.stiffness==b.stiffness && a.damping==b.damping);
    assert(a.rest_length_explicit && a.rest_angle_explicit);
}
static void locks(bool facade) {
    Entity e=body((Position){1,2},30), driver=body((Position){4,5},60), other=body((Position){6,7},90);
    OK(rohr_physics_angle_lock_set(e,-90,90));
    OK(rohr_physics_axis_lock_description_set(e,(AxisLockDescription){.axis={3,4},.point_on_axis={5,6}}));
    OK(rohr_physics_transform_lock_description_set(e,(TransformLockDescription){.driver=driver,
        .local_offset={1,2},.local_angle=3,.lock_position=true,.lock_orientation=true,.inherit_velocity=true}));
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=true;
#endif
    OK(CALL(angle_lock_min_set,e,-45)); assert(rohr_physics_angle_lock_get(e).result.value.max==90);
    OK(CALL(angle_lock_max_set,e,45)); assert(rohr_physics_angle_lock_get(e).result.value.min==-45);
    OK(CALL(angle_lock_min_radians_set,e,-PI_F/2));
    OK(CALL(angle_lock_max_radians_set,e,PI_F/4));
    AngleLock lock=rohr_physics_angle_lock_get(e).result.value;
    assert(near(lock.min,-90) && near(lock.max,45));
    BAD(CALL(angle_lock_min_set,e,NAN)); BAD(CALL(angle_lock_max_set,e,INFINITY));
    BAD(CALL(angle_lock_min_radians_set,e,FLT_MAX)); BAD(CALL(angle_lock_max_radians_set,e,NAN));
    assert(near(rohr_physics_angle_lock_get(e).result.value.min,-90));
    assert(near(rohr_physics_angle_lock_get(e).result.value.max,45));
    OK(CALL(axis_lock_axis_set,e,((Axis){0,2})));
    point(rohr_physics_axis_lock_point_on_axis_get(e).result.value,(Position){5,6});
    OK(CALL(axis_lock_axis_x_set,e,1));
    point(rohr_physics_axis_lock_axis_get(e).result.value,(Axis){sqrtf(.5f),sqrtf(.5f)});
    OK(CALL(axis_lock_axis_y_set,e,0));
    point(rohr_physics_axis_lock_axis_get(e).result.value,(Axis){1,0});
    BAD(CALL(axis_lock_axis_x_set,e,0)); BAD(CALL(axis_lock_axis_y_set,e,NAN));
    BAD(CALL(axis_lock_axis_set,e,((Axis){INFINITY,1})));
    OK(CALL(axis_lock_point_on_axis_set,e,((Position){7,8})));
    OK(CALL(axis_lock_point_on_axis_x_set,e,9));
    point(rohr_physics_axis_lock_point_on_axis_get(e).result.value,(Position){9,8});
    OK(CALL(axis_lock_point_on_axis_y_set,e,10));
    BAD(CALL(axis_lock_point_on_axis_x_set,e,FLT_MAX));
    BAD(CALL(axis_lock_point_on_axis_y_set,e,NAN));
    point(rohr_physics_axis_lock_point_on_axis_get(e).result.value,(Position){9,10});
    point(rohr_physics_axis_lock_axis_get(e).result.value,(Axis){1,0});
    OK(CALL(transform_lock_driver_set,e,other));
    OK(CALL(transform_lock_local_offset_set,e,((Position){3,4})));
    OK(CALL(transform_lock_local_offset_x_set,e,5));
    point(rohr_physics_transform_lock_local_offset_get(e).result.value,(Position){5,4});
    OK(CALL(transform_lock_local_offset_y_set,e,6));
    OK(CALL(transform_lock_local_angle_set,e,45));
    OK(CALL(transform_lock_local_angle_radians_set,e,PI_F/2));
    OK(CALL(transform_lock_lock_position_set,e,false));
    assert(rohr_physics_transform_lock_lock_orientation_check(e));
    OK(CALL(transform_lock_lock_orientation_set,e,false));
    assert(rohr_physics_transform_lock_inherit_velocity_check(e));
    OK(CALL(transform_lock_inherit_velocity_set,e,false));
    BAD(CALL(transform_lock_local_angle_radians_set,e,FLT_MAX));
    BAD(CALL(transform_lock_local_angle_set,e,NAN));
    BAD(CALL(transform_lock_local_offset_x_set,e,INFINITY));
    BAD(CALL(transform_lock_local_offset_y_set,e,NAN));
    BAD(CALL(transform_lock_driver_set,e,ENTITY_INVALID));
    TransformLockDescription t=rohr_physics_transform_lock_description_get(e).result.value;
    assert(t.driver==other && near(t.local_angle,90)); point(t.local_offset,(Position){5,6});
    assert(!t.lock_position && !t.lock_orientation && !t.inherit_velocity);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=false;
#endif
    OK(rohr_entity_delete(other));
    BAD(CALL(transform_lock_local_angle_set,e,0));
    OK(CALL(transform_lock_driver_set,e,driver)); /* Repair a stale borrowed reference. */
    point(rohr_physics_transform_lock_local_offset_get(e).result.value,(Position){5,6});
    OK(rohr_entity_delete(e)); assert(rohr_entity_alive_check(driver)); OK(rohr_entity_delete(driver));
}
static void joint_fields(bool facade) {
    Entity owner=made(rohr_entity_add()), a=body((Position){0,0},10), b=body((Position){3,4},40);
    Entity c=body((Position){100,100},80);
    JointAnchorId aa=anchor(a,(Position){0}), ab=anchor(b,(Position){0}), ac=anchor(c,(Position){2,3});
    Entity j=made(rohr_physics_joint_add((JointDescription){.owner=owner,.type=JOINT_SPRING,
        .anchor_a=aa,.anchor_b=ab,.stiffness=4,.damping=5}));
    JointDescription expected=joint_read(j); assert(near(expected.rest_length,5) && near(expected.rest_angle,30));
    OK(rohr_entity_lifetime_description_set(j,(EntityLifetimeDescription){.tick_enabled=true,.expiry_tick=100}));
    OK(rohr_physics_position_set(b,(Position){30,40})); OK(rohr_physics_orientation_set(b,100));
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=true;
#endif
    OK(CALL(joint_stiffness_set,j,12)); expected.stiffness=12; joint_equal(joint_read(j),expected);
    OK(CALL(joint_damping_set,j,13)); expected.damping=13; joint_equal(joint_read(j),expected);
    OK(CALL(joint_type_set,j,JOINT_PIN)); expected.type=JOINT_PIN; joint_equal(joint_read(j),expected);
    OK(CALL(joint_anchor_a_set,j,ac)); expected.anchor_a=ac; joint_equal(joint_read(j),expected);
    OK(CALL(joint_anchor_b_set,j,JOINT_ANCHOR_INVALID)); expected.anchor_b=JOINT_ANCHOR_INVALID; joint_equal(joint_read(j),expected);
    OK(CALL(joint_stiffness_set,j,14)); expected.stiffness=14; joint_equal(joint_read(j),expected);
    OK(CALL(joint_anchor_b_set,j,ab)); expected.anchor_b=ab; joint_equal(joint_read(j),expected);
    OK(CALL(joint_rest_length_set,j,0)); expected.rest_length=0; joint_equal(joint_read(j),expected);
    OK(CALL(joint_rest_angle_set,j,0)); expected.rest_angle=0; joint_equal(joint_read(j),expected);
    OK(CALL(joint_rest_angle_radians_set,j,PI_F/2)); expected.rest_angle=90; joint_equal(joint_read(j),expected);
    BAD(CALL(joint_rest_length_set,j,-1)); BAD(CALL(joint_stiffness_set,j,NAN));
    BAD(CALL(joint_damping_set,j,-1)); BAD(CALL(joint_type_set,j,(JointType)100));
    BAD(CALL(joint_rest_angle_set,j,INFINITY)); BAD(CALL(joint_rest_angle_radians_set,j,FLT_MAX));
    joint_equal(joint_read(j),expected);
    OK(CALL(joint_anchor_entity_set,ac,a));
    assert(CALL(joint_anchor_entity_get,ac).result.value==a);
    point(rohr_physics_joint_anchor_local_position_get(ac).result.value,(Position){2,3});
    joint_equal(joint_read(j),expected);
    BAD(CALL(joint_anchor_entity_set,ac,ENTITY_INVALID));
    assert(CALL(joint_anchor_entity_get,ac).result.value==a);
    assert(rohr_entity_lifetime_description_get(j).result.value.expiry_tick==100);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=false;
#endif
    OK(rohr_entity_delete(c)); /* Transferred anchor survives its old entity. */
    assert(!error_check(CALL(joint_anchor_entity_get,ac)));
    OK(rohr_physics_joint_anchor_remove(aa));
    JointAnchorId replacement=anchor(a,(Position){8,9}); assert(replacement!=aa);
    BAD(CALL(joint_anchor_a_set,j,aa)); BAD(CALL(joint_anchor_b_set,j,aa));
    BAD(CALL(joint_anchor_entity_set,aa,b)); BAD(CALL(joint_anchor_entity_get,aa));
    joint_equal(joint_read(j),expected);
    point(rohr_physics_joint_anchor_local_position_get(replacement).result.value,(Position){8,9});
    OK(rohr_entity_delete(owner)); assert(!rohr_entity_alive_check(j));
    assert(rohr_entity_alive_check(a) && rohr_entity_alive_check(b));
    OK(rohr_entity_delete(a)); OK(rohr_entity_delete(b));
}
static SoftBodyBeamDescription beam_read(Entity e) {
    SoftBodyBeamDescriptionResult r=rohr_physics_soft_body_beam_description_get(e); assert(!error_check(r)); return r.result.value;
}
static void beam_equal(SoftBodyBeamDescription a,SoftBodyBeamDescription b) {
    assert(a.owner==b.owner && a.node_a==b.node_a && a.node_b==b.node_b);
    assert(near(a.rest_length,b.rest_length) && a.rest_length_explicit);
    assert(a.stiffness==b.stiffness && a.damping==b.damping && a.friction==b.friction && a.restitution==b.restitution);
    assert(a.collision.enabled==b.collision.enabled && a.collision.thickness==b.collision.thickness);
    assert(a.collision.filter.category==b.collision.filter.category && a.collision.filter.responds_to==b.collision.filter.responds_to);
    assert(a.draw_color.red==b.draw_color.red && a.draw_color.green==b.draw_color.green && a.draw_color.blue==b.draw_color.blue && a.draw_color.alpha==b.draw_color.alpha);
    assert(a.draw_color_overridden==b.draw_color_overridden);
}
static void soft_body_fields(bool facade) {
    Entity owner=made(rohr_physics_soft_body_add((SoftBodyDescription){.position={10,20},.orientation=90}));
    Entity nodes[3]; Position local_positions[]={{0,0},{3,4},{10,10}};
    for(size_t i=0;i<3;++i) nodes[i]=made(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .owner=owner,.position=local_positions[i],.mass=2,.radius=1,.velocity={2,3},.acceleration={4,5},
        .friction=.3f,.restitution=.4f,.collision_filter={3,5},.gravity_enabled=true,.collision_enabled=true,
        .draw_color={1,2,3,4},.draw_color_overridden=true}));
    Entity beam=made(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=owner,.node_a=nodes[0],.node_b=nodes[1],
        .stiffness=4,.damping=5,.friction=.6f,.restitution=.7f,.collision={.enabled=true,.thickness=2,.filter={3,5}},
        .draw_color={6,7,8,9},.draw_color_overridden=true}));
    SoftBodyBeamDescription expected=beam_read(beam); assert(near(expected.rest_length,5));
    OK(rohr_physics_position_set(nodes[1],(Position){100,100}));
    Shape polygon=rohr_physics_hitbox_get(nodes[0]).result.value;
    SoftBodyNodeDescription before=rohr_physics_soft_body_node_description_get(nodes[0]).result.value;
    Position local_before=rohr_physics_soft_body_node_local_position_get(nodes[0]).result.value;
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=true;
#endif
    OK(CALL(soft_body_beam_stiffness_set,beam,12)); expected.stiffness=12; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_damping_set,beam,13)); expected.damping=13; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_node_a_set,beam,nodes[2])); expected.node_a=nodes[2]; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_node_b_set,beam,ENTITY_INVALID)); expected.node_b=ENTITY_INVALID; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_damping_set,beam,14)); expected.damping=14; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_node_b_set,beam,nodes[1])); expected.node_b=nodes[1]; beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_beam_rest_length_set,beam,0)); expected.rest_length=0; beam_equal(beam_read(beam),expected);
    BAD(CALL(soft_body_beam_stiffness_set,beam,NAN)); BAD(CALL(soft_body_beam_damping_set,beam,-1));
    BAD(CALL(soft_body_beam_rest_length_set,beam,-1)); BAD(CALL(soft_body_beam_node_a_set,beam,owner));
    beam_equal(beam_read(beam),expected);
    OK(CALL(soft_body_node_radius_set,nodes[0],3)); BAD(CALL(soft_body_node_radius_set,nodes[0],0));
    BAD(CALL(soft_body_node_radius_set,nodes[0],NAN));
    SoftBodyNodeDescription after=rohr_physics_soft_body_node_description_get(nodes[0]).result.value;
    assert(after.radius==3 && after.owner==before.owner && after.mass==before.mass);
    point(after.position,before.position); point(after.velocity,before.velocity); point(after.acceleration,before.acceleration);
    assert(after.friction==before.friction && after.restitution==before.restitution);
    assert(after.collision_filter.category==before.collision_filter.category && after.collision_filter.responds_to==before.collision_filter.responds_to);
    assert(after.static_body==before.static_body && after.kinematic_driven==before.kinematic_driven);
    assert(after.gravity_enabled==before.gravity_enabled && after.collision_enabled==before.collision_enabled);
    assert(after.draw_color.red==1 && after.draw_color.green==2 && after.draw_color.blue==3 && after.draw_color.alpha==4 && after.draw_color_overridden);
    point(rohr_physics_soft_body_node_local_position_get(nodes[0]).result.value,local_before);
    Shape same=rohr_physics_hitbox_get(nodes[0]).result.value; assert(same.amount_of_vertices==polygon.amount_of_vertices);
    for(uint16_t i=0;i<same.amount_of_vertices;++i) point(same.vertices[i],polygon.vertices[i]);
    /* Existing collision setters already preserve other fields; audit all families. */
    for(size_t i=0;i<2;++i) {
        Entity e=i?beam:nodes[0];
        OK(rohr_physics_collision_category_set(e,9)); assert(rohr_physics_collision_response_get(e).result.value==5);
        OK(rohr_physics_collision_response_set(e,17)); assert(rohr_physics_collision_category_get(e).result.value==9);
    }
    SoftBodyNode current_node=rohr_physics_soft_body_node_get(nodes[0]).result.value;
    assert(current_node.category==9 && current_node.responds_to==17);
    OK(rohr_physics_soft_body_beam_collision_disable(beam));
    assert(rohr_physics_soft_body_beam_collision_thickness_get(beam).result.value==2);
    OK(rohr_physics_soft_body_beam_collision_enable(beam));
    assert(rohr_physics_collision_response_get(beam).result.value==17);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=false;
#endif
    Entity stale=nodes[0]; OK(rohr_entity_delete(stale));
    Entity replacement=made(rohr_entity_add()); assert(replacement!=stale);
    BAD(CALL(soft_body_beam_node_b_set,beam,stale)); BAD(CALL(soft_body_node_radius_set,stale,2));
    OK(rohr_entity_delete(owner)); assert(!rohr_entity_alive_check(beam));
    assert(rohr_entity_alive_check(replacement)); OK(rohr_entity_delete(replacement));
}
static void particles(bool facade) {
    Entity owner=made(rohr_entity_add());
    ParticleDescription d=rohr_physics_particle_description_default_get();
    d.owner=owner; d.radius=2; d.local_origin=(Position){3,4}; d.position=(Position){5,6};
    d.velocity=(Velocity){7,8}; d.acceleration=(Acceleration){9,10}; d.rigid_vertices=8;
    d.static_body=true; d.kinematic_driven=true; d.gravity_enabled=true; d.collision_filter=(CollisionFilterDescription){3,5};
    Entity e=made(rohr_physics_particle_add(d));
    OK(rohr_physics_hitbox_add(e,rohr_math_square_create(7,8)));
    HitboxId active=rohr_physics_hitbox_active_id_get(e).result.value;
    HitboxId inactive=rohr_physics_hitbox_id_at_get(e,1).result.value;
    Shape other=rohr_physics_hitbox_by_id_get(e,inactive).result.value;
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=true;
#endif
    OK(CALL(particle_rigid_vertices_set,e,16));
    ParticleDescription after=rohr_physics_particle_description_get(e).result.value;
    assert(after.rigid_vertices==16 && after.radius==d.radius && after.owner==owner);
    point(after.local_origin,d.local_origin); point(after.position,d.position);
    point(after.velocity,d.velocity); point(after.acceleration,d.acceleration);
    assert(after.mass_value==d.mass_value && after.friction==d.friction && after.restitution==d.restitution);
    assert(after.static_body && after.kinematic_driven && after.gravity_enabled && after.collision_enabled);
    assert(after.collision_filter.category==3 && after.collision_filter.responds_to==5);
    assert(rohr_physics_hitbox_active_id_get(e).result.value==active);
    Shape same=rohr_physics_hitbox_by_id_get(e,inactive).result.value;
    assert(same.amount_of_vertices==other.amount_of_vertices);
    for(uint16_t i=0;i<same.amount_of_vertices;++i) point(same.vertices[i],other.vertices[i]);
    Shape shape=rohr_physics_hitbox_get(e).result.value; assert(shape.amount_of_vertices==16);
    for(uint16_t i=0;i<shape.amount_of_vertices;++i)
        assert(near(hypotf(shape.vertices[i].x-3,shape.vertices[i].y-4),2));
    BAD(CALL(particle_rigid_vertices_set,e,2)); BAD(CALL(particle_rigid_vertices_set,e,UINT32_MAX));
    assert(rohr_physics_particle_rigid_vertices_get(e).result.value==16);
    OK(CALL(particle_rigid_vertices_set,e,0));
    assert(rohr_physics_particle_rigid_vertices_get(e).result.value==ROHR_PARTICLE_RIGID_VERTICES_DEFAULT);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=false;
#endif
    OK(rohr_entity_delete(owner)); assert(!rohr_entity_alive_check(e));
}
static void invalid(Entity entity, bool facade) {
    BAD(CALL(angle_lock_min_set,entity,(Orientation)0));
    BAD(CALL(angle_lock_min_radians_set,entity,(Orientation)0));
    BAD(CALL(angle_lock_max_set,entity,(Orientation)0));
    BAD(CALL(angle_lock_max_radians_set,entity,(Orientation)0));
    BAD(CALL(axis_lock_axis_set,entity,((Axis){0})));
    BAD(CALL(axis_lock_axis_x_set,entity,(float)0));
    BAD(CALL(axis_lock_axis_y_set,entity,(float)0));
    BAD(CALL(axis_lock_point_on_axis_set,entity,((Position){0})));
    BAD(CALL(axis_lock_point_on_axis_x_set,entity,(float)0));
    BAD(CALL(axis_lock_point_on_axis_y_set,entity,(float)0));
    BAD(CALL(transform_lock_driver_set,entity,(Entity)0));
    BAD(CALL(transform_lock_local_offset_set,entity,((Position){0})));
    BAD(CALL(transform_lock_local_offset_x_set,entity,(float)0));
    BAD(CALL(transform_lock_local_offset_y_set,entity,(float)0));
    BAD(CALL(transform_lock_local_angle_set,entity,(Orientation)0));
    BAD(CALL(transform_lock_lock_position_set,entity,(bool)0));
    BAD(CALL(transform_lock_lock_orientation_set,entity,(bool)0));
    BAD(CALL(transform_lock_inherit_velocity_set,entity,(bool)0));
    BAD(CALL(transform_lock_local_angle_radians_set,entity,(Orientation)0));
    BAD(CALL(joint_type_set,entity,(JointType)0));
    BAD(CALL(joint_anchor_a_set,entity,(JointAnchorId)0));
    BAD(CALL(joint_anchor_b_set,entity,(JointAnchorId)0));
    BAD(CALL(joint_rest_length_set,entity,(float)0));
    BAD(CALL(joint_stiffness_set,entity,(float)0));
    BAD(CALL(joint_damping_set,entity,(float)0));
    BAD(CALL(joint_rest_angle_set,entity,(Orientation)0));
    BAD(CALL(joint_rest_angle_radians_set,entity,(Orientation)0));
    BAD(CALL(soft_body_beam_node_a_set,entity,(Entity)0));
    BAD(CALL(soft_body_beam_node_b_set,entity,(Entity)0));
    BAD(CALL(soft_body_beam_rest_length_set,entity,(float)0));
    BAD(CALL(soft_body_beam_stiffness_set,entity,(float)0));
    BAD(CALL(soft_body_beam_damping_set,entity,(float)0));
    BAD(CALL(soft_body_node_radius_set,entity,(float)0));
    BAD(CALL(particle_rigid_vertices_set,entity,(uint32_t)0));
}
int main(void) {
    rohr_engine_start();
    for(int facade=0;facade<2;++facade) {
        invalid(ENTITY_INVALID,facade);
        Entity bare=made(rohr_entity_add()); invalid(bare,facade);
        OK(rohr_entity_delete(bare));
        Entity replacement=body((Position){99,101},0);
        invalid(bare,facade); point(rohr_physics_position_get(replacement).result.value,(Position){99,101});
        OK(rohr_entity_delete(replacement));
        BAD(CALL(joint_anchor_entity_set,JOINT_ANCHOR_INVALID,ENTITY_INVALID));
        BAD(CALL(joint_anchor_entity_get,JOINT_ANCHOR_INVALID));
        locks(facade); joint_fields(facade); soft_body_fields(facade); particles(facade);
        assert(rohr_entity_alive_count_get()==0);
    }
    rohr_engine_stop();
    return 0;
}
