/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef COLLISION_EXPERIMENT_H
#define COLLISION_EXPERIMENT_H
#include "rohr.h"

/* Diagnostic-only link wrappers; no shipped engine API or runtime setting. */
typedef struct ExperimentCounts {
    uint64_t integrations, candidates, casts, cast_iterations, impacts;
    uint64_t predictive_rows, correction_rows, initial_overlaps;
    uint64_t unsupported, capacity_limits, search_limits, secondary_risks;
} ExperimentCounts;
ExperimentCounts experiment_counts_get(void);
void experiment_counts_clear(void);

typedef struct ExperimentPose { Position p; float angle; } ExperimentPose;
typedef struct ExperimentContact { float gap; Vec2D normal, point; } ExperimentContact;
ExperimentContact experiment_separation_get(const Shape *a, const Shape *b);
bool experiment_cast_check(EntityIndex a, EntityIndex b,
    ExperimentPose start_a, ExperimentPose end_a,
    ExperimentPose start_b, ExperimentPose end_b,
    double *fraction, ExperimentContact *contact, unsigned *iterations, bool *limited);
#endif
