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
    EntityResult (*force_create)(Entity, Force);
    EngineResult (*force_set)(Entity, Force);
    ForceResult (*force_get)(Entity);
    EngineResult (*force_apply)(Entity, Force);
    EntityResult (*torque_create)(Entity, Torque);
    EngineResult (*torque_set)(Entity, Torque);
    TorqueResult (*torque_get)(Entity);
    EngineResult (*torque_apply)(Entity, Torque);
    EngineResult (*impulse_apply)(Entity, Vec2D);
    EngineResult (*angular_impulse_apply)(Entity, float);
} ForceApi;

static const Shape rectangle = {.amount_of_vertices = 4,
    .vertices = {{2,3}, {6,3}, {6,5}, {2,5}}};
static bool near(float a, float b) { return fabsf(a - b) < 0.0001f; }

static bool body_create(Entity *body, EntityIndex *index) {
    EntityResult result = rohr_entity_add();
    OK(result);
    *body = result.result.value;
    *index = rohr_entity_index_get(*body).result.value;
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
    EntityResult force = api->force_create(body, (Force){12,0});
    EntityResult torque = api->torque_create(body, 20);
    OK(force); OK(torque);
    Entity f = force.result.value, t = torque.result.value;
    CHECK(f != body && t != body && f != t);
    CHECK(api->force_get(f).result.value.x == 12);
    CHECK(api->torque_get(t).result.value == 20);
    /* Replace, rather than accumulate; preserve source targets. */
    OK(api->force_set(f, (Force){24,0}));
    OK(api->torque_set(t, 40));
    CHECK(api->force_get(f).result.value.x == 24);
    CHECK(api->torque_get(t).result.value == 40);
    OK(api->force_apply(body, (Force){12,0}));
    OK(api->force_apply(body, (Force){0,24}));
    OK(api->torque_apply(body, 20));
    OK(api->torque_apply(body, -10));
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.3f) && near(velocities[index].y, 0.2f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.25f));
    /* Direct physics updates do not advance engine tick lifetimes. */
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.6f) && near(velocities[index].y, 0.4f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.5f));
    SDL_Delay(2);
    CHECK(rohr_system_tick_update() > 0);
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.8f) && near(velocities[index].y, 0.4f));
    CHECK(near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.7f));
    CHECK(rohr_entity_alive_check(f) && rohr_entity_alive_check(t));
    OK(api->force_set(f, (Force){0}));
    OK(api->torque_set(t, 0));
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.8f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.7f));
    CHECK(rohr_entity_alive_check(f) && rohr_entity_alive_check(t));
    OK(api->force_set(f, (Force){12,0}));
    OK(api->torque_set(t, 20));
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.9f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.8f));
    OK(rohr_entity_delete(f)); OK(rohr_entity_delete(t));
    OK(rohr_system_physics_update(0.1));
    CHECK(rohr_entity_alive_check(body));
    CHECK(near(velocities[index].x, 0.9f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.8f));

    /* Stale handles remain invalid even after an entity slot is reused. */
    EntityResult plain = rohr_entity_add();
    OK(plain);
    Entity p = plain.result.value;
    ERROR(api->force_get(f), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_get(t), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->force_set(f, (Force){0}), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_set(t, 0), ERROR_ENGINE_INVALID_ENTITY);
    /* State loading can still attach components before assigning a target. */
    OK(api->force_set(p, (Force){3,4}));
    OK(api->torque_set(p, -5));
    CHECK(api->force_get(p).result.value.y == 4 && api->torque_get(p).result.value == -5);
    CHECK(!rohr_entity_components_check(p, ROHR_TARGETABLE));
    OK(rohr_entity_components_delete(p, ROHR_FORCE | ROHR_TORQUE));
    ERROR(api->force_get(p), ERROR_ENGINE_COMPONENT_MISSING);
    ERROR(api->torque_get(p), ERROR_ENGINE_COMPONENT_MISSING);
    ERROR(api->force_create(ENTITY_INVALID, (Force){0}), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_create(ENTITY_INVALID, 0), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->force_apply(ENTITY_INVALID, (Force){0}), ERROR_ENGINE_INVALID_ENTITY);
    ERROR(api->torque_apply(ENTITY_INVALID, 0), ERROR_ENGINE_INVALID_ENTITY);
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
    for(int mode = 0; mode < 7; mode += 1) {
        Entity e;
        EntityIndex i;
        CHECK(body_create(&e, &i));
        if(mode == 0) OK(rohr_physics_static_set(e));
        if(mode == 1) OK(rohr_physics_entity_hold(e));
        if(mode == 2) OK(rohr_physics_kinematic_driven_set(e));
        if(mode == 3) OK(rohr_physics_mass_set(e, 0));
        if(mode == 4) OK(rohr_physics_hitbox_remove(e));
        if(mode == 5) OK(rohr_physics_angle_lock_set(e, 0, 0));
        if(mode == 6) OK(rohr_physics_particle_radius_set(e, 2));
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
    EntityResult force = api->force_create(body, (Force){24,0});
    EntityResult torque = api->torque_create(body, 40);
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
    ForceResult f = api->force_get(force.result.value);
    TorqueResult t = api->torque_get(torque.result.value);
    OK(f); OK(t);
    CHECK(f.result.value.x == 24 && f.result.value.y == 0 && t.result.value == 40);
    index = rohr_entity_index_get(restored.result.value).result.value;
    OK(rohr_system_physics_update(0.1));
    CHECK(near(velocities[index].x, 0.2f) && near(rohr_math_degrees_to_radians(angular_velocities[index]), 0.2f));
    CHECK(remove(path) == 0);
    return true;
}

int main(void) {
    const ForceApi apis[] = {
        {physics_force_create, physics_force_set, physics_force_get, physics_force_apply,
         physics_torque_create, physics_torque_set, physics_torque_get, physics_torque_apply,
         physics_impulse_apply, physics_angular_impulse_apply},
        {rohr_physics_force_create, rohr_physics_force_set, rohr_physics_force_get, rohr_physics_force_apply,
         rohr_physics_torque_create, rohr_physics_torque_set, rohr_physics_torque_get, rohr_physics_torque_apply,
         rohr_physics_impulse_apply, rohr_physics_angular_impulse_apply}
    };
    for(size_t api = 0; api < sizeof(apis) / sizeof(apis[0]); api += 1) {
        for(unsigned substeps = 1; substeps <= 4; substeps += 3) {
            if(rohr_error_check(rohr_engine_start())) return 1;
            bool passed = sources_check(&apis[api], substeps) && impulses_check(&apis[api]);
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
