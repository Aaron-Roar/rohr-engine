/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef CAR_CONTROLS_H
#define CAR_CONTROLS_H
#include "project.h"

/* A press accepted on the ground may wait for the next physics tick only. */
typedef struct CarControls {
    bool jump_pending, scripted;
    Entity scripted_jump_force; /* Owned until the scripted physics tick ends. */
} CarControls;
bool car_grounded_check(const Car *car);
void car_jump_request(CarControls *controls, const Car *car, bool pressed);
EngineResult car_controls_update(CarControls *controls, const Car *car, float drive_axis);
EngineResult car_controls_step_end(CarControls *controls);
#endif
