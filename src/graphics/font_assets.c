/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/font_assets.h"

#include "graphics/asset_path.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define FONT_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define FONT_HANDLE_GENERATION_SHIFT 16

typedef struct GraphicsFontResource {
    TTF_Font *font;
    char *path;
    float point_size;
    FontAsset asset;
    size_t references;
    bool pinned;
    bool used;
} GraphicsFontResource;

static GraphicsFontResource font_resources[MAX_FONT_ASSETS];
static uint16_t font_generations[MAX_FONT_ASSETS];
static FontAsset default_font;
static bool font_assets_initialized;
static bool font_assets_started;

static FontHandle graphics_font_handle_create(size_t slot) {
    return ((uint32_t)font_generations[slot] << FONT_HANDLE_GENERATION_SHIFT) |
        (uint32_t)(slot + 1);
}

static GraphicsFontResource *graphics_font_resource_get(FontHandle handle) {
    uint32_t stored_slot;
    size_t slot;

    if(handle == FONT_HANDLE_INVALID) return NULL;
    stored_slot = handle & FONT_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= MAX_FONT_ASSETS || !font_resources[slot].used ||
            font_resources[slot].asset.handle != handle ||
            (!font_resources[slot].pinned &&
                font_resources[slot].references == 0)) return NULL;
    return &font_resources[slot];
}

static void graphics_font_resource_destroy(GraphicsFontResource *resource) {
    if(resource == NULL || !resource->used) return;
    if(resource->font != NULL) TTF_CloseFont(resource->font);
    SDL_free(resource->path);
    *resource = (GraphicsFontResource){0};
}

EngineResult graphics_font_assets_init(void) {
    if(font_assets_initialized) return error_result_value(true);
    memset(font_resources, 0, sizeof(font_resources));
    default_font = (FontAsset){0};
    font_assets_started = false;
    font_assets_initialized = true;
    return error_result_value(true);
}

EngineResult graphics_font_assets_start(void) {
    size_t slot;

    if(!font_assets_initialized)
        return error_result_error(ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    if(font_assets_started) return error_result_value(true);
    for(slot = 0; slot < MAX_FONT_ASSETS; slot += 1)
        if(!font_resources[slot].used) break;
    if(slot == MAX_FONT_ASSETS)
        return error_result_error(ERROR_ENGINE_FONT_CAPACITY_EXCEEDED);
    font_generations[slot] += 1;
    if(font_generations[slot] == 0) font_generations[slot] = 1;
    default_font = (FontAsset){.handle = graphics_font_handle_create(slot)};
    font_resources[slot] = (GraphicsFontResource){
        .asset = default_font,
        .pinned = true,
        .used = true,
    };
    font_assets_started = true;
    return error_result_value(true);
}

void graphics_font_assets_clear(void) {
    for(size_t slot = 0; slot < MAX_FONT_ASSETS; slot += 1)
        graphics_font_resource_destroy(&font_resources[slot]);
    default_font = (FontAsset){0};
    font_assets_started = false;
}

void graphics_font_assets_destroy(void) {
    graphics_font_assets_clear();
    font_assets_initialized = false;
}

GraphicsFontAssetStats graphics_font_assets_stats_get(void) {
    GraphicsFontAssetStats stats = {0};
    for(size_t slot = 0; slot < MAX_FONT_ASSETS; slot += 1) {
        if(!font_resources[slot].used) continue;
        stats.live_resources += 1;
        stats.owner_references += font_resources[slot].references;
        if(font_resources[slot].pinned) stats.pinned_resources += 1;
    }
    return stats;
}

FontAssetResult graphics_font_load(FontDescriptor descriptor) {
    GraphicsFontResource *resource;
    char detail[256];
    char *path;
    TTF_Font *font;
    size_t slot;

    if(!font_assets_initialized || !font_assets_started)
        return ERROR_RESULT_MAKE_ERROR(
            FontAssetResult, ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    if(!isfinite(descriptor.point_size) || descriptor.point_size <= 0.0f) {
        error_detail_set(ERROR_ENGINE_FONT_LOAD_FAILED,
            "font point size must be finite and positive");
        return ERROR_RESULT_MAKE_ERROR(
            FontAssetResult, ERROR_ENGINE_FONT_LOAD_FAILED);
    }
    path = graphics_asset_path_resolve(descriptor.file);
    if(path == NULL) {
        error_detail_set(ERROR_ENGINE_FONT_LOAD_FAILED,
            descriptor.file == NULL ? "font path is null" :
                "font path resolution failed");
        return ERROR_RESULT_MAKE_ERROR(
            FontAssetResult, ERROR_ENGINE_FONT_LOAD_FAILED);
    }
    for(slot = 0; slot < MAX_FONT_ASSETS; slot += 1) {
        resource = &font_resources[slot];
        if(resource->used && !resource->pinned && resource->references > 0 &&
                resource->point_size == descriptor.point_size &&
                strcmp(resource->path, path) == 0) {
            if(resource->references == SIZE_MAX) {
                SDL_free(path);
                return ERROR_RESULT_MAKE_ERROR(FontAssetResult,
                    ERROR_ENGINE_FONT_CAPACITY_EXCEEDED);
            }
            resource->references += 1;
            SDL_free(path);
            return ERROR_RESULT_MAKE_VALUE(FontAssetResult, resource->asset);
        }
    }
    for(slot = 0; slot < MAX_FONT_ASSETS; slot += 1)
        if(!font_resources[slot].used) break;
    if(slot == MAX_FONT_ASSETS) {
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            FontAssetResult, ERROR_ENGINE_FONT_CAPACITY_EXCEEDED);
    }
    font = TTF_OpenFont(path, descriptor.point_size);
    if(font == NULL) {
        snprintf(detail, sizeof(detail), "font path '%s': %s",
            descriptor.file, SDL_GetError());
        error_detail_set(ERROR_ENGINE_FONT_LOAD_FAILED, detail);
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            FontAssetResult, ERROR_ENGINE_FONT_LOAD_FAILED);
    }
    font_generations[slot] += 1;
    if(font_generations[slot] == 0) font_generations[slot] = 1;
    font_resources[slot] = (GraphicsFontResource){
        .font = font,
        .path = path,
        .point_size = descriptor.point_size,
        .asset = {.handle = graphics_font_handle_create(slot)},
        .references = 1,
        .used = true,
    };
    return ERROR_RESULT_MAKE_VALUE(FontAssetResult,
        font_resources[slot].asset);
}

FontAsset graphics_font_default_get(void) {
    return font_assets_started ? default_font : (FontAsset){0};
}

EngineResult graphics_font_retain(FontAsset asset) {
    GraphicsFontResource *resource =
        graphics_font_resource_get(asset.handle);
    if(resource == NULL)
        return error_result_error(ERROR_ENGINE_FONT_NOT_FOUND);
    if(resource->pinned) return error_result_value(true);
    if(resource->references == SIZE_MAX)
        return error_result_error(ERROR_ENGINE_FONT_CAPACITY_EXCEEDED);
    resource->references += 1;
    return error_result_value(true);
}

EngineResult graphics_font_release(FontAsset *asset) {
    GraphicsFontResource *resource;

    if(asset == NULL) return error_result_error(ERROR_ENGINE_FONT_NOT_FOUND);
    if(asset->handle == FONT_HANDLE_INVALID) {
        *asset = (FontAsset){0};
        return error_result_value(true);
    }
    resource = graphics_font_resource_get(asset->handle);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_FONT_NOT_FOUND);
    if(!resource->pinned) {
        resource->references -= 1;
        if(resource->references == 0) graphics_font_resource_destroy(resource);
    }
    *asset = (FontAsset){0};
    return error_result_value(true);
}

bool graphics_font_valid_check(FontAsset asset) {
    return graphics_font_resource_get(asset.handle) != NULL;
}

bool graphics_font_builtin_check(FontAsset asset) {
    GraphicsFontResource *resource =
        graphics_font_resource_get(asset.handle);
    return resource != NULL && resource->pinned;
}

TTF_Font *graphics_font_native_get(FontAsset asset) {
    GraphicsFontResource *resource =
        graphics_font_resource_get(asset.handle);
    return resource == NULL || resource->pinned ? NULL : resource->font;
}
