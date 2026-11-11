/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "car_controls.h"
#include <math.h>
#include <assert.h>
#define OK(c) assert(!rohr_error_check(c))
static void drive_check(float direction) {
    OK(rohr_engine_start());
    Car car={0};CarControls controls={0};
    EntityResult created = rohr_object_create(objects.car); OK(created);
    car.source_instance = created.result.value;
    const char *names[] = {"chassis", "cabin", "rear_hub", "rear_disk", "front_hub", "front_disk", "rear_tire", "front_tire", "rear_drive"};
    Entity *ids[] = {&car.chassis, &car.cabin, &car.rear_hub, &car.rear_disk, &car.front_hub, &car.front_disk, &car.rear_tire, &car.front_tire, &car.torque_rear_drive};
    for(size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        EntityResult found = rohr_entity_by_name_get(names[i]); OK(found);
        *ids[i] = found.result.value;
    }
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
    OK(rohr_entity_delete(car.source_instance));rohr_engine_stop();
}
int main(void) { drive_check(1);drive_check(-1);return 0; }
