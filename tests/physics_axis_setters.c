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
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t size) {
    assert(!allocation_forbidden);
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size) {
    assert(!allocation_forbidden);
    return __real_calloc(count, size);
}
void *__wrap_realloc(void *data, size_t size) {
    assert(!allocation_forbidden);
    return __real_realloc(data, size);
}
#endif

static void success_at(EngineResult result, int line) {
    if(result.kind == ERROR_RESULT_ERROR)
        fprintf(stderr, "line %d: %s\n", line, error_code_message_get(result.result.error));
    assert(result.kind == ERROR_RESULT_VALUE && result.result.value);
}
#define success(result) success_at((result), __LINE__)

static void failure(EngineResult result, EngineError expected) {
    if(result.kind != ERROR_RESULT_ERROR || result.result.error != expected)
        fprintf(stderr, "expected error %d, got kind %d error %d\n",
            expected, result.kind, result.result.error);
    assert(result.kind == ERROR_RESULT_ERROR && result.result.error == expected);
}

static Entity created(EntityResult result) {
    assert(result.kind == ERROR_RESULT_VALUE);
    return result.result.value;
}

static void point(Position actual, Position expected) {
    assert(fabsf(actual.x - expected.x) < 0.001f);
    assert(fabsf(actual.y - expected.y) < 0.001f);
}

static PositionResult velocity_get(Entity entity) {
    EntityInfoResult result = rohr_entity_info_get(entity, ROHR_ENTITY_INFO_PHYSICS_VELOCITY);
    assert(result.kind == ERROR_RESULT_VALUE);
    EntityInfo info = result.result.value;
    assert(rohr_entity_info_check(&info, ROHR_ENTITY_INFO_PHYSICS_VELOCITY));
    Velocity value = info.velocity;
    success(rohr_entity_info_release(&info));
    return ERROR_RESULT_MAKE_VALUE(PositionResult, value);
}

static PositionResult acceleration_get(Entity entity) {
    EntityInfoResult result = rohr_entity_info_get(entity, ROHR_ENTITY_INFO_PHYSICS_ACCELERATION);
    assert(result.kind == ERROR_RESULT_VALUE);
    EntityInfo info = result.result.value;
    assert(rohr_entity_info_check(&info, ROHR_ENTITY_INFO_PHYSICS_ACCELERATION));
    Acceleration value = info.acceleration;
    success(rohr_entity_info_release(&info));
    return ERROR_RESULT_MAKE_VALUE(PositionResult, value);
}

typedef struct AxisApi {
    EngineResult (*x_set)(Entity, float);
    EngineResult (*y_set)(Entity, float);
    PositionResult (*get)(Entity);
    EngineError invalid_value;
} AxisApi;

static Position read_value(const AxisApi *api, Entity entity) {
    PositionResult result = api->get(entity);
    assert(result.kind == ERROR_RESULT_VALUE);
    return result.result.value;
}

/* Every exported entity axis setter is exercised through both entry points. */
static const AxisApi entity_apis[] = {
    {physics_velocity_x_set, physics_velocity_y_set, velocity_get, ERROR_ENGINE_STATE_INVALID},
    {rohr_physics_velocity_x_set, rohr_physics_velocity_y_set, velocity_get, ERROR_ENGINE_STATE_INVALID},
    {physics_acceleration_x_set, physics_acceleration_y_set, acceleration_get, ERROR_ENGINE_STATE_INVALID},
    {rohr_physics_acceleration_x_set, rohr_physics_acceleration_y_set, acceleration_get, ERROR_ENGINE_STATE_INVALID},
    {physics_position_x_set, physics_position_y_set, physics_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {rohr_physics_position_x_set, rohr_physics_position_y_set, rohr_physics_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {physics_center_of_mass_local_position_x_set, physics_center_of_mass_local_position_y_set,
        physics_center_of_mass_local_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {rohr_physics_center_of_mass_local_position_x_set, rohr_physics_center_of_mass_local_position_y_set,
        rohr_physics_center_of_mass_local_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {physics_particle_origin_x_set, physics_particle_origin_y_set, physics_particle_origin_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {rohr_physics_particle_origin_x_set, rohr_physics_particle_origin_y_set, rohr_physics_particle_origin_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {physics_soft_body_node_local_position_x_set, physics_soft_body_node_local_position_y_set,
        physics_soft_body_node_local_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE},
    {rohr_physics_soft_body_node_local_position_x_set, rohr_physics_soft_body_node_local_position_y_set,
        rohr_physics_soft_body_node_local_position_get, ERROR_ENGINE_POSITION_OUT_OF_RANGE}
};

static void entity_axes(void) {
    for(size_t i = 0; i < sizeof(entity_apis) / sizeof(entity_apis[0]); ++i) {
        const AxisApi *api = &entity_apis[i];
        Entity body = ENTITY_INVALID;
        Entity entity;
        if(i >= 10) {
            body = created(rohr_physics_soft_body_add((SoftBodyDescription){
                .position = {10, 20}, .orientation = 37}));
            entity = created(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
                .owner = body, .mass = 1, .radius = 1}));
        } else entity = created(rohr_entity_add());
        failure(api->x_set(ENTITY_INVALID, 1), ERROR_ENGINE_INVALID_ENTITY);
        failure(api->y_set(ENTITY_INVALID, 1), ERROR_ENGINE_INVALID_ENTITY);
        success(api->x_set(entity, 5));
        point(read_value(api, entity), (Position){5, 0});
        success(api->y_set(entity, -7));
        point(read_value(api, entity), (Position){5, -7});
        success(api->x_set(entity, -2));
        point(read_value(api, entity), (Position){-2, -7});
        success(api->y_set(entity, 0));
        point(read_value(api, entity), (Position){-2, 0});
        const float invalid[] = {NAN, INFINITY, -INFINITY};
        for(size_t j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
            failure(api->x_set(entity, invalid[j]), api->invalid_value);
            failure(api->y_set(entity, invalid[j]), api->invalid_value);
            point(read_value(api, entity), (Position){-2, 0});
        }
        if(i >= 4 && i < 10) {
            failure(api->x_set(entity, ROHR_WORLD_COORDINATE_MAX * 2), api->invalid_value);
            failure(api->y_set(entity, -ROHR_WORLD_COORDINATE_MAX * 2), api->invalid_value);
            point(read_value(api, entity), (Position){-2, 0});
        }
        success(rohr_entity_delete(entity));
        Entity replacement = created(rohr_entity_add());
        success(rohr_physics_position_set(replacement, (Position){81, 92}));
        failure(api->x_set(entity, 9), ERROR_ENGINE_INVALID_ENTITY);
        failure(api->y_set(entity, 9), ERROR_ENGINE_INVALID_ENTITY);
        point(rohr_physics_position_get(replacement).result.value, (Position){81, 92});
        success(rohr_entity_delete(replacement));
        if(body != ENTITY_INVALID) success(rohr_entity_delete(body));
    }
}

static void gravity_axes(void) {
    EngineResult (*x_set[])(float) = {physics_gravity_x_set, rohr_physics_gravity_x_set};
    EngineResult (*y_set[])(float) = {physics_gravity_y_set, rohr_physics_gravity_y_set};
    Acceleration saved = rohr_physics_gravity_get();
    for(size_t i = 0; i < 2; ++i) {
        success(rohr_physics_gravity_set((Acceleration){3, 4}));
        success(x_set[i](-8));
        point(rohr_physics_gravity_get(), (Position){-8, 4});
        success(y_set[i](0));
        point(rohr_physics_gravity_get(), (Position){-8, 0});
        failure(x_set[i](NAN), ERROR_ENGINE_STATE_INVALID);
        failure(y_set[i](INFINITY), ERROR_ENGINE_STATE_INVALID);
        point(rohr_physics_gravity_get(), (Position){-8, 0});
    }
    success(rohr_physics_gravity_set(saved));
}

static void geometry_axes(void) {
    Entity body = created(rohr_entity_add());
    Shape rectangle = {.amount_of_vertices = 4, .vertices = {{2,3},{6,3},{6,5},{2,5}}};
    success(rohr_physics_hitbox_set(body, rectangle));
    success(rohr_physics_center_of_mass_local_position_x_set(body, 8));
    point(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){8,4});
    assert(!rohr_physics_center_of_mass_automatic_check(body).result.value);
    success(rohr_physics_center_of_mass_automatic_set(body));
    success(rohr_physics_center_of_mass_local_position_y_set(body, 9));
    point(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){4,9});

    EngineResult (*x_set[])(JointAnchorId, float) = {
        physics_joint_anchor_local_position_x_set, rohr_physics_joint_anchor_local_position_x_set};
    EngineResult (*y_set[])(JointAnchorId, float) = {
        physics_joint_anchor_local_position_y_set, rohr_physics_joint_anchor_local_position_y_set};
    success(rohr_physics_position_set(body, (Position){10,20}));
    success(rohr_physics_orientation_set(body, 90));
    for(size_t i = 0; i < 2; ++i) {
        JointAnchorIdResult added = rohr_physics_joint_anchor_add(
            (JointAnchorDescription){.entity = body, .local_offset = {2,3}});
        assert(added.kind == ERROR_RESULT_VALUE);
        JointAnchorId anchor = added.result.value;
        success(x_set[i](anchor, 5));
        point(rohr_physics_joint_anchor_local_position_get(anchor).result.value, (Position){5,3});
        success(y_set[i](anchor, -7));
        point(rohr_physics_joint_anchor_local_position_get(anchor).result.value, (Position){5,-7});
        Position rotated = rohr_math_vector_rotate((Position){5,-7}, 90);
        point(rohr_physics_joint_anchor_world_position_get(anchor).result.value,
            (Position){10 + rotated.x, 20 + rotated.y});
        failure(x_set[i](anchor, NAN), ERROR_ENGINE_POSITION_OUT_OF_RANGE);
        failure(y_set[i](anchor, ROHR_WORLD_COORDINATE_MAX * 2), ERROR_ENGINE_POSITION_OUT_OF_RANGE);
        point(rohr_physics_joint_anchor_local_position_get(anchor).result.value, (Position){5,-7});
        success(rohr_physics_joint_anchor_remove(anchor));
        added = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = body, .local_offset = {1,2}});
        assert(added.kind == ERROR_RESULT_VALUE);
        failure(x_set[i](anchor, 0), ERROR_ENGINE_ENTITY_NOT_FOUND);
        failure(y_set[i](anchor, 0), ERROR_ENGINE_ENTITY_NOT_FOUND);
        point(rohr_physics_joint_anchor_local_position_get(added.result.value).result.value, (Position){1,2});
    }
    success(rohr_entity_delete(body));

    Entity particle = created(rohr_physics_particle_add((ParticleDescription){
        .position = {10,20}, .local_origin = {2,3}, .radius = 4, .mass_value = 1,
        .rigid_vertices = 8}));
    success(rohr_physics_particle_origin_x_set(particle, -5));
    point(rohr_physics_particle_origin_get(particle).result.value, (Position){-5,3});
    success(rohr_physics_particle_origin_y_set(particle, 7));
    point(rohr_physics_particle_origin_get(particle).result.value, (Position){-5,7});
    point(rohr_physics_center_of_mass_local_position_get(particle).result.value, (Position){-5,7});
    Shape shape = rohr_physics_hitbox_get(particle).result.value;
    assert(shape.amount_of_vertices == 8);
    point(rohr_math_polygon_centroid(shape), (Position){-5,7});
    assert(rohr_physics_particle_radius_get(particle).result.value == 4);
    failure(rohr_physics_center_of_mass_local_position_x_set(particle, 0), ERROR_ENGINE_STATE_INVALID);
    failure(rohr_physics_center_of_mass_local_position_y_set(particle, 0), ERROR_ENGINE_STATE_INVALID);
    point(rohr_physics_particle_origin_get(particle).result.value, (Position){-5,7});
    success(rohr_entity_delete(particle));
}

static void soft_body_axes(void) {
    Entity body = created(rohr_physics_soft_body_add((SoftBodyDescription){.position = {10,20}, .orientation = 30}));
    Entity node = created(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .owner = body, .position = {2,3}, .radius = 1, .mass = 1, .velocity = {5,6}, .acceleration = {7,8}}));
    Position old = rohr_physics_position_get(node).result.value;
    success(rohr_physics_position_x_set(body, 40));
    point(rohr_physics_position_get(node).result.value, (Position){old.x+30,old.y});
    success(rohr_physics_position_y_set(body, 50));
    point(rohr_physics_position_get(node).result.value, (Position){old.x+30,old.y+30});
    point(rohr_physics_soft_body_node_local_position_get(node).result.value, (Position){2,3});
    point(velocity_get(node).result.value, (Position){5,6});
    point(acceleration_get(node).result.value, (Position){7,8});
    /* COM edits redistribute mass, and reject unreachable centers. */
    failure(rohr_physics_center_of_mass_local_position_x_set(body, -4), ERROR_ENGINE_STATE_INVALID);
    point(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){2,3});
    created(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .owner = body, .position = {6,3}, .radius = 1, .mass = 1}));
    created(rohr_physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .owner = body, .position = {2,7}, .radius = 1, .mass = 1}));
    success(rohr_physics_center_of_mass_local_position_x_set(body, 3.5f));
    point(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){3.5f,13.0f/3});
    success(rohr_physics_center_of_mass_local_position_y_set(body, 4.2f));
    point(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){3.5f,4.2f});
    assert(fabsf(rohr_physics_mass_get(body).result.value - 3) < 0.001f);
    point(rohr_physics_position_get(node).result.value, (Position){old.x+30,old.y+30});
    success(rohr_physics_position_set(node, (Position){ROHR_WORLD_COORDINATE_MAX, 0}));
    failure(rohr_physics_position_x_set(body, 41), ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    point(rohr_physics_position_get(body).result.value, (Position){40,50});
    point(rohr_physics_position_get(node).result.value, (Position){ROHR_WORLD_COORDINATE_MAX,0});
    success(rohr_entity_delete(body));
    Entity plain = created(rohr_entity_add());
    failure(rohr_physics_soft_body_node_local_position_x_set(plain, 1), ERROR_ENGINE_COMPONENT_MISSING);
    failure(rohr_physics_soft_body_node_local_position_y_set(plain, 1), ERROR_ENGINE_COMPONENT_MISSING);
    success(rohr_entity_delete(plain));
}

static void current_values(void) {
    Entity body = created(rohr_entity_add());
    success(rohr_physics_velocity_set(body, (Velocity){1,2}));
    success(rohr_physics_acceleration_set(body, (Acceleration){3,4}));
    success(rohr_physics_position_set(body, (Position){5,6}));
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = true;
#endif
    success(rohr_physics_velocity_x_set(body, 9));
    success(rohr_physics_acceleration_y_set(body, 8));
    success(rohr_physics_position_x_set(body, 7));
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = false;
#endif
    point(velocity_get(body).result.value, (Position){9,2});
    point(acceleration_get(body).result.value, (Position){3,8});
    point(rohr_physics_position_get(body).result.value, (Position){7,6});
    success(rohr_physics_velocity_set(body, (Velocity){10,11}));
    success(rohr_physics_velocity_x_set(body, 12));
    point(velocity_get(body).result.value, (Position){12,11});
    success(rohr_entity_delete(body));
}

int main(void) {
    success(rohr_engine_start());
    entity_axes();
    gravity_axes();
    geometry_axes();
    soft_body_axes();
    current_values();
    rohr_engine_stop();
    return 0;
}
