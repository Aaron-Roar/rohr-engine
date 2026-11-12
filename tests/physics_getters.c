/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool allocation_forbidden;
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void *__real_realloc(void *,size_t);
void *__wrap_malloc(size_t n) { assert(!allocation_forbidden); return __real_malloc(n); }
void *__wrap_calloc(size_t n,size_t s) { assert(!allocation_forbidden); return __real_calloc(n,s); }
void *__wrap_realloc(void *p,size_t n) { assert(!allocation_forbidden); return __real_realloc(p,n); }
#endif
#define OK(r) assert(!error_check(r))
#define BAD(r) assert(error_check(r))
static Entity made(EntityResult value) { OK(value); return value.result.value; }
static bool near(float a,float b) { return fabsf(a-b)<0.001f; }
static void point(Position a,Position b) { assert(near(a.x,b.x) && near(a.y,b.y)); }

static void invalid_getters(Entity entity) {
    /* All new entity/anchor entry points reject invalid and stale identities. */
    BAD(physics_velocity_get(entity));
    BAD(rohr_physics_velocity_get(entity));
    BAD(physics_acceleration_get(entity));
    BAD(rohr_physics_acceleration_get(entity));
    BAD(physics_orientation_get(entity));
    BAD(rohr_physics_orientation_get(entity));
    BAD(physics_angular_acceleration_get(entity));
    BAD(rohr_physics_angular_acceleration_get(entity));
    BAD(physics_friction_get(entity));
    BAD(rohr_physics_friction_get(entity));
    BAD(physics_restitution_get(entity));
    BAD(rohr_physics_restitution_get(entity));
    BAD(physics_angle_lock_get(entity));
    BAD(rohr_physics_angle_lock_get(entity));
    BAD(physics_target_get(entity));
    BAD(rohr_physics_target_get(entity));
    BAD(physics_orientation_radians_get(entity));
    BAD(rohr_physics_orientation_radians_get(entity));
    BAD(physics_angular_acceleration_radians_get(entity));
    BAD(rohr_physics_angular_acceleration_radians_get(entity));
    BAD(physics_velocity_x_get(entity));
    BAD(rohr_physics_velocity_x_get(entity));
    BAD(physics_velocity_y_get(entity));
    BAD(rohr_physics_velocity_y_get(entity));
    BAD(physics_acceleration_x_get(entity));
    BAD(rohr_physics_acceleration_x_get(entity));
    BAD(physics_acceleration_y_get(entity));
    BAD(rohr_physics_acceleration_y_get(entity));
    BAD(physics_position_x_get(entity));
    BAD(rohr_physics_position_x_get(entity));
    BAD(physics_position_y_get(entity));
    BAD(rohr_physics_position_y_get(entity));
    BAD(physics_center_of_mass_local_position_x_get(entity));
    BAD(rohr_physics_center_of_mass_local_position_x_get(entity));
    BAD(physics_center_of_mass_local_position_y_get(entity));
    BAD(rohr_physics_center_of_mass_local_position_y_get(entity));
    BAD(physics_center_of_mass_world_position_x_get(entity));
    BAD(rohr_physics_center_of_mass_world_position_x_get(entity));
    BAD(physics_center_of_mass_world_position_y_get(entity));
    BAD(rohr_physics_center_of_mass_world_position_y_get(entity));
    BAD(physics_particle_origin_x_get(entity));
    BAD(rohr_physics_particle_origin_x_get(entity));
    BAD(physics_particle_origin_y_get(entity));
    BAD(rohr_physics_particle_origin_y_get(entity));
    BAD(physics_soft_body_node_local_position_x_get(entity));
    BAD(rohr_physics_soft_body_node_local_position_x_get(entity));
    BAD(physics_soft_body_node_local_position_y_get(entity));
    BAD(rohr_physics_soft_body_node_local_position_y_get(entity));
    BAD(physics_collision_category_get(entity));
    BAD(rohr_physics_collision_category_get(entity));
    BAD(physics_collision_response_get(entity));
    BAD(rohr_physics_collision_response_get(entity));
    assert(!physics_dynamic_check(entity));
    assert(!rohr_physics_dynamic_check(entity));
    assert(!physics_static_check(entity));
    assert(!rohr_physics_static_check(entity));
    assert(!physics_entity_held_check(entity));
    assert(!rohr_physics_entity_held_check(entity));
    assert(!physics_collision_check(entity));
    assert(!rohr_physics_collision_check(entity));
    BAD(physics_axis_lock_axis_get(entity));
    BAD(rohr_physics_axis_lock_axis_get(entity));
    BAD(physics_axis_lock_axis_x_get(entity));
    BAD(rohr_physics_axis_lock_axis_x_get(entity));
    BAD(physics_axis_lock_axis_y_get(entity));
    BAD(rohr_physics_axis_lock_axis_y_get(entity));
    BAD(physics_axis_lock_point_on_axis_get(entity));
    BAD(rohr_physics_axis_lock_point_on_axis_get(entity));
    BAD(physics_axis_lock_point_on_axis_x_get(entity));
    BAD(rohr_physics_axis_lock_point_on_axis_x_get(entity));
    BAD(physics_axis_lock_point_on_axis_y_get(entity));
    BAD(rohr_physics_axis_lock_point_on_axis_y_get(entity));
    BAD(physics_transform_lock_driver_get(entity));
    BAD(rohr_physics_transform_lock_driver_get(entity));
    BAD(physics_transform_lock_local_offset_get(entity));
    BAD(rohr_physics_transform_lock_local_offset_get(entity));
    BAD(physics_transform_lock_local_offset_x_get(entity));
    BAD(rohr_physics_transform_lock_local_offset_x_get(entity));
    BAD(physics_transform_lock_local_offset_y_get(entity));
    BAD(rohr_physics_transform_lock_local_offset_y_get(entity));
    BAD(physics_transform_lock_local_angle_get(entity));
    BAD(rohr_physics_transform_lock_local_angle_get(entity));
    BAD(physics_transform_lock_local_angle_radians_get(entity));
    BAD(rohr_physics_transform_lock_local_angle_radians_get(entity));
    BAD(physics_joint_type_get(entity));
    BAD(rohr_physics_joint_type_get(entity));
    BAD(physics_joint_rest_length_get(entity));
    BAD(rohr_physics_joint_rest_length_get(entity));
    BAD(physics_joint_stiffness_get(entity));
    BAD(rohr_physics_joint_stiffness_get(entity));
    BAD(physics_joint_damping_get(entity));
    BAD(rohr_physics_joint_damping_get(entity));
    BAD(physics_joint_rest_angle_get(entity));
    BAD(rohr_physics_joint_rest_angle_get(entity));
    BAD(physics_joint_rest_angle_radians_get(entity));
    BAD(rohr_physics_joint_rest_angle_radians_get(entity));
    BAD(physics_soft_body_node_radius_get(entity));
    BAD(rohr_physics_soft_body_node_radius_get(entity));
    BAD(physics_soft_body_beam_node_a_get(entity));
    BAD(rohr_physics_soft_body_beam_node_a_get(entity));
    BAD(physics_soft_body_beam_node_b_get(entity));
    BAD(rohr_physics_soft_body_beam_node_b_get(entity));
    BAD(physics_soft_body_beam_rest_length_get(entity));
    BAD(rohr_physics_soft_body_beam_rest_length_get(entity));
    BAD(physics_soft_body_beam_stiffness_get(entity));
    BAD(rohr_physics_soft_body_beam_stiffness_get(entity));
    BAD(physics_soft_body_beam_damping_get(entity));
    BAD(rohr_physics_soft_body_beam_damping_get(entity));
    BAD(physics_particle_rigid_vertices_get(entity));
    BAD(rohr_physics_particle_rigid_vertices_get(entity));
    assert(!physics_transform_lock_lock_position_check(entity));
    assert(!rohr_physics_transform_lock_lock_position_check(entity));
    assert(!physics_transform_lock_lock_orientation_check(entity));
    assert(!rohr_physics_transform_lock_lock_orientation_check(entity));
    assert(!physics_transform_lock_inherit_velocity_check(entity));
    assert(!rohr_physics_transform_lock_inherit_velocity_check(entity));
    BAD(physics_angle_lock_min_get(entity));
    BAD(rohr_physics_angle_lock_min_get(entity));
    BAD(physics_angle_lock_min_radians_get(entity));
    BAD(rohr_physics_angle_lock_min_radians_get(entity));
    BAD(physics_angle_lock_max_get(entity));
    BAD(rohr_physics_angle_lock_max_get(entity));
    BAD(physics_angle_lock_max_radians_get(entity));
    BAD(rohr_physics_angle_lock_max_radians_get(entity));
    BAD(physics_angle_lock_radians_get(entity));
    BAD(rohr_physics_angle_lock_radians_get(entity));
    BAD(physics_soft_body_beam_collision_thickness_get(entity));
    BAD(rohr_physics_soft_body_beam_collision_thickness_get(entity));
    assert(!physics_soft_body_beam_collision_check(entity));
    assert(!rohr_physics_soft_body_beam_collision_check(entity));
    BAD(physics_soft_body_area_geometry_get(entity));
    BAD(rohr_physics_soft_body_area_geometry_get(entity));
    BAD(physics_soft_body_area_order_get(entity));
    BAD(rohr_physics_soft_body_area_order_get(entity));
    BAD(physics_soft_body_node_count_get(entity));
    BAD(rohr_physics_soft_body_node_count_get(entity));
    BAD(physics_soft_body_beam_count_get(entity));
    BAD(rohr_physics_soft_body_beam_count_get(entity));
    BAD(physics_soft_body_area_count_get(entity));
    BAD(rohr_physics_soft_body_area_count_get(entity));
}

static void invalid_anchor_getters(JointAnchorId anchor) {
    BAD(physics_joint_anchor_local_position_x_get(anchor));
    BAD(rohr_physics_joint_anchor_local_position_x_get(anchor));
    BAD(physics_joint_anchor_local_position_y_get(anchor));
    BAD(rohr_physics_joint_anchor_local_position_y_get(anchor));
    BAD(physics_joint_anchor_world_position_x_get(anchor));
    BAD(rohr_physics_joint_anchor_world_position_x_get(anchor));
    BAD(physics_joint_anchor_world_position_y_get(anchor));
    BAD(rohr_physics_joint_anchor_world_position_y_get(anchor));
    BAD(physics_joint_anchor_a_get(anchor));
    BAD(rohr_physics_joint_anchor_a_get(anchor));
    BAD(physics_joint_anchor_b_get(anchor));
    BAD(rohr_physics_joint_anchor_b_get(anchor));
}

typedef struct ScalarCase { ScalarResult (*get)(Entity); Entity entity; float expected; } ScalarCase;
#define SCALAR(n,e,v) {physics_##n##_get,e,v},{rohr_physics_##n##_get,e,v}
#define VALUE(n,e,expected) do { \
    assert(physics_##n##_get(e).result.value==(expected)); \
    assert(rohr_physics_##n##_get(e).result.value==(expected)); \
} while(0)
#define POINT(n,e,x,y) do { \
    point(physics_##n##_get(e).result.value,(Position){x,y}); \
    point(rohr_physics_##n##_get(e).result.value,(Position){x,y}); \
} while(0)

int main(void) {
    OK(rohr_engine_start());
    invalid_getters(ENTITY_INVALID);
    invalid_anchor_getters(JOINT_ANCHOR_INVALID);
    Entity plain=made(rohr_entity_add());
    BAD(rohr_physics_velocity_get(plain)); BAD(physics_acceleration_get(plain));
    BAD(rohr_physics_velocity_x_get(plain)); BAD(rohr_physics_velocity_y_get(plain));
    BAD(rohr_physics_orientation_get(plain)); BAD(rohr_physics_angular_acceleration_get(plain));
    BAD(rohr_physics_friction_get(plain)); BAD(rohr_physics_restitution_get(plain));
    BAD(rohr_physics_angle_lock_get(plain)); BAD(rohr_physics_target_get(plain));
    BAD(rohr_physics_soft_body_beam_collision_thickness_get(plain));
    assert(!rohr_physics_transform_lock_lock_position_check(plain));
    assert(!rohr_physics_soft_body_beam_collision_check(plain));
    /* Ordinary motion reads agree with snapshots but do not acquire one. */
    OK(rohr_physics_position_set(plain,(Position){10,20}));
    OK(rohr_physics_velocity_set(plain,(Velocity){3,-4}));
    OK(rohr_physics_acceleration_set(plain,(Acceleration){5,-6}));
    OK(rohr_physics_orientation_set(plain,90));
    OK(rohr_physics_angular_acceleration_set(plain,180));
    OK(rohr_physics_friction_set(plain,.4f)); OK(rohr_physics_restitution_set(plain,.6f));
    OK(rohr_physics_center_of_mass_local_position_set(plain,(Position){2,3}));
    OK(rohr_physics_angle_lock_set(plain,-90,90));
    OK(rohr_physics_axis_lock_description_set(plain,(AxisLockDescription){.axis={3,4},.point_on_axis={5,6}}));
    Entity driver=made(rohr_entity_add());
    OK(rohr_physics_position_set(driver,(Position){30,40})); OK(rohr_physics_orientation_set(driver,0));
    OK(rohr_physics_target_set(plain,driver));
    OK(rohr_physics_transform_lock_description_set(plain,(TransformLockDescription){.driver=driver,
        .local_offset={7,8},.local_angle=45,.lock_position=true,.lock_orientation=true,.inherit_velocity=true}));
    JointAnchorIdResult added=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=plain,.local_offset={1,2}}); OK(added);
    JointAnchorId anchor=added.result.value;
    added=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=driver,.local_offset={3,4}}); OK(added);
    Entity joint=made(rohr_physics_joint_add((JointDescription){.type=JOINT_SPRING,.anchor_a=anchor,.anchor_b=added.result.value,
        .rest_length=12,.rest_length_explicit=true,.rest_angle=30,.rest_angle_explicit=true,.stiffness=5,.damping=6}));
    Entity soft=made(rohr_physics_soft_body_add((SoftBodyDescription){.position={50,60},.orientation=90}));
    Entity nodes[3];
    const Position local[]={{0,0},{4,0},{0,4}};
    for(size_t i=0;i<3;++i) nodes[i]=made(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .owner=soft,.position=local[i],.radius=2,.mass=1}));
    Entity beam=made(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=soft,.node_a=nodes[0],.node_b=nodes[1],
        .rest_length=10,.rest_length_explicit=true,.stiffness=7,.damping=8,.friction=.7f,.restitution=.8f,
        .collision={.enabled=true,.thickness=2,.filter={.category=3,.responds_to=5}}}));
    SoftBodyAreaGeometry geometry={.outer={.nodes={nodes[0],nodes[1],nodes[2]},.node_count=3}};
    Entity area=made(rohr_physics_soft_body_area_add((SoftBodyAreaDescription){.owner=soft,.geometry=geometry}));
    Entity area2=made(rohr_physics_soft_body_area_add((SoftBodyAreaDescription){.owner=soft,.geometry=geometry}));
    OK(rohr_physics_soft_body_area_order_set(area,1));
    Entity particle=made(rohr_physics_particle_add((ParticleDescription){.radius=2,.rigid_vertices=8,.mass_value=1,.local_origin={9,10}}));
    OK(rohr_physics_collision_filter_description_set(plain,(CollisionFilterDescription){.category=9,.responds_to=17}));
    ScalarCase cases[]={
        SCALAR(velocity_x,plain,3),SCALAR(velocity_y,plain,-4),SCALAR(acceleration_x,plain,5),SCALAR(acceleration_y,plain,-6),
        SCALAR(position_x,plain,10),SCALAR(position_y,plain,20),SCALAR(orientation,plain,90),SCALAR(orientation_radians,plain,PI_F/2),
        SCALAR(angular_acceleration,plain,180),SCALAR(angular_acceleration_radians,plain,PI_F),
        SCALAR(friction,plain,.4f),SCALAR(restitution,plain,.6f),SCALAR(friction,beam,.7f),SCALAR(restitution,beam,.8f),
        SCALAR(center_of_mass_local_position_x,plain,2),SCALAR(center_of_mass_local_position_y,plain,3),
        SCALAR(center_of_mass_world_position_x,plain,13),SCALAR(center_of_mass_world_position_y,plain,18),
        SCALAR(particle_origin_x,particle,9),SCALAR(particle_origin_y,particle,10),
        SCALAR(soft_body_node_local_position_x,nodes[1],4),SCALAR(soft_body_node_local_position_y,nodes[1],0),
        SCALAR(axis_lock_axis_x,plain,.6f),SCALAR(axis_lock_axis_y,plain,.8f),
        SCALAR(axis_lock_point_on_axis_x,plain,5),SCALAR(axis_lock_point_on_axis_y,plain,6),
        SCALAR(transform_lock_local_offset_x,plain,7),SCALAR(transform_lock_local_offset_y,plain,8),
        SCALAR(transform_lock_local_angle,plain,45),SCALAR(transform_lock_local_angle_radians,plain,PI_F/4),
        SCALAR(angle_lock_min,plain,-90),SCALAR(angle_lock_max,plain,90),
        SCALAR(angle_lock_min_radians,plain,-PI_F/2),SCALAR(angle_lock_max_radians,plain,PI_F/2),
        SCALAR(joint_rest_length,joint,12),SCALAR(joint_stiffness,joint,5),SCALAR(joint_damping,joint,6),
        SCALAR(joint_rest_angle,joint,30),SCALAR(joint_rest_angle_radians,joint,PI_F/6),
        SCALAR(soft_body_beam_rest_length,beam,10),SCALAR(soft_body_beam_stiffness,beam,7),SCALAR(soft_body_beam_damping,beam,8),
        SCALAR(soft_body_beam_collision_thickness,beam,2),SCALAR(soft_body_node_radius,nodes[0],2)
    };
    struct { ScalarResult (*get)(JointAnchorId); JointAnchorId anchor; float expected; } anchors[]={
        SCALAR(joint_anchor_local_position_x,anchor,1),SCALAR(joint_anchor_local_position_y,anchor,2),
        SCALAR(joint_anchor_world_position_x,anchor,12),SCALAR(joint_anchor_world_position_y,anchor,19),
    };
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=true;
#endif
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        ScalarResult result=cases[i].get(cases[i].entity); OK(result);
        if(!near(result.result.value,cases[i].expected)) fprintf(stderr,"scalar case %zu: %g expected %g\n",i,result.result.value,cases[i].expected);
        assert(near(result.result.value,cases[i].expected));
    }
    for(size_t i=0;i<sizeof(anchors)/sizeof(anchors[0]);++i) {
        ScalarResult result=anchors[i].get(anchors[i].anchor); OK(result);
        assert(near(result.result.value,anchors[i].expected));
    }
    POINT(velocity,plain,3,-4); POINT(acceleration,plain,5,-6);
    POINT(axis_lock_axis,plain,.6f,.8f); POINT(axis_lock_point_on_axis,plain,5,6);
    POINT(transform_lock_local_offset,plain,7,8);
    VALUE(target,plain,driver); VALUE(transform_lock_driver,plain,driver);
    VALUE(collision_category,plain,9); VALUE(collision_response,plain,17);
    VALUE(joint_type,joint,JOINT_SPRING); VALUE(joint_anchor_a,joint,anchor); VALUE(joint_anchor_b,joint,added.result.value);
    VALUE(soft_body_node_radius,nodes[0],2); VALUE(soft_body_beam_node_a,beam,nodes[0]); VALUE(soft_body_beam_node_b,beam,nodes[1]);
    VALUE(particle_rigid_vertices,particle,8);
    VALUE(soft_body_node_count,soft,3); VALUE(soft_body_beam_count,soft,1); VALUE(soft_body_area_count,soft,2);
    VALUE(soft_body_area_order,area,1); VALUE(soft_body_area_order,area2,0);
    assert(physics_angle_lock_get(plain).result.value.min==-90 && rohr_physics_angle_lock_get(plain).result.value.max==90);
    assert(near(physics_angle_lock_radians_get(plain).result.value.min,-PI_F/2));
    assert(near(rohr_physics_angle_lock_radians_get(plain).result.value.max,PI_F/2));
    SoftBodyAreaGeometryResult shape=physics_soft_body_area_geometry_get(area); OK(shape);
    assert(shape.result.value.outer.node_count==3 && shape.result.value.outer.nodes[1]==nodes[1]);
    shape=rohr_physics_soft_body_area_geometry_get(area); OK(shape);
    assert(shape.result.value.outer.nodes[2]==nodes[2]);
    assert(physics_transform_lock_lock_position_check(plain) && rohr_physics_transform_lock_lock_position_check(plain));
    assert(physics_transform_lock_lock_orientation_check(plain) && rohr_physics_transform_lock_lock_orientation_check(plain));
    assert(physics_transform_lock_inherit_velocity_check(plain) && rohr_physics_transform_lock_inherit_velocity_check(plain));
    assert(physics_soft_body_beam_collision_check(beam) && rohr_physics_soft_body_beam_collision_check(beam));
    assert(physics_dynamic_check(plain) && rohr_physics_dynamic_check(plain));
    assert(!physics_static_check(plain) && !rohr_physics_static_check(plain));
    assert(physics_collision_check(plain) && rohr_physics_collision_check(plain));
    OK(rohr_physics_entity_hold(plain)); assert(physics_entity_held_check(plain) && rohr_physics_entity_held_check(plain));
    OK(rohr_physics_static_set(plain)); assert(physics_static_check(plain) && rohr_physics_static_check(plain));
    Acceleration gravity=rohr_physics_gravity_get();
    assert(physics_gravity_x_get()==gravity.x && rohr_physics_gravity_x_get()==gravity.x);
    assert(physics_gravity_y_get()==gravity.y && rohr_physics_gravity_y_get()==gravity.y);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden=false;
#endif
    /* A returned vector is an independent value, and subsequent edits are fresh. */
    VelocityResult captured=rohr_physics_velocity_get(plain); OK(captured);
    OK(rohr_physics_velocity_y_set(plain,42)); assert(captured.result.value.y==-4);
    assert(rohr_physics_velocity_y_get(plain).result.value==42);
    OK(rohr_physics_soft_body_beam_collision_disable(beam)); assert(!rohr_physics_soft_body_beam_collision_check(beam));
    OK(rohr_entity_delete(plain)); Entity replacement=made(rohr_entity_add());
    invalid_getters(plain); invalid_anchor_getters(anchor); assert(!error_check(rohr_entity_name_get(replacement)));
    OK(rohr_entity_delete(soft)); OK(rohr_entity_delete(particle)); OK(rohr_entity_delete(joint));
    OK(rohr_entity_delete(driver)); OK(rohr_entity_delete(replacement));
    rohr_engine_stop(); return 0;
}
