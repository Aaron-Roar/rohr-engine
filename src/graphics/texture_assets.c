/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/texture_assets.h"

#include <limits.h>
#include <string.h>

#define TEXTURE_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define TEXTURE_HANDLE_GENERATION_SHIFT 16

typedef struct GraphicsTextureResource {
    SDL_Texture *texture;
    char *path;
    TextureHandle handle;
    size_t references;
    size_t command_references;
    bool used;
} GraphicsTextureResource;

static GraphicsTextureResource texture_resources[MAX_TEXTURE_ASSETS];
static uint16_t texture_generations[MAX_TEXTURE_ASSETS];
static SDL_Renderer *texture_renderer;
static bool texture_assets_initialized;

static TextureHandle graphics_texture_handle_create(size_t slot) {
    return ((uint32_t)texture_generations[slot]
        << TEXTURE_HANDLE_GENERATION_SHIFT) | (uint32_t)(slot + 1);
}

static GraphicsTextureResource *graphics_texture_resource_get(
        TextureHandle handle, bool allow_unowned) {
    uint32_t stored_slot;
    size_t slot;

    if(handle == TEXTURE_HANDLE_INVALID) return NULL;
    stored_slot = handle & TEXTURE_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= MAX_TEXTURE_ASSETS || !texture_resources[slot].used ||
            texture_resources[slot].handle != handle ||
            (!allow_unowned && texture_resources[slot].references == 0)) return NULL;
    return &texture_resources[slot];
}

static void graphics_texture_resource_destroy(GraphicsTextureResource *resource) {
    if(resource == NULL || !resource->used) return;
    if(resource->texture != NULL) SDL_DestroyTexture(resource->texture);
    SDL_free(resource->path);
    *resource = (GraphicsTextureResource){0};
}

static void graphics_texture_resource_unused_destroy(
        GraphicsTextureResource *resource) {
    if(resource != NULL && resource->references == 0 &&
            resource->command_references == 0)
        graphics_texture_resource_destroy(resource);
}

static bool graphics_texture_path_absolute_check(const char *path) {
    if(path == NULL || path[0] == '\0') return false;
    return path[0] == '/' || path[0] == '\\' ||
        (((path[0] >= 'A' && path[0] <= 'Z') ||
          (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':');
}

static bool graphics_texture_path_normalize(char *path) {
    size_t *segments;
    size_t length;
    size_t read = 0;
    size_t written = 0;
    size_t segment_count = 0;

    if(path == NULL) return false;
    length = strlen(path);
    segments = SDL_malloc((length + 1) * sizeof(*segments));
    if(segments == NULL) return false;
    for(size_t i = 0; i < length; i += 1)
        if(path[i] == '\\') path[i] = '/';
    if(length >= 2 && path[1] == ':') {
        path[written++] = path[read++];
        path[written++] = path[read++];
        if(path[read] == '/') path[written++] = path[read++];
    } else if(path[0] == '/') {
        path[written++] = '/';
        read += 1;
        if(path[read] == '/') {
            path[written++] = '/';
            read += 1;
        }
    }
    while(read < length) {
        size_t start;
        size_t amount;
        size_t rollback;
        while(read < length && path[read] == '/') read += 1;
        start = read;
        while(read < length && path[read] != '/') read += 1;
        amount = read - start;
        if(amount == 0 || (amount == 1 && path[start] == '.')) continue;
        if(amount == 2 && path[start] == '.' && path[start + 1] == '.') {
            if(segment_count > 0) written = segments[--segment_count];
            continue;
        }
        rollback = written;
        if(written > 0 && path[written - 1] != '/') path[written++] = '/';
        segments[segment_count++] = rollback;
        memmove(path + written, path + start, amount);
        written += amount;
    }
    if(written > 1 && path[written - 1] == '/') written -= 1;
    path[written] = '\0';
    SDL_free(segments);
    return true;
}

static char *graphics_texture_path_resolve(const char *path) {
    char *directory = NULL;
    char *resolved = NULL;
    size_t length;

    if(path == NULL || path[0] == '\0') return NULL;
    if(graphics_texture_path_absolute_check(path)) {
        resolved = SDL_strdup(path);
    } else {
        directory = SDL_GetCurrentDirectory();
        if(directory == NULL) return NULL;
        if(SDL_asprintf(&resolved, "%s%s%s", directory,
                directory[0] != '\0' &&
                    directory[strlen(directory) - 1] != '/' &&
                    directory[strlen(directory) - 1] != '\\' ? "/" : "",
                path) < 0) resolved = NULL;
        SDL_free(directory);
    }
    if(resolved == NULL) return NULL;
    length = strlen(resolved);
    if(length == 0 || !graphics_texture_path_normalize(resolved)) {
        SDL_free(resolved);
        return NULL;
    }
    return resolved;
}

EngineResult graphics_texture_assets_init(void) {
    if(texture_assets_initialized) return error_result_value(true);
    memset(texture_resources, 0, sizeof(texture_resources));
    texture_renderer = NULL;
    texture_assets_initialized = true;
    return error_result_value(true);
}

void graphics_texture_assets_clear(void) {
    for(size_t i = 0; i < MAX_TEXTURE_ASSETS; i += 1)
        graphics_texture_resource_destroy(&texture_resources[i]);
}

void graphics_texture_assets_destroy(void) {
    graphics_texture_assets_clear();
    texture_renderer = NULL;
    texture_assets_initialized = false;
}

void graphics_texture_assets_renderer_set(SDL_Renderer *renderer) {
    texture_renderer = renderer;
}

GraphicsTextureAssetStats graphics_texture_assets_stats_get(void) {
    GraphicsTextureAssetStats stats = {0};
    for(size_t slot = 0; slot < MAX_TEXTURE_ASSETS; slot += 1) {
        if(!texture_resources[slot].used) continue;
        stats.live_resources += 1;
        stats.owner_references += texture_resources[slot].references;
        stats.command_references += texture_resources[slot].command_references;
    }
    return stats;
}

TextureAssetResult graphics_texture_load(TextureDescriptor descriptor) {
    TextureAsset asset = {.size = descriptor.size};
    GraphicsTextureResource *resource;
    SDL_Surface *surface;
    char *path;
    size_t slot;

    if(!texture_assets_initialized || texture_renderer == NULL)
        return ERROR_RESULT_MAKE_ERROR(
            TextureAssetResult, ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    path = graphics_texture_path_resolve(descriptor.file);
    if(path == NULL) {
        error_detail_set(ERROR_ENGINE_TEXTURE_LOAD_FAILED,
            descriptor.file == NULL ? "texture path is null" :
                "texture path resolution failed");
        return ERROR_RESULT_MAKE_ERROR(
            TextureAssetResult, ERROR_ENGINE_TEXTURE_LOAD_FAILED);
    }
    for(slot = 0; slot < MAX_TEXTURE_ASSETS; slot += 1) {
        resource = &texture_resources[slot];
        if(resource->used && resource->references > 0 &&
                strcmp(resource->path, path) == 0) {
            if(resource->references == SIZE_MAX) {
                SDL_free(path);
                return ERROR_RESULT_MAKE_ERROR(
                    TextureAssetResult, ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED);
            }
            resource->references += 1;
            asset.handle = resource->handle;
            SDL_free(path);
            return ERROR_RESULT_MAKE_VALUE(TextureAssetResult, asset);
        }
    }
    for(slot = 0; slot < MAX_TEXTURE_ASSETS; slot += 1)
        if(!texture_resources[slot].used) break;
    if(slot == MAX_TEXTURE_ASSETS) {
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            TextureAssetResult, ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED);
    }
    surface = SDL_LoadPNG(path);
    if(surface == NULL) {
        const char *detail = SDL_GetError();
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_TEXTURE_LOAD_FAILED, detail);
        return ERROR_RESULT_MAKE_ERROR(
            TextureAssetResult, ERROR_ENGINE_TEXTURE_LOAD_FAILED);
    }
    resource = &texture_resources[slot];
    resource->texture = SDL_CreateTextureFromSurface(texture_renderer, surface);
    SDL_DestroySurface(surface);
    if(resource->texture == NULL) {
        const char *detail = SDL_GetError();
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_TEXTURE_LOAD_FAILED, detail);
        return ERROR_RESULT_MAKE_ERROR(
            TextureAssetResult, ERROR_ENGINE_TEXTURE_LOAD_FAILED);
    }
    texture_generations[slot] += 1;
    if(texture_generations[slot] == 0) texture_generations[slot] = 1;
    resource->path = path;
    resource->handle = graphics_texture_handle_create(slot);
    resource->references = 1;
    resource->used = true;
    asset.handle = resource->handle;
    return ERROR_RESULT_MAKE_VALUE(TextureAssetResult, asset);
}

EngineResult graphics_texture_retain(TextureAsset asset) {
    GraphicsTextureResource *resource =
        graphics_texture_resource_get(asset.handle, false);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_TEXTURE_NOT_FOUND);
    if(resource->references == SIZE_MAX)
        return error_result_error(ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED);
    resource->references += 1;
    return error_result_value(true);
}

EngineResult graphics_texture_release(TextureAsset *asset) {
    GraphicsTextureResource *resource;

    if(asset == NULL) return error_result_error(ERROR_ENGINE_TEXTURE_NOT_FOUND);
    if(asset->handle == TEXTURE_HANDLE_INVALID) {
        *asset = (TextureAsset){0};
        return error_result_value(true);
    }
    resource = graphics_texture_resource_get(asset->handle, false);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_TEXTURE_NOT_FOUND);
    resource->references -= 1;
    *asset = (TextureAsset){0};
    graphics_texture_resource_unused_destroy(resource);
    return error_result_value(true);
}

bool graphics_texture_valid_check(TextureAsset asset) {
    return graphics_texture_resource_get(asset.handle, false) != NULL;
}

bool graphics_texture_command_reference_add(TextureHandle handle) {
    GraphicsTextureResource *resource =
        graphics_texture_resource_get(handle, false);
    if(resource == NULL || resource->command_references == SIZE_MAX) return false;
    resource->command_references += 1;
    return true;
}

void graphics_texture_command_reference_remove(TextureHandle handle) {
    GraphicsTextureResource *resource =
        graphics_texture_resource_get(handle, true);
    if(resource == NULL || resource->command_references == 0) return;
    resource->command_references -= 1;
    graphics_texture_resource_unused_destroy(resource);
}

SDL_Texture *graphics_texture_native_get(TextureHandle handle) {
    GraphicsTextureResource *resource =
        graphics_texture_resource_get(handle, true);
    return resource == NULL ? NULL : resource->texture;
}
