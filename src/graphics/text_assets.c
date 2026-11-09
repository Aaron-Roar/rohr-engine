/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/text_assets.h"

#include "graphics/font_assets.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define TEXT_HANDLE_GENERATION_SHIFT 16

struct GraphicsTextPayload {
    TTF_Text *text;
    SDL_Texture *texture;
    TextHandle owner;
    FontAsset font;
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
    (void)graphics_font_release(&payload->font);
    free(payload);
    text_payload_count -= 1;
}

static void graphics_text_resource_destroy(GraphicsTextResource *resource) {
    FontAsset font;
    if(resource == NULL || !resource->used) return;
    font = resource->font;
    if(resource->payload != NULL) graphics_text_payload_release(resource->payload);
    else (void)graphics_font_release(&font);
    SDL_free(resource->value);
    *resource = (GraphicsTextResource){0};
}

static void graphics_text_resource_destroy_if_unused(
        GraphicsTextResource *resource) {
    if(resource != NULL && resource->used && !resource->public_owned &&
            resource->dependencies == 0 && resource->command_references == 0)
        graphics_text_resource_destroy(resource);
}

/* Layout and rendering use the same ASCII rows. Decode UTF-8 once so unsupported
 * codepoints occupy one fallback glyph, rather than one cell per UTF-8 byte. */
static char *graphics_builtin_text_normalize(const char *value) {
    size_t length = strlen(value);
    if(length > (SIZE_MAX - 1) / 4) return NULL;
    char *normalized = SDL_malloc(length * 4 + 1);
    if(normalized == NULL) return NULL;
    char *output = normalized;
    while(*value != '\0') {
        Uint32 codepoint = SDL_StepUTF8(&value, NULL);
        if(codepoint == '\r') {
            if(*value == '\n') value += 1;
            *output++ = '\n';
        } else if(codepoint == '\t') {
            for(int i = 0; i < 4; i += 1) *output++ = ' ';
        } else {
            *output++ = codepoint == '\n' || (codepoint >= 32 && codepoint <= 126)
                ? (char)codepoint : '?';
        }
    }
    *output = '\0';
    return normalized;
}

static char *graphics_builtin_text_line_end(char *line, size_t columns,
        char **next) {
    char *end = line;
    char *space = NULL;
    while(*end != '\0' && *end != '\n' && (size_t)(end - line) < columns) {
        if(*end == ' ') space = end;
        end += 1;
    }
    if(*end == '\0') *next = NULL;
    else if(*end == '\n') *next = end + 1;
    else if(*end == ' ') {
        *next = end;
        while(**next == ' ') *next += 1;
    } else if(space != NULL && space > line) {
        end = space;
        *next = space + 1;
        while(**next == ' ') *next += 1;
    } else *next = end;
    if(*next != NULL && *next > end && *end != '\n') {
        if(**next == '\0') *next = NULL;
        else if(**next == '\n') *next += 1;
    }
    return end;
}

static SDL_Texture *graphics_builtin_text_texture_create(const char *value,
        Color color, int wrap_width, Scale *size) {
    const size_t cell = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    size_t columns = wrap_width > 0 ? (size_t)wrap_width / cell : INT_MAX / cell;
    size_t width = 0, rows = 0;
    char *normalized = graphics_builtin_text_normalize(value);
    SDL_Texture *texture = NULL;
    SDL_Texture *previous = SDL_GetRenderTarget(text_renderer);
    Uint8 red, green, blue, alpha;
    bool rendered = true;
    if(normalized == NULL) return NULL;
    if(columns == 0) columns = 1;
    for(char *line = normalized; line != NULL;) {
        char *next;
        char *end = graphics_builtin_text_line_end(line, columns, &next);
        if((size_t)(end - line) > width) width = (size_t)(end - line);
        rows += 1;
        if(rows > INT_MAX / cell || width > INT_MAX / cell) goto done;
        line = next;
    }
    *size = (Scale){(float)(width * cell), (float)(rows * cell)};
    /* Blank lines have height but need a nonzero texture allocation. */
    texture = SDL_CreateTexture(text_renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, width == 0 ? 1 : (int)(width * cell), (int)(rows * cell));
    if(texture == NULL) goto done;
    SDL_GetRenderDrawColor(text_renderer, &red, &green, &blue, &alpha);
    if(!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) ||
            !SDL_SetRenderTarget(text_renderer, texture)) {
        SDL_DestroyTexture(texture);
        texture = NULL;
        goto done;
    }
    rendered = SDL_SetRenderDrawColor(text_renderer, 0, 0, 0, 0) &&
        SDL_RenderClear(text_renderer) &&
        SDL_SetRenderDrawColor(text_renderer, color.red, color.green, color.blue, color.alpha);
    size_t row = 0;
    for(char *line = normalized; rendered && line != NULL; row += 1) {
        char *next;
        char *end = graphics_builtin_text_line_end(line, columns, &next);
        char saved = *end;
        *end = '\0';
        rendered = SDL_RenderDebugText(text_renderer, 0, (float)(row * cell), line);
        *end = saved;
        line = next;
    }
    if(!SDL_SetRenderTarget(text_renderer, previous)) rendered = false;
    if(!SDL_SetRenderDrawColor(text_renderer, red, green, blue, alpha)) rendered = false;
    if(!rendered) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
done:
    SDL_free(normalized);
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
            wrap_width, size);
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
    if(error_check(graphics_font_retain(font))) {
        if(payload->text != NULL) TTF_DestroyText(payload->text);
        SDL_DestroyTexture(payload->texture);
        free(payload);
        return false;
    }
    payload->font = font;
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

TextAssetResult (graphics_text_create)(TextDescription description) {
    FontAsset font = description.font.handle == FONT_HANDLE_INVALID ?
        graphics_font_default_get() : description.font;
    const char *value = description.value;
    Color color = description.color;
    GraphicsTextPayload *payload = NULL;
    char *owned_value;
    Scale size = {0};
    bool built_in;
    size_t slot;

    if(!text_assets_initialized || text_renderer == NULL || text_engine == NULL)
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    if(value == NULL || description.wrap_width < 0 || !graphics_font_valid_check(font)) {
        error_detail_set(ERROR_ENGINE_TEXT_CREATE_FAILED, NULL);
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    for(slot = 0; slot < MAX_TEXT_ASSETS; slot += 1)
        if(!text_resources[slot].used) break;
    if(slot == MAX_TEXT_ASSETS)
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED);
    built_in = graphics_font_builtin_check(font);
    owned_value = SDL_strdup(value);
    if(owned_value == NULL || !graphics_text_payload_create(
            font, built_in, value, color, description.wrap_width,
            &payload, &size)) {
        SDL_free(owned_value);
        error_detail_set(ERROR_ENGINE_TEXT_CREATE_FAILED, SDL_GetError());
        return ERROR_RESULT_MAKE_ERROR(
            TextAssetResult, ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    if(payload == NULL && error_check(graphics_font_retain(font))) {
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
        .font = font,
        .wrap_width = description.wrap_width,
        .color = color,
        .asset = {.handle = graphics_text_handle_create(slot), .size = size},
        .built_in = built_in,
        .public_owned = true,
        .used = true,
    };
    if(payload != NULL) payload->owner = text_resources[slot].asset.handle;
    return ERROR_RESULT_MAKE_VALUE(TextAssetResult, text_resources[slot].asset);
}

TextDescription graphics_text_description_default_get(void) {
    return (TextDescription){.value = "", .color = {255, 255, 255, 255}};
}

TextDescriptionResult graphics_text_description_get(TextAsset text) {
    GraphicsTextResource *resource = graphics_text_resource_get(text.handle, true);
    if(resource == NULL)
        return ERROR_RESULT_MAKE_ERROR(TextDescriptionResult, ERROR_ENGINE_TEXT_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(TextDescriptionResult, ((TextDescription){
        .font = resource->font, .value = resource->value,
        .color = resource->color, .wrap_width = resource->wrap_width}));
}

EngineResult graphics_text_description_set(TextAsset *text, TextDescription description) {
    if(text == NULL) return error_result_error(ERROR_ENGINE_TEXT_NOT_FOUND);
    GraphicsTextResource *resource = graphics_text_resource_get(text->handle, true);
    if(resource == NULL) return error_result_error(ERROR_ENGINE_TEXT_NOT_FOUND);
    FontAsset font = description.font.handle == FONT_HANDLE_INVALID ?
        graphics_font_default_get() : description.font;
    if(description.value == NULL || description.wrap_width < 0 ||
            !graphics_font_valid_check(font))
        return error_result_error(ERROR_ENGINE_TEXT_CREATE_FAILED);
    if(font.handle == resource->font.handle &&
            description.wrap_width == resource->wrap_width &&
            description.color.red == resource->color.red &&
            description.color.green == resource->color.green &&
            description.color.blue == resource->color.blue &&
            description.color.alpha == resource->color.alpha &&
            strcmp(description.value, resource->value) == 0) {
        text->size = resource->asset.size;
        return error_result_value(true);
    }
    GraphicsTextPayload *payload = NULL;
    Scale size = {0};
    bool built_in = graphics_font_builtin_check(font);
    /* The caller may pass a borrowed string from this very instance. */
    char *owned_value = SDL_strdup(description.value);
    if(owned_value == NULL || !graphics_text_payload_create(font, built_in,
            owned_value, description.color, description.wrap_width, &payload, &size)) {
        SDL_free(owned_value);
        return error_result_error(ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    if(payload == NULL && error_check(graphics_font_retain(font))) {
        SDL_free(owned_value);
        return error_result_error(ERROR_ENGINE_TEXT_CREATE_FAILED);
    }
    GraphicsTextPayload *previous = resource->payload;
    FontAsset previous_font = resource->font;
    char *previous_value = resource->value;
    if(payload != NULL) payload->owner = resource->asset.handle;
    resource->payload = payload;
    resource->value = owned_value;
    resource->font = font;
    resource->built_in = built_in;
    resource->color = description.color;
    resource->wrap_width = description.wrap_width;
    resource->asset.size = size;
    text->size = size;
    if(previous != NULL) graphics_text_payload_release(previous);
    else (void)graphics_font_release(&previous_font);
    SDL_free(previous_value);
    return error_result_value(true);
}

bool graphics_text_value_set(TextAsset *text, const char *value) {
    if(text == NULL) return false;
    TextDescriptionResult current = graphics_text_description_get(*text);
    if(error_check(current)) return false;
    current.result.value.value = value;
    return !error_check(graphics_text_description_set(text, current.result.value));
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
    if(asset == NULL) return false;
    TextDescriptionResult current = graphics_text_description_get(*asset);
    if(error_check(current)) return false;
    current.result.value.wrap_width = wrap_width;
    return !error_check(graphics_text_description_set(asset, current.result.value));
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
