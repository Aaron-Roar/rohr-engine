/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef SOFT_BODY_MASS_H
#define SOFT_BODY_MASS_H
#include "physics.h"

/* Borrowed arrays in one coordinate system. No allocation or entity mutation.
 * Output is written only after the entire operation has been validated. */
bool physics_soft_mass_distribution_get(const Position *points, const Mass *prior,
    uint32_t count, Position target, Mass *output);
bool physics_soft_mass_scale_get(const Mass *prior, uint32_t count,
    Mass total, Mass *output);
MassResult physics_soft_body_mass_get(EntityIndex body);
PositionResult physics_soft_body_com_get(EntityIndex body);
EngineResult physics_soft_body_mass_set(EntityIndex body, Mass total);
EngineResult physics_soft_body_com_set(EntityIndex body, Position world);
#endif
