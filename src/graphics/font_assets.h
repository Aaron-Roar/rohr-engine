/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef FONT_ASSETS_H
#define FONT_ASSETS_H

#include "graphics.h"

typedef struct GraphicsFontAssetStats {
    size_t live_resources;
    size_t owner_references;
    size_t pinned_resources;
} GraphicsFontAssetStats;

EngineResult graphics_font_assets_init(void);
EngineResult graphics_font_assets_start(void);
void graphics_font_assets_clear(void);
void graphics_font_assets_destroy(void);
GraphicsFontAssetStats graphics_font_assets_stats_get(void);

bool graphics_font_builtin_check(FontAsset asset);
TTF_Font *graphics_font_native_get(FontAsset asset);

#endif
