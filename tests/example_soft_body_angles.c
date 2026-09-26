/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
/* Use the wheel construction and drive control from the actual example. */
#define main soft_body_example_main
#include "../examples/soft-body/src/main.c"
#undef main

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "wheel angles line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))

static bool drive_check(float direction) {
    OK(rohr_engine_start());
    Entity chassis = rigid_body_create((Position){0,50},
        rohr_math_square_create(20,10), chassis_mass, chassis_category, 0);
    CHECK(chassis != ENTITY_INVALID);
    Wheel wheel = {0};
    CHECK(wheel_create(&wheel, chassis, (Position){0,0}, (Vec2D){0,-50}));
    AngularVelocityResult maximum = rohr_physics_angular_velocity_maximum_get(wheel.disk);
    OK(maximum);
    CHECK(fabsf(rohr_math_degrees_to_radians(maximum.result.value) - 7.0f) < 0.001f);
    OK(wheel_drive_apply(wheel.disk, direction));
    OK(rohr_system_physics_update(physics_tick_time));
    AngularVelocityResult rate = rohr_physics_angular_velocity_get(wheel.disk); OK(rate);
    CHECK(rate.result.value * direction > 100.0f);
    CHECK(fabsf(rate.result.value) <= maximum.result.value + 0.001f);
    Vec2D rim_velocity = rohr_math_angular_velocity_cross_vec(rate.result.value,
        (Vec2D){0, disk_radius});
    CHECK(rim_velocity.x * direction > 0); /* D drives the top of the wheel right. */
    rohr_engine_stop();
    return true;
}

int main(void) {
    return drive_check(1) && drive_check(-1) ? 0 : 1;
}
