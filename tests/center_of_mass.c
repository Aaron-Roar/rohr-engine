/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "COM line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
static bool near(float a, float b) { return fabsf(a - b) < 0.002f; }
static bool point_near(Position a, Position b) { return near(a.x, b.x) && near(a.y, b.y); }
static const Shape rectangle = {.amount_of_vertices = 4,
    .vertices = {{2,3}, {6,3}, {6,5}, {2,5}}};

static bool properties_check(void) {
    EntityResult result = rohr_entity_add();
    OK(result);
    Entity e = result.result.value;
    OK(rohr_physics_center_of_mass_automatic_check(e));
    CHECK(rohr_physics_center_of_mass_automatic_check(e).result.value);
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_get(e)));
    CHECK(rohr_error_check(rohr_physics_moment_of_inertia_get(e)));
    CHECK(rohr_error_check(rohr_entity_components_add(e, ROHR_HIT_BOX | ROHR_DYNAMIC)));
    CHECK(!rohr_entity_components_check(e, ROHR_HIT_BOX));
    CHECK(!rohr_entity_components_check(e, ROHR_DYNAMIC));
    OK(rohr_entity_components_add(e, ROHR_COLLISION));
    OK(rohr_physics_center_of_mass_local_position_set(e, (Position){2,3}));
    CHECK(point_near(rohr_physics_center_of_mass_local_position_get(e).result.value,
        (Position){2,3}));
    CHECK(!rohr_entity_components_check(e, ROHR_DYNAMIC | ROHR_MASS));
    OK(rohr_physics_hitbox_set(e, rectangle));
    OK(rohr_entity_components_delete(e, ROHR_HIT_BOX));
    OK(rohr_entity_components_add(e, ROHR_HIT_BOX));
    OK(rohr_physics_mass_set(e, 12));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 80));
    OK(rohr_physics_center_of_mass_automatic_set(e));
    CHECK(point_near(rohr_physics_center_of_mass_local_position_get(e).result.value,
        (Position){4,4}));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 20));
    OK(rohr_physics_position_set(e, (Position){10,20}));
    OK(rohr_physics_orientation_set(e, 1.57079632679f));
    CHECK(point_near(rohr_physics_center_of_mass_world_position_get(e).result.value,
        (Position){6,24}));
    OK(rohr_physics_mass_set(e, 24));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 40));
    OK(rohr_physics_mass_set(e, 0));
    CHECK(rohr_physics_moment_of_inertia_get(e).result.value == 0);
    OK(rohr_physics_mass_set(e, 12));
    Shape reversed = rectangle;
    for(size_t i = 0; i < 4; i += 1) reversed.vertices[i] = rectangle.vertices[3-i];
    OK(rohr_physics_hitbox_set(e, reversed));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 20));
    Shape far = rectangle;
    for(size_t i = 0; i < 4; i += 1) {
        far.vertices[i].x += 100000;
        far.vertices[i].y -= 100000;
    }
    OK(rohr_physics_hitbox_set(e, far));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 20));
    OK(rohr_physics_hitbox_set(e, rectangle));
    Shape concave = {.amount_of_vertices = 6,
        .vertices = {{0,0},{3,0},{3,1},{1,1},{1,3},{0,3}}};
    OK(rohr_physics_hitbox_add(e, concave));
    CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, 20));
    OK(rohr_physics_hitbox_active_index_set(e, 1));
    CHECK(point_near(rohr_physics_center_of_mass_local_position_get(e).result.value,
        (Position){1.1f,1.1f}));
    OK(rohr_physics_center_of_mass_local_position_set(e, (Position){8,-2}));
    float inertia = rohr_physics_moment_of_inertia_get(e).result.value;
    CHECK(near(inertia, rohr_physics_polygon_moment_of_inertia(concave,12) +
        12 * (6.9f*6.9f + 3.1f*3.1f)));
    OK(rohr_physics_hitbox_active_index_set(e, 0));
    CHECK(point_near(rohr_physics_center_of_mass_local_position_get(e).result.value,
        (Position){8,-2}));
    inertia = rohr_physics_moment_of_inertia_get(e).result.value;
    const Shape invalid[] = {
        {.amount_of_vertices=3,.vertices={{0,0},{1,0},{2,0}}},
        {.amount_of_vertices=3,.vertices={{0,0},{0,0},{0,0}}},
        {.amount_of_vertices=4,.vertices={{0,0},{2,2},{0,2},{2,0}}},
        {.amount_of_vertices=3,.vertices={{0,0},{INFINITY,0},{0,2}}},
        {.amount_of_vertices=3,.vertices={{0,0},{NAN,0},{0,2}}}
    };
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);i+=1) {
        CHECK(rohr_error_check(rohr_physics_hitbox_set(e, invalid[i])));
        CHECK(rohr_entity_components_check(e, ROHR_HIT_BOX));
        CHECK(near(rohr_physics_moment_of_inertia_get(e).result.value, inertia));
        CHECK(point_near(rohr_physics_hitbox_get(e).result.value.vertices[0],
            rectangle.vertices[0]));
    }
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(e,
        (Position){NAN,0})));
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(e,
        (Position){ROHR_WORLD_COORDINATE_MAX+1,0})));
    CHECK(point_near(rohr_physics_center_of_mass_local_position_get(e).result.value,
        (Position){8,-2}));
    OK(rohr_physics_hitbox_remove(e));
    CHECK(!rohr_entity_components_check(e, ROHR_HIT_BOX));
    CHECK(rohr_error_check(rohr_entity_components_add(e, ROHR_HIT_BOX)));
    CHECK(rohr_error_check(rohr_physics_moment_of_inertia_get(e)));
    CHECK(!rohr_error_check(rohr_physics_center_of_mass_local_position_get(e)));
    OK(rohr_entity_delete(e));
    CHECK(rohr_error_check(rohr_physics_center_of_mass_automatic_check(e)));
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(e, (Position){0})));
    result = rohr_entity_add();
    OK(result);
    CHECK(rohr_physics_center_of_mass_automatic_check(result.result.value).result.value);
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_get(result.result.value)));
    CHECK(rohr_error_check(rohr_physics_center_of_mass_automatic_check(ENTITY_INVALID)));
    CHECK(rohr_error_check(rohr_physics_hitbox_set(result.result.value, invalid[0])));
    CHECK(!rohr_entity_components_check(result.result.value, ROHR_HIT_BOX));
    return true;
}

static bool motion_check(void) {
    EntityResult result=rohr_entity_add();
    OK(result);
    Entity e=result.result.value;
    OK(rohr_physics_position_set(e,(Position){0}));
    OK(rohr_physics_orientation_set(e,0));
    OK(rohr_entity_components_add(e, ROHR_COLLISION));
    OK(rohr_physics_dynamic_set(e));
    OK(rohr_physics_velocity_set(e,(Velocity){2,0}));
    OK(rohr_physics_angular_velocity_set(e,1));
    OK(rohr_system_physics_update(0.25));
    CHECK(point_near(rohr_physics_position_get(e).result.value,(Position){0.5f,0}));
    CHECK(!rohr_entity_components_check(e,ROHR_HIT_BOX));
    CHECK(!rohr_physics_overlap_check(e,e));
    OK(rohr_physics_center_of_mass_local_position_set(e,(Position){2,0}));
    OK(rohr_physics_orientation_set(e,0));
    OK(rohr_physics_velocity_set(e,(Velocity){0}));
    Position com=rohr_physics_center_of_mass_world_position_get(e).result.value;
    OK(rohr_system_physics_update(0.25));
    CHECK(point_near(rohr_physics_center_of_mass_world_position_get(e).result.value,com));
    CHECK(!near(rohr_physics_position_get(e).result.value.y,0));
    OK(rohr_physics_hitbox_set(e,rectangle));
    OK(rohr_physics_mass_set(e,12));
    OK(rohr_physics_center_of_mass_automatic_set(e));
    com=rohr_physics_center_of_mass_world_position_get(e).result.value;
    OK(rohr_system_physics_update(0.25));
    CHECK(point_near(rohr_physics_center_of_mass_world_position_get(e).result.value,com));
    Position origin=rohr_physics_position_get(e).result.value;
    OK(rohr_physics_orientation_set(e,0));
    CHECK(point_near(rohr_physics_position_get(e).result.value,origin));
    OK(rohr_physics_position_set(e,(Position){10,20}));
    EntityIndex i=rohr_entity_index_get(e).result.value;
    CHECK(angular_velocities[i]==1 && point_near(velocities[i],(Velocity){0}));
    OK(rohr_physics_angular_velocity_set(e,0));
    EntityResult torque=rohr_physics_torque_create(e,20);
    OK(torque);
    OK(rohr_system_physics_update(0.1));
    CHECK(near(angular_velocities[i],0.1f));
    OK(rohr_physics_hitbox_remove(e));
    OK(rohr_system_physics_update(0.1));
    CHECK(near(angular_velocities[i],0.1f));
    OK(rohr_physics_hitbox_set(e,rectangle));
    OK(rohr_physics_kinematic_driven_set(e));
    OK(rohr_system_physics_update(0.1));
    CHECK(near(angular_velocities[i],0.1f));
    OK(rohr_physics_entity_hold(e));
    origin=rohr_physics_position_get(e).result.value;
    float angle=orientations[i];
    OK(rohr_system_physics_update(0.1));
    CHECK(point_near(positions[i],origin) && orientations[i]==angle);
    OK(rohr_physics_entity_unhold(e));
    OK(rohr_physics_static_set(e));
    OK(rohr_system_physics_update(0.1));
    CHECK(point_near(positions[i],origin) && orientations[i]==angle);
    OK(rohr_physics_dynamic_set(e));
    OK(rohr_physics_kinematic_driven_remove(e));
    OK(rohr_physics_particle_radius_set(e,2));
    OK(rohr_physics_angular_velocity_set(e,3));
    OK(rohr_system_physics_update(0.1));
    CHECK(angular_velocities[i]==0 && orientations[i]==angle);
    OK(rohr_physics_mass_set(e,0));
    OK(rohr_system_physics_update(0.1));
    CHECK(isfinite(positions[i].x) && angular_velocities[i]==0);
    return true;
}

static bool locks_check(void) {
    EntityResult result=rohr_entity_add(), follower=rohr_entity_add();
    OK(result); OK(follower);
    Entity e=result.result.value, f=follower.result.value;
    OK(rohr_physics_position_set(e,(Position){0}));
    OK(rohr_physics_orientation_set(e,0));
    OK(rohr_physics_position_set(f,(Position){0}));
    OK(rohr_physics_orientation_set(f,0));
    EntityIndex i=rohr_entity_index_get(e).result.value;
    EntityIndex j=rohr_entity_index_get(f).result.value;
    OK(rohr_physics_center_of_mass_local_position_set(e,(Position){2,0}));
    OK(rohr_physics_angular_velocity_set(e,1));
    OK(rohr_physics_axis_lock_set(e,(Axis){1,0},(Position){0}));
    OK(rohr_system_physics_update(0.2));
    CHECK(near(positions[i].y,0));
    /* Origin velocity is COM velocity minus omega cross COM offset. */
    CHECK(near(velocities[i].y-angular_velocities[i]*2*cosf(orientations[i]),0));
    OK(rohr_physics_angle_lock_set(e,0,0));
    OK(rohr_system_physics_update(0.2));
    CHECK(near(orientations[i],0) && near(positions[i].y,0) && angular_velocities[i]==0);
    OK(rohr_entity_components_delete(e,ROHR_AXIS_LOCK | ROHR_ANGLE_LOCK));
    OK(rohr_physics_angular_velocity_set(e,1));
    OK(rohr_physics_center_of_mass_local_position_set(f,(Position){0,3}));
    OK(rohr_physics_dynamic_set(f));
    OK(rohr_physics_transform_lock_set(f,e,(Vec2D){4,0},0,true,true,true));
    OK(rohr_system_physics_update(0.1));
    Vec2D r=rohr_math_vector_rotate((Vec2D){4,0},orientations[i]);
    CHECK(point_near(positions[j],(Position){positions[i].x+r.x,positions[i].y+r.y}));
    Position ca=rohr_physics_center_of_mass_world_position_get(e).result.value;
    Position cb=rohr_physics_center_of_mass_world_position_get(f).result.value;
    CHECK(near(velocities[j].x,velocities[i].x-(cb.y-ca.y)));
    CHECK(near(velocities[j].y,velocities[i].y+(cb.x-ca.x)));
    return true;
}

typedef struct Snapshot { Position com; Velocity velocity; float angle, omega; } Snapshot;
/* Two descriptions of identical physical geometry must follow the same
 * trajectory even when their origins differ. Exercises actual solver stages. */
static bool scenario_get(int kind, Position shift, Snapshot *snapshot) {
    OK(rohr_engine_start());
    EntityResult moving=rohr_entity_add(), fixed=rohr_entity_add();
    OK(moving); OK(fixed);
    Entity a=moving.result.value,b=fixed.result.value;
    Shape shape=rohr_math_square_create(2,2);
    for(size_t i=0;i<shape.amount_of_vertices;i+=1) {
        shape.vertices[i].x+=shift.x; shape.vertices[i].y+=shift.y;
    }
    OK(rohr_physics_hitbox_set(a,shape));
    OK(rohr_physics_orientation_set(a,0));
    OK(rohr_physics_mass_set(a,2));
    OK(rohr_physics_center_of_mass_local_position_set(a,
        (Position){shift.x+0.6f,shift.y+0.4f}));
    OK(rohr_physics_position_set(a,(Position){5-shift.x,3-shift.y}));
    OK(rohr_physics_dynamic_set(a));
    OK(rohr_physics_hitbox_set(b,rohr_math_square_create(30,2)));
    OK(rohr_physics_static_set(b));
    OK(rohr_physics_velocity_set(a,(Velocity){0,-2}));
    OK(rohr_physics_restitution_set(a,0));
    OK(rohr_physics_restitution_set(b,0));
    if(kind==0) {
        OK(rohr_entity_components_add(a, ROHR_COLLISION)); OK(rohr_entity_components_add(b, ROHR_COLLISION));
    } else if(kind==4 || kind==5) {
        OK(rohr_physics_hitbox_remove(b));
        EntityResult soft=rohr_physics_soft_body_create();
        OK(soft);
        EntityResult na=rohr_physics_soft_body_node_create(soft.result.value,
            (Position){kind==4 ? 0 : 5,1},1,0.5f);
        OK(na);
        OK(rohr_physics_static_set(na.result.value));
        if(kind==4) {
            EntityResult nb=rohr_physics_soft_body_node_create(soft.result.value,
                (Position){10,1},1,0.5f);
            OK(nb);
            OK(rohr_physics_static_set(nb.result.value));
            EntityResult beam=rohr_physics_soft_body_beam_create(soft.result.value,
                na.result.value,nb.result.value,10,1);
            OK(beam);
            OK(rohr_physics_soft_body_beam_collision_enable(beam.result.value));
        }
    } else {
        JointAnchorIdResult aa=rohr_physics_joint_anchor_create(a,
            (Position){shift.x-1,shift.y});
        JointAnchorIdResult bb=rohr_physics_joint_anchor_create(b,(Position){4,4});
        EntityResult joint=rohr_entity_add();
        OK(aa); OK(bb); OK(joint);
        if(kind==1) OK(rohr_physics_joint_pin_set(joint.result.value,aa.result.value,bb.result.value));
        if(kind==2) OK(rohr_physics_joint_weld_set(joint.result.value,aa.result.value,bb.result.value));
        if(kind==3) OK(rohr_physics_joint_spring_set(joint.result.value,
            aa.result.value,bb.result.value,1,8,0.5f));
    }
    for(int step=0;step<100;step+=1) OK(rohr_system_physics_update(0.01));
    EntityIndex i=rohr_entity_index_get(a).result.value;
    snapshot->com=rohr_physics_center_of_mass_world_position_get(a).result.value;
    snapshot->velocity=velocities[i];
    snapshot->angle=orientations[i]; snapshot->omega=angular_velocities[i];
    CHECK(isfinite(snapshot->com.x) && isfinite(snapshot->com.y) &&
        isfinite(snapshot->omega));
    rohr_engine_stop();
    return true;
}

int main(void) {
    bool (*checks[])(void)={properties_check,motion_check,locks_check};
    for(size_t i=0;i<sizeof(checks)/sizeof(checks[0]);i+=1) {
        if(rohr_error_check(rohr_engine_start())) return 1;
        bool passed=checks[i]();
        rohr_engine_stop();
        if(!passed) return 1;
    }
    for(int kind=0;kind<6;kind+=1) {
        Snapshot a,b;
        if(!scenario_get(kind,(Position){0},&a) ||
                !scenario_get(kind,(Position){7,-3},&b)) return 1;
        if(!point_near(a.com,b.com) || !point_near(a.velocity,b.velocity) ||
                !near(a.angle,b.angle) || !near(a.omega,b.omega)) {
            fprintf(stderr,"COM solver %d origin invariance failed: %g %g, %g %g\n",
                kind,a.com.x,b.com.x,a.omega,b.omega);
            return 1;
        }
    }
    return 0;
}
