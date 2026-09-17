/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_mode_controls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool editor_mode_text_create(FontAsset *font, const char *value,
        TextAsset *output) {
    TextAssetResult result;
    if(font == NULL || value == NULL || output == NULL) return false;
    result = rohr_graphics_text_create(font, value, (Color){230, 234, 242, 255});
    if(rohr_error_check(result)) return false;
    *output = result.result.value;
    return true;
}

void editor_mode_numeric_disabled_draw(TextAsset *display,
        float value,
        UIRect bounds) {
    char text[32];
    if(display == NULL) return;
    snprintf(text, sizeof(text), "%.1f", value);
    (void)rohr_graphics_text_value_set(display, text);
    rohr_ui_button_disabled(bounds, NULL);
    rohr_ui_label(display, bounds);
}

bool editor_mode_checkbox_left(const char *id,
        const TextAsset *label,
        UIRect bounds,
        bool *checked) {
    UIButtonResult interaction;
    UIRect box;
    Color background;
    if(id == NULL || label == NULL || checked == NULL) return false;
    interaction = rohr_ui_interaction(id, bounds);
    if(interaction.clicked) *checked = !*checked;
    background = interaction.pressed ? (Color){58, 65, 78, 255} :
        interaction.hovered || interaction.focused ? (Color){67, 75, 90, 255} :
        (Color){48, 54, 66, 255};
    rohr_ui_surface(bounds, background);
    box = (UIRect){bounds.x + bounds.width - bounds.height + 4.0f,
        bounds.y + 4.0f, bounds.height - 8.0f, bounds.height - 8.0f};
    rohr_ui_surface(box, (Color){22, 25, 31, 255});
    rohr_ui_border(box, 2.0f, (Color){8, 9, 12, 255});
    if(*checked)
        rohr_ui_surface((UIRect){box.x + 5.0f, box.y + 5.0f,
            box.width - 10.0f, box.height - 10.0f},
            (Color){225, 230, 240, 255});
    rohr_ui_label(label, (UIRect){bounds.x + 4.0f, bounds.y,
        bounds.width - bounds.height - 4.0f, bounds.height});
    return interaction.clicked;
}

bool editor_mode_color_swatch(const char *id,
        uint32_t *color,
        bool disabled,
        UIRect bounds,
        const EditorModeContext *context,
        EditorItemKind kind,
        EditorObjectId object,
        uint32_t parent,
        uint32_t item,
        EditorPropertyKind property) {
    UIButtonStyle style;
    Color displayed;
    UIButtonResult result;
    if(id == NULL || color == NULL || context == NULL) return false;
    style = rohr_ui_button_style_default_get();
    displayed = disabled ? (Color){70, 72, 78, 255} :
        rohr_graphics_color_hex_create(*color);
    style.idle = displayed;
    style.hovered = disabled ? displayed :
        (Color){displayed.red, displayed.green, displayed.blue, 220};
    style.pressed = displayed;
    style.disabled = displayed;
    if(disabled) {
        rohr_ui_button_disabled(bounds, &style);
        rohr_ui_border(bounds, 2.0f, (Color){18, 20, 24, 255});
        return false;
    }
    result = rohr_ui_button(id, NULL, bounds, &style);
    rohr_ui_border(bounds, 2.0f, (Color){8, 9, 12, 255});
    if(result.clicked && context->color_open != NULL)
        context->color_open(context->color_context, color, kind, object,
            parent, item, property);
    return result.clicked;
}

bool editor_mode_named_text_sync(FontAsset *font,
        const char *name,
        TextAsset *label,
        char *cache,
        size_t cache_capacity) {
    if(font == NULL || name == NULL || label == NULL || cache == NULL ||
            cache_capacity == 0) return false;
    if(strncmp(cache, name, cache_capacity) == 0) return true;
    if(label->text == NULL) {
        if(!editor_mode_text_create(font, name, label)) return false;
    } else if(!rohr_graphics_text_value_set(label, name)) return false;
    snprintf(cache, cache_capacity, "%s", name);
    return true;
}

bool editor_mode_text_cache_reserve(EditorModeTextCache *cache, size_t required) {
    TextAsset *labels;
    char (*values)[EDITOR_OBJECT_NAME_MAX];
    size_t capacity;
    if(cache == NULL || required > EDITOR_HITBOX_VERTEX_MAX) return false;
    if(required <= cache->capacity) return true;
    capacity = cache->capacity == 0 ? EDITOR_HITBOX_VERTEX_MIN : cache->capacity;
    while(capacity < required) capacity *= 2;
    if(capacity > EDITOR_HITBOX_VERTEX_MAX) capacity = EDITOR_HITBOX_VERTEX_MAX;
    labels = realloc(cache->labels, capacity * sizeof(*labels));
    if(labels == NULL) return false;
    cache->labels = labels;
    values = realloc(cache->values, capacity * sizeof(*values));
    if(values == NULL) return false;
    cache->values = values;
    memset(cache->labels + cache->capacity, 0,
        (capacity - cache->capacity) * sizeof(*cache->labels));
    memset(cache->values + cache->capacity, 0,
        (capacity - cache->capacity) * sizeof(*cache->values));
    cache->capacity = capacity;
    return true;
}

void editor_mode_text_cache_destroy(EditorModeTextCache *cache) {
    if(cache == NULL) return;
    for(size_t i = 0; i < cache->capacity; i += 1)
        rohr_graphics_text_destroy(&cache->labels[i]);
    free(cache->labels);
    free(cache->values);
    *cache = (EditorModeTextCache){0};
}

UIFieldResult editor_mode_name_field(const char *id,
        char *name,
        size_t capacity,
        TextAsset *display,
        UIRect bounds) {
    UIFieldResult result = rohr_ui_field(id,
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = capacity}, display, bounds, NULL);
    if(result.changed) editor_project_property_name_format(name, capacity, name);
    return result;
}

UIButtonStyle editor_mode_delete_style_get(void) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    style.idle = (Color){120, 38, 42, 255};
    style.hovered = (Color){155, 46, 52, 255};
    style.pressed = (Color){92, 29, 33, 255};
    style.disabled = (Color){70, 45, 47, 255};
    return style;
}

bool editor_mode_layer_control_create(EditorModeLayerControl *control,
        FontAsset *font) {
    if(control == NULL || font == NULL) return false;
    *control = (EditorModeLayerControl){.font = font};
    if(!editor_mode_text_create(font, "Render Layer", &control->layer_label) ||
            !editor_mode_text_create(font, "Direct", &control->direct_label) ||
            !editor_mode_text_create(font, "Inherit Parent Layer",
                &control->inherit_label) ||
            !editor_mode_text_create(font, "", &control->value_field)) {
        editor_mode_layer_control_destroy(control);
        return false;
    }
    return true;
}

void editor_mode_layer_control_destroy(EditorModeLayerControl *control) {
    if(control == NULL) return;
    rohr_graphics_text_destroy(&control->layer_label);
    rohr_graphics_text_destroy(&control->direct_label);
    rohr_graphics_text_destroy(&control->inherit_label);
    rohr_graphics_text_destroy(&control->value_field);
    for(size_t i = 0; i < MAX_GRAPHICS_LAYERS; i += 1)
        rohr_graphics_text_destroy(&control->names[i]);
    *control = (EditorModeLayerControl){0};
}

bool editor_mode_layer_control_draw(EditorModeLayerControl *control,
        const char *id_prefix, EditorProject *project,
        EditorGraphicsLayerBinding *binding, bool *inherited,
        float x, float y, float width) {
    const TextAsset *options[MAX_GRAPHICS_LAYERS + 1];
    char dropdown_id[128], value_id[128], inherit_id[128];
    size_t selected = 0;
    bool active = false;
    float value;
    if(control == NULL || id_prefix == NULL || project == NULL || binding == NULL)
        return false;
    if(inherited != NULL) {
        bool value_inherited = *inherited;
        snprintf(inherit_id, sizeof(inherit_id), "%s.inherit_layer", id_prefix);
        if(editor_mode_checkbox_left(inherit_id, &control->inherit_label,
                (UIRect){x + 10.0f, y, width - 20.0f, 28.0f},
                &value_inherited)) *inherited = value_inherited;
        y += 38.0f;
        if(*inherited) return false;
    }
    options[0] = &control->direct_label;
    for(size_t i = 0; i < project->graphics_layer_count; i += 1) {
        if(!editor_mode_named_text_sync(control->font,
                project->graphics_layers[i].name, &control->names[i],
                control->name_cache[i], GRAPHICS_LAYER_NAME_MAX)) return false;
        options[i + 1] = &control->names[i];
        if(binding->layer == project->graphics_layers[i].id) selected = i + 1;
    }
    rohr_ui_label(&control->layer_label, (UIRect){x + 8.0f, y, 92.0f, 28.0f});
    snprintf(dropdown_id, sizeof(dropdown_id), "%s.layer", id_prefix);
    UIDropdownResult selected_result = rohr_ui_dropdown(dropdown_id, options,
        project->graphics_layer_count + 1, selected,
        (UIRect){x + 104.0f, y, width - 114.0f, 28.0f}, NULL);
    if(selected_result.changed) binding->layer = selected_result.selected_index == 0
        ? 0 : project->graphics_layers[selected_result.selected_index - 1].id;
    if(binding->layer == 0) {
        value = (float)binding->value;
        y += 38.0f;
        rohr_ui_label(&control->direct_label,
            (UIRect){x + 8.0f, y, 92.0f, 28.0f});
        snprintf(value_id, sizeof(value_id), "%s.layer_value", id_prefix);
        UIFieldResult result = rohr_ui_field(value_id,
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &value},
            &control->value_field,
            (UIRect){x + 104.0f, y, width - 114.0f, 28.0f}, NULL);
        if(result.changed) binding->value = (int)value;
        active = result.active;
    }
    return active;
}
