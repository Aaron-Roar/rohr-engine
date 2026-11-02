/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef PHYSICS_MEASUREMENT_CLOCK_H
#define PHYSICS_MEASUREMENT_CLOCK_H
#include <stdbool.h>
#include <stdint.h>

#define MEASUREMENT_STACK_CAPACITY 64
typedef struct MeasurementMetric { uint64_t calls, inclusive_ns, exclusive_ns; } MeasurementMetric;
typedef struct MeasurementFrame { unsigned metric; uint64_t started, children; } MeasurementFrame;
typedef struct MeasurementClock {
    MeasurementFrame stack[MEASUREMENT_STACK_CAPACITY];
    unsigned depth;
    bool invalid;
} MeasurementClock;

/* The caller supplies timestamps, allowing deterministic accounting tests. */
static inline void measurement_enter(MeasurementClock *clock, unsigned metric, uint64_t now) {
    if(clock->invalid) return;
    if(clock->depth == MEASUREMENT_STACK_CAPACITY) { clock->invalid = true; return; }
    clock->stack[clock->depth++] = (MeasurementFrame){metric, now, 0};
}
static inline void measurement_leave(MeasurementClock *clock, MeasurementMetric *metrics, uint64_t now) {
    if(clock->invalid) return;
    if(!clock->depth) { clock->invalid = true; return; }
    MeasurementFrame frame = clock->stack[--clock->depth];
    if(now < frame.started || now - frame.started < frame.children) { clock->invalid = true; return; }
    uint64_t elapsed = now - frame.started;
    MeasurementMetric *metric = &metrics[frame.metric];
    ++metric->calls;
    metric->inclusive_ns += elapsed;
    metric->exclusive_ns += elapsed - frame.children;
    if(clock->depth) clock->stack[clock->depth - 1].children += elapsed;
}
#endif
