/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_RUNTIME_NAME_H
#define ROHR_RUNTIME_NAME_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Creation tries the exact base, then base_1, base_2, ... in its own registry.
 * Never strip an authored suffix or silently truncate a requested name. */
static inline bool runtime_name_candidate_get(char *name, size_t capacity,
        const char *base, uint32_t suffix) {
    if(!base || !base[0] || !memchr(base, '\0', capacity)) return false;
    int length = suffix ? snprintf(name, capacity, "%s_%u", base, suffix) :
        snprintf(name, capacity, "%s", base);
    return length >= 0 && (size_t)length < capacity;
}
#endif
