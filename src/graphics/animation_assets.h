/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ANIMATION_ASSETS_H
#define ANIMATION_ASSETS_H

#include "graphics.h"

typedef struct GraphicsAnimationAssetStats {
    size_t live_resources;
    size_t owner_references;
    size_t frame_texture_references;
} GraphicsAnimationAssetStats;

EngineResult graphics_animation_assets_init(void);
void graphics_animation_assets_clear(void);
void graphics_animation_assets_destroy(void);
GraphicsAnimationAssetStats graphics_animation_assets_stats_get(void);

#endif
