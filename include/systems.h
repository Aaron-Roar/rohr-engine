/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef SYSTEMS_H
#define SYSTEMS_H
#include "entity_components.h"
#include "physics.h"

/** Advance engine time and clear entities whose lifetimes have expired. */
Tick system_tick_update(void);

/**
 * Run the physics pipeline for one frame.
 *
 * Prepare animated hitboxes, then apply loads, integrate, gather contacts and
 * joints, and solve constraints for each configured substep. Returns owned
 * timings, statistics, and diagnostics. A fatal report describes partial work;
 * existing quarantine behavior is preserved. Zero dt still gathers and solves.
 *
 * @param dt Delta time in seconds.
 */
Report system_physics_update(double dt);

/**
 * Delete entities whose lifetime has expired.
 */
void system_entities_past_lifetime_clean(void);
#endif
