/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr, "particle sweep: %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while(0)
#define OK(x) CHECK(!rohr_error_check(x))

typedef struct SweepFixture {
    Entity entities[2];
    EntityIndex indices[2];
    Vec2D axis;
    bool grazing, contact;
    double elapsed;
} SweepFixture;

static void observe(double dt, void *context) {
    SweepFixture *f = context;
    Position a = positions[f->indices[0]], b = positions[f->indices[1]];
    double dx = (double)b.x-a.x, dy = (double)b.y-a.y;
    CHECK(isfinite(dx) && isfinite(dy));
    CHECK(hypot(dx, dy) >= 1-.001);
    if(!f->grazing) CHECK(dx*f->axis.x+dy*f->axis.y > 0);
    f->contact |= rohr_physics_contact_check(f->entities[0], f->entities[1]);
    f->elapsed += dt;
}

static void impact(float angle, float offset, float restitution, bool grazing) {
    OK(rohr_engine_start());
    OK(rohr_physics_substeps_set(1));
    OK(rohr_physics_solver_iterations_set(16));
    SweepFixture f = {.axis = rohr_math_vector_rotate((Vec2D){1, 0}, angle), .grazing = grazing};
    for(unsigned i = 0; i < 2; ++i) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.radius = .5f; d.rigid_vertices = 3; d.friction = 0;
        d.restitution = i ? 1 : restitution; d.gravity_enabled = false;
        d.position = rohr_math_vector_rotate(grazing ?
            (Position){i ? 0 : -10, i ? 0 : .9999f} : (Position){i ? 100 : -100, 0}, angle);
        d.position.x += offset; d.position.y += offset;
        d.velocity = rohr_math_vector_rotate((Velocity){i ? (grazing ? 0 : -10000) : 10000, 0}, angle);
        EntityResult entity = rohr_physics_particle_add(d); OK(entity);
        f.entities[i] = entity.result.value;
        EntityIndexResult index = rohr_entity_index_get(f.entities[i]); OK(index);
        f.indices[i] = index.result.value;
    }
    physics_substep_observer_set(observe, &f);
    double duration = grazing ? .002 : .2;
    Report report = rohr_system_physics_update(duration);
    physics_substep_observer_set(NULL, NULL);
    CHECK(!report.has_fatal && f.contact && fabs(f.elapsed-duration) < 1e-10);
    if(!grazing) for(unsigned i = 0; i < 2; ++i) {
        Velocity v = velocities[f.indices[i]];
        double normal = v.x*f.axis.x+v.y*f.axis.y;
        double expected = (i ? 1 : -1)*10000*restitution;
        CHECK(fabs(normal-expected) < .1);
    }
    rohr_engine_stop();
}

static void turning_observe(double dt, void *context) {
    SweepFixture *f = context;
    CHECK(positions[f->indices[0]].x+.1 <= -.5+.001);
    f->contact |= rohr_physics_contact_check(f->entities[0], f->entities[1]);
    f->elapsed += dt;
}

/* The proposed-step endpoint returns to its initial position, but the path
 * crosses the wall. Broadphase must include the intervening excursion. */
static void turning_path(bool circle) {
    OK(rohr_engine_start()); OK(rohr_physics_substeps_set(1));
    ParticleDescription d = rohr_physics_particle_description_default_get();
    d.position = (Position){-.75f, 0}; d.velocity = (Velocity){100, 0};
    d.acceleration = (Acceleration){-10000, 0}; d.radius = .1f;
    d.gravity_enabled = false; d.friction = 0;
    EntityResult p = rohr_physics_particle_add(d); OK(p);
    EntityResult wall;
    if(circle) {
        ParticleDescription target = rohr_physics_particle_description_default_get();
        target.radius = .5f; target.static_body = true; target.rigid_vertices = 3;
        wall = rohr_physics_particle_add(target); OK(wall);
    } else {
        wall = rohr_entity_add(); OK(wall);
        OK(rohr_physics_position_set(wall.result.value, (Position){0}));
        OK(rohr_physics_hitbox_set(wall.result.value, rohr_math_square_create(1, 10)));
        OK(rohr_physics_static_set(wall.result.value));
        OK(rohr_entity_components_add(wall.result.value, ROHR_COLLISION));
    }
    EntityIndexResult index = rohr_entity_index_get(p.result.value); OK(index);
    SweepFixture f = {.entities = {p.result.value, wall.result.value}, .indices = {index.result.value}};
    physics_substep_observer_set(turning_observe, &f);
    Report report = rohr_system_physics_update(.01);
    physics_substep_observer_set(NULL, NULL);
    if(report.has_fatal || !f.contact || fabs(f.elapsed-.01) >= 1e-10)
        fprintf(stderr, "turning path fatal=%d contact=%d time=%.12g x=%g vx=%g\n",
            report.has_fatal, f.contact, f.elapsed, positions[f.indices[0]].x, velocities[f.indices[0]].x);
    CHECK(!report.has_fatal && f.contact && fabs(f.elapsed-.01) < 1e-10);
    rohr_engine_stop();
}

int main(void) {
    for(unsigned angle = 0; angle < 2; ++angle)
        for(unsigned offset = 0; offset < 2; ++offset)
            for(unsigned rebound = 0; rebound < 2; ++rebound)
                impact(angle*37, offset*100, rebound*.8f, false);
    impact(0, 0, 0, true);
    turning_path(false);
    turning_path(true);
    puts("PASS particle sweep: high-speed circle contacts, rebound, translated/rotated scenes and grazing impact");
    return 0;
}
