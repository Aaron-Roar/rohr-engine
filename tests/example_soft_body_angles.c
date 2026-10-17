/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "car_controls.h"
#include <math.h>
#include <assert.h>
#define OK(c) assert(!rohr_error_check(c))
static void drive_check(float direction) {
    OK(rohr_engine_start());
    Car car={0};CarControls controls={0};OK(car_create(&car,(Position){0}));
    const float maximum=7*180/PI_F;
    OK(rohr_physics_angular_velocity_maximum_set(car.rear_disk,maximum));
    OK(car_controls_update(&controls,&car,direction));
    assert(rohr_system_physics_update(1.0/240).header.status!=ROHR_REPORT_FAILED);
    AngularVelocityResult rate=rohr_physics_angular_velocity_get(car.rear_disk);OK(rate);
    assert(rate.result.value*direction>100 && fabsf(rate.result.value)<=maximum+0.001f);
    Vec2D rim=rohr_math_angular_velocity_cross_vec(rate.result.value,(Vec2D){0,15});
    assert(rim.x*direction>0);
    OK(car_controls_update(&controls,&car,0));
    TorqueResult magnitude=rohr_physics_torque_magnitude_get(car.torque_rear_drive);OK(magnitude);assert(magnitude.result.value==0);
    car_destroy(&car);rohr_engine_stop();
}
int main(void) { drive_check(1);drive_check(-1);return 0; }
