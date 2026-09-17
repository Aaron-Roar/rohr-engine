/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_MODE_CONTROLS_H
#define ROHR_EDITOR_MODE_CONTROLS_H

#include "editors/editor_mode_context.h"

typedef struct EditorModeTextCache {
    TextAsset *labels;
    char (*values)[EDITOR_OBJECT_NAME_MAX];
    size_t capacity;
} EditorModeTextCache;

typedef struct EditorModeLayerControl {
    FontAsset *font;
    TextAsset layer_label;
    TextAsset direct_label;
    TextAsset inherit_label;
    TextAsset value_field;
    TextAsset names[MAX_GRAPHICS_LAYERS];
    char name_cache[MAX_GRAPHICS_LAYERS][GRAPHICS_LAYER_NAME_MAX];
} EditorModeLayerControl;

bool editor_mode_text_create(FontAsset *font, const char *value,
    TextAsset *output);
void editor_mode_numeric_disabled_draw(TextAsset *display, float value,
    UIRect bounds);
bool editor_mode_checkbox_left(const char *id, const TextAsset *label,
    UIRect bounds, bool *checked);
bool editor_mode_color_swatch(const char *id, uint32_t *color, bool disabled,
    UIRect bounds,
    const EditorModeContext *context, EditorItemKind kind,
    EditorObjectId object, uint32_t parent, uint32_t item,
    EditorPropertyKind property);
bool editor_mode_named_text_sync(FontAsset *font, const char *name,
    TextAsset *label, char *cache, size_t cache_capacity);
bool editor_mode_text_cache_reserve(EditorModeTextCache *cache, size_t required);
void editor_mode_text_cache_destroy(EditorModeTextCache *cache);
UIFieldResult editor_mode_name_field(const char *id, char *name,
    size_t capacity, TextAsset *display, UIRect bounds);
UIButtonStyle editor_mode_delete_style_get(void);
bool editor_mode_layer_control_create(EditorModeLayerControl *control,
    FontAsset *font);
void editor_mode_layer_control_destroy(EditorModeLayerControl *control);
bool editor_mode_layer_control_draw(EditorModeLayerControl *control,
    const char *id_prefix, EditorProject *project,
    EditorGraphicsLayerBinding *binding, bool *inherited,
    float x, float y, float width);

#endif
