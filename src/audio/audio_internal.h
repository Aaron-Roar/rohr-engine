/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef AUDIO_INTERNAL_H
#define AUDIO_INTERNAL_H

#include <stddef.h>

typedef struct AudioOwnershipStats {
    size_t sound_players;
    size_t sound_resources;
    size_t sound_references;
    size_t music_instances;
} AudioOwnershipStats;

AudioOwnershipStats audio_ownership_stats_get(void);

#endif
