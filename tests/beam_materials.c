/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define OK(value) assert(!rohr_error_check(value))
static void close_to(float value, float expected) {
    if(fabsf(value - expected) >= .02f)
        fprintf(stderr, "Expected velocity %.3f, got %.3f\n", expected, value);
    assert(fabsf(value - expected) < .02f);
}
static void impact(float x, float edge_bounce, float target_bounce,
        float edge_friction, float expected_x, float expected_y, bool node_contact) {
    static float scene_x = 100;
    float offset = scene_x; scene_x += 100;
    EntityResult soft = rohr_physics_soft_body_create(); OK(soft);
    EntityResult a = rohr_physics_soft_body_node_create(soft.result.value, (Position){offset - 10, 0}, 1, 1);
    EntityResult b = rohr_physics_soft_body_node_create(soft.result.value, (Position){offset + 10, 0}, 1, 1);
    OK(a); OK(b);
    OK(rohr_physics_static_set(a.result.value)); OK(rohr_physics_static_set(b.result.value));
    OK(rohr_physics_restitution_set(a.result.value, node_contact ? .1f : 0));
    OK(rohr_physics_restitution_set(b.result.value, 0));
    EntityResult beam = rohr_physics_soft_body_beam_create(soft.result.value, a.result.value, b.result.value, 0, 0);
    OK(beam);
    OK(rohr_physics_restitution_set(beam.result.value, edge_bounce));
    OK(rohr_physics_friction_set(beam.result.value, edge_friction));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.position = (Position){offset + x, 1.99f}; config.radius = 1;
    config.velocity = (Velocity){6, -10}; config.mass_value = 1;
    config.restitution = target_bounce; config.friction = 1;
    config.gravity_enabled = false;
    EntityResult particle = rohr_physics_particle_create(config); OK(particle);
    rohr_system_physics_update(0);
    EntityIndexResult index = rohr_entity_index_get(particle.result.value); OK(index);
    if(node_contact) {
        ContactInfo contact = rohr_physics_contact_get(a.result.value, particle.result.value);
        assert(contact.detected);
        EntityIndexResult node_index = rohr_entity_index_get(a.result.value); OK(node_index);
        assert(restitutions[node_index.result.value] == .1f && frictions[node_index.result.value] == 0);
        /* The octagonal node's contact normal may change during correction.
         * It still produces a small rebound using its own .1 restitution,
         * rather than the beam's perfectly elastic material. */
        assert(velocities[index.result.value].y > 0 && velocities[index.result.value].y < 3);
    } else {
        if(fabsf(velocities[index.result.value].x - expected_x) >= .02f ||
                fabsf(velocities[index.result.value].y - expected_y) >= .02f)
        {
            ContactInfo c = rohr_physics_contact_get(beam.result.value, particle.result.value);
            fprintf(stderr, "Impact x=%g, beam restitution=%g, particle restitution=%g, friction=%g, normal=(%g,%g), velocity=(%g,%g)\n",
                x, edge_bounce, target_bounce, edge_friction, c.normal.x, c.normal.y,
                velocities[index.result.value].x, velocities[index.result.value].y);
        }
        close_to(velocities[index.result.value].x, expected_x);
        close_to(velocities[index.result.value].y, expected_y);
        assert(rohr_physics_contact_check(beam.result.value, particle.result.value));
    }
}
int main(void) {
    OK(rohr_engine_start());
    EntityResult soft = rohr_physics_soft_body_create(); OK(soft);
    EntityResult a = rohr_physics_soft_body_node_create(soft.result.value, (Position){-10, 0}, 1, 1);
    EntityResult b = rohr_physics_soft_body_node_create(soft.result.value, (Position){10, 0}, 1, 1);
    OK(a); OK(b);
    OK(rohr_physics_restitution_set(a.result.value, 1));
    OK(rohr_physics_friction_set(b.result.value, 2));
    EntityResult created = rohr_physics_soft_body_beam_create(soft.result.value, a.result.value, b.result.value, 0, 0);
    OK(created); Entity beam = created.result.value;
    SoftBodyBeamResult value = rohr_physics_soft_body_beam_get(beam); OK(value);
    assert(value.result.value.friction == 0 && value.result.value.restitution == .25f);
    OK(rohr_physics_soft_body_beam_collision_disable(beam));
    OK(rohr_physics_friction_set(beam, .75f)); OK(rohr_physics_restitution_set(beam, .8f));
    /* Beam materials neither enable collision nor create rigid collision components. */
    assert(!rohr_entity_components_check(beam, ROHR_COLLISION));
    OK(rohr_physics_restitution_set(a.result.value, 0));
    OK(rohr_physics_restitution_set(b.result.value, 1));
    float invalid[] = {-1, NAN, INFINITY, -INFINITY};
    for(size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i += 1) {
        assert(rohr_error_check(rohr_physics_friction_set(beam, invalid[i])));
        assert(rohr_error_check(rohr_physics_restitution_set(beam, invalid[i])));
    }
    assert(rohr_error_check(rohr_physics_restitution_set(beam, 1.1f)));
    value = rohr_physics_soft_body_beam_get(beam); OK(value);
    assert(value.result.value.friction == .75f && value.result.value.restitution == .8f);
    assert(!value.result.value.collision_enabled);
    OK(rohr_physics_soft_body_beam_collision_enable(beam));
    value = rohr_physics_soft_body_beam_get(beam); OK(value);
    assert(value.result.value.friction == .75f && value.result.value.restitution == .8f);
    OK(rohr_entity_delete(beam));
    assert(rohr_error_check(rohr_physics_friction_set(beam, 1)));
    assert(rohr_error_check(rohr_physics_restitution_set(beam, 1)));
    /* Interior impacts use the beam regardless of contact position or node materials. */
    impact(-5, 1, 1, 0, 6, 10, false);
    impact(0, 1, 1, 0, 6, 10, false);
    impact(5, 1, 1, 0, 6, 10, false);
    impact(0, 0, 1, 0, 6, 0, false);
    impact(0, 1, .25f, 0, 6, 2.5f, false);
    impact(0, 1, 1, 1, 0, 10, false);
    /* Node-owned contacts keep their own material, including zero node friction. */
    impact(-10, 1, 1, 1, 0, 0, true);
    rohr_engine_stop();
    return 0;
}
