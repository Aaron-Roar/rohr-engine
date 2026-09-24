/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/text_assets.h"

#include "graphics/font_assets.h"

#include <stdlib.h>
#include <string.h>

#define TEXT_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define TEXT_HANDLE_GENERATION_SHIFT 16

struct GraphicsTextPayload {
    TTF_Text *text;
    SDL_Texture *texture;
    TextHandle owner;
    size_t references;
    bool built_in;
};

typedef struct GraphicsTextResource {
    GraphicsTextPayload *payload;
    char *value;
    FontAsset font;
    Color color;
    TextAsset asset;
    size_t dependencies;
    size_t command_references;
    bool built_in;
    bool public_owned;
    bool used;
    int wrap_width;
} GraphicsTextResource;

static GraphicsTextResource text_resources[MAX_TEXT_ASSETS];
static uint16_t text_generations[MAX_TEXT_ASSETS];
static SDL_Renderer *text_renderer;
static TTF_TextEngine *text_engine;
static size_t text_payload_count;
static bool text_assets_initialized;

static TextHandle graphics_text_handle_create(size_t slot) {
    return ((uint32_t)text_generations[slot] << TEXT_HANDLE_GENERATION_SHIFT) |
        (uint32_t)(slot + 1);
}

static GraphicsTextResource *graphics_text_resource_get(TextHandle handle,
        bool public_required) {
    uint32_t stored_slot;
    size_t slot;

    if(handle == TEXT_HANDLE_INVALID) return NULL;
    stored_slot = handle & TEXT_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= MAX_TEXT_ASSETS || !text_resources[slot].used ||
            text_resources[slot].asset.handle != handle ||
            (public_required && !text_resources[slot].public_owned)) return NULL;
    return &text_resources[slot];
}

static void graphics_text_payload_release(GraphicsTextPayload *payload) {
    if(payload == NULL || payload->references == 0) return;
    payload->references -= 1;
    if(payload->references != 0) return;
    if(payload->text != NULL) TTF_DestroyText(payload->text);
    if(payload->texture != NULL) SDL_DestroyTexture(payload->texture);
    free(payload);
    text_payload_count -= 1;
}

static void graphics_text_resource_destroy(GraphicsTextResource *resource) {
    FontAsset font;
    if(resource == NULL || !resource->used) return;
    graphics_text_payload_release(resource->payload);
    SDL_free(resource->value);
    font = resource->font;
    (void)graphics_font_release(&font);
    *resource = (GraphicsTextResource){0};
}

static void graphics_text_resource_destroy_if_unused(
        GraphicsTextResource *resource) {
    if(resource != NULL && resource->used && !resource->public_owned &&
            resource->dependencies == 0 && resource->command_references == 0)
        graphics_text_resource_destroy(resource);
}

static SDL_Texture *graphics_builtin_text_texture_create(const char *value,
        Color color, Scale *size) {
    SDL_Texture *texture;
    SDL_Texture *previous;
    size_t length;
    if(value == NULL || size == NULL || text_renderer == NULL) return NULL;
    length = strlen(value);
    *size = (Scale){(float)(length * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE),
        length > 0 ? (float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE : 0.0f};
    if(length == 0) return NULL;
    texture = SDL_CreateTexture(text_renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, (int)size->x, (int)size->y);
    if(texture == NULL) return NULL;
    (void)SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    previous = SDL_GetRenderTarget(text_renderer);
    if(!SDL_SetRenderTarget(text_renderer, texture)) {
        SDL_DestroyTexture(texture);
        return NULL;
    }
    (void)SDL_SetRenderDrawColor(text_renderer, 0, 0, 0, 0);
    (void)SDL_RenderClear(text_renderer);
    (void)SDL_SetRenderDrawColor(text_renderer, color.red, color.green,
        color.blue, color.alpha);
    if(!SDL_RenderDebugText(text_renderer, 0.0f, 0.0f, value)) {
        (void)SDL_SetRenderTarget(text_renderer, previous);
        SDL_DestroyTexture(texture);
        return NULL;
    }
    (void)SDL_SetRenderTarget(text_renderer, previous);
    return texture;
}

static bool graphics_text_payload_create(FontAsset font, bool built_in,
        const char *value, Color color, int wrap_width,
        GraphicsTextPayload **output, Scale *size) {
    GraphicsTextPayload *payload;
    SDL_Surface *surface;
    TTF_Font *native_font;
    int width;
    int height;

    if(value == NULL || output == NULL || size == NULL) return false;
    *output = NULL;
    *size = (Scale){0};
    if(value[0] == '\0') return true;
    payload = calloc(1, sizeof(*payload));
    if(payload == NULL) return false;
    payload->references = 1;
    payload->built_in = built_in;
    if(built_in) {
        payload->texture = graphics_builtin_text_texture_create(value, color,
            size);
        if(payload->texture == NULL) {
            free(payload);
            return false;
        }
    } else {
        native_font = graphics_font_native_get(font);
        if(native_font == NULL || text_engine == NULL || text_renderer == NULL) {
            free(payload);
            return false;
        }
        payload->text = TTF_CreateText(text_engine, native_font, value, 0);
        if(payload->text == NULL || !TTF_SetTextColor(payload->text,
                color.red, color.green, color.blue, color.alpha) ||
                (wrap_width > 0 && !TTF_SetTextWrapWidth(
                    payload->text, wrap_width)) ||
                !TTF_GetTextSize(payload->text, &width, &height)) {
            if(payload->text != NULL) TTF_DestroyText(payload->text);
            free(payload);
            return false;
        }
        surface = wrap_width > 0 ? TTF_RenderText_Blended_Wrapped(native_font,
            value, 0, (SDL_Color){color.red, color.green, color.blue,
                color.alpha}, wrap_width) :
            TTF_RenderText_Blended(native_font, value, 0,
                (SDL_Color){color.red, color.green, color.blue, color.alpha});
        payload->texture = surface == NULL ? NULL :
            SDL_CreateTextureFromSurface(text_renderer, surface);
        if(surface != NULL) SDL_DestroySurface(surface);
        if(payload->texture == NULL) {
            TTF_DestroyText(payload->text);
            free(payload);
            return false;
        }
        *size = (Scale){(float)width, (float)height};
    }
    text_payload_count += 1;
    *output = payload;
    return true;
}

EngineResult graphics_text_assets_init(void) {
    if(text_assets_initialized) return error_result_value(true);
    memset(text_resources, 0, sizeof(text_resources));
    text_renderer = NULL;
    text_engine = NULL;
    text_payload_count = 0;
    text_assets_initialized = true;
    return error_result_value(true);
}

void graphics_text_assets_renderer_set(SDL_Renderer *renderer,
        TTF_TextEngine *engine) {
    text_renderer = renderer;
    text_engine = engine;
}

void graphics_text_assets_clear(void) {
    for(size_t slot = 0; slot < MAX_TEXT_ASSETS; slot += 1)
        graphics_text_resource_destroy(&text_resources[slot]);
    text_renderer = NULL;
    text_engine = NULL;
}

void graphics_text_assets_destroy(void) {
    graphics_text_assets_clear();
    text_assets_initialized = false;
}

GraphicsTextAssetStats graphics_text_assets_stats_get(void) {
    GraphicsTextAssetStats stats = {.live_payloads = text_payload_count};
    for(size_t slot = 0; slot < MAX_TEXT_ASSETS; slot += 1) {
        GraphicsTextResource *resource = &text_resources[slot];
        if(!resource->used) continue;
        stats.live_resources += 1;
        if(resource->public_owned) stats.public_owners += 1;
        stats.dependency_references += resource->dependencies;
        stats.command_references += resource->command_references;
    }
    return stats;
}

TextAssetResult graphics_text_create(const FontAsset *font, const char *value,
        Color color) {
    GraphicsTextPayload *payload = NULL;
    char *owned_value;
    Scale size = {0};
    bool built_in;
    size_t slot;

    if(!text_assets_initialized || text_renderer == NULL || text_engine == NULL)
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    if(font == NULL || value == NULL || !graphics_font_valid_check(*font)) {
        error_detail_set(ERROR_ENGINE_TEXT_CREATE_FAILED, NULL);
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    for(slot = 0; slot < MAX_TEXT_ASSETS; slot += 1)
        if(!text_resources[slot].used) break;
    if(slot == MAX_TEXT_ASSETS)
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED);
    built_in = graphics_font_builtin_check(*font);
    owned_value = SDL_strdup(value);
    if(owned_value == NULL || !graphics_text_payload_create(
            *font, built_in, value, color, 0,
            &payload, &size)) {
        SDL_free(owned_value);
        error_detail_set(ERROR_ENGINE_TEXT_CREATE_FAILED, SDL_GetError());
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    if(error_check(graphics_font_retain(*font))) {
        graphics_text_payload_release(payload);
        SDL_free(owned_value);
        error_detail_set(ERROR_ENGINE_TEXT_CREATE_FAILED, NULL);
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    text_generations[slot] += 1;
    if(text_generations[slot] == 0) text_generations[slot] = 1;
    text_resources[slot] = (GraphicsTextResource){
        .payload = payload,
        .value = owned_value,
        .font = *font,
        .color = color,
        .asset = {.handle = graphics_text_handle_create(slot), .size = size},
        .built_in = built_in,
        .public_owned = true,
        .used = true,
    };
    if(payload != NULL) payload->owner = text_resources[slot].asset.handle;
    return ERROR_RESULT_MAKE_VALUE(TextAssetResult, text_resources[slot].asset);
}

bool graphics_text_value_set(TextAsset *text, const char *value) {
    GraphicsTextResource *resource;
    GraphicsTextPayload *payload = NULL;
    GraphicsTextPayload *previous;
    char *owned_value;
    char *previous_value;
    Scale size = {0};

    if(text == NULL || value == NULL) return false;
    resource = graphics_text_resource_get(text->handle, true);
    if(resource == NULL) return false;
    owned_value = SDL_strdup(value);
    if(owned_value == NULL || !graphics_text_payload_create(resource->font,
            resource->built_in, value, resource->color,
            resource->wrap_width, &payload, &size)) {
        SDL_free(owned_value);
        return false;
    }
    if(payload != NULL) payload->owner = resource->asset.handle;
    previous = resource->payload;
    previous_value = resource->value;
    resource->payload = payload;
    resource->value = owned_value;
    resource->asset.size = size;
    text->size = size;
    graphics_text_payload_release(previous);
    SDL_free(previous_value);
    return true;
}

EngineResult graphics_text_destroy(TextAsset *text) {
    GraphicsTextResource *resource;

    if(text == NULL) return error_result_error(ERROR_ENGINE_TEXT_NOT_FOUND);
    if(text->handle == TEXT_HANDLE_INVALID) {
        *text = (TextAsset){0};
        return error_result_value(true);
    }
    resource = graphics_text_resource_get(text->handle, true);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_TEXT_NOT_FOUND);
    resource->public_owned = false;
    *text = (TextAsset){0};
    graphics_text_resource_destroy_if_unused(resource);
    return error_result_value(true);
}

bool graphics_text_valid_check(TextAsset text) {
    return graphics_text_resource_get(text.handle, true) != NULL;
}

EngineResult graphics_text_dependency_add(TextAsset asset) {
    GraphicsTextResource *resource;
    if(asset.handle == TEXT_HANDLE_INVALID) return error_result_value(true);
    resource = graphics_text_resource_get(asset.handle, true);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_TEXT_NOT_FOUND);
    if(resource->dependencies == SIZE_MAX)
        return error_result_error(ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED);
    resource->dependencies += 1;
    return error_result_value(true);
}

void graphics_text_dependency_remove(TextHandle handle) {
    GraphicsTextResource *resource;
    if(handle == TEXT_HANDLE_INVALID) return;
    resource = graphics_text_resource_get(handle, false);
    if(resource == NULL || resource->dependencies == 0) return;
    resource->dependencies -= 1;
    graphics_text_resource_destroy_if_unused(resource);
}

bool graphics_text_command_reference_add(TextAsset asset,
        GraphicsTextPayload **payload) {
    GraphicsTextResource *resource;
    if(payload == NULL) return false;
    *payload = NULL;
    resource = graphics_text_resource_get(asset.handle, true);
    if(resource == NULL) return false;
    if(resource->payload == NULL) return true;
    if(resource->payload->references == SIZE_MAX ||
            resource->command_references == SIZE_MAX) return false;
    resource->payload->references += 1;
    resource->command_references += 1;
    *payload = resource->payload;
    return true;
}

void graphics_text_command_reference_remove(GraphicsTextPayload *payload) {
    GraphicsTextResource *resource;
    if(payload == NULL) return;
    resource = graphics_text_resource_get(payload->owner, false);
    if(resource != NULL && resource->command_references > 0)
        resource->command_references -= 1;
    graphics_text_payload_release(payload);
    graphics_text_resource_destroy_if_unused(resource);
}

GraphicsTextPayload *graphics_text_internal_payload_get(TextHandle handle) {
    GraphicsTextResource *resource = graphics_text_resource_get(handle, false);
    return resource == NULL ? NULL : resource->payload;
}

bool graphics_text_internal_size_get(TextHandle handle, Scale *size) {
    GraphicsTextResource *resource;
    if(size == NULL) return false;
    resource = graphics_text_resource_get(handle, false);
    if(resource == NULL) return false;
    *size = resource->asset.size;
    return true;
}

TTF_Text *graphics_text_native_get(TextAsset asset) {
    GraphicsTextResource *resource = graphics_text_resource_get(
        asset.handle, true);
    return resource == NULL || resource->payload == NULL ? NULL :
        resource->payload->text;
}

bool graphics_text_wrap_width_set(TextAsset *asset, int wrap_width) {
    GraphicsTextResource *resource;
    GraphicsTextPayload *payload = NULL;
    GraphicsTextPayload *previous;
    Scale size = {0};

    if(asset == NULL || wrap_width < 0) return false;
    resource = graphics_text_resource_get(asset->handle, true);
    if(resource == NULL) return false;
    if(resource->built_in || resource->wrap_width == wrap_width) return true;
    if(!graphics_text_payload_create(resource->font, false, resource->value,
            resource->color, wrap_width, &payload, &size)) return false;
    if(payload != NULL) payload->owner = resource->asset.handle;
    previous = resource->payload;
    resource->payload = payload;
    resource->wrap_width = wrap_width;
    resource->asset.size = size;
    asset->size = size;
    graphics_text_payload_release(previous);
    return true;
}

TTF_Text *graphics_text_payload_native_get(GraphicsTextPayload *payload) {
    return payload == NULL ? NULL : payload->text;
}

SDL_Texture *graphics_text_payload_texture_get(GraphicsTextPayload *payload) {
    return payload == NULL ? NULL : payload->texture;
}

bool graphics_text_payload_builtin_check(GraphicsTextPayload *payload) {
    return payload != NULL && payload->built_in;
}
