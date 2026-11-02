/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "profile_clock.h"
#include <assert.h>
int main(void) {
    MeasurementClock clock = {0}; MeasurementMetric metrics[3] = {{0}};
    measurement_enter(&clock, 0, 10);
    measurement_enter(&clock, 1, 20);
    measurement_enter(&clock, 1, 25); /* Recursive work must be subtracted once. */
    measurement_leave(&clock, metrics, 30);
    measurement_leave(&clock, metrics, 40);
    measurement_enter(&clock, 2, 45);
    measurement_leave(&clock, metrics, 60);
    measurement_leave(&clock, metrics, 100);
    assert(!clock.invalid && clock.depth == 0);
    assert(metrics[0].inclusive_ns == 90 && metrics[0].exclusive_ns == 55);
    assert(metrics[1].calls == 2 && metrics[1].inclusive_ns == 25 && metrics[1].exclusive_ns == 20);
    assert(metrics[2].exclusive_ns == 15);
    assert(metrics[0].exclusive_ns + metrics[1].exclusive_ns + metrics[2].exclusive_ns == 90);
    measurement_leave(&clock, metrics, 101);
    assert(clock.invalid);
    clock = (MeasurementClock){0};
    for(unsigned i = 0; i <= MEASUREMENT_STACK_CAPACITY; ++i) measurement_enter(&clock, 0, i);
    assert(clock.invalid);
    clock = (MeasurementClock){0};
    measurement_enter(&clock, 0, 10); measurement_leave(&clock, metrics, 9);
    assert(clock.invalid);
    return 0;
}
