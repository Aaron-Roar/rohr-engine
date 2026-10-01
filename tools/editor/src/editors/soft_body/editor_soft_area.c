/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_soft_area.h"
#include <editor_soft_area.h>
#include "editor_navigation.h"
#include <stdio.h>
#include <string.h>

bool editor_soft_area_editor_create(EditorSoftAreaEditor *editor, FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorSoftAreaEditor){.font = font};
#define TEXT(field, value) if(!editor_mode_text_create(font, value, &editor->field)) goto fail
    TEXT(add_area, "Add Area"); TEXT(add_hole, "Add Hole"); TEXT(name_label, "Name");
    TEXT(name_field, ""); TEXT(visible, "[X]"); TEXT(hidden, "[ ]"); TEXT(visibility, "Visibility");
    TEXT(select_nodes, "Select nodes to define area");
    TEXT(picking, "Picking nodes (click to finish)"); TEXT(inactive, "Click to select nodes");
    TEXT(up, "Up"); TEXT(down, "Down"); TEXT(remove, "X"); TEXT(color, "Color");
    TEXT(delete_area, "Delete Area"); TEXT(delete_hole, "Delete Hole"); TEXT(status, "");
#undef TEXT
    if(!editor_mode_accordion_section_create(&editor->areas_section, font, "Areas", true) ||
            !editor_mode_accordion_section_create(&editor->appearance_section, font, "Appearance", true)) goto fail;
    return true;
fail:
    editor_soft_area_editor_destroy(editor);
    return false;
}
void editor_soft_area_editor_destroy(EditorSoftAreaEditor *editor) {
    if(editor == NULL) return;
#define TEXT(field) rohr_graphics_text_destroy(&editor->field)
    TEXT(add_area); TEXT(add_hole); TEXT(name_label); TEXT(name_field); TEXT(visible);
    TEXT(hidden); TEXT(visibility); TEXT(select_nodes); TEXT(picking); TEXT(inactive);
    TEXT(up); TEXT(down); TEXT(remove); TEXT(color); TEXT(delete_area); TEXT(delete_hole); TEXT(status);
#undef TEXT
    for(size_t i = 0; i < SOFT_BODY_MAX_AREAS; i += 1) rohr_graphics_text_destroy(&editor->area_names[i]);
    for(size_t i = 0; i < SOFT_BODY_MAX_AREA_HOLES; i += 1) rohr_graphics_text_destroy(&editor->hole_names[i]);
    for(size_t i = 0; i < SOFT_BODY_MAX_NODES; i += 1) rohr_graphics_text_destroy(&editor->node_names[i]);
    editor_mode_accordion_section_destroy(&editor->areas_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    *editor = (EditorSoftAreaEditor){0};
}
static void item_open(const EditorModeContext *context, EditorSelectionRef ref) {
    (void)editor_viewport_selection_set(context->project, context->viewport, ref, false);
    if(editor_navigation_selected_open(context->project, context->viewport))
        (void)editor_mode_name_focus_request(context->viewport);
}
static void row_draw(const EditorModeContext *context, EditorSelectionRef ref,
        TextAsset *text, UIRect bounds, const char *id, bool last) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    if(editor_viewport_selection_contains(context->viewport, ref)) {
        style.idle = (Color){118,96,35,255}; style.hovered = (Color){145,119,45,255};
    }
    UIButtonResult result = rohr_ui_button(id, text, bounds, &style);
    editor_mode_element_icon_draw(ref.kind,
        (UIRect){bounds.x + 2, bounds.y + 2, 20, 20});
    if(context->hierarchy_row != NULL)
        context->hierarchy_row(context->hierarchy_context, context->viewport, ref, bounds, result, last);
    if(result.clicked || result.focus_changed) {
        if(context->hierarchy_row == NULL)
            (void)editor_viewport_selection_set(context->project, context->viewport, ref, false);
        else {
            /* The shared row callback owns additive selection and drag history. */
            context->viewport->selection = ref.kind;
            context->viewport->selected_soft_body = ref.parent;
            context->viewport->selected_soft_area = ref.kind == EDITOR_SELECTION_SOFT_HOLE ? ref.container : ref.item;
            if(ref.kind == EDITOR_SELECTION_SOFT_HOLE) context->viewport->selected_soft_hole = ref.item;
        }
        if(result.double_clicked) (void)editor_navigation_selected_open(context->project, context->viewport);
    }
}
float editor_soft_area_list_draw(EditorSoftAreaEditor *editor,
        const EditorModeContext *context, EditorSoftBody *body, float y) {
    EditorObject *object = editor_project_selected_get(context->project);
    if(body == NULL || object == NULL) return y;
    EditorModeAccordionLayoutCursor cursor = editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    EditorModeAccordionLayoutGroup groups[] = {
        {.row_count = 1, .row_height = 30},
        {.row_count = body->area_count, .row_height = 28, .gap_before = 6}};
    EditorModeAccordionLayoutResult layout = editor_mode_accordion_layout_nested_section(&cursor,
        &editor->areas_section, "editor.soft_body.areas", groups, 2);
    if(!layout.expanded) return cursor.y;
    float add_y = editor_mode_accordion_layout_group_row_y(&layout, groups, 2, 0, 0);
    UIRect button = {context->x + 10, add_y, context->width - 20, 30};
    if(body->area_count == SOFT_BODY_MAX_AREAS) {
        rohr_ui_button_disabled(button, NULL); rohr_ui_label(&editor->add_area, button);
    } else if(rohr_ui_button("editor.soft_body.add_area", &editor->add_area, button, NULL).clicked) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
            .data.item_add = {.kind = EDITOR_ITEM_SOFT_AREA, .object = object->id, .parent = body->id}};
        EditorCommandResult result = editor_command_execute(context->project, &command);
        if(result.kind == ERROR_RESULT_VALUE) item_open(context,
            (EditorSelectionRef){EDITOR_SELECTION_SOFT_AREA, object->id, body->id, 0, result.result.object});
        return cursor.y;
    }
    for(size_t i = 0; i < body->area_count; i += 1) {
        EditorSoftArea *area = &body->areas[i];
        if(!editor_mode_named_text_sync(editor->font, area->name, &editor->area_names[i],
                editor->area_cache[i], sizeof(editor->area_cache[i]))) continue;
        char id[80]; snprintf(id, sizeof(id), "editor.area.%u.%u", body->id, area->id);
        float row_y = editor_mode_accordion_layout_group_row_y(&layout, groups, 2, 1, i);
        EditorSelectionRef ref = {EDITOR_SELECTION_SOFT_AREA, object->id, body->id, 0, area->id};
        row_draw(context, ref, &editor->area_names[i],
            (UIRect){context->x + 10, row_y, context->width - 54, 26}, id, i + 1 == body->area_count);
        snprintf(id, sizeof(id), "editor.area.%u.%u.visibility", body->id, area->id);
        if(rohr_ui_button(id, area->visible ? &editor->visible : &editor->hidden,
                (UIRect){context->x + context->width - 40, row_y, 30, 26}, NULL).clicked)
            (void)editor_navigation_selection_visibility_set(context->project, ref, !area->visible);
    }
    return cursor.y;
}

bool editor_soft_area_editor_draw(EditorSoftAreaEditor *editor, const EditorModeContext *context) {
    EditorViewportState *state = context->viewport;
    EditorObject *object = editor_project_selected_get(context->project);
    EditorSoftBody *body = editor_soft_body_get(object, state->selected_soft_body);
    EditorSoftArea *area = editor_soft_area_get(body, state->selected_soft_area);
    bool is_hole = state->mode == EDITOR_VIEWPORT_SOFT_HOLE;
    EditorSoftHole *hole = editor_soft_hole_get(area, state->selected_soft_hole);
    if(area == NULL || (is_hole && hole == NULL)) {
        state->soft_area_picking = false;
        return false;
    }
    EditorSoftAreaLoop *loop = is_hole ? &hole->loop : &area->outer;
    EditorSelectionRef ref = {is_hole ? EDITOR_SELECTION_SOFT_HOLE : EDITOR_SELECTION_SOFT_AREA,
        object->id, body->id, is_hole ? area->id : 0, is_hole ? hole->id : area->id};
    char name[EDITOR_OBJECT_NAME_MAX];
    snprintf(name, sizeof(name), "%s", is_hole ? hole->name : area->name);
    (void)rohr_graphics_text_value_set(&editor->name_field, name);
    rohr_ui_label(&editor->name_label, (UIRect){context->x + 10,40,65,30});
    UIFieldResult name_field = editor_mode_name_field(is_hole ? "editor.soft_hole.name" : "editor.soft_area.name",
        name, sizeof(name), &editor->name_field, (UIRect){context->x + 80,40,context->width - 90,30});
    if(name_field.changed) (void)editor_navigation_selection_name_set(context->project, ref, name);
    bool layer_active = false;
    float y = 80;
    if(!is_hole) {
        bool visible = area->visible;
        if(editor_mode_visibility_field("editor.soft_area.visibility", &editor->visibility,
                &editor->visible, &editor->hidden, (UIRect){context->x + 10,y,context->width - 20,28}, &visible))
            (void)editor_navigation_selection_visibility_set(context->project, ref, visible);
        y += 36;
        EditorModeAccordionLayoutCursor cursor = editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
        float rows[] = {28, context->layer_control != NULL &&
            (context->layer_control->adding || context->layer_control->edited_layer != 0) ? 86 : 48};
        EditorModeAccordionLayoutResult appearance = editor_mode_accordion_layout_section(&cursor,
            &editor->appearance_section, "editor.soft_area.appearance", rows, 2, 6);
        if(appearance.expanded) {
            float color_y = editor_mode_accordion_layout_row_y(&appearance, rows, 0, 6);
            rohr_ui_label(&editor->color, (UIRect){context->x + 10,color_y,65,28});
            (void)editor_mode_color_swatch("editor.soft_area.color", &area->color, false,
                (UIRect){context->x + 80,color_y,context->width - 90,28}, context,
                EDITOR_ITEM_SOFT_AREA, object->id, body->id, area->id, EDITOR_PROPERTY_COLOR);
            if(context->layer_control != NULL) {
                EditorGraphicsLayerBinding binding = area->graphics_layer;
                bool inherited = area->graphics_layer_inherited;
                layer_active = editor_mode_layer_control_draw(context->layer_control, "editor.soft_area",
                    context->project, &binding, &inherited,
                    context->x, editor_mode_accordion_layout_row_y(&appearance, rows, 1, 6), context->width);
                if(binding.value != area->graphics_layer.value || binding.layer != area->graphics_layer.layer ||
                        inherited != area->graphics_layer_inherited) {
                    EditorCommand command = {.type = EDITOR_COMMAND_SOFT_AREA_LAYER_SET,
                        .data.soft_area_layer = {object->id, body->id, area->id, binding, inherited}};
                    (void)editor_command_execute(context->project, &command);
                }
            }
        }
        y = cursor.y;
    }
    (void)rohr_graphics_text_value_set(&editor->select_nodes,
        is_hole ? "Select nodes to define hole" : "Select nodes to define area");
    rohr_ui_label(&editor->select_nodes, (UIRect){context->x + 10,y,context->width - 20,28}); y += 32;
    UIRect box = {context->x + 10,y,context->width - 20,32};
    if(rohr_ui_button("editor.soft_area.nodes", state->soft_area_picking ? &editor->picking : &editor->inactive,
            box, NULL).clicked) {
        editor_viewport_transform_cancel(state);
        state->soft_area_picking = !state->soft_area_picking;
    }
    rohr_ui_border(box, 2, state->soft_area_picking ? (Color){255,215,70,255} : (Color){100,110,130,255});
    y += 36;
    for(uint32_t i = 0; i < loop->node_count; i += 1) {
        const char *node_name = "unavailable";
        for(size_t n = 0; n < body->node_count; n += 1)
            if(body->nodes[n].id == loop->nodes[i]) node_name = body->nodes[n].name;
        char text[EDITOR_OBJECT_NAME_MAX + 16], id[80];
        snprintf(text, sizeof(text), "%u. %s", i + 1, node_name);
        (void)editor_mode_named_text_sync(editor->font, text, &editor->node_names[i],
            editor->node_cache[i], sizeof(editor->node_cache[i]));
        rohr_ui_label(&editor->node_names[i], (UIRect){context->x + 10,y,context->width - 125,26});
        EditorSoftAreaLoop edited = *loop; bool changed = false;
        const TextAsset *buttons[] = {&editor->up, &editor->down, &editor->remove};
        for(uint32_t b = 0; b < 3; b += 1) {
            snprintf(id, sizeof(id), "editor.soft_area.node.%u.%u", loop->nodes[i], b);
            UIRect bounds = {context->x + context->width - 112 + b * 34,y,32,26};
            bool enabled = b == 2 || (b == 0 ? i > 0 : i + 1 < loop->node_count);
            if(!enabled) { rohr_ui_button_disabled(bounds, NULL); rohr_ui_label(buttons[b], bounds); }
            else if(rohr_ui_button(id, buttons[b], bounds, NULL).clicked) {
                if(b == 2) { memmove(&edited.nodes[i], &edited.nodes[i + 1],
                    (edited.node_count - i - 1) * sizeof(edited.nodes[0])); edited.nodes[--edited.node_count] = 0; }
                else { uint32_t j = b == 0 ? i - 1 : i + 1;
                    EditorSoftNodeId node = edited.nodes[i]; edited.nodes[i] = edited.nodes[j]; edited.nodes[j] = node; }
                changed = true;
            }
        }
        if(changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_SOFT_AREA_LOOP_SET,
                .data.soft_area_loop = {object->id, body->id, area->id, is_hole ? hole->id : 0, edited}};
            (void)editor_command_execute(context->project, &command);
            return name_field.active || layer_active;
        }
        y += 30;
    }
    EditorSoftAreaFill fill = editor_soft_area_fill_get(body, area);
    (void)rohr_graphics_text_value_set(&editor->status, fill.complete ?
        "Last node closes to first" : "Incomplete area: generation blocked");
    rohr_ui_label(&editor->status, (UIRect){context->x + 10,y,context->width - 20,28}); y += 34;
    if(!is_hole) {
        UIRect add = {context->x + 10,y,context->width - 20,30}; y += 36;
        if(area->hole_count == SOFT_BODY_MAX_AREA_HOLES) {
            rohr_ui_button_disabled(add, NULL);
            (void)rohr_graphics_text_value_set(&editor->add_hole, "Maximum 16 holes");
            rohr_ui_label(&editor->add_hole, add);
        } else {
            (void)rohr_graphics_text_value_set(&editor->add_hole, "Add Hole");
            if(rohr_ui_button("editor.soft_area.add_hole", &editor->add_hole, add, NULL).clicked) {
                EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                    .data.item_add = {.kind = EDITOR_ITEM_SOFT_HOLE, .object = object->id,
                        .parent = body->id, .first = area->id}};
                EditorCommandResult result = editor_command_execute(context->project, &command);
                if(result.kind == ERROR_RESULT_VALUE) item_open(context,
                    (EditorSelectionRef){EDITOR_SELECTION_SOFT_HOLE, object->id, body->id, area->id, result.result.object});
                return name_field.active || layer_active;
            }
        }
        for(uint32_t i = 0; i < area->hole_count; i += 1) {
            EditorSoftHole *item = &area->holes[i];
            (void)editor_mode_named_text_sync(editor->font, item->name, &editor->hole_names[i],
                editor->hole_cache[i], sizeof(editor->hole_cache[i]));
            char id[80]; snprintf(id, sizeof(id), "editor.soft_hole.%u.%u", area->id, item->id);
            row_draw(context, (EditorSelectionRef){EDITOR_SELECTION_SOFT_HOLE, object->id, body->id, area->id, item->id},
                &editor->hole_names[i], (UIRect){context->x + 10,y,context->width - 20,26}, id, i + 1 == area->hole_count);
            y += 30;
        }
    }
    editor_mode_accordion_layout_measure_include(y + 12);
    return name_field.active || layer_active;
}
