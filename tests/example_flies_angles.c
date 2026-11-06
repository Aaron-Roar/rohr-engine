/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
/* Exercise the control used by the example, with its bundled scene. */
#define main flies_example_main
#include "../examples/flies-in-pit/src/main.c"
#undef main
#include <math.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "flies angles line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))

static bool particles_check(void) {
    EntityIndex particles[499];
    unsigned count = 0, overlapping_pairs = 0, failures = 0;
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    OK(rohr_game_state_file_load("assets/flies-in-pit/game.json"));
    CHECK(rohr_entity_alive_count_get() == 503);
    for(unsigned i = 0; i < rohr_entity_alive_count_get(); ++i) {
        EntityResult entity = rohr_entity_alive_at_get(i); OK(entity);
        if(!rohr_entity_components_check(entity.result.value, ROHR_PARTICLE)) continue;
        ParticleDescriptionResult result =
            rohr_physics_particle_description_get(entity.result.value); OK(result);
        ParticleDescription d = result.result.value;
        CHECK(count < 499);
        EntityIndexResult index = rohr_entity_index_get(entity.result.value); OK(index);
        particles[count++] = index.result.value;
        CHECK(fabsf(d.radius - 4.681777477264404f) < 0.00001f);
        CHECK(d.local_origin.x == 0 && d.local_origin.y == 0);
        CHECK(d.rigid_vertices == 3 && d.mass_value == 1);
        CHECK(d.collision_enabled && !d.static_body);
        CHECK(fabsf(d.friction - 0.4f) < 0.00001f);
        CHECK(fabsf(d.restitution - 0.2f) < 0.00001f);
        CHECK(d.velocity.x == 0 && d.velocity.y == 3);
        CHECK(d.acceleration.x == 0 && d.acceleration.y == -50);
        CHECK(fabsf(hypotf(d.position.x - 9, d.position.y - 122) - 80) < 0.001f);
    }
    CHECK(count == 499);
    /* Keep the authored overlapping stress spawn, not a separated particle grid. */
    for(unsigned i = 0; i < count; ++i)
        for(unsigned j = i + 1; j < count; ++j) {
            Position a = positions[particles[i]], b = positions[particles[j]];
            overlapping_pairs += hypotf(a.x - b.x, a.y - b.y) < 2 * 4.681777477264404f;
        }
    CHECK(overlapping_pairs == 4491);
    /* Complete one simulated second at the example's tick rate even if an
     * orientation check fails; fatal or invalid state makes it incomplete. */
    for(unsigned tick = 0; tick < 120; ++tick) {
        Report report = rohr_system_physics_update(1.0 / 120.0);
        CHECK(!report.has_fatal && report.header.status != ROHR_REPORT_FAILED);
        for(unsigned i = 0; i < count; ++i) {
            EntityIndex index = particles[i];
            CHECK(isfinite(positions[index].x) && isfinite(positions[index].y));
            CHECK(isfinite(velocities[index].x) && isfinite(velocities[index].y));
            failures += orientations[index] != 0 || angular_velocities[index] != 0;
        }
    }
    rohr_graphics_stop();
    rohr_engine_stop();
    fprintf(stderr, "flies particles: endpoint=120 ticks, rotation failures=%u\n", failures);
    return failures == 0;
}

static bool turn_check(float direction) {
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    OK(rohr_game_state_file_load("assets/flies-in-pit/game.json"));
    EntityResult wall = rohr_entity_by_name_get("wall_1"); OK(wall);
    EntityIndex wi = rohr_entity_index_get(wall.result.value).result.value;
    CHECK(fabsf(orientations[wi] + 120.0f) < 0.001f);
    /* This authored local-up direction pointed down-left before conversion. */
    Vec2D up = rohr_math_vector_rotate((Vec2D){0,1}, orientations[wi]);
    CHECK(fabsf(up.x + 0.8660254f) < 0.001f && fabsf(up.y + 0.5f) < 0.001f);

    EntityResult fly = rohr_entity_by_name_get("large_fly"); OK(fly);
    CHECK(!rohr_entity_components_check(fly.result.value, ROHR_PARTICLE));
    EntityIndex fi = rohr_entity_index_get(fly.result.value).result.value;
    OK(rohr_physics_collision_response_set(fly.result.value, ROHR_COLLISION_CATEGORY_NONE));
    OK(rohr_physics_angular_velocity_set(fly.result.value, 0));
    MomentOfInertiaResult inertia = rohr_physics_moment_of_inertia_get(fly.result.value);
    OK(inertia);
    OK(fly_turn_apply(fly.result.value, direction));
    CHECK((rohr_system_physics_update(0.1)).header.status != ROHR_REPORT_FAILED);
    float rate = rohr_math_degrees_to_radians(angular_velocities[fi]);
    CHECK(rate * direction > 0);
    CHECK(fabsf(rate - direction * large_fly_control_torque * 0.1f /
        inertia.result.value) < 0.001f);
    up = rohr_math_vector_rotate((Vec2D){0,1}, orientations[fi]);
    CHECK(up.x * direction > 0); /* Right input turns the fly's nose right. */
    rohr_graphics_stop();
    rohr_engine_stop();
    return true;
}

int main(void) {
    bool particles = particles_check();
    bool right = turn_check(1);
    bool left = turn_check(-1);
    return particles && right && left ? 0 : 1;
}
