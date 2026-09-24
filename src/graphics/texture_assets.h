/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef TEXTURE_ASSETS_H
#define TEXTURE_ASSETS_H

#include "graphics.h"

typedef struct GraphicsTextureAssetStats {
    size_t live_resources;
    size_t owner_references;
    size_t command_references;
} GraphicsTextureAssetStats;

EngineResult graphics_texture_assets_init(void);
void graphics_texture_assets_clear(void);
void graphics_texture_assets_destroy(void);
void graphics_texture_assets_renderer_set(SDL_Renderer *renderer);
GraphicsTextureAssetStats graphics_texture_assets_stats_get(void);

bool graphics_texture_command_reference_add(TextureHandle handle);
void graphics_texture_command_reference_remove(TextureHandle handle);
SDL_Texture *graphics_texture_native_get(TextureHandle handle);

#endif
