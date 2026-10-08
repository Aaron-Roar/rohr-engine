/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/collision/overlap_geometry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define OK(value) assert(!rohr_error_check(value))
static EntityIndex index_get(Entity entity) {
    EntityIndexResult result = rohr_entity_index_get(entity);
    OK(result);
    return result.result.value;
}
static void close_to(float actual, float expected, float tolerance) {
    if(fabsf(actual - expected) > tolerance)
        fprintf(stderr, "Expected %g, got %g (tolerance %g)\n", expected, actual, tolerance);
    assert(fabsf(actual - expected) <= tolerance);
}
static Position transform(Position point, Position offset, float angle) {
    point = rohr_math_vector_rotate(point, angle);
    return (Position){point.x + offset.x, point.y + offset.y};
}
static void separation_test(float angle, Position offset) {
    Shape beam = rohr_math_square_create(20, .01f);
    Shape particle = rohr_math_circle_create(1, 16);
    for(int i = 0; i < beam.amount_of_vertices; i++)
        beam.vertices[i] = transform(beam.vertices[i], offset, angle);
    for(int i = 0; i < particle.amount_of_vertices; i++) {
        particle.vertices[i].y += .5f;
        particle.vertices[i] = transform(particle.vertices[i], offset, angle);
    }
    OverlapInfo overlap = physics_sat_overlap_get(beam, particle);
    OverlapInfo reverse = physics_sat_overlap_get(particle, beam);
    assert(overlap.detected && reverse.detected);
    close_to(overlap.depth, .505f, .001f);
    close_to(reverse.depth, overlap.depth, .001f);
    close_to(math_dot_product(overlap.normal, reverse.normal), -1, .001f);
    for(int i = 0; i < particle.amount_of_vertices; i++) {
        particle.vertices[i].x += overlap.normal.x * (overlap.depth + .001f);
        particle.vertices[i].y += overlap.normal.y * (overlap.depth + .001f);
    }
    assert(!physics_sat_overlap_get(beam, particle).detected);
}

static void impact_test(float height, float thickness, float restitution,
        float angle, bool movable, uint32_t iterations) {
    static float scene_x = 100;
    Position offset = {scene_x, 200}; scene_x += 100;
    OK(rohr_physics_solver_iterations_set(iterations));
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    EntityResult a = rohr_physics_soft_body_node_create(body.result.value,
        transform((Position){-10, 0}, offset, angle), 1, 1); OK(a);
    EntityResult b = rohr_physics_soft_body_node_create(body.result.value,
        transform((Position){10, 0}, offset, angle), 1, 1); OK(b);
    if(!movable) {
        OK(rohr_physics_static_set(a.result.value));
        OK(rohr_physics_static_set(b.result.value));
    }
    EntityResult beam = rohr_physics_soft_body_beam_create(body.result.value,
        a.result.value, b.result.value, 0, 0); OK(beam);
    OK(rohr_physics_soft_body_beam_collision_thickness_set(beam.result.value, thickness));
    OK(rohr_physics_restitution_set(beam.result.value, restitution));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.position = transform((Position){0, height}, offset, angle);
    config.velocity = rohr_math_vector_rotate((Velocity){0, -10}, angle);
    config.radius = 1; config.mass_value = 10; config.restitution = 1;
    config.gravity_enabled = false;
    EntityResult particle = rohr_physics_particle_create(config); OK(particle);
    OK(rohr_system_physics_update(0));
    EntityIndex p = index_get(particle.result.value);
    EntityIndex ai = index_get(a.result.value), bi = index_get(b.result.value);
    Vec2D normal = rohr_math_vector_rotate((Vec2D){0, 1}, angle);
    Velocity relative = {velocities[p].x - .5f * (velocities[ai].x + velocities[bi].x),
        velocities[p].y - .5f * (velocities[ai].y + velocities[bi].y)};
    assert(rohr_physics_contact_check(beam.result.value, particle.result.value));
    close_to(math_dot_product(relative, normal), 10 * restitution, .025f);
    if(movable) {
        close_to(10 * velocities[p].x + velocities[ai].x + velocities[bi].x,
            10 * config.velocity.x, .025f);
        close_to(10 * velocities[p].y + velocities[ai].y + velocities[bi].y,
            10 * config.velocity.y, .025f);
    }
    Position center = {.5f * (positions[ai].x + positions[bi].x),
        .5f * (positions[ai].y + positions[bi].y)};
    float distance = math_dot_product(math_vector_subtract(positions[p], center), normal);
    Projection circle = math_project_shape_on_axis(rohr_math_circle_create(1, config.rigid_vertices), normal);
    close_to(distance, -circle.min + thickness * .5f, .005f);
    OK(rohr_entity_delete(particle.result.value));
    OK(rohr_entity_delete(body.result.value));
}

static void supported_impact_test(uint32_t iterations, float restitution) {
    OK(rohr_physics_solver_iterations_set(iterations));
    EntityResult floor = rohr_entity_add(); OK(floor);
    OK(rohr_physics_position_set(floor.result.value, (Position){0, -5}));
    OK(rohr_physics_hitbox_set(floor.result.value, rohr_math_square_create(200, 10)));
    OK(rohr_physics_restitution_set(floor.result.value, 0));
    OK(rohr_physics_static_set(floor.result.value));
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    EntityResult a = rohr_physics_soft_body_node_create(body.result.value,
        (Position){-50, .99f}, 1, 1); OK(a);
    EntityResult b = rohr_physics_soft_body_node_create(body.result.value,
        (Position){50, .99f}, 1, 1); OK(b);
    OK(rohr_physics_restitution_set(a.result.value, 0));
    OK(rohr_physics_restitution_set(b.result.value, 0));
    EntityResult beam = rohr_physics_soft_body_beam_create(body.result.value,
        a.result.value, b.result.value, 0, 0); OK(beam);
    OK(rohr_physics_soft_body_beam_collision_thickness_set(beam.result.value, .01f));
    OK(rohr_physics_restitution_set(beam.result.value, restitution));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.position = (Position){0, 10.8f}; config.radius = 10;
    config.mass_value = 10; config.velocity = (Velocity){0, -60};
    config.restitution = 1; config.gravity_enabled = false;
    EntityResult particle = rohr_physics_particle_create(config); OK(particle);
    OK(rohr_system_physics_update(0));
    EntityIndex p = index_get(particle.result.value);
    EntityIndex ai = index_get(a.result.value), bi = index_get(b.result.value);
    ContactInfo c = rohr_physics_contact_get(beam.result.value, particle.result.value);
    assert(c.detected);
    float relative_y = velocities[p].y - .5f * (velocities[ai].y + velocities[bi].y);
    close_to(relative_y, 60 * restitution, .02f);
    close_to(c.points[0].normal_impulse.y, 10 * (velocities[p].y + 60), .02f);
    if(restitution > 0) assert(velocities[p].y > 0);
    else assert(velocities[p].y <= .01f);
    float energy = 10 * math_dot_product(velocities[p], velocities[p]) +
        math_dot_product(velocities[ai], velocities[ai]) +
        math_dot_product(velocities[bi], velocities[bi]);
    assert(energy <= 10 * 60 * 60 + .1f);
    /* No repeated bounce injection while the bodies separate on later steps. */
    float rebound = velocities[p].y;
    if(restitution > 0) {
        for(int tick = 0; tick < 20; tick++) OK(rohr_system_physics_update(1.0 / 60));
        close_to(velocities[p].y, rebound, .02f);
        assert(!rohr_physics_contact_check(beam.result.value, particle.result.value));
    }
    OK(rohr_entity_delete(particle.result.value));
    OK(rohr_entity_delete(body.result.value));
    OK(rohr_entity_delete(floor.result.value));
}

int main(void) {
    OK(rohr_engine_start());
    OK(rohr_physics_gravity_set((Acceleration){0, 0}));
    separation_test(0, (Position){0, 0});
    separation_test(37, (Position){200, -170});
    for(int movable = 0; movable <= 1; movable++)
        for(int iterations = 1; iterations <= 16; iterations *= 4) {
            impact_test(.999f, .01f, .8f, 0, movable, iterations);
            impact_test(.5f, .01f, .8f, 0, movable, iterations);
            impact_test(.5f, .01f, 0, 37, movable, iterations);
            impact_test(1.5f, 2, 1, 0, movable, iterations);
            impact_test(1.5f, 2, .8f, 37, movable, iterations);
        }
    supported_impact_test(8, .8f);
    supported_impact_test(32, .8f);
    supported_impact_test(8, 0);
    rohr_engine_stop();
    return 0;
}
