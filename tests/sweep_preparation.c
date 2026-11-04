/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr,"sweep reuse %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define OK(x) CHECK(!rohr_error_check(x))
static Entity particle(Position position,bool fixed) {
    ParticleDescription d=rohr_physics_particle_description_default_get();
    d.position=position;d.radius=.5f;d.rigid_vertices=3;
    d.gravity_enabled=false;d.static_body=fixed;
    EntityResult result=rohr_physics_particle_add(d);OK(result);return result.result.value;
}
static EntityIndex index_get(Entity entity) {
    EntityIndexResult result=rohr_entity_index_get(entity);OK(result);return result.result.value;
}
static double duration_get(double requested) {
    Report report=physics_report_create(ROHR_PHYSICS_INTEGRATION_REPORT_ID);
    double duration=physics_rigid_sweep_duration_get(requested,&report);
    CHECK(!report.has_fatal&&isfinite(duration)&&duration>0&&duration<=requested);
    return duration;
}
static double beam_duration_get(double requested) {
    Report report=physics_report_create(ROHR_PHYSICS_INTEGRATION_REPORT_ID);
    double duration=physics_soft_sweep_duration_get(requested,&report);
    CHECK(!report.has_fatal&&isfinite(duration)&&duration>0&&duration<=requested);
    return duration;
}
static void beam_reuse_check(void) {
    OK(rohr_engine_start());
    EntityResult owner=rohr_physics_soft_body_add((SoftBodyDescription){0});OK(owner);
    Entity nodes[2];EntityIndex indices[2];
    for(unsigned i=0;i<2;++i) {
        EntityResult node=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .owner=owner.result.value,.position={i?10:-10,0},.mass=1,.radius=.1f});
        OK(node);nodes[i]=node.result.value;indices[i]=index_get(nodes[i]);
        velocities[indices[i]].y=i?100:-100;
    }
    EntityResult beam=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner=owner.result.value,.node_a=nodes[0],.node_b=nodes[1],
        .collision={.enabled=true,.thickness=2,.filter={.category=1,.responds_to=2}}});
    OK(beam);
    Entity target=particle((Position){3,1.5f},false);EntityIndex t=index_get(target);
    OK(rohr_physics_collision_filter_description_set(target,
        (CollisionFilterDescription){.category=2,.responds_to=1}));
    double first=beam_duration_get(.02);CHECK(first<.02);
    CHECK(beam_duration_get(.02)==first);
    velocities[indices[0]].y=velocities[indices[1]].y=0;
    CHECK(beam_duration_get(.02)==.02);
    velocities[t].y=-10;CHECK(beam_duration_get(.02)<.02);
    CHECK(beam_duration_get(.00001)==.00001);
    positions[indices[0]].y=positions[indices[1]].y=100;
    CHECK(beam_duration_get(.02)==.02);
    positions[indices[0]].y=positions[indices[1]].y=0;
    velocities[t].y=0;velocities[indices[0]].y=-100;velocities[indices[1]].y=100;
    CHECK(beam_duration_get(.02)==first);
    /* Rigid sweep preparation must not consume the soft selector's inputs. */
    (void)duration_get(.01);CHECK(beam_duration_get(.02)==first);
    positions[t].y=1000;CHECK(beam_duration_get(.02)==.02);
    positions[t].y=1.5f;CHECK(beam_duration_get(.02)==first);
    rohr_engine_stop();
}
int main(void) {
    /* Repeat across engine teardown. Current poses, masks, radii and generations
     * must win over any retained spatial topology or previous invocation. */
    for(unsigned epoch=0;epoch<3;++epoch) {
        OK(rohr_engine_start());
        Entity mover=particle((Position){-1.05f,0},false);
        Entity wall=particle((Position){0,0},true);
        EntityIndex m=index_get(mover),w=index_get(wall);
        velocities[m]=(Velocity){10,0};
        double first=duration_get(.01);CHECK(first>.004&&first<.006);
        CHECK(duration_get(.01)==first);
        CHECK(duration_get(.001)==.001);
        velocities[m].x=-10;CHECK(duration_get(.01)==.01);
        velocities[m].x=0;CHECK(duration_get(.01)==.01);
        accelerations[m].x=1000;
        double accelerated=duration_get(.01);CHECK(accelerated>.006&&accelerated<.009);
        accelerations[m].x=0;velocities[m].x=20;
        double fast=duration_get(.01);CHECK(fast>.002&&fast<.003);
        positions[m].x=-4;CHECK(duration_get(.01)==.01);
        positions[m].x=-2.05f;velocities[m].x=10;
        CHECK(duration_get(.01)==.01);
        OK(rohr_physics_particle_radius_set(wall,1.5f));
        double resized=duration_get(.01);CHECK(resized>.004&&resized<.006);
        CollisionFilterDescription filter=rohr_physics_collision_filter_description_default_get();
        filter.responds_to=0;
        OK(rohr_physics_collision_filter_description_set(mover,filter));
        OK(rohr_physics_collision_filter_description_set(wall,filter));
        CHECK(duration_get(.01)==.01);
        filter=rohr_physics_collision_filter_description_default_get();
        OK(rohr_physics_collision_filter_description_set(mover,filter));
        OK(rohr_physics_collision_filter_description_set(wall,filter));
        CHECK(duration_get(.01)==resized);
        OK(rohr_entity_delete(wall));
        wall=particle((Position){100,0},true);
        CHECK(index_get(wall)==w);CHECK(duration_get(.01)==.01);
        positions[m].x=-1.05f;positions[w].x=0;
        CHECK(duration_get(.01)==first);
        Entity remote[48];
        for(unsigned i=0;i<48;++i)remote[i]=particle((Position){100+(float)i*4,100},true);
        CHECK(duration_get(.01)==first);
        for(unsigned pass=0;pass<4;++pass) {
            for(unsigned i=0;i<48;++i) {
                EntityIndex r=index_get(remote[i]);
                positions[r]=(Position){100+(float)((i*17+pass*7)%48)*20,100+(float)(i%7)*30};
            }
            CHECK(duration_get(.01)==first);
        }
        for(unsigned i=0;i<48;++i)OK(rohr_entity_delete(remote[i]));
        CHECK(duration_get(.01)==first);
        rohr_engine_stop();
    }
    beam_reuse_check();
    puts("PASS sweep reuse: rigid/beam motion, duration, geometry, filters, membership, generation reuse and engine restart");
    return 0;
}
