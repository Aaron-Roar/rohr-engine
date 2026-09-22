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

typedef struct EditorModeAccordionSection {
    TextAsset label;
    char title[EDITOR_OBJECT_NAME_MAX];
    bool expanded;
} EditorModeAccordionSection;

typedef struct EditorModeAccordionLayoutCursor {
    float x;
    float width;
    float y;
    float padding;
    float section_gap;
} EditorModeAccordionLayoutCursor;

typedef struct EditorModeAccordionLayoutResult {
    float content_y;
    float content_height;
    bool expanded;
} EditorModeAccordionLayoutResult;

typedef struct EditorModeAccordionLayoutMetrics {
    float content_y;
    float content_height;
    float next_y;
} EditorModeAccordionLayoutMetrics;

typedef struct EditorModeAccordionLayoutGroup {
    size_t row_count;
    float row_height;
    float row_gap;
    float gap_before;
} EditorModeAccordionLayoutGroup;

typedef struct EditorModeLayerControl {
    FontAsset *font;
    TextAsset layer_label;
    TextAsset direct_label;
    TextAsset add_label;
    TextAsset edit_label;
    TextAsset save_label;
    TextAsset cancel_label;
    TextAsset delete_label;
    TextAsset inherit_label;
    TextAsset name_field;
    TextAsset value_field;
    TextAsset names[MAX_GRAPHICS_LAYERS];
    char name_cache[MAX_GRAPHICS_LAYERS][GRAPHICS_LAYER_NAME_MAX + 32];
    char edited_name[GRAPHICS_LAYER_NAME_MAX];
    float edited_value;
    EditorGraphicsLayerId edited_layer;
    bool adding;
} EditorModeLayerControl;

bool editor_mode_text_create(FontAsset *font, const char *value,
    TextAsset *output);
bool editor_mode_accordion_section_create(EditorModeAccordionSection *section,
    FontAsset *font, const char *title, bool expanded);
void editor_mode_accordion_section_destroy(EditorModeAccordionSection *section);
bool editor_mode_accordion_section_draw(EditorModeAccordionSection *section,
    const char *id, UIRect bounds, float content_height);
void editor_mode_accordion_section_expanded_set(
    EditorModeAccordionSection *section, bool expanded);
EditorModeAccordionLayoutCursor editor_mode_accordion_layout_cursor_get(
    float x, float width, float y);
EditorModeAccordionLayoutMetrics editor_mode_accordion_layout_metrics_get(
    float header_y, float padding, float section_gap,
    const float *row_heights, size_t row_count, float row_gap, bool expanded);
EditorModeAccordionLayoutResult editor_mode_accordion_layout_section(
    EditorModeAccordionLayoutCursor *cursor,
    EditorModeAccordionSection *section, const char *id,
    const float *row_heights, size_t row_count, float row_gap);
EditorModeAccordionLayoutResult editor_mode_accordion_layout_nested_section(
    EditorModeAccordionLayoutCursor *cursor,
    EditorModeAccordionSection *section, const char *id,
    const EditorModeAccordionLayoutGroup *groups, size_t group_count);
float editor_mode_accordion_layout_groups_height_get(
    const EditorModeAccordionLayoutGroup *groups, size_t group_count);
float editor_mode_accordion_layout_group_row_y(
    const EditorModeAccordionLayoutResult *section,
    const EditorModeAccordionLayoutGroup *groups, size_t group_count,
    size_t group_index, size_t row_index);
float editor_mode_accordion_layout_row_y(
    const EditorModeAccordionLayoutResult *section,
    const float *row_heights, size_t row_index, float row_gap);
void editor_mode_accordion_layout_measure_reset(void);
void editor_mode_accordion_layout_measure_include(float bottom);
float editor_mode_accordion_layout_measure_get(void);
UIButtonStyle editor_mode_section_field_style_get(void);
UIFieldResult editor_mode_field(const char *id, UIFieldBinding binding,
    TextAsset *display, UIRect bounds, const UIButtonStyle *style);
UIDropdownResult editor_mode_dropdown(const char *id,
    const TextAsset *const *options, size_t option_count,
    size_t selected_index, UIRect bounds, const UIButtonStyle *style);
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
const char *editor_mode_name_field_id_get(EditorViewportMode mode);
bool editor_mode_name_focus_request(EditorViewportState *state);
bool editor_mode_name_focus_pending_check(const EditorViewportState *state);
void editor_mode_name_focus_apply(EditorViewportState *state);
UIButtonStyle editor_mode_delete_style_get(void);
bool editor_mode_layer_control_create(EditorModeLayerControl *control,
    FontAsset *font);
void editor_mode_layer_control_destroy(EditorModeLayerControl *control);
bool editor_mode_layer_control_draw(EditorModeLayerControl *control,
    const char *id_prefix, EditorProject *project,
    EditorGraphicsLayerBinding *binding, bool *inherited,
    float x, float y, float width);

#endif
