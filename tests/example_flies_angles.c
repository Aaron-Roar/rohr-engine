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
    EntityIndex fi = rohr_entity_index_get(fly.result.value).result.value;
    OK(rohr_physics_collision_with_set(fly.result.value, ROHR_COLLISION_CATEGORY_NONE));
    OK(rohr_physics_angular_velocity_set(fly.result.value, 0));
    MomentOfInertiaResult inertia = rohr_physics_moment_of_inertia_get(fly.result.value);
    OK(inertia);
    OK(fly_turn_apply(fly.result.value, direction));
    OK(rohr_system_physics_update(0.1));
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
    return turn_check(1) && turn_check(-1) ? 0 : 1;
}
