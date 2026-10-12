/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_internal.h"
#include "physics/soft_body/soft_body_motion.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))
static void near(double actual, double expected, double tolerance) {
    if(fabs(actual - expected) > tolerance)
        fprintf(stderr, "Expected %.12g, got %.12g\n", expected, actual);
    assert(isfinite(actual) && fabs(actual - expected) <= tolerance);
}
static Entity body_create(void) {
    EntityResult result = rohr_physics_soft_body_create(); OK(result);
    return result.result.value;
}
static Entity node_create(Entity body, Position point, float mass_value) {
    EntityResult result = rohr_physics_soft_body_node_create(body, point, mass_value, .1f);
    OK(result); return result.result.value;
}
static Velocity velocity_get(Entity node) {
    EntityIndexResult index = rohr_entity_index_get(node); OK(index);
    return velocities[index.result.value];
}
static double rate_get(Entity body) {
    AngularVelocityResult value = rohr_physics_angular_velocity_get(body); OK(value);
    return value.result.value;
}

static void motion_check(void) {
    Entity body = body_create(), nodes[4];
    Position points[] = {{10,20},{30,20},{30,50},{10,50}};
    float masses[] = {1,2,3,4};
    Position center = {20,41};
    Velocity original[4], expected[4], repeated[4];
    OK(rohr_physics_position_set(body, (Position){-50,90}));
    OK(rohr_physics_orientation_set(body, 37));
    for(int i = 0; i < 4; i += 1) {
        double rx = points[i].x - center.x, ry = points[i].y - center.y;
        /* Translation + radial deformation + a known clockwise rotation. */
        original[i] = (Velocity){4 + .2 * rx + .3 * ry, -3 + .2 * ry - .3 * rx};
        nodes[i] = node_create(body, points[i], masses[i]);
        OK(rohr_physics_velocity_set(nodes[i], original[i]));
    }
    near(rate_get(body), rohr_math_radians_to_degrees(.3f), 1e-4);
    const float rates[] = {90, 90, -450, 0};
    for(size_t step = 0; step < sizeof(rates)/sizeof(*rates); step += 1) {
        OK(rohr_physics_angular_velocity_set(body, rates[step]));
        near(rate_get(body), rates[step], 1e-4);
        double x = 0, y = 0;
        for(int i = 0; i < 4; i += 1) {
            double rx = points[i].x - center.x, ry = points[i].y - center.y;
            double omega = rohr_math_degrees_to_radians(rates[step]);
            Velocity actual = velocity_get(nodes[i]);
            expected[i] = (Velocity){4 + .2 * rx + omega * ry, -3 + .2 * ry - omega * rx};
            near(actual.x, expected[i].x, 3e-5);
            near(actual.y, expected[i].y, 3e-5);
            if(step == 0) repeated[i] = actual;
            if(step == 1) {
                near(actual.x, repeated[i].x, 1e-5);
                near(actual.y, repeated[i].y, 1e-5);
            }
            x += masses[i] * actual.x; y += masses[i] * actual.y;
            PositionResult position = rohr_physics_position_get(nodes[i]); OK(position);
            assert(position.result.value.x == points[i].x && position.result.value.y == points[i].y);
        }
        near(x / 10, 4, 1e-5); near(y / 10, -3, 1e-5);
    }
    OK(rohr_physics_angular_velocity_radians_set(body, -2));
    AngularVelocityResult radians = rohr_physics_angular_velocity_radians_get(body); OK(radians);
    near(radians.result.value, -2, 1e-6);
    for(int i = 0; i < 4; i += 1) original[i] = velocity_get(nodes[i]);
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, NAN)));
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, INFINITY)));
    OK(rohr_physics_position_set(nodes[3], (Position){10000,10000}));
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, FLT_MAX)));
    OK(rohr_physics_position_set(nodes[3], points[3]));
    for(int i = 0; i < 4; i += 1) {
        Velocity actual = velocity_get(nodes[i]);
        assert(memcmp(&actual, &original[i], sizeof(actual)) == 0);
    }
    OK(rohr_physics_mass_set(nodes[3], 0));
    assert(rohr_error_check(rohr_physics_angular_velocity_get(body)));
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, 30)));
    for(int i = 0; i < 4; i += 1) {
        Velocity actual = velocity_get(nodes[i]);
        assert(memcmp(&actual, &original[i], sizeof(actual)) == 0);
    }
    OK(rohr_entity_delete(body));
}

static void simulation_check(void) {
    Entity body = body_create();
    OK(rohr_physics_orientation_set(body, 23));
    Entity a = node_create(body, (Position){-10,0}, 1);
    Entity b = node_create(body, (Position){10,0}, 1);
    EntityResult beam = rohr_physics_soft_body_beam_create(body, a, b, 100, 0); OK(beam);
    OK(rohr_physics_angular_velocity_radians_set(body, 1));
    near(velocity_get(a).y, 10, 1e-5);
    near(velocity_get(b).y, -10, 1e-5);
    /* Stale scalar state from an older save must not spin the container. */
    EntityIndex index; assert(entity_index_get(body, &index));
    OK(AngularVelocityPool_store_at(&angular_velocities_pool, index, 720));
    OK(rohr_physics_angular_acceleration_set(body, 100));
    rohr_physics_substeps_set(1);
    assert((rohr_physics_pipeline_update(.1)).header.status != ROHR_REPORT_FAILED);
    near(orientations[index], 23, 1e-6);
    PositionResult pa = rohr_physics_position_get(a), pb = rohr_physics_position_get(b);
    OK(pa); OK(pb);
    assert(pa.result.value.y > 0 && pb.result.value.y < 0);
    double dx = pb.result.value.x - pa.result.value.x;
    double dy = pb.result.value.y - pa.result.value.y;
    assert(hypot(dx, dy) > 20);
    near(rate_get(body), rohr_math_radians_to_degrees(1.0f / 1.01f), 1e-3);
    assert((rohr_physics_pipeline_update(.1)).header.status != ROHR_REPORT_FAILED);
    assert(velocity_get(a).x > 0 && velocity_get(b).x < 0);
    near(orientations[index], 23, 1e-6);
    OK(rohr_physics_orientation_set(body, 113));
    PositionResult rotated = rohr_physics_position_get(a); OK(rotated);
    assert(fabsf(rotated.result.value.x - pa.result.value.x) > 1);
    OK(rohr_entity_delete(body));
}

static void invalid_check(void) {
    Entity body = body_create();
    assert(rohr_error_check(rohr_physics_angular_velocity_get(body)));
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, 0)));
    Entity a = node_create(body, (Position){1,1}, 1);
    Entity b = node_create(body, (Position){1,1}, 2);
    OK(rohr_physics_velocity_set(a, (Velocity){3,4}));
    assert(rohr_error_check(rohr_physics_angular_velocity_set(body, 5)));
    near(velocity_get(a).x, 3, 0); near(velocity_get(b).x, 0, 0);
    Position points[] = {{0,0},{1,0}};
    Mass masses[] = {1,2};
    Velocity values[] = {{1,2},{3,4}}, output[] = {{5,6},{7,8}}, before[2];
    memcpy(before, output, sizeof(output));
    points[1].x = NAN;
    assert(!physics_soft_angular_velocities_get(points, masses, values, 2, 20, output));
    assert(memcmp(before, output, sizeof(output)) == 0);
    OK(rohr_entity_delete(body));
}

int main(void) {
    OK(rohr_engine_start());
    motion_check();
    invalid_check();
    simulation_check();
    rohr_engine_stop();
    return 0;
}
