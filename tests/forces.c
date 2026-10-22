/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "forces line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
#define ERROR(c, code) do { CHECK(rohr_error_check(c)); CHECK((c).result.error == (code)); } while(0)

typedef struct ForceApi {
    EntityResult (*force_add)(ForceDescription);
    EngineResult (*force_set)(Entity, float);
    ForceMagnitudeResult (*force_get)(Entity);
    EngineResult (*force_apply)(ForceDescription);
    EntityResult (*torque_add)(TorqueDescription);
    EngineResult (*torque_set)(Entity, Torque);
    TorqueResult (*torque_get)(Entity);
    EngineResult (*torque_apply)(TorqueDescription);
    EngineResult (*impulse_apply)(Entity, Vec2D);
    EngineResult (*angular_impulse_apply)(Entity, float);
} ForceApi;

static ForceDescription vector_description(Entity target, Force vector) {
    return (ForceDescription){.target = target,
        .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = target},
        .orientation.offset = vector.x == 0 && vector.y == 0 ? 0 :
            math_radians_to_degrees(atan2f(vector.x, vector.y)),
        .magnitude = hypotf(vector.x, vector.y)};
}

static const Shape rectangle = {.amount_of_vertices = 4,
    .vertices = {{2,3}, {6,3}, {6,5}, {2,5}}};
static bool near(float a, float b) { return fabsf(a - b) < 0.0001f; }

static bool body_create(Entity *body, EntityIndex *index) {
    EntityResult result = rohr_entity_add();
    OK(result);
    *body = result.result.value;
    *index = rohr_entity_index_get(*body).result.value;
    OK(rohr_physics_position_set(*body, (Position){0}));
    OK(rohr_physics_orientation_set(*body, 0));
    OK(rohr_physics_hitbox_set(*body, rectangle));
    OK(rohr_physics_mass_set(*body, 12));
    OK(rohr_physics_dynamic_set(*body));
    return true;
}

static bool sources_check(const ForceApi *api, unsigned substeps) {
    Entity body;
    EntityIndex index;
    CHECK(body_create(&body, &index));
    OK(rohr_physics_substeps_set(substeps));
    OK(rohr_engine_time_per_tick_set(0.001));
    rohr_engine_clock_reset();
    ERROR(api->force_get(body), ERROR_ENGINE_COMPONENT_MISSING);
    ERROR(api->torque_get(body), ERROR_ENGINE_COMPONENT_MISSING);
    EntityResult force = api->force_add(vector_description(body, (Force){12,0}));
    EntityResult torque = api->torque_add((TorqueDescription){.target = body, .magnitude = 20});
    OK(force); OK(torque);
    Entity f = force.result.value, t = torque.result.value;
    CHECK(f != body && t != body && f != t);
    CHECK(api->force_get(f).result.value == 12);
    CHECK(api->torque_get(t).result.value == 20);
    /* Replace, rather than accumulate; preserve source targets. */
    OK(api->force_set(f, 24));
    OK(api->torque_set(t, 40));
    CHECK(api->force_get(f).result.value == 24);
    CHECK(api->torque_get(t).result.value == 40);
    OK(api->force_apply(vector_description(body, (Force){12,0})));
    OK(api->force_apply(vector_description(body, (Force){0,24})));
    OK(api->torque_apply((TorqueDescription){.target = body, .magnitude = 20}));
    OK(api->torque_apply((TorqueDescription){.target = body, .magnitude = -10}));
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.3f) && near(velocities[index].y, 0.2f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.25f));
    /* Direct physics updates do not advance engine tick lifetimes. */
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.6f) && near(velocities[index].y, 0.4f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.5f));
    SDL_Delay(2);
    CHECK(rohr_engine_update().header.status == ROHR_REPORT_COMPLETE);
    CHECK(rohr_engine_tick_delta_get() > 0);
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.8f) && near(velocities[index].y, 0.4f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.7f));
    CHECK(rohr_entity_alive_check(f) && rohr_entity_alive_check(t));
    OK(api->force_set(f, 0));
    OK(api->torque_set(t, 0));
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.8f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.7f));
    CHECK(rohr_entity_alive_check(f) && rohr_entity_alive_check(t));
    OK(api->force_set(f, 12));
    OK(api->torque_set(t, 20));
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.9f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.8f));
    OK(rohr_entity_delete(f)); OK(rohr_entity_delete(t));
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(rohr_entity_alive_check(body));
    CHECK(near(velocities[index].x, 0.9f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.8f));

    /* Stale handles remain invalid even after an entity slot is reused. */
    EntityResult plain = rohr_entity_add();
    OK(plain);
    Entity p = plain.result.value;
    ERROR(api->force_get(f), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_get(t), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->force_set(f, 0), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_set(t, 0), ERROR_ENGINE_INVALID_ENTITY);
    OK(physics_force_description_set(p, (ForceDescription){.magnitude = 5}));
    OK(physics_torque_description_set(p, (TorqueDescription){.magnitude = -5}));
    CHECK(api->force_get(p).result.value == 5 && api->torque_get(p).result.value == -5);
    OK(rohr_entity_components_delete(p, ROHR_FORCE | ROHR_TORQUE));
    ERROR(api->force_get(p), ERROR_ENGINE_COMPONENT_MISSING);
    ERROR(api->torque_get(p), ERROR_ENGINE_COMPONENT_MISSING);
    OK(api->force_add((ForceDescription){0}));
    OK(api->torque_add((TorqueDescription){0}));
    OK(api->force_apply((ForceDescription){0}));
    OK(api->torque_apply((TorqueDescription){0}));
    ERROR(api->force_get(ENTITY_INVALID), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_get(ENTITY_INVALID), ERROR_ENGINE_INVALID_ENTITY);
    return true;
}

static bool impulses_check(const ForceApi *api) {
    Entity body;
    EntityIndex index;
    CHECK(body_create(&body, &index));
    OK(api->impulse_apply(body, (Vec2D){24,-12}));
    OK(api->impulse_apply(body, (Vec2D){-12,0}));
    CHECK(near(velocities[index].x, 1) && near(velocities[index].y, -1));
    /* Inertia is 20 at centroid, 80 at the explicit corner COM. */
    OK(api->angular_impulse_apply(body, 20));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 1));
    OK(api->angular_impulse_apply(body, -10));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.5f));
    OK(rohr_physics_center_of_mass_local_position_set(body, (Position){2,3}));
    OK(api->angular_impulse_apply(body, 80));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 1.5f));
    CHECK(near(velocities[index].x, 1) && near(velocities[index].y, -1));
    CHECK(orientations[index] == 0);
    ERROR(api->angular_impulse_apply(body, NAN), ERROR_ENGINE_STATE_INVALID);
    ERROR(api->angular_impulse_apply(body, INFINITY), ERROR_ENGINE_STATE_INVALID);
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 1.5f));

    OK(rohr_physics_angular_velocity_set(body, FLT_MAX));
    ERROR(api->angular_impulse_apply(body, FLT_MAX), ERROR_ENGINE_STATE_INVALID);
    CHECK(angular_velocities[index] == FLT_MAX);
    OK(rohr_physics_angular_velocity_set(body, 0));
    OK(rohr_physics_mass_set(body, 0.001f));
    ERROR(api->angular_impulse_apply(body, FLT_MAX), ERROR_ENGINE_STATE_INVALID);
    CHECK(angular_velocities[index] == 0);
    OK(rohr_physics_mass_set(body, 12));

    /* No-op cases must preserve angular velocity and the body's motion type. */
    for(int mode = 0; mode < 6; mode += 1) {
        Entity e;
        EntityIndex i;
        CHECK(body_create(&e, &i));
        if(mode == 0) OK(rohr_physics_static_set(e));
        if(mode == 1) OK(rohr_physics_entity_hold(e));
        if(mode == 2) OK(rohr_physics_kinematic_driven_set(e));
        if(mode == 3) OK(rohr_physics_mass_set(e, 0));
        if(mode == 4) OK(rohr_physics_hitbox_remove(e));
        if(mode == 5) OK(rohr_physics_angle_lock_set(e, 0, 0));
        bool dynamic = rohr_entity_components_check(e, ROHR_DYNAMIC);
        bool kinematic = rohr_physics_kinematic_driven_check(e);
        float omega = angular_velocities[i];
        OK(api->angular_impulse_apply(e, 20));
        CHECK(angular_velocities[i] == omega);
        CHECK(rohr_entity_components_check(e, ROHR_DYNAMIC) == dynamic);
        CHECK(rohr_physics_kinematic_driven_check(e) == kinematic);
        ERROR(api->angular_impulse_apply(e, NAN), ERROR_ENGINE_STATE_INVALID);
    }
    OK(rohr_entity_delete(body));
    ERROR(api->angular_impulse_apply(body, 1), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->angular_impulse_apply(ENTITY_INVALID, 1), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->impulse_apply(ENTITY_INVALID, (Vec2D){0}), ERROR_ENGINE_INVALID_ENTITY);
    return true;
}

static bool persistence_check(const ForceApi *api) {
    const char *path = "forces_state.json";
    Entity body;
    EntityIndex index;
    CHECK(body_create(&body, &index));
    OK(rohr_entity_name_set(body, "force_target"));
    EntityResult force = api->force_add(vector_description(body, (Force){24,0}));
    EntityResult torque = api->torque_add((TorqueDescription){.target = body, .magnitude = 40});
    OK(force); OK(torque);
    OK(rohr_entity_name_set(force.result.value, "force_source"));
    OK(rohr_entity_name_set(torque.result.value, "torque_source"));
    OK(rohr_game_state_file_save(path));
    rohr_engine_stop();
    OK(rohr_engine_start());
    OK(rohr_game_state_file_load(path));
    EntityResult restored = rohr_entity_by_name_get("force_target");
    force = rohr_entity_by_name_get("force_source");
    torque = rohr_entity_by_name_get("torque_source");
    OK(restored); OK(force); OK(torque);
    ForceMagnitudeResult f = api->force_get(force.result.value);
    TorqueResult t = api->torque_get(torque.result.value);
    OK(f); OK(t);
    CHECK(f.result.value == 24 && t.result.value == 40);
    index = rohr_entity_index_get(restored.result.value).result.value;
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    CHECK(near(velocities[index].x, 0.2f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.2f));
    CHECK(remove(path) == 0);
    return true;
}

static bool beam_sources_check(const ForceApi *api, unsigned substeps) {
    OK(rohr_physics_substeps_set(substeps));
    for(unsigned variant=0;variant<3;++variant) {
        EntityResult owner=rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
        EntityResult a=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .restitution = 0.25f, .collision_enabled = true,
            .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
            .owner = owner.result.value,
            .position = (Position){0,0},
            .mass = 2,
            .radius = 0.1f
        });
        EntityResult b=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .restitution = 0.25f, .collision_enabled = true,
            .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
            .owner = owner.result.value,
            .position = (Position){10,0},
            .mass = 4,
            .radius = 0.1f
        });
        OK(a); OK(b);
        OK(rohr_entity_components_delete(a.result.value,ROHR_GRAVITY|ROHR_COLLISION));
        OK(rohr_entity_components_delete(b.result.value,ROHR_GRAVITY|ROHR_COLLISION));
        EntityResult beam=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
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
        OK(beam); OK(rohr_physics_soft_body_beam_collision_disable(beam.result.value));
        EntityIndex ia=rohr_entity_index_get(a.result.value).result.value;
        EntityIndex ib=rohr_entity_index_get(b.result.value).result.value;
        ForceDescription d={.target=beam.result.value,.magnitude=12,.orientation.offset=90,
            .location={.reference={.kind=ROHR_FORCE_BEAM_POSITION,
                .beam={beam.result.value,ROHR_FORCE_BEAM_CENTER,0}}}};
        if(variant==1) {
            d.location.reference.beam.anchor=ROHR_FORCE_BEAM_START;
            d.location.offset.x=5;
        } else if(variant==2) d.location=(ForceLocation){.offset={5,0}};
        /* Compare a single step at the coincident point, before stretching can
         * legitimately move the different references apart. */
        OK(rohr_physics_substeps_set(1));
        EntityResult force=api->force_add(d); OK(force);
        CHECK(rohr_system_physics_update(0.01).header.status!=ROHR_REPORT_FAILED);
        CHECK(near(velocities[ia].x,0.03f) && near(velocities[ib].x,0.015f));
        CHECK(near(positions[ia].x,0.0003f) && near(positions[ib].x,10.00015f));
        OK(rohr_entity_delete(force.result.value));
        OK(rohr_physics_position_set(a.result.value,(Position){0,0}));
        OK(rohr_physics_position_set(b.result.value,(Position){10,0}));
        OK(rohr_physics_velocity_set(a.result.value,(Velocity){0}));
        OK(rohr_physics_velocity_set(b.result.value,(Velocity){0}));
        OK(api->force_apply(d));
        CHECK(rohr_system_physics_update(0.01).header.status!=ROHR_REPORT_FAILED);
        CHECK(near(velocities[ia].x,0.03f) && near(velocities[ib].x,0.015f));
        /* Deleting the owner removes endpoints/beam; the one-tick source remains
         * until cleanup and can only warn, never retarget a reused entity slot. */
        OK(rohr_entity_delete(owner.result.value));
        Report rejected=rohr_system_physics_update(0.01);
        CHECK(!rejected.has_fatal && rejected.diagnostic_count>0);
    }
    /* Pure torque via both API layers; multiple substeps preserve net momentum. */
    OK(rohr_physics_substeps_set(substeps));
    EntityResult owner=rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    EntityResult a=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){0,0},
        .mass = 2,
        .radius = 0.1f
    });
    EntityResult b=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){10,0},
        .mass = 4,
        .radius = 0.1f
    });
    OK(a); OK(b);
    OK(rohr_entity_components_delete(a.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    OK(rohr_entity_components_delete(b.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    EntityResult beam=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
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
    OK(beam); OK(rohr_physics_soft_body_beam_collision_disable(beam.result.value));
    EntityResult torque=api->torque_add((TorqueDescription){.target = beam.result.value, .magnitude = 20}); OK(torque);
    OK(api->torque_apply((TorqueDescription){.target = beam.result.value, .magnitude = 20}));
    CHECK(rohr_system_physics_update(0.01).header.status!=ROHR_REPORT_FAILED);
    EntityIndex ia=rohr_entity_index_get(a.result.value).result.value;
    EntityIndex ib=rohr_entity_index_get(b.result.value).result.value;
    CHECK(near(velocities[ia].y,0.02f) && near(velocities[ib].y,-0.01f));
    CHECK(near(2*velocities[ia].x+4*velocities[ib].x,0));
    CHECK(near(2*velocities[ia].y+4*velocities[ib].y,0));
    return true;
}

static bool soft_sources_check(const ForceApi *api, unsigned substeps) {
    EntityResult owner=rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    EntityResult a=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){0,0},
        .mass = 2,
        .radius = 0.1f
    });
    EntityResult b=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner.result.value,
        .position = (Position){10,0},
        .mass = 4,
        .radius = 0.1f
    });
    OK(a); OK(b);
    OK(rohr_entity_components_delete(a.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    OK(rohr_entity_components_delete(b.result.value,ROHR_GRAVITY|ROHR_COLLISION));
    ForceDescription description=vector_description(owner.result.value,(Force){0,6});
    EntityResult source=api->force_add(description); OK(source);
    OK(api->force_apply(description));
    EntityResult torque=api->torque_add((TorqueDescription){.target = owner.result.value, .magnitude = 10}); OK(torque);
    OK(api->torque_apply((TorqueDescription){.target = owner.result.value, .magnitude = 10}));
    OK(rohr_physics_substeps_set(substeps));
    uint32_t count=entity_alive_count_get();
    Report report=rohr_system_physics_update(0.01);
    CHECK(!report.has_fatal && report.diagnostic_count==0 && entity_alive_count_get()==count);
    EntityIndex ia=rohr_entity_index_get(a.result.value).result.value;
    EntityIndex ib=rohr_entity_index_get(b.result.value).result.value;
    CHECK(near(velocities[ia].y,0.03f) && near(velocities[ib].y,0.015f));
    CHECK(near(2*velocities[ia].x+4*velocities[ib].x,0));
    CHECK(near(2*velocities[ia].y+4*velocities[ib].y,0.12f));
    EntityInfoResult info=rohr_entity_info_get(source.result.value,ROHR_ENTITY_INFO_PHYSICS_FORCE);
    OK(info); CHECK(info.result.value.force_eligible);
    OK(rohr_entity_info_release(&info.result.value));
    return true;
}

int main(void) {
    const ForceApi apis[] = {
        {physics_force_add, physics_force_magnitude_set, physics_force_magnitude_get, physics_force_apply,
         physics_torque_add, physics_torque_magnitude_set, physics_torque_magnitude_get, physics_torque_apply,
         physics_impulse_apply, physics_angular_impulse_apply},
        {rohr_physics_force_add, rohr_physics_force_magnitude_set, rohr_physics_force_magnitude_get, rohr_physics_force_apply,
         rohr_physics_torque_add, rohr_physics_torque_magnitude_set, rohr_physics_torque_magnitude_get, rohr_physics_torque_apply,
         rohr_physics_impulse_apply, rohr_physics_angular_impulse_apply}
    };
    for(size_t api = 0; api < sizeof(apis) / sizeof(apis[0]); api += 1) {
        for(unsigned substeps = 1; substeps <= 4; substeps += 3) {
            if(rohr_error_check(rohr_engine_start())) return 1;
            bool passed = sources_check(&apis[api], substeps) && impulses_check(&apis[api]);
            rohr_engine_stop();
            if(!passed) return 1;
            if(rohr_error_check(rohr_engine_start())) return 1;
            passed = soft_sources_check(&apis[api], substeps);
            rohr_engine_stop();
            if(!passed) return 1;
            if(rohr_error_check(rohr_engine_start())) return 1;
            passed = beam_sources_check(&apis[api], substeps);
            rohr_engine_stop();
            if(!passed) return 1;
        }
        if(rohr_error_check(rohr_engine_start())) return 1;
        bool passed = persistence_check(&apis[api]);
        rohr_engine_stop();
        if(!passed) return 1;
    }
    return 0;
}
