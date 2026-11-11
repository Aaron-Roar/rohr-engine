/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#define main authored_car_example_main
#include "../examples/soft-body/src/main.c"
#undef main
#include <assert.h>
#include "../src/entity/entity_internal.h"
#include <math.h>
#include <stdio.h>
#define OK(c) assert(!rohr_error_check(c))
static Entity take(EntityResult r) { OK(r);return r.result.value; }
static Entity member(Entity root,const char *name) { return take(entity_object_member_by_name_get(root,name)); }
static void collision_disable(Entity root) {
    OK(rohr_entity_components_delete(root,ROHR_COLLISION));
    GroupIdResult id=entity_object_members_get(root);
    if(error_check(id)) return;
    EntityGroupResult group=entity_group_get(id.result.value);OK(group);
    for(size_t i=0;i<group.result.value.entities.capacity;++i)
        if(group.result.value.entities.used[i]) collision_disable(group.result.value.entities.objects[i]);
}
static Entity box(Position p,Vec2D dimensions,bool fixed,Entity owner) {
    Entity e=take(rohr_entity_add());
    OK(rohr_physics_position_set(e,p));OK(rohr_physics_hitbox_set(e,rohr_math_square_create(dimensions.x,dimensions.y)));
    OK(rohr_physics_orientation_set(e,0));OK(rohr_physics_velocity_set(e,(Velocity){0}));
    if(fixed) OK(rohr_physics_static_set(e));else {OK(rohr_physics_mass_set(e,1));OK(rohr_physics_dynamic_set(e));}
    OK(rohr_physics_collision_filter_description_set(e,(CollisionFilterDescription){ROHR_COLLISION_CATEGORY_ALL,ROHR_COLLISION_CATEGORY_ALL}));
    OK(rohr_entity_components_add(e,ROHR_COLLISION));
    OK(rohr_entity_owner_set(e,owner));return e;
}
static void contact_case(unsigned kind,bool two,bool scripted) {
    OK(rohr_engine_start());
    Car car={0};CarControls controls={.scripted=scripted};car.source_instance=take(rohr_entity_add());
    car.chassis=box((Position){0,10},(Vec2D){2,1},false,car.source_instance);
    car.rear_disk=box((Position){-5,1.4f},(Vec2D){2,2},false,car.source_instance);
    car.front_disk=box((Position){5,1.4f},(Vec2D){2,2},false,car.source_instance);
    car.rear_tire=take(rohr_physics_soft_body_add((SoftBodyDescription){.owner=car.source_instance}));
    car.front_tire=take(rohr_physics_soft_body_add((SoftBodyDescription){.owner=car.source_instance}));
    car.torque_rear_drive=take(rohr_physics_torque_add((TorqueDescription){.owner=car.source_instance,.target=car.rear_disk}));
    for(unsigned i=0;i<(two?2u:1u);++i) {
        Position p={i?5:-5,0};
        if(kind==1) {
            ParticleDescription particle=rohr_physics_particle_description_default_get();
            particle.position=p;particle.radius=0.6f;particle.static_body=true;take(rohr_physics_particle_add(particle));
        } else if(kind==3) {
            Entity soft=take(rohr_physics_soft_body_add((SoftBodyDescription){0}));
            Entity node=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
                .restitution = 0.25f, .collision_enabled = true,
                .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
                .owner=soft,.position=p,.radius=0.6f,.mass=1}));
            OK(rohr_physics_static_set(node));
        } else box(p,(Vec2D){3,1},true,kind==2?car.source_instance:ENTITY_NONE);
    }
    assert(rohr_physics_dt_update(0.001).header.status!=ROHR_REPORT_FAILED);
    assert(rohr_physics_contact_count_get(car.rear_disk)>0);
    if(two) assert(rohr_physics_contact_count_get(car.front_disk)>0);
    bool eligible=kind==0 && two;assert(car_grounded_check(&car)==eligible);
    size_t before=entity_alive_count_get();
    car_jump_request(&controls,&car,true);OK(car_controls_update(&controls,&car,0));
    assert(entity_alive_count_get()==before+(eligible?1u:0u));
    car_jump_request(&controls,&car,false);OK(car_controls_update(&controls,&car,0));
    assert(entity_alive_count_get()==before+(eligible?1u:0u));
    if(eligible) {
        assert(rohr_physics_dt_update(0.00001).header.status!=ROHR_REPORT_FAILED);
        EntityIndexResult index=rohr_entity_index_get(car.chassis);OK(index);
        Velocity velocity=velocities[index.result.value];
        assert(velocity.y>0 && fabsf(velocity.x)<0.001f);
        if(scripted) {
            Entity force=controls.scripted_jump_force;
            assert(rohr_entity_alive_check(force));
            OK(car_controls_step_end(&controls));
            assert(!controls.scripted_jump_force && !rohr_entity_alive_check(force));
            assert(entity_alive_count_get()==before);
            assert(rohr_physics_dt_update(0.00001).header.status!=ROHR_REPORT_FAILED);
            assert(velocities[index.result.value].y==velocity.y);
        }
    } else {
        /* An airborne/rejected press never becomes a later jump. */
        OK(rohr_physics_position_set(car.rear_disk,(Position){50,50}));
        car_jump_request(&controls,&car,false);assert(!controls.jump_pending);
    }
    OK(rohr_entity_delete(car.source_instance));rohr_engine_stop();
}
static void catalog_parity(void) {
    OK(rohr_engine_start());
    Car generated={0};    generated.source_instance = take(rohr_object_create(project.car));
    const char *car_names[] = {"chassis", "cabin", "rear_hub", "rear_disk", "front_hub",
        "front_disk", "rear_tire", "front_tire", "rear_drive"};
    Entity *car_ids[] = {&generated.chassis, &generated.cabin, &generated.rear_hub, &generated.rear_disk, &generated.front_hub,
        &generated.front_disk, &generated.rear_tire, &generated.front_tire, &generated.torque_rear_drive};
    for(size_t i = 0; i < sizeof(car_ids) / sizeof(car_ids[0]); ++i) {
        EntityResult named = rohr_entity_by_name_get(car_names[i]);
        if(rohr_error_check(named)) { fprintf(stderr, "%s\n", rohr_error_message_get(named)); assert(false); }
        *car_ids[i] = named.result.value;
    }

    /* The bundled fixture retains its old allowed/excluded pairs explicitly.
     * Hubs formerly shared the chassis category but accepted only particles. */
    Entity floor=wall_create((Position){0,-1000},(Vec2D){100,1});
    assert(floor != ENTITY_INVALID);
    ParticleDescription particle_config=rohr_physics_particle_description_default_get();
    particle_config.collision_filter=(CollisionFilterDescription){32,UINT64_MAX};
    Entity particle=take(rohr_physics_particle_add(particle_config));
    Entity profiles[]={generated.chassis,generated.rear_hub,generated.rear_disk,
        rohr_entity_by_name_get("rear_node_1").result.value,floor,particle};
    const CollisionFilterDescription old[]={
        {16,50},{16,32},{8,38},{4,42},{2,UINT64_MAX},{32,UINT64_MAX}};
    for(size_t i=0;i<6;++i) for(size_t j=0;j<6;++j) {
        bool accepted=(old[i].responds_to & old[j].category) &&
            (old[j].responds_to & old[i].category);
        bool actual=rohr_physics_collision_response_to_check(profiles[i],profiles[j]);
        if(actual!=accepted) fprintf(stderr,"car response profile %zu -> %zu: expected %d, got %d\n",
            i,j,accepted,actual);
        assert(actual==accepted);
    }
    OK(rohr_entity_delete(floor)); OK(rohr_entity_delete(particle));
    ObjectDescriptionResult loaded=rohr_object_load(CAR_SOURCE_DIRECTORY "/objects/project.rohr.json");OK(loaded);
    Entity runtime=take(rohr_entity_object_by_name_add(loaded.result.value,"Car"));
    Entity other=take(rohr_entity_object_add_with_options(objects.car,ROHR_ENTITY_OBJECT_ALL));
    Entity rt_torque=member(runtime,"rear_drive"),other_torque=member(other,"rear_drive");
    TorqueDescriptionResult a=rohr_physics_torque_description_get(generated.torque_rear_drive);OK(a);
    TorqueDescriptionResult b=rohr_physics_torque_description_get(rt_torque);OK(b);
    assert(a.result.value.target==generated.rear_disk && b.result.value.target==member(runtime,"rear_disk"));
    assert(a.result.value.owner==generated.source_instance && b.result.value.owner==runtime);
    OK(rohr_physics_torque_magnitude_set(generated.torque_rear_drive,42));
    assert(rohr_physics_torque_magnitude_get(other_torque).result.value==0);
    SoftBodyResult tire=rohr_physics_soft_body_get(generated.rear_tire);OK(tire);
    SoftBodyResult loaded_tire=rohr_physics_soft_body_get(member(runtime,"rear_tire"));OK(loaded_tire);
    assert(tire.result.value.node_count==50 && tire.result.value.beam_count==170);
    for(unsigned n=0;n<50;++n) {
        PositionResult p=rohr_physics_position_get(tire.result.value.nodes[n]);OK(p);
        PositionResult q=rohr_physics_position_get(loaded_tire.result.value.nodes[n]);OK(q);
        assert(fabsf(p.result.value.x-q.result.value.x)<0.001f && fabsf(p.result.value.y-q.result.value.y)<0.001f);
    }
    /* Compare actual motion against the direct API on the loaded topology. */
    collision_disable(generated.source_instance);collision_disable(runtime);collision_disable(other);
    OK(rohr_entity_delete(rt_torque));
    b.result.value.magnitude=1000;
    Entity direct=take(rohr_physics_torque_add(b.result.value));
    OK(rohr_physics_torque_magnitude_set(generated.torque_rear_drive,1000));
    assert(rohr_physics_dt_update(1.0/240).header.status!=ROHR_REPORT_FAILED);
    AngularVelocityResult generated_rate=rohr_physics_angular_velocity_get(generated.rear_disk);OK(generated_rate);
    AngularVelocityResult direct_rate=rohr_physics_angular_velocity_get(b.result.value.target);OK(direct_rate);
    assert(generated_rate.result.value>0 && fabsf(generated_rate.result.value-direct_rate.result.value)<0.001f);
    OK(rohr_entity_delete(direct));
    Entity initial=member(generated.source_instance,"initial_load");
    Entity transferred=generated.torque_rear_drive;
    OK(rohr_entity_owner_set(transferred,other));
    OK(rohr_entity_delete(generated.source_instance));assert(!rohr_entity_alive_check(initial));
    assert(rohr_entity_alive_check(other_torque) && rohr_entity_alive_check(transferred));
    OK(rohr_entity_delete(runtime));OK(rohr_entity_delete(other));
    OK(rohr_object_destroy(loaded.result.value));assert(entity_alive_count_get()==0);
    rohr_engine_stop();
}
static void starting_platform(void) {
    OK(rohr_engine_start());OK(rohr_physics_substeps_set(4));
    Car car={0};CarControls controls={0};    car.source_instance = take(rohr_object_create(project.car));
    const char *car_names[] = {"chassis", "cabin", "rear_hub", "rear_disk", "front_hub",
        "front_disk", "rear_tire", "front_tire", "rear_drive"};
    Entity *car_ids[] = {&car.chassis, &car.cabin, &car.rear_hub, &car.rear_disk, &car.front_hub,
        &car.front_disk, &car.rear_tire, &car.front_tire, &car.torque_rear_drive};
    for(size_t i = 0; i < sizeof(car_ids) / sizeof(car_ids[0]); ++i) {
        EntityResult named = rohr_entity_by_name_get(car_names[i]);
        if(rohr_error_check(named)) { fprintf(stderr, "%s\n", rohr_error_message_get(named)); assert(false); }
        *car_ids[i] = named.result.value;
    }

    Entity platform=wall_create(level_wall_positions[0],level_wall_dimensions[0]);
    assert(platform && rohr_entity_components_check(platform,ROHR_COLLISION));
    bool grounded=false;
    for(unsigned step=0;step<480;++step) {
        assert(rohr_physics_dt_update(physics_tick_time).header.status!=ROHR_REPORT_FAILED);
        if(car_grounded_check(&car)) {grounded=true;break;}
    }
    assert(grounded);
    car_jump_request(&controls,&car,true);assert(controls.jump_pending);
    OK(car_controls_update(&controls,&car,0));
    assert(rohr_physics_dt_update(physics_tick_time).header.status!=ROHR_REPORT_FAILED);
    EntityIndexResult index=rohr_entity_index_get(car.chassis);OK(index);
    assert(velocities[index.result.value].y>0);
    OK(rohr_entity_delete(car.source_instance));OK(rohr_entity_delete(platform));rohr_engine_stop();
}
int main(void) {
    starting_platform();catalog_parity();contact_case(0,true,false);contact_case(0,false,false);
    contact_case(1,true,false);contact_case(2,true,false);contact_case(3,true,false);
    contact_case(0,true,true);return 0;
}
