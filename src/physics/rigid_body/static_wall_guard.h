/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_STATIC_WALL_GUARD_H
#define ROHR_STATIC_WALL_GUARD_H

#include "physics/physics_operation.h"

/* Synchronous scopes nest for complete updates; manual stages own fresh scopes.
 * No wall geometry or handles survive the outer scope. */
void physics_wall_guard_begin(Report *report);
void physics_wall_guard_end(void);
void physics_wall_guard_dirty_add(EntityIndex index);
void physics_wall_guard_change(PhysicsChange *change, Report *report);

/* Internal diagnostic counts only; reset outside simulation, not per child. */
typedef struct PhysicsWallGuardCounts {
    uint64_t builds, queries, candidates, casts, hits, clamped;
    uint64_t prepare_ticks, change_ticks, query_ticks, cast_ticks;
} PhysicsWallGuardCounts;
PhysicsWallGuardCounts physics_wall_guard_counts_get(void);
void physics_wall_guard_counts_clear(void);

#endif
