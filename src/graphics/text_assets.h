/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef TEXT_ASSETS_H
#define TEXT_ASSETS_H

#include "graphics.h"

typedef struct GraphicsTextPayload GraphicsTextPayload;

typedef struct GraphicsTextAssetStats {
    size_t live_resources;
    size_t public_owners;
    size_t dependency_references;
    size_t live_payloads;
    size_t command_references;
} GraphicsTextAssetStats;

EngineResult graphics_text_assets_init(void);
void graphics_text_assets_renderer_set(SDL_Renderer *renderer,
    TTF_TextEngine *text_engine);
void graphics_text_assets_clear(void);
void graphics_text_assets_destroy(void);
GraphicsTextAssetStats graphics_text_assets_stats_get(void);

EngineResult graphics_text_dependency_add(TextAsset asset);
void graphics_text_dependency_remove(TextHandle handle);

bool graphics_text_command_reference_add(TextAsset asset,
    GraphicsTextPayload **payload);
void graphics_text_command_reference_remove(GraphicsTextPayload *payload);

GraphicsTextPayload *graphics_text_internal_payload_get(TextHandle handle);
bool graphics_text_internal_size_get(TextHandle handle, Scale *size);
TTF_Text *graphics_text_native_get(TextAsset asset);
bool graphics_text_wrap_width_set(TextAsset *asset, int wrap_width);
TTF_Text *graphics_text_payload_native_get(GraphicsTextPayload *payload);
SDL_Texture *graphics_text_payload_texture_get(GraphicsTextPayload *payload);
bool graphics_text_payload_builtin_check(GraphicsTextPayload *payload);

#endif
