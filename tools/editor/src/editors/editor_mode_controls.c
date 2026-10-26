/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_log.h"
#include "editor_mode_controls.h"
#include "editor_layout.h"
#include "entity/object_visuals.h"

#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool editor_mode_checkbox_mixed(const char *id, TextAsset *label, UIRect bounds, bool all, bool any) {
    UIButtonResult result = rohr_ui_interaction(id, bounds);
    rohr_ui_surface(bounds, result.hovered ? (Color){67,75,90,255} : (Color){48,54,66,255});
    UIRect box = {bounds.x + 4, bounds.y + 4, 20, 20};
    rohr_ui_border(box, 2, (Color){200,205,215,255});
    if(all) rohr_ui_surface((UIRect){box.x + 5,box.y + 5,10,10}, (Color){225,230,240,255});
    else if(any) rohr_ui_surface((UIRect){box.x + 4,box.y + 9,12,2}, (Color){255,215,70,255});
    rohr_ui_label(label, (UIRect){bounds.x + 32,bounds.y,bounds.width - 32,bounds.height});
    return result.clicked;
}
static UIFieldResult number_commit_field(EditorMixedNumberField *state,
        const char *id, TextAsset *display, UIRect bounds, bool mixed, float *value,
        float minimum, float maximum, bool commit_on_blur) {
    if(state == NULL || value == NULL) return (UIFieldResult){0};
    if(!state->active) {
        if(mixed) snprintf(state->text, sizeof(state->text), "Mixed");
        else snprintf(state->text, sizeof(state->text), "%.9g", (double)*value);
    }
    bool was_active = state->active;
    UIFieldResult result = editor_mode_field(id, (UIFieldBinding){.kind = UI_FIELD_STRING,
        .string = state->text, .string_capacity = sizeof(state->text)}, display, bounds, NULL);
    bool cancelled = rohr_ui_key_pressed_check(SDLK_ESCAPE);
    state->active = result.active && !result.submitted && !cancelled;
    result.changed = false;
    if(!cancelled && (result.submitted || (commit_on_blur && was_active && !result.active))) {
        char *end = NULL;
        float entered = strtof(state->text, &end);
        if(end != state->text && *end == '\0' && isfinite(entered) &&
                entered >= minimum && entered <= maximum) {
            *value = entered; result.changed = true;
        }
    }
    return result;
}

UIFieldResult editor_mode_mixed_number_field(EditorMixedNumberField *state,
        const char *id, TextAsset *display, UIRect bounds, bool mixed, float *value,
        float minimum, float maximum) {
    return number_commit_field(state,id,display,bounds,mixed,value,minimum,maximum,false);
}
UIFieldResult editor_mode_number_blur_field(EditorMixedNumberField *state,
        const char *id, TextAsset *display, UIRect bounds, float *value) {
    return number_commit_field(state,id,display,bounds,false,value,-FLT_MAX,FLT_MAX,true);
}

static float editor_mode_accordion_measured_bottom;
static const char *editor_mode_pending_field_focus;
static char editor_mode_text_error[512];

typedef struct EditorModeNamedTextEntry {
    TextAsset *label;
    char *cache;
    size_t cache_capacity;
} EditorModeNamedTextEntry;

static EditorModeNamedTextEntry editor_mode_named_text_entries[MAX_TEXT_ASSETS];

static void editor_mode_text_error_set(const char *message) {
    snprintf(editor_mode_text_error, sizeof(editor_mode_text_error), "%s",
        message == NULL || message[0] == '\0' ? "unknown text creation failure" :
        message);
    editor_log_error("editor_text", "%s", editor_mode_text_error);
}

void editor_mode_text_error_clear(void) {
    editor_mode_text_error[0] = '\0';
}

const char *editor_mode_text_error_get(void) {
    return editor_mode_text_error[0] == '\0' ? NULL : editor_mode_text_error;
}

bool editor_mode_text_create_color(const FontAsset *font, const char *value,
        Color color, TextAsset *output) {
    TextAssetResult result;
    char message[512];
    if(font == NULL || value == NULL || output == NULL) {
        editor_mode_text_error_set("invalid editor text creation arguments");
        return false;
    }
    result = rohr_graphics_text_create(
        (TextDescription){.font = font != NULL ? *font : (FontAsset){0},
            .value = value,
            .color = color});
    if(rohr_error_check(result)) {
        snprintf(message, sizeof(message), "error %d: %s",
            (int)result.result.error, rohr_error_message_get(result));
        editor_mode_text_error_set(message);
        return false;
    }
    *output = result.result.value;
    return true;
}

bool editor_mode_text_create(const FontAsset *font, const char *value,
        TextAsset *output) {
    return editor_mode_text_create_color(font, value,
        (Color){230, 234, 242, 255}, output);
}

static bool editor_mode_accordion_label_sync(EditorModeAccordionSection *section) {
    char value[EDITOR_OBJECT_NAME_MAX + 8];
    if(section == NULL || !rohr_graphics_text_valid_check(section->label))
        return false;
    snprintf(value, sizeof(value), "%s  %s",
        section->expanded ? "[-]" : "[+]", section->title);
    return rohr_graphics_text_value_set(&section->label, value);
}

bool editor_mode_accordion_section_create(EditorModeAccordionSection *section,
        FontAsset *font,
        const char *title,
        bool expanded) {
    char value[EDITOR_OBJECT_NAME_MAX + 8];
    if(section == NULL || font == NULL || title == NULL || title[0] == '\0')
        return false;
    *section = (EditorModeAccordionSection){.expanded = expanded};
    snprintf(section->title, sizeof(section->title), "%s", title);
    snprintf(value, sizeof(value), "%s  %s", expanded ? "[-]" : "[+]", title);
    return editor_mode_text_create(font, value, &section->label);
}

void editor_mode_accordion_section_destroy(EditorModeAccordionSection *section) {
    if(section == NULL) return;
    rohr_graphics_text_destroy(&section->label);
    *section = (EditorModeAccordionSection){0};
}

bool editor_mode_accordion_section_draw(EditorModeAccordionSection *section,
        const char *id,
        UIRect bounds,
        float content_height) {
    UIButtonStyle style;
    UIButtonResult result;
    if(section == NULL || id == NULL ||
            !rohr_graphics_text_valid_check(section->label)) return false;
    if(content_height < 0.0f) content_height = 0.0f;
    if(section->expanded) {
        UIRect panel = {bounds.x, bounds.y, bounds.width,
            bounds.height + content_height};
        rohr_ui_surface(panel, (Color){62, 66, 74, 255});
        rohr_ui_border(panel, 1.0f, (Color){184, 190, 202, 255});
    }
    style = rohr_ui_button_style_default_get();
    style.idle = section->expanded ? (Color){82, 88, 100, 255} :
        (Color){42, 47, 57, 255};
    style.hovered = (Color){96, 103, 117, 255};
    style.pressed = (Color){64, 69, 79, 255};
    result = rohr_ui_button(id, &section->label, bounds, &style);
    rohr_ui_border(bounds, 1.0f, section->expanded ?
        (Color){184, 190, 202, 255} : (Color){92, 98, 110, 255});
    if(result.clicked) {
        section->expanded = !section->expanded;
        (void)editor_mode_accordion_label_sync(section);
    }
    {
        float bottom = bounds.y + (section->expanded ?
            bounds.height + content_height + 6.0f : bounds.height + 6.0f);
        if(bottom > editor_mode_accordion_measured_bottom)
            editor_mode_accordion_measured_bottom = bottom;
    }
    return section->expanded;
}

void editor_mode_accordion_section_expanded_set(
        EditorModeAccordionSection *section, bool expanded) {
    if(section == NULL || section->expanded == expanded) return;
    section->expanded = expanded;
    (void)editor_mode_accordion_label_sync(section);
}

EditorModeAccordionLayoutCursor editor_mode_accordion_layout_cursor_get(
        float x, float width, float y) {
    return (EditorModeAccordionLayoutCursor){x, width, y, 6.0f, 6.0f};
}

EditorModeAccordionLayoutMetrics editor_mode_accordion_layout_metrics_get(
        float header_y, float padding, float section_gap,
        const float *row_heights, size_t row_count, float row_gap,
        bool expanded) {
    EditorModeAccordionLayoutMetrics metrics = {0};
    float rows_height = 0.0f;
    if(row_count > 0 && row_heights == NULL) return metrics;
    if(padding < 0.0f) padding = 0.0f;
    if(section_gap < 0.0f) section_gap = 0.0f;
    if(row_gap < 0.0f) row_gap = 0.0f;
    for(size_t row = 0; row < row_count; row += 1)
        rows_height += row_heights[row] < 0.0f ? 0.0f : row_heights[row];
    if(row_count > 1) rows_height += row_gap * (float)(row_count - 1);
    metrics.content_y = header_y + 30.0f + padding;
    metrics.content_height = padding * 2.0f + rows_height;
    metrics.next_y = header_y + 30.0f + section_gap +
        (expanded ? metrics.content_height : 0.0f);
    return metrics;
}

EditorModeAccordionLayoutResult editor_mode_accordion_layout_section(
        EditorModeAccordionLayoutCursor *cursor,
        EditorModeAccordionSection *section, const char *id,
        const float *row_heights, size_t row_count, float row_gap) {
    EditorModeAccordionLayoutResult result = {0};
    EditorModeAccordionLayoutMetrics metrics;
    if(cursor == NULL || section == NULL || id == NULL ||
            (row_count > 0 && row_heights == NULL)) return result;
    metrics = editor_mode_accordion_layout_metrics_get(cursor->y,
        cursor->padding, cursor->section_gap, row_heights, row_count, row_gap,
        section->expanded);
    result.content_height = metrics.content_height;
    result.content_y = metrics.content_y;
    result.expanded = editor_mode_accordion_section_draw(section, id,
        (UIRect){cursor->x + 8.0f, cursor->y,
            cursor->width - 16.0f, 30.0f}, result.content_height);
    metrics = editor_mode_accordion_layout_metrics_get(cursor->y,
        cursor->padding, cursor->section_gap, row_heights, row_count, row_gap,
        result.expanded);
    cursor->y = metrics.next_y;
    if(cursor->y > editor_mode_accordion_measured_bottom)
        editor_mode_accordion_measured_bottom = cursor->y;
    return result;
}

float editor_mode_accordion_layout_groups_height_get(
        const EditorModeAccordionLayoutGroup *groups, size_t group_count) {
    float height = 0.0f;
    if(group_count > 0 && groups == NULL) return 0.0f;
    for(size_t group = 0; group < group_count; group += 1) {
        const EditorModeAccordionLayoutGroup *current = &groups[group];
        if(current->row_count == 0) continue;
        height += current->gap_before > 0.0f ? current->gap_before : 0.0f;
        height += (current->row_height > 0.0f ? current->row_height : 0.0f) *
            (float)current->row_count;
        if(current->row_count > 1 && current->row_gap > 0.0f)
            height += current->row_gap * (float)(current->row_count - 1);
    }
    return height;
}

float editor_mode_accordion_layout_group_row_y(
        const EditorModeAccordionLayoutResult *section,
        const EditorModeAccordionLayoutGroup *groups, size_t group_count,
        size_t group_index, size_t row_index) {
    float y;
    if(section == NULL || groups == NULL || group_index >= group_count ||
            row_index >= groups[group_index].row_count) return 0.0f;
    y = section->content_y;
    for(size_t group = 0; group < group_index; group += 1)
        y += editor_mode_accordion_layout_groups_height_get(
            &groups[group], 1);
    y += groups[group_index].gap_before > 0.0f ?
        groups[group_index].gap_before : 0.0f;
    y += (groups[group_index].row_height + groups[group_index].row_gap) *
        (float)row_index;
    return y;
}

EditorModeAccordionLayoutResult editor_mode_accordion_layout_nested_section(
        EditorModeAccordionLayoutCursor *cursor,
        EditorModeAccordionSection *section, const char *id,
        const EditorModeAccordionLayoutGroup *groups, size_t group_count) {
    float content = editor_mode_accordion_layout_groups_height_get(
        groups, group_count);
    return editor_mode_accordion_layout_section(cursor, section, id,
        &content, 1, 0.0f);
}

float editor_mode_accordion_layout_row_y(
        const EditorModeAccordionLayoutResult *section,
        const float *row_heights, size_t row_index, float row_gap) {
    float y;
    if(section == NULL || row_heights == NULL) return 0.0f;
    y = section->content_y;
    for(size_t row = 0; row < row_index; row += 1)
        y += row_heights[row] + row_gap;
    return y;
}

void editor_mode_accordion_layout_measure_reset(void) {
    editor_mode_accordion_measured_bottom = 0.0f;
}

void editor_mode_accordion_layout_measure_include(float bottom) {
    if(bottom > editor_mode_accordion_measured_bottom)
        editor_mode_accordion_measured_bottom = bottom;
}

float editor_mode_accordion_layout_measure_get(void) {
    return editor_mode_accordion_measured_bottom;
}

void editor_mode_divider_draw(float x, float y, float width) {
    float inset;
    if(width <= 0.0f) return;
    inset = width * 0.05f;
    rohr_ui_surface((UIRect){x + inset, y,
        width - inset * 2.0f, 1.0f}, (Color){184, 190, 202, 255});
}

static void editor_mode_icon_line(Position start,
        Position end,
        float thickness,
        Color color) {
    Vec2D delta = {end.x - start.x, end.y - start.y};
    float length = hypotf(delta.x, delta.y);
    if(length <= 0.0f) return;
    rohr_ui_quad((Position){(start.x + end.x) * 0.5f,
        (start.y + end.y) * 0.5f}, length, thickness,
        math_radians_to_degrees(atan2f(delta.y, delta.x)), color);
}

UIButtonResult editor_mode_visibility_button(const char *id,
        bool visible,
        bool disabled,
        UIRect bounds) {
    UIButtonResult result = {0};
    Color color = disabled ? (Color){121, 126, 136, 180} :
        (Color){225, 230, 240, 255};
    Position center = {bounds.x + bounds.width * 0.5f,
        bounds.y + bounds.height * 0.5f};
    float scale = fminf(bounds.width, bounds.height) / 28.0f;
    if(id == NULL || bounds.width <= 0.0f || bounds.height <= 0.0f)
        return result;
    if(disabled) rohr_ui_button_disabled(bounds, NULL);
    else result = rohr_ui_button(id, NULL, bounds, NULL);
    if(visible) {
        Position left = {center.x - 8.0f * scale, center.y};
        Position upper_left = {center.x - 4.0f * scale,
            center.y - 3.0f * scale};
        Position upper = {center.x, center.y - 4.0f * scale};
        Position upper_right = {center.x + 4.0f * scale,
            center.y - 3.0f * scale};
        Position right = {center.x + 8.0f * scale, center.y};
        Position lower_right = {center.x + 4.0f * scale,
            center.y + 3.0f * scale};
        Position lower = {center.x, center.y + 4.0f * scale};
        Position lower_left = {center.x - 4.0f * scale,
            center.y + 3.0f * scale};
        editor_mode_icon_line(left, upper_left, 1.5f * scale, color);
        editor_mode_icon_line(upper_left, upper, 1.5f * scale, color);
        editor_mode_icon_line(upper, upper_right, 1.5f * scale, color);
        editor_mode_icon_line(upper_right, right, 1.5f * scale, color);
        editor_mode_icon_line(right, lower_right, 1.5f * scale, color);
        editor_mode_icon_line(lower_right, lower, 1.5f * scale, color);
        editor_mode_icon_line(lower, lower_left, 1.5f * scale, color);
        editor_mode_icon_line(lower_left, left, 1.5f * scale, color);
        rohr_ui_quad(center, 4.5f * scale, 4.5f * scale, 45.0f,
            color);
    } else {
        Position left = {center.x - 8.0f * scale,
            center.y - 1.0f * scale};
        Position left_lid = {center.x - 4.0f * scale,
            center.y + 2.0f * scale};
        Position lid = {center.x, center.y + 3.0f * scale};
        Position right_lid = {center.x + 4.0f * scale,
            center.y + 2.0f * scale};
        Position right = {center.x + 8.0f * scale,
            center.y - 1.0f * scale};
        editor_mode_icon_line(left, left_lid, 1.8f * scale, color);
        editor_mode_icon_line(left_lid, lid, 1.8f * scale, color);
        editor_mode_icon_line(lid, right_lid, 1.8f * scale, color);
        editor_mode_icon_line(right_lid, right, 1.8f * scale, color);
        editor_mode_icon_line((Position){center.x - 5.0f * scale,
                center.y + 1.0f * scale},
            (Position){center.x - 6.0f * scale,
                center.y + 5.0f * scale}, 1.5f * scale, color);
        editor_mode_icon_line(lid,
            (Position){center.x, center.y + 6.0f * scale},
            1.5f * scale, color);
        editor_mode_icon_line((Position){center.x + 5.0f * scale,
                center.y + 1.0f * scale},
            (Position){center.x + 6.0f * scale,
                center.y + 5.0f * scale}, 1.5f * scale, color);
    }
    return result;
}

static Color editor_mode_element_category_color_get(
        EditorHierarchySelection kind) {
    switch(kind) {
        case EDITOR_SELECTION_LAYOUT_VIEWPORT:
        case EDITOR_SELECTION_UI_SHAPE:
        case EDITOR_SELECTION_UI_TEXT:
        case EDITOR_SELECTION_UI_SLIDER:
        case EDITOR_SELECTION_UI_VERTEX:
        case EDITOR_SELECTION_UI_LINE:
            return (Color){190, 132, 245, 255};
        case EDITOR_SELECTION_INPUT_CONTROLLER:
        case EDITOR_SELECTION_INPUT_ACTION:
        case EDITOR_SELECTION_INPUT_BINDING:
            return (Color){245, 166, 78, 255};
        default:
            return (Color){86, 184, 235, 255};
    }
}

void editor_mode_element_icon_draw(EditorHierarchySelection kind,
        UIRect bounds) {
    Color color;
    Position center;
    float scale;
    float half;
    UIRect box;
    if(kind == EDITOR_SELECTION_NONE || bounds.width <= 0.0f ||
            bounds.height <= 0.0f) return;
    color = editor_mode_element_category_color_get(kind);
    center = (Position){bounds.x + bounds.width * 0.5f,
        bounds.y + bounds.height * 0.5f};
    scale = fminf(bounds.width, bounds.height) / 20.0f;
    half = 7.0f * scale;
    box = (UIRect){center.x - half, center.y - half, half * 2.0f,
        half * 2.0f};
    switch(kind) {
        case EDITOR_SELECTION_OBJECT:
            rohr_ui_border(box, 1.5f * scale, color);
            rohr_ui_border((UIRect){box.x + 4.0f * scale,
                box.y + 4.0f * scale, box.width - 8.0f * scale,
                box.height - 8.0f * scale}, 1.0f * scale, color);
            break;
        case EDITOR_SELECTION_RIGID_BODY:
            rohr_ui_surface((UIRect){center.x - 5.0f * scale,
                center.y - 5.0f * scale, 10.0f * scale, 10.0f * scale},
                color);
            break;
        case EDITOR_SELECTION_PARTICLE:
        case EDITOR_SELECTION_SOFT_NODE:
        case EDITOR_SELECTION_VERTEX:
        case EDITOR_SELECTION_UI_VERTEX:
            rohr_ui_quad(center,
                kind == EDITOR_SELECTION_PARTICLE ? 10.0f * scale :
                    7.0f * scale,
                kind == EDITOR_SELECTION_PARTICLE ? 10.0f * scale :
                    7.0f * scale,
                45.0f, color);
            break;
        case EDITOR_SELECTION_SOFT_AREA:
        case EDITOR_SELECTION_SOFT_HOLE:
        case EDITOR_SELECTION_HITBOX:
            rohr_ui_border(box, 1.0f * scale, color);
            for(size_t corner = 0; corner < 4; corner += 1)
                rohr_ui_surface((UIRect){
                    corner % 2 == 0 ? box.x - scale :
                        box.x + box.width - scale,
                    corner < 2 ? box.y - scale : box.y + box.height - scale,
                    2.0f * scale, 2.0f * scale}, color);
            break;
        case EDITOR_SELECTION_JOINT:
            editor_mode_icon_line((Position){center.x - 5.0f * scale,
                center.y + 4.0f * scale},
                (Position){center.x + 5.0f * scale,
                    center.y - 4.0f * scale}, 2.0f * scale, color);
            rohr_ui_quad((Position){center.x - 5.0f * scale,
                center.y + 4.0f * scale}, 5.0f * scale, 5.0f * scale,
                45.0f, color);
            rohr_ui_quad((Position){center.x + 5.0f * scale,
                center.y - 4.0f * scale}, 5.0f * scale, 5.0f * scale,
                45.0f, color);
            break;
        case EDITOR_SELECTION_TORQUE: {
            Position previous={center.x,center.y-half};
            for(size_t i=1;i<=16;++i) {
                float angle=math_degrees_to_radians(270.0f*i/16);
                Position next={center.x+half*sinf(angle),center.y-half*cosf(angle)};
                editor_mode_icon_line(previous,next,2*scale,color); previous=next;
            }
            editor_mode_icon_line(previous,(Position){previous.x-3*scale,previous.y+4*scale},2*scale,color);
            editor_mode_icon_line(previous,(Position){previous.x+3*scale,previous.y+4*scale},2*scale,color);
            break;
        }
        case EDITOR_SELECTION_FORCE:
            editor_mode_icon_line((Position){center.x,center.y+half},(Position){center.x,center.y-half},2*scale,color);
            editor_mode_icon_line((Position){center.x,center.y-half},(Position){center.x-half,center.y},2*scale,color);
            editor_mode_icon_line((Position){center.x,center.y-half},(Position){center.x+half,center.y},2*scale,color);
            break;
        case EDITOR_SELECTION_ANCHOR:
        case EDITOR_SELECTION_ORIGIN:
            editor_mode_icon_line((Position){center.x - half, center.y},
                (Position){center.x + half, center.y}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x, center.y - half},
                (Position){center.x, center.y + half}, 2.0f * scale, color);
            if(kind == EDITOR_SELECTION_ANCHOR)
                rohr_ui_quad(center, 5.0f * scale, 5.0f * scale,
                    45.0f, color);
            break;
        case EDITOR_SELECTION_SOFT_BODY:
            editor_mode_icon_line((Position){center.x, center.y - half},
                (Position){center.x + half, center.y}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x + half, center.y},
                (Position){center.x, center.y + half}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x, center.y + half},
                (Position){center.x - half, center.y}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x - half, center.y},
                (Position){center.x, center.y - half}, 2.0f * scale, color);
            break;
        case EDITOR_SELECTION_SOFT_BEAM:
        case EDITOR_SELECTION_LINE:
        case EDITOR_SELECTION_UI_LINE:
            editor_mode_icon_line((Position){center.x - 6.0f * scale,
                center.y + 5.0f * scale},
                (Position){center.x + 6.0f * scale,
                    center.y - 5.0f * scale}, 2.5f * scale, color);
            break;
        case EDITOR_SELECTION_UI_SHAPE:
            editor_mode_icon_line((Position){center.x, center.y - half},
                (Position){center.x + half, center.y + half},
                2.0f * scale, color);
            editor_mode_icon_line((Position){center.x + half,
                center.y + half}, (Position){center.x - half,
                center.y + half}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x - half,
                center.y + half}, (Position){center.x,
                center.y - half}, 2.0f * scale, color);
            break;
        case EDITOR_SELECTION_SPRITE:
        case EDITOR_SELECTION_ANIMATED_SPRITE:
        case EDITOR_SELECTION_ANIMATION_FRAME:
            if(kind == EDITOR_SELECTION_ANIMATED_SPRITE)
                rohr_ui_border((UIRect){box.x - 2.0f * scale,
                    box.y - 2.0f * scale, box.width, box.height},
                    1.0f * scale, color);
            rohr_ui_border(box, 1.5f * scale, color);
            if(kind == EDITOR_SELECTION_ANIMATION_FRAME) {
                for(int side = -1; side <= 1; side += 2)
                    for(int row = -1; row <= 1; row += 2)
                        rohr_ui_surface((UIRect){center.x +
                                (float)side * 5.0f * scale - scale,
                            center.y + (float)row * 4.0f * scale - scale,
                            2.0f * scale, 2.0f * scale}, color);
            } else {
                editor_mode_icon_line((Position){box.x + 2.0f * scale,
                    box.y + box.height - 3.0f * scale},
                    (Position){center.x - scale, center.y},
                    1.5f * scale, color);
                editor_mode_icon_line((Position){center.x - scale, center.y},
                    (Position){box.x + box.width - 2.0f * scale,
                        box.y + box.height - 3.0f * scale},
                    1.5f * scale, color);
            }
            break;
        case EDITOR_SELECTION_CAMERA:
            rohr_ui_surface((UIRect){center.x - 7.0f * scale,
                center.y - 4.0f * scale, 9.0f * scale, 8.0f * scale},
                color);
            editor_mode_icon_line((Position){center.x + 2.0f * scale,
                center.y - 3.0f * scale},
                (Position){center.x + 7.0f * scale,
                    center.y - 6.0f * scale}, 3.0f * scale, color);
            editor_mode_icon_line((Position){center.x + 2.0f * scale,
                center.y + 3.0f * scale},
                (Position){center.x + 7.0f * scale,
                    center.y + 6.0f * scale}, 3.0f * scale, color);
            break;
        case EDITOR_SELECTION_LAYOUT_VIEWPORT:
            editor_mode_icon_line((Position){box.x, box.y},
                (Position){box.x + 5.0f * scale, box.y}, 2.0f * scale, color);
            editor_mode_icon_line((Position){box.x, box.y},
                (Position){box.x, box.y + 5.0f * scale}, 2.0f * scale, color);
            editor_mode_icon_line((Position){box.x + box.width, box.y},
                (Position){box.x + box.width - 5.0f * scale, box.y},
                2.0f * scale, color);
            editor_mode_icon_line((Position){box.x + box.width, box.y},
                (Position){box.x + box.width, box.y + 5.0f * scale},
                2.0f * scale, color);
            editor_mode_icon_line((Position){box.x, box.y + box.height},
                (Position){box.x + 5.0f * scale, box.y + box.height},
                2.0f * scale, color);
            editor_mode_icon_line((Position){box.x + box.width,
                box.y + box.height}, (Position){box.x + box.width -
                    5.0f * scale, box.y + box.height}, 2.0f * scale, color);
            break;
        case EDITOR_SELECTION_UI_TEXT:
            editor_mode_icon_line((Position){center.x - 6.0f * scale,
                center.y - 6.0f * scale},
                (Position){center.x + 6.0f * scale,
                    center.y - 6.0f * scale}, 2.0f * scale, color);
            editor_mode_icon_line((Position){center.x,
                center.y - 6.0f * scale},
                (Position){center.x, center.y + 6.0f * scale},
                2.0f * scale, color);
            break;
        case EDITOR_SELECTION_UI_SLIDER:
            editor_mode_icon_line((Position){center.x - half, center.y},
                (Position){center.x + half, center.y}, 2.0f * scale, color);
            rohr_ui_quad(center, 6.0f * scale, 10.0f * scale, 0.0f, color);
            break;
        case EDITOR_SELECTION_INPUT_CONTROLLER:
            rohr_ui_surface((UIRect){center.x - 2.0f * scale,
                center.y - 7.0f * scale, 4.0f * scale, 14.0f * scale},
                color);
            rohr_ui_surface((UIRect){center.x - 7.0f * scale,
                center.y - 2.0f * scale, 14.0f * scale, 4.0f * scale},
                color);
            break;
        case EDITOR_SELECTION_INPUT_ACTION:
            editor_mode_icon_line((Position){center.x - 3.0f * scale,
                center.y - 7.0f * scale},
                (Position){center.x + 2.0f * scale, center.y - scale},
                2.5f * scale, color);
            editor_mode_icon_line((Position){center.x + 2.0f * scale,
                center.y - scale},
                (Position){center.x - 2.0f * scale,
                    center.y + 2.0f * scale}, 2.5f * scale, color);
            editor_mode_icon_line((Position){center.x - 2.0f * scale,
                center.y + 2.0f * scale},
                (Position){center.x + 3.0f * scale,
                    center.y + 7.0f * scale}, 2.5f * scale, color);
            break;
        case EDITOR_SELECTION_INPUT_BINDING:
            rohr_ui_border(box, 1.5f * scale, color);
            rohr_ui_surface((UIRect){center.x - 3.0f * scale,
                center.y + 3.0f * scale, 6.0f * scale, 2.0f * scale},
                color);
            break;
        case EDITOR_SELECTION_CENTER_OF_MASS:
        case EDITOR_SELECTION_NONE:
            break;
    }
}

UIButtonStyle editor_mode_section_field_style_get(void) {
    return (UIButtonStyle){
        .idle = {36, 40, 48, 255},
        .hovered = {49, 55, 66, 255},
        .pressed = {28, 32, 39, 255},
        .disabled = {42, 44, 49, 210}
    };
}

static const char *editor_mode_external_name;
void editor_mode_external_name_set(const char *id) { editor_mode_external_name = id; }

UIFieldResult editor_mode_field(const char *id, UIFieldBinding binding,
        TextAsset *display, UIRect bounds, const UIButtonStyle *style) {
    UIButtonStyle baseline;
    if(id && editor_mode_external_name && !strcmp(id, editor_mode_external_name)) return (UIFieldResult){0};
    if(id != NULL && editor_mode_pending_field_focus != NULL &&
            strcmp(id, editor_mode_pending_field_focus) == 0) {
        ui_field_focus_set(id, binding, display, true);
        editor_mode_pending_field_focus = NULL;
    }
    if(style == NULL) {
        baseline = editor_mode_section_field_style_get();
        style = &baseline;
    }
    return rohr_ui_field(id, binding, display, bounds, style);
}

UIDropdownResult editor_mode_dropdown(const char *id,
        const TextAsset *const *options, size_t option_count,
        size_t selected_index, UIRect bounds, const UIButtonStyle *style) {
    UIButtonStyle baseline;
    if(style == NULL) {
        baseline = editor_mode_section_field_style_get();
        style = &baseline;
    }
    return rohr_ui_dropdown(id, options, option_count, selected_index,
        (UIDropdownConfig){
            .visible_row_limit = EDITOR_DROPDOWN_VISIBLE_ROW_LIMIT,
            .fit_window = true
        }, bounds, style);
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
    background = interaction.pressed ? (Color){28, 32, 39, 255} :
        interaction.hovered || interaction.focused ? (Color){49, 55, 66, 255} :
        (Color){36, 40, 48, 255};
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

bool editor_mode_visibility_field(const char *id,
        const TextAsset *label,
        const TextAsset *visible_icon,
        const TextAsset *hidden_icon,
        UIRect bounds,
        bool *visible) {
    UIRect icon_bounds;
    if(id == NULL || label == NULL || visible_icon == NULL ||
            hidden_icon == NULL || visible == NULL) return false;
    icon_bounds = (UIRect){bounds.x, bounds.y, bounds.height, bounds.height};
    bool clicked;
    (void)visible_icon;
    (void)hidden_icon;
    clicked = editor_mode_visibility_button(id, *visible, false,
        icon_bounds).clicked;
    if(clicked) *visible = !*visible;
    rohr_ui_label(label, (UIRect){bounds.x + bounds.height + 6.0f, bounds.y,
        bounds.width - bounds.height - 6.0f, bounds.height});
    return clicked;
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
    EditorModeNamedTextEntry *entry = NULL;
    if(font == NULL || name == NULL || label == NULL || cache == NULL ||
            cache_capacity == 0) return false;
    for(size_t i = 0; i < MAX_TEXT_ASSETS; i += 1) {
        if(editor_mode_named_text_entries[i].label == label) {
            entry = &editor_mode_named_text_entries[i];
            break;
        }
        if(entry == NULL && editor_mode_named_text_entries[i].label == NULL)
            entry = &editor_mode_named_text_entries[i];
    }
    if(entry == NULL) {
        editor_mode_text_error_set("editor named-text cache capacity exceeded");
        return false;
    }
    entry->label = label;
    entry->cache = cache;
    entry->cache_capacity = cache_capacity;
    if(rohr_graphics_text_valid_check(*label) &&
            strncmp(cache, name, cache_capacity) == 0) return true;
    if(!rohr_graphics_text_valid_check(*label)) {
        if(!editor_mode_text_create(font, name, label)) return false;
    } else if(!rohr_graphics_text_value_set(label, name)) {
        editor_mode_text_error_set("editor text value update failed");
        return false;
    }
    snprintf(cache, cache_capacity, "%s", name);
    return true;
}

void editor_mode_named_text_destroy(TextAsset *label, char *cache,
        size_t cache_capacity) {
    if(label == NULL || cache == NULL || cache_capacity == 0) return;
    for(size_t i = 0; i < MAX_TEXT_ASSETS; i += 1) {
        if(editor_mode_named_text_entries[i].label != label) continue;
        editor_mode_named_text_entries[i] = (EditorModeNamedTextEntry){0};
        break;
    }
    rohr_graphics_text_destroy(label);
    memset(cache, 0, cache_capacity);
}

void editor_mode_named_text_clear_all(void) {
    for(size_t i = 0; i < MAX_TEXT_ASSETS; i += 1) {
        EditorModeNamedTextEntry *entry = &editor_mode_named_text_entries[i];
        if(entry->label == NULL) continue;
        rohr_graphics_text_destroy(entry->label);
        if(entry->cache != NULL && entry->cache_capacity > 0)
            memset(entry->cache, 0, entry->cache_capacity);
        *entry = (EditorModeNamedTextEntry){0};
    }
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
        editor_mode_named_text_destroy(&cache->labels[i], cache->values[i],
            EDITOR_OBJECT_NAME_MAX);
    free(cache->labels);
    free(cache->values);
    *cache = (EditorModeTextCache){0};
}

UIFieldResult editor_mode_name_field(const char *id,
        char *name,
        size_t capacity,
        TextAsset *display,
        UIRect bounds) {
    UIFieldResult result = editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = capacity}, display, bounds, NULL);
    if(result.changed) editor_project_property_name_format(name, capacity, name);
    return result;
}

const char *editor_mode_name_field_id_get(EditorViewportMode mode) {
    switch(mode) {
        case EDITOR_VIEWPORT_OBJECT: return "editor.object.name";
        case EDITOR_VIEWPORT_RIGID_BODY: return "editor.rigid_body.name";
        case EDITOR_VIEWPORT_PARTICLE: return "editor.particle.name";
        case EDITOR_VIEWPORT_HITBOX: return "editor.hitbox.name";
        case EDITOR_VIEWPORT_JOINT: return "editor.joint.name";
        case EDITOR_VIEWPORT_FORCE: return "editor.force.name";
        case EDITOR_VIEWPORT_TORQUE: return "editor.torque.name";
        case EDITOR_VIEWPORT_ANCHOR: return "editor.anchor.name";
        case EDITOR_VIEWPORT_SOFT_BODY: return "editor.soft_body.name";
        case EDITOR_VIEWPORT_SOFT_NODE: return "editor.soft_node.name";
        case EDITOR_VIEWPORT_SOFT_BEAM: return "editor.soft_beam.name";
        case EDITOR_VIEWPORT_SOFT_AREA: return "editor.soft_area.name";
        case EDITOR_VIEWPORT_SOFT_HOLE: return "editor.soft_hole.name";
        case EDITOR_VIEWPORT_LINE: return "editor.line.name";
        case EDITOR_VIEWPORT_VERTEX: return "editor.vertex.name";
        case EDITOR_VIEWPORT_SPRITE: return "editor.sprite.name";
        case EDITOR_VIEWPORT_ANIMATED_SPRITE:
            return "editor.animated_sprite.name";
        case EDITOR_VIEWPORT_ANIMATION_FRAME:
            return "editor.animation_frame.name";
        case EDITOR_VIEWPORT_CAMERA_ENTITY: return "editor.camera.name";
        case EDITOR_VIEWPORT_LAYOUT: return "editor.layout.name";
        case EDITOR_VIEWPORT_LAYOUT_CAMERA_EDITOR:
            return "editor.layout.camera.name";
        case EDITOR_VIEWPORT_UI_SHAPE_EDITOR:
        case EDITOR_VIEWPORT_UI_TEXT_EDITOR:
        case EDITOR_VIEWPORT_UI_SLIDER_EDITOR:
            return "editor.layout.ui.name";
        case EDITOR_VIEWPORT_INPUT_CONTROLLER:
            return "editor.input.controller.name";
        case EDITOR_VIEWPORT_INPUT_ACTION:
            return "editor.input.action.name";
        case EDITOR_VIEWPORT_INPUT_BINDING:
            return "editor.input.binding.name";
        default: return NULL;
    }
}

bool editor_mode_name_focus_request(EditorViewportState *state) {
    if(state == NULL || editor_mode_name_field_id_get(state->mode) == NULL)
        return false;
    state->name_focus_mode = state->mode;
    state->name_focus_requested = true;
    return true;
}

bool editor_mode_name_focus_pending_check(const EditorViewportState *state) {
    return state != NULL && state->name_focus_requested &&
        state->name_focus_mode == state->mode;
}

void editor_mode_name_focus_apply(EditorViewportState *state) {
    const char *id;
    editor_mode_pending_field_focus = NULL;
    if(state == NULL || !state->name_focus_requested) return;
    if(state->name_focus_mode != state->mode) {
        state->name_focus_requested = false;
        return;
    }
    id = editor_mode_name_field_id_get(state->mode);
    state->name_focus_requested = false;
    if(id != NULL) editor_mode_pending_field_focus = id;
}

UIButtonStyle editor_mode_delete_style_get(void) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    style.idle = (Color){120, 38, 42, 255};
    style.hovered = (Color){155, 46, 52, 255};
    style.pressed = (Color){92, 29, 33, 255};
    style.disabled = (Color){70, 45, 47, 255};
    return style;
}

static int editor_mode_layer_value_get(float value) {
    if(!isfinite(value)) return 0;
    if((double)value >= INT_MAX) return INT_MAX;
    if((double)value <= INT_MIN) return INT_MIN;
    return (int)value;
}

bool editor_mode_layer_control_create(EditorModeLayerControl *control,
        FontAsset *font) {
    if(control == NULL || font == NULL) return false;
    *control = (EditorModeLayerControl){.font = font};
    if(!editor_mode_text_create(font, "Render Layer", &control->layer_label) ||
            !editor_mode_text_create(font, "Direct", &control->direct_label) ||
            !editor_mode_text_create(font, "Add Layer", &control->add_label) ||
            !editor_mode_text_create(font, "Edit", &control->edit_label) ||
            !editor_mode_text_create(font, "Save", &control->save_label) ||
            !editor_mode_text_create(font, "Cancel", &control->cancel_label) ||
            !editor_mode_text_create(font, "Delete", &control->delete_label) ||
            !editor_mode_text_create(font, "Inherit Parent Layer",
                &control->inherit_label) ||
            !editor_mode_text_create(font, "", &control->name_field) ||
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
    rohr_graphics_text_destroy(&control->add_label);
    rohr_graphics_text_destroy(&control->edit_label);
    rohr_graphics_text_destroy(&control->save_label);
    rohr_graphics_text_destroy(&control->cancel_label);
    rohr_graphics_text_destroy(&control->delete_label);
    rohr_graphics_text_destroy(&control->inherit_label);
    rohr_graphics_text_destroy(&control->name_field);
    rohr_graphics_text_destroy(&control->value_field);
    for(size_t i = 0; i < MAX_GRAPHICS_LAYERS; i += 1)
        rohr_graphics_text_destroy(&control->names[i]);
    *control = (EditorModeLayerControl){0};
}

float editor_mode_layer_control_height_get(const EditorModeLayerControl *control,
        EditorGraphicsLayerBinding binding, const bool *inherited) {
    if(!control) return 0;
    if(inherited && *inherited) return 38;
    return (inherited ? 38 : 0) +
        (binding.layer == 0 || control->adding || control->edited_layer ? 86 : 48);
}

bool editor_mode_layer_control_draw(EditorModeLayerControl *control,
        const char *id_prefix, EditorProject *project,
        EditorGraphicsLayerBinding *binding, bool *inherited,
        float x, float y, float width) {
    const TextAsset *options[MAX_GRAPHICS_LAYERS + 2];
    UIButtonStyle field_style = editor_mode_section_field_style_get();
    char dropdown_id[128], value_id[128], inherit_id[128];
    char name_id[128], save_id[128], cancel_id[128], delete_id[128];
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
    options[1] = &control->add_label;
    for(size_t i = 0; i < project->graphics_layer_count; i += 1) {
        char option_text[GRAPHICS_LAYER_NAME_MAX + 32];
        snprintf(option_text, sizeof(option_text), "%s : %d",
            project->graphics_layers[i].name, project->graphics_layers[i].value);
        if(!editor_mode_named_text_sync(control->font, option_text,
                &control->names[i], control->name_cache[i],
                sizeof(control->name_cache[i]))) return false;
        options[i + 2] = &control->names[i];
        if(binding->layer == project->graphics_layers[i].id) selected = i + 2;
    }
    rohr_ui_label(&control->layer_label, (UIRect){x + 8.0f, y, 92.0f, 28.0f});
    snprintf(dropdown_id, sizeof(dropdown_id), "%s.layer", id_prefix);
    UIDropdownResult selected_result = rohr_ui_dropdown_actions(dropdown_id, options,
        project->graphics_layer_count + 2, selected, &control->edit_label, 2,
        (UIDropdownConfig){
            .visible_row_limit = EDITOR_DROPDOWN_VISIBLE_ROW_LIMIT,
            .fit_window = true
        }, (UIRect){x + 104.0f, y, width - 114.0f, 28.0f}, &field_style);
    if(selected_result.changed && selected_result.selected_index == 0) {
        binding->layer = 0;
        control->adding = false;
        control->edited_layer = 0;
    } else if(selected_result.changed && selected_result.selected_index == 1) {
        snprintf(control->edited_name, sizeof(control->edited_name), "layer_%u",
            project->next_graphics_layer_id);
        control->edited_value = 0.0f;
        control->edited_layer = 0;
        control->adding = true;
    } else if(selected_result.changed) {
        binding->layer = project->graphics_layers[
            selected_result.selected_index - 2].id;
        control->adding = false;
        control->edited_layer = 0;
    }
    if(selected_result.action_index >= 2) {
        EditorGraphicsLayer *layer = &project->graphics_layers[
            (size_t)selected_result.action_index - 2];
        snprintf(control->edited_name, sizeof(control->edited_name), "%s",
            layer->name);
        control->edited_value = (float)layer->value;
        control->edited_layer = layer->id;
        control->adding = false;
    }
    if(control->adding || control->edited_layer != 0) {
        float row_width = width - 20.0f;
        float name_width = row_width * 0.38f;
        float value_width = row_width * 0.18f;
        float button_width = row_width * 0.14f;
        y += 38.0f;
        snprintf(name_id, sizeof(name_id), "%s.layer_edit_name", id_prefix);
        snprintf(value_id, sizeof(value_id), "%s.layer_edit_value", id_prefix);
        snprintf(save_id, sizeof(save_id), "%s.layer_edit_save", id_prefix);
        snprintf(cancel_id, sizeof(cancel_id), "%s.layer_edit_cancel", id_prefix);
        snprintf(delete_id, sizeof(delete_id), "%s.layer_edit_delete", id_prefix);
        UIFieldResult name_result = rohr_ui_field(name_id,
            (UIFieldBinding){.kind = UI_FIELD_STRING,
            .string = control->edited_name,
                .string_capacity = sizeof(control->edited_name)},
            &control->name_field, (UIRect){x + 10.0f, y, name_width, 28.0f},
            &field_style);
        UIFieldResult edit_value = rohr_ui_field(value_id,
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &control->edited_value}, &control->value_field,
            (UIRect){x + 10.0f + name_width, y, value_width, 28.0f},
            &field_style);
        UIButtonResult save_result = rohr_ui_button(save_id, &control->save_label,
                (UIRect){x + 10.0f + name_width + value_width, y,
                    button_width, 28.0f}, &field_style);
        float cancel_width = control->adding ?
            row_width - name_width - value_width - button_width : button_width;
        UIButtonResult cancel_result = rohr_ui_button(cancel_id,
            &control->cancel_label,
            (UIRect){x + 10.0f + name_width + value_width + button_width, y,
                cancel_width, 28.0f}, &field_style);
        UIButtonResult delete_result = {0};
        if(save_result.clicked) {
            editor_project_property_name_format(control->edited_name,
                sizeof(control->edited_name), control->edited_name);
            if(control->edited_name[0] != '\0') {
                bool saved = false;
                if(control->adding) {
                    EditorGraphicsLayer *created = editor_project_graphics_layer_add(
                        project, control->edited_name, editor_mode_layer_value_get(control->edited_value));
                    if(created != NULL) {
                        binding->layer = created->id;
                        saved = true;
                    }
                } else {
                    saved = editor_project_graphics_layer_set(project,
                        control->edited_layer, control->edited_name,
                        editor_mode_layer_value_get(control->edited_value));
                }
                if(saved) {
                    control->adding = false;
                    control->edited_layer = 0;
                }
            }
        }
        if(!control->adding) delete_result = rohr_ui_button(delete_id,
                &control->delete_label,
                (UIRect){x + 10.0f + name_width + value_width +
                        button_width * 2.0f, y,
                    row_width - name_width - value_width - button_width * 2.0f,
                    28.0f}, &field_style);
        if(delete_result.clicked) {
            EditorGraphicsLayer *removed = editor_project_graphics_layer_get(project, control->edited_layer);
            int value = removed ? removed->value : binding->value;
            if(editor_project_graphics_layer_remove(project, control->edited_layer) &&
                    binding->layer == control->edited_layer)
                *binding = (EditorGraphicsLayerBinding){.value = value};
            control->edited_layer = 0;
        }
        if(cancel_result.clicked || rohr_ui_key_pressed_check(SDLK_ESCAPE) ||
                ((control->adding || control->edited_layer != 0) &&
                    rohr_ui_primary_pressed_check())) {
            bool inside = selected_result.button_hovered || name_result.hovered ||
                edit_value.hovered || save_result.hovered || cancel_result.hovered ||
                delete_result.hovered;
            if(cancel_result.clicked || rohr_ui_key_pressed_check(SDLK_ESCAPE) ||
                    !inside) {
                control->adding = false;
                control->edited_layer = 0;
                rohr_ui_field_focus_clear();
            }
        }
        return name_result.active || edit_value.active;
    }
    if(binding->layer == 0) {
        value = (float)binding->value;
        y += 38.0f;
        rohr_ui_label(&control->direct_label,
            (UIRect){x + 8.0f, y, 92.0f, 28.0f});
        snprintf(value_id, sizeof(value_id), "%s.layer_value", id_prefix);
        UIFieldResult result = rohr_ui_field(value_id,
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &value},
            &control->value_field,
            (UIRect){x + 104.0f, y, width - 114.0f, 28.0f}, &field_style);
        if(result.changed) binding->value = editor_mode_layer_value_get(value);
        active = result.active;
    }
    return active;
}

static const char *visual_labels[]={"Fill Color","Border Color","Border Thickness","Scale X","Scale Y","Offset X","Offset Y","Rotation (deg)"};
bool editor_visual_controls_create(EditorVisualControls *v,FontAsset *font) {
    if(!v || !font) return false;
    *v=(EditorVisualControls){0};
    if(!editor_mode_accordion_section_create(&v->outer,font,"Appearance",true)) goto fail;
    const char *parts[]={"Spring","Endpoint A","Endpoint B"};
    for(size_t p=0;p<3;++p) {
        if(!editor_mode_accordion_section_create(&v->parts[p],font,parts[p],true)) goto fail;
        for(size_t i=0;i<8;++i) if(!editor_mode_text_create(font,"",&v->fields[p][i])) goto fail;
    }
    for(size_t i=0;i<8;++i) if(!editor_mode_text_create(font,visual_labels[i],&v->labels[i])) goto fail;
    return true;
fail: editor_visual_controls_destroy(v); return false;
}
void editor_visual_controls_destroy(EditorVisualControls *v) {
    if(!v) return;
    editor_mode_accordion_section_destroy(&v->outer);
    for(size_t p=0;p<3;++p) {
        editor_mode_accordion_section_destroy(&v->parts[p]);
        for(size_t i=0;i<8;++i) rohr_graphics_text_destroy(&v->fields[p][i]);
    }
    for(size_t i=0;i<8;++i) rohr_graphics_text_destroy(&v->labels[i]);
    *v=(EditorVisualControls){0};
}
bool editor_visual_controls_draw(EditorVisualControls *v,const EditorModeContext *c,
        EditorModeAccordionLayoutCursor *cursor,EditorItemKind kind,EditorObjectId object,
        uint32_t item,bool spring) {
    ObjectVisuals *visuals=editor_command_visuals_get(c->project,kind,object,item);
    if(!visuals) return false;
    bool reset=v->object!=object || v->item!=item || v->kind!=kind,editing=false;
    v->object=object; v->item=item; v->kind=kind;
    float rows[8]={28,28,28,28,28,28,28,28};
    EditorGraphicsLayerBinding *binding = editor_command_layer_get(c->project, kind, object, 0, item, NULL);
    float layer_height = binding ? editor_mode_layer_control_height_get(c->layer_control, *binding, NULL) : 0;
    float height=0;
    if(spring) for(size_t p=0;p<3;++p) {
        EditorModeAccordionLayoutMetrics m=editor_mode_accordion_layout_metrics_get(height,cursor->padding,cursor->section_gap,rows,8,6,v->parts[p].expanded);
        height=m.next_y;
    }
    float outer_rows[] = {layer_height, spring ? height : 8 * 28 + 7 * 6};
    EditorModeAccordionLayoutResult outer=editor_mode_accordion_layout_section(cursor,&v->outer,
        "editor.visuals",outer_rows,2,6);
    if(!outer.expanded) return false;
    if(binding) editing |= editor_mode_layer_control_draw(c->layer_control,
        "editor.visuals", c->project, binding, NULL, cursor->x, outer.content_y, cursor->width);
    outer.content_y += layer_height + 6;
    EditorModeAccordionLayoutCursor nested=editor_mode_accordion_layout_cursor_get(cursor->x+8,cursor->width-16,outer.content_y);
    for(size_t p=0;p<(spring?3u:1u);++p) {
        char section_id[64]; snprintf(section_id,sizeof(section_id),"editor.visuals.part.%zu",p);
        EditorModeAccordionLayoutResult section=spring?editor_mode_accordion_layout_section(&nested,&v->parts[p],section_id,rows,8,6):outer;
        if(!section.expanded) continue;
        ObjectVisualStyle style=object_visual_style_get(p==0?visuals->main:p==1?visuals->endpoint_a:visuals->endpoint_b,
            kind==EDITOR_ITEM_JOINT?OBJECT_JOINT:OBJECT_FORCE);
        float numbers[]={style.border_thickness,style.scale.x,style.scale.y,style.offset.x,style.offset.y,style.rotation};
        uint32_t colors[]={style.fill_color,style.border_color};
        for(size_t i=0;i<8;++i) {
            char id[80]; snprintf(id,sizeof(id),"editor.visuals.%zu.%zu",p,i);
            float x=spring?nested.x:cursor->x,width=spring?nested.width:cursor->width;
            float y=editor_mode_accordion_layout_row_y(&section,rows,i,6);
            rohr_ui_label(&v->labels[i],(UIRect){x+10,y,116,28});
            UIRect bounds={x+130,y,width-140,28};
            EditorPropertyKind property=EDITOR_PROPERTY_VISUAL_FILL_COLOR+p*8+i;
            if(i<2) {
                if(reset || colors[i]!=v->stored_colors[p][i]) v->colors[p][i]=colors[i];
                v->stored_colors[p][i]=colors[i];
                (void)editor_mode_color_swatch(id,&v->colors[p][i],false,bounds,c,kind,object,0,item,property);
            } else {
                UIFieldResult result=editor_mode_field(id,(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=&numbers[i-2]},&v->fields[p][i],bounds,NULL);
                editing|=result.active;
                if(result.changed) {
                    EditorCommand command={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={
                        .kind=kind,.object=object,.item=item,.property=property,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=numbers[i-2]}};
                    (void)editor_command_execute(c->project,&command);
                }
            }
        }
    }
    return editing;
}
