/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr, "beam motion: %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while(0)
#define OK(x) CHECK(!rohr_error_check(x))

static EntityIndex index_get(Entity entity) {
    EntityIndexResult result = rohr_entity_index_get(entity);
    OK(result);
    return result.result.value;
}

/* Sample the actual position formulas, including intermediate float velocity
 * rounding in the ordinary integrator. Geometry is checked independently of
 * the sweep's polynomial coefficients and root search. */
static Position moved(Position p, Velocity v, Acceleration a, double t, bool elastic) {
    if(elastic) return (Position){p.x + v.x*t + .5*a.x*t*t, p.y + v.y*t + .5*a.y*t*t};
    Velocity next = {v.x + a.x*t, v.y + a.y*t};
    return (Position){p.x + next.x*t, p.y + next.y*t};
}

static void side_path(unsigned motion, bool elastic, bool reverse, float angle, float offset) {
    OK(rohr_engine_start());
    EntityResult owner = rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(owner);
    Position start[3] = {{-10, 0}, {10, 0}, {3, 1.5f}};
    Velocity velocity[3] = {{0, 0}, {0, 0}, {0, 0}};
    Acceleration acceleration[3] = {{0, 0}, {0, 0}, {0, 0}};
    if(motion == 0) { velocity[0].y = -100; velocity[1].y = 100; }
    if(motion == 1) { velocity[0].x = 800; velocity[1].x = -800; velocity[2].y = -10; }
    if(motion == 2) { acceleration[0].y = -400; acceleration[1].y = 400; acceleration[2].y = -200; }
    if(motion == 3) { velocity[0].x = -100; velocity[1].x = 100; velocity[2].y = -1000; }
    if(motion == 4) { start[2].y = 1.4900001f; velocity[2].y = -1; }
    for(unsigned i = 0; i < 3; ++i) {
        start[i] = rohr_math_vector_rotate(start[i], angle);
        start[i].x += offset; start[i].y += offset;
        velocity[i] = rohr_math_vector_rotate(velocity[i], angle);
        acceleration[i] = rohr_math_vector_rotate(acceleration[i], angle);
        /* Common translation must not become a relative collision speed. */
        velocity[i].x += 100;
        acceleration[i].x += 30;
    }
    Entity nodes[2]; EntityIndex indices[3];
    for(unsigned i = 0; i < 2; ++i) {
        EntityResult result = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .owner = owner.result.value, .position = start[i], .mass = 1, .radius = .1f});
        OK(result); nodes[i] = result.result.value; indices[i] = index_get(nodes[i]);
    }
    EntityResult beam = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = owner.result.value, .node_a = nodes[reverse ? 1 : 0], .node_b = nodes[reverse ? 0 : 1],
        .stiffness = elastic ? 10 : 0,
        .collision = {.enabled = true, .thickness = 2, .filter = {.category = 1, .responds_to = 2}}});
    OK(beam);
    ParticleDescription description = rohr_physics_particle_description_default_get();
    description.position = start[2]; description.radius = .5f; description.mass_value = 1;
    description.gravity_enabled = false; description.rigid_vertices = 16;
    EntityResult particle = rohr_physics_particle_add(description); OK(particle);
    OK(rohr_physics_collision_filter_description_set(particle.result.value,
        (CollisionFilterDescription){.category = 2, .responds_to = 1}));
    indices[2] = index_get(particle.result.value);
    for(unsigned i = 0; i < 3; ++i) {
        velocities[indices[i]] = velocity[i];
        accelerations[indices[i]] = acceleration[i];
    }
    Report report = {0};
    physics_soft_motion_enabled_set(true);
    double requested = physics_soft_duration_get(.05);
    double duration = physics_soft_sweep_duration_get(requested, &report);
    physics_soft_motion_enabled_set(false);
    CHECK(!report.has_fatal && duration > 0 && duration <= requested);
    CHECK(duration < requested);
    /* A nearly exhausted side-plane margin must not replace the legacy
     * closing-travel bound with a timestep too small to change float poses. */
    if(motion == 4) CHECK(duration > .009);
    Shape shape = hit_boxes[indices[2]];
    for(unsigned sample = 0; sample <= 256; ++sample) {
        double t = duration * sample / 256;
        Position a = moved(start[0], velocity[0], acceleration[0], t, elastic);
        Position b = moved(start[1], velocity[1], acceleration[1], t, elastic);
        Position p = moved(start[2], velocity[2], acceleration[2], t, false);
        double dx = (double)b.x-a.x, dy = (double)b.y-a.y, length = hypot(dx, dy);
        CHECK(length > .0001);
        for(unsigned v = 0; v < shape.amount_of_vertices; ++v) {
            double qx = (double)p.x + shape.vertices[v].x-a.x;
            double qy = (double)p.y + shape.vertices[v].y-a.y;
            double distance = (dx*qy-dy*qx)/length;
            /* Existing .005 * thickness margin, plus float pose rounding. */
            CHECK(distance >= (motion == 4 ? .98 : .99) - .00005);
        }
    }
    rohr_engine_stop();
}

int main(void) {
    for(unsigned motion = 0; motion < 4; ++motion)
        for(unsigned elastic = 0; elastic < 2; ++elastic)
            for(unsigned reverse = 0; reverse < 2; ++reverse)
                for(unsigned angle = 0; angle < 3; ++angle)
                    for(unsigned offset = 0; offset < 2; ++offset)
                        side_path(motion, elastic, reverse, angle*37, offset*100);
    side_path(4, false, false, 0, 0);
    puts("PASS beam motion: sampled rotating, shortening, stretching and accelerated side-contact paths");
    return 0;
}
