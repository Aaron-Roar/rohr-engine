/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_internal.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))
static void near(double a, double b) {
    if(fabs(a-b)>.002) fprintf(stderr,"Expected %.9g, got %.9g\n",b,a);
    assert(isfinite(a) && fabs(a-b)<.002);
}
static void point(Position a, Position b) { near(a.x,b.x); near(a.y,b.y); }
static Position position(Entity e) { PositionResult r=rohr_physics_position_get(e); OK(r); return r.result.value; }
static float angle(Entity e) { EntityIndexResult r=rohr_entity_index_get(e); OK(r); return orientations[r.result.value]; }
static Position rotated(Position p, float degrees) {
    double a=degrees*(3.14159265358979323846/180),c=cos(a),s=sin(a);
    return (Position){p.x*c+p.y*s,p.y*c-p.x*s};
}
static Entity body(Position p, float a) {
    EntityResult r=rohr_physics_soft_body_add((SoftBodyDescription){.position=p,.orientation=a}); OK(r); return r.result.value;
}
static Entity node(Entity b, Position p, float m) {
    EntityResult r=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.owner=b,.position=p,.mass=m,.radius=.1f}); OK(r);
    OK(rohr_physics_collision_filter_set(r.result.value,((CollisionFilterConfig){0}))); return r.result.value;
}
static void tracked(void) {
    Report r=rohr_physics_pipeline_forces_apply();
    assert(!r.has_fatal && !r.diagnostic_count);
}
static void move(Entity *nodes, size_t count, Position from, Position to, float degrees, float scale) {
    for(size_t i=0;i<count;++i) {
        Position p=position(nodes[i]); p=rotated((Position){(p.x-from.x)*scale,(p.y-from.y)*scale},degrees);
        OK(rohr_physics_position_set(nodes[i],((Position){p.x+to.x,p.y+to.y})));
    }
}
static void motion_check(void) {
    Entity b=body((Position){10,20},17);
    Entity nodes[]={node(b,(Position){-2,-1},1),node(b,(Position){2,-1},2),node(b,(Position){2,1},9),node(b,(Position){-2,1},4)};
    OK(rohr_physics_static_set(nodes[0])); /* Geometry, not mass/mobility weighting. */
    EntityResult f=rohr_physics_force_add((ForceDescription){.target=b,.magnitude=-3,
        .location={.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=b},.offset={4,5}},
        .orientation={.reference={.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=b},.offset=8}}); OK(f);
    Position center={0},expected={10,20};
    for(unsigned step=0;step<10;++step) {
        Position next={center.x+3,center.y-2};
        move(nodes,4,center,next,80,1);
        Position offset=rotated((Position){expected.x-center.x,expected.y-center.y},80);
        expected=(Position){next.x+offset.x,next.y+offset.y};
        Position before[4]; for(size_t n=0;n<4;++n) before[n]=position(nodes[n]);
        tracked(); point(position(b),expected); near(angle(b),17+80*(step+1));
        for(size_t n=0;n<4;++n) point(position(nodes[n]),before[n]);
        Position application; Vec2D direction; float magnitude;
        assert(physics_source_geometry_get(f.result.value,false,&application,&direction,&magnitude));
        offset=rotated((Position){4,5},angle(b));
        point(application,(Position){expected.x+offset.x,expected.y+offset.y});
        point(direction,rotated((Position){0,-1},angle(b)+8)); near(magnitude,-3);
        tracked(); point(position(b),expected); center=next;
    }
    /* Symmetric deformation changes size, not the origin's local offset. */
    move(nodes,4,center,center,0,1.7f); tracked(); point(position(b),expected); near(angle(b),817);
    OK(rohr_physics_mass_set(nodes[2],1000)); tracked(); point(position(b),expected);
    /* Explicit setters transform the nodes exactly once and rebase tracking. */
    OK(rohr_physics_position_set(b,((Position){-20,30})));
    OK(rohr_physics_orientation_set(b,-430));
    Position before[4]; for(size_t i=0;i<4;++i) before[i]=position(nodes[i]);
    tracked(); point(position(b),(Position){-20,30}); near(angle(b),-430);
    for(size_t i=0;i<4;++i) point(position(nodes[i]),before[i]);
    /* Failed sampling keeps the origin and the baseline recoverable. */
    EntityIndexResult index=rohr_entity_index_get(nodes[0]); OK(index);
    positions[index.result.value].x=NAN;
    Report failed=rohr_physics_pipeline_forces_apply(); assert(!failed.has_fatal && failed.diagnostic_count);
    point(position(b),(Position){-20,30});
    positions[index.result.value]=before[0]; tracked(); point(position(b),(Position){-20,30});
    OK(rohr_entity_delete(f.result.value)); OK(rohr_entity_delete(b));
}
static void topology_check(void) {
    Entity a=body((Position){3,4},31),b=body((Position){100,200},72);
    Entity nodes[]={node(a,(Position){-1,0},1),node(a,(Position){1,0},1)};
    tracked(); point(position(a),(Position){3,4});
    Entity added=node(a,(Position){100,100},1); tracked(); point(position(a),(Position){3,4});
    OK(rohr_entity_owner_set(added,b)); tracked(); point(position(a),(Position){3,4}); point(position(b),(Position){100,200});
    OK(rohr_physics_position_set(added,((Position){105,102}))); tracked(); point(position(b),(Position){105,202}); near(angle(b),72);
    /* An external beam connection does not add its endpoint to a body's frame. */
    EntityResult beam=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=a,.node_a=nodes[0],.node_b=added}); OK(beam);
    OK(rohr_physics_position_set(added,((Position){110,102}))); tracked(); point(position(a),(Position){3,4});
    OK(rohr_entity_delete(beam.result.value));
    OK(rohr_entity_delete(nodes[1])); tracked(); point(position(a),(Position){3,4});
    OK(rohr_physics_position_set(nodes[0],((Position){4,2}))); tracked(); point(position(a),(Position){8,6}); near(angle(a),31);
    OK(rohr_entity_delete(nodes[0])); tracked(); point(position(a),(Position){8,6});
    /* Collapse and reopening retain the last angle when no rotation is observable. */
    nodes[0]=node(a,(Position){0,0},1); nodes[1]=node(a,(Position){0,0},1);
    OK(rohr_physics_position_set(nodes[1],((Position){2,0}))); tracked(); near(angle(a),31); point(position(a),(Position){9,6});
    OK(rohr_physics_position_set(nodes[0],((Position){1,0}))); OK(rohr_physics_position_set(nodes[1],((Position){1,0})));
    tracked(); near(angle(a),31);
    OK(rohr_entity_delete(a)); OK(rohr_entity_delete(b));
    a=body((Position){-30,-40},-9); (void)node(a,(Position){0,0},1); tracked(); point(position(a),(Position){-30,-40}); near(angle(a),-9);
    OK(rohr_entity_delete(a));
}
static void pipeline_check(void) {
    Entity b=body((Position){0},0),n[]={node(b,(Position){-1,0},1),node(b,(Position){1,0},1)};
    for(size_t i=0;i<2;++i) OK(rohr_physics_velocity_set(n[i],((Velocity){4,2})));
    Report r=rohr_physics_pipeline_integrate(.5); assert(!r.has_fatal);
    point(position(b),(Position){2,1}); point(position(n[0]),(Position){1,1});
    /* A real constraint correction must be reflected before rendering. */
    EntityResult joint=rohr_entity_add(); OK(joint);
    JointAnchorIdResult anchor=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=n[0]}); OK(anchor);
    EntityResult world=rohr_entity_add(); OK(world);
    OK(rohr_physics_position_set(world.result.value,((Position){4,5})));
    JointAnchorIdResult fixed=rohr_physics_joint_anchor_add((JointAnchorDescription){.entity=world.result.value}); OK(fixed);
    OK(rohr_physics_joint_pin_set(joint.result.value,anchor.result.value,fixed.result.value));
    rohr_physics_pipeline_substep_begin(); r=rohr_physics_pipeline_joints_gather(); assert(!r.has_fatal);
    r=rohr_physics_pipeline_constraints_solve(8); assert(!r.has_fatal);
    Position a=position(n[0]),c=position(n[1]); assert(a.y>1);
    point(position(b),(Position){(a.x+c.x)/2,(a.y+c.y)/2});
    OK(rohr_entity_delete(joint.result.value)); OK(rohr_entity_delete(b));
    OK(rohr_entity_delete(world.result.value));
}
static void persistence_check(void) {
    Entity b=body((Position){5,7},23),n[]={node(b,(Position){-1,0},1),node(b,(Position){1,0},1)};
    OK(rohr_entity_name_set(b,"tracking_body"));
    move(n,2,(Position){0},(Position){4,5},70,1); tracked();
    Position saved=position(b); float saved_angle=angle(b);
    OK(rohr_game_state_file_save("soft_body_frame_state.json"));
    move(n,2,(Position){4,5},(Position){8,7},30,1); tracked();
    Position expected=position(b); float expected_angle=angle(b);
    OK(rohr_entity_delete(b));
    OK(rohr_game_state_file_load("soft_body_frame_state.json"));
    EntityResult loaded=rohr_entity_by_name_get("tracking_body"); OK(loaded); b=loaded.result.value;
    SoftBodyResult soft=rohr_physics_soft_body_get(b); OK(soft);
    point(position(b),saved); near(angle(b),saved_angle); tracked(); point(position(b),saved);
    move(soft.result.value.nodes,2,(Position){4,5},(Position){8,7},30,1); tracked();
    point(position(b),expected); near(angle(b),expected_angle);
    OK(rohr_entity_delete(b));
}
static void source_pipeline_check(void) {
    for(unsigned standalone=0;standalone<2;++standalone) {
        Entity b=body((Position){0},0),n[]={node(b,(Position){-1,0},1),node(b,(Position){1,0},1)};
        EntityResult force=rohr_physics_force_add((ForceDescription){.owner=b,.target=b,.magnitude=2,
            .location.reference={.kind=ROHR_FORCE_ORIGIN_POSITION,.entity=b},
            .orientation.reference={.kind=ROHR_FORCE_BODY_ORIENTATION,.entity=b}}); OK(force);
        /* Evaluation must see node edits even without an earlier loads stage. */
        move(n,2,(Position){0},(Position){5,4},90,1);
        if(standalone) rohr_physics_pipeline_accelerations_clear();
        Report r=standalone ? rohr_physics_pipeline_integrate(.1) : rohr_physics_pipeline_update(.1);
        assert(!r.has_fatal && !r.diagnostic_count);
        near(angle(b),90);
        for(size_t i=0;i<2;++i) {
            EntityIndexResult index=rohr_entity_index_get(n[i]); OK(index);
            point(velocities[index.result.value],(Position){.1,0});
        }
        Position a=position(n[0]),c=position(n[1]);
        point(position(b),(Position){(a.x+c.x)/2,(a.y+c.y)/2});
        OK(rohr_entity_delete(b));
    }
}
int main(void) {
    OK(rohr_engine_start());
    motion_check(); topology_check(); pipeline_check(); persistence_check(); source_pipeline_check();
    rohr_engine_stop();
    puts("Soft-body frame motion, topology, pipeline, force geometry and persistence passed");
    return 0;
}
