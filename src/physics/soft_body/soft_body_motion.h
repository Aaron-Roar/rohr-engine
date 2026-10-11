/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef SOFT_BODY_MOTION_H
#define SOFT_BODY_MOTION_H
#include "physics.h"

/* Borrowed world-space arrays. Allocation-free; output is unchanged on failure.
 * Replace aggregate rotation while retaining translation and deformation. */
bool physics_soft_angular_velocities_get(const Position *points, const Mass *masses,
    const Velocity *prior, uint32_t count, AngularVelocity degrees, Velocity *output);
AngularVelocityResult physics_soft_body_angular_velocity_get(EntityIndex body);
EngineResult physics_soft_body_angular_velocity_set(EntityIndex body, AngularVelocity degrees);
#endif
