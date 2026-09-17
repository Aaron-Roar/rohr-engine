/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_hierarchy.h"

#include "editor_navigation.h"
#include "editors/editor_mode_controls.h"

#include <stdio.h>

bool editor_hierarchy_editor_create(EditorHierarchyEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorHierarchyEditor){.font = font};
    if(!editor_mode_text_create(font, "Add Object", &editor->add_object_label) ||
            !editor_mode_text_create(font, "Add Viewport", &editor->add_viewport_label) ||
            !editor_mode_text_create(font, "[X]", &editor->visible_label) ||
            !editor_mode_text_create(font, "[ ]", &editor->hidden_label) ||
            !editor_mode_text_create(font, "Name", &editor->name_label) ||
            !editor_mode_text_create(font, "X", &editor->x_label) ||
            !editor_mode_text_create(font, "Y", &editor->y_label) ||
            !editor_mode_text_create(font, "", &editor->name_field) ||
            !editor_mode_text_create(font, "0", &editor->x_field) ||
            !editor_mode_text_create(font, "0", &editor->y_field) ||
            !editor_mode_text_create(font, "Delete Object",
                &editor->delete_object_label) ||
            !editor_mode_text_create(font, "Delete Viewport",
                &editor->delete_viewport_label)) {
        editor_hierarchy_editor_destroy(editor);
        return false;
    }
    return true;
}

void editor_hierarchy_editor_destroy(EditorHierarchyEditor *editor) {
    if(editor == NULL) return;
    rohr_graphics_text_destroy(&editor->add_object_label);
    rohr_graphics_text_destroy(&editor->add_viewport_label);
    rohr_graphics_text_destroy(&editor->visible_label);
    rohr_graphics_text_destroy(&editor->hidden_label);
    rohr_graphics_text_destroy(&editor->name_label);
    rohr_graphics_text_destroy(&editor->x_label);
    rohr_graphics_text_destroy(&editor->y_label);
    rohr_graphics_text_destroy(&editor->name_field);
    rohr_graphics_text_destroy(&editor->x_field);
    rohr_graphics_text_destroy(&editor->y_field);
    rohr_graphics_text_destroy(&editor->delete_object_label);
    rohr_graphics_text_destroy(&editor->delete_viewport_label);
    for(size_t i = 0; i < EDITOR_OBJECT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->object_names[i]);
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->viewport_names[i]);
    *editor = (EditorHierarchyEditor){0};
}

void editor_hierarchy_editor_draw(EditorHierarchyEditor *editor,
        const EditorModeContext *context) {
    EditorObject *selected_object;
    EditorLayoutViewport *selected_viewport;
    Position *overview_position = NULL;
    const char *selected_name = NULL;
    float list_y = 144.0f;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return;
    if(rohr_ui_button("editor.add_object", &editor->add_object_label,
            (UIRect){context->x + 10.0f, 42.0f,
                (context->width - 30.0f) * 0.5f, 38.0f}, NULL).clicked) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
            .data.item_add = {.kind = EDITOR_ITEM_OBJECT}};
        snprintf(command.data.item_add.name, sizeof(command.data.item_add.name),
            "Object%u", context->project->next_id);
        EditorCommandResult result = editor_command_execute(context->project, &command);
        if(result.kind == ERROR_RESULT_VALUE)
            context->viewport->selection = EDITOR_SELECTION_OBJECT;
    }
    if(rohr_ui_button("editor.add_viewport", &editor->add_viewport_label,
            (UIRect){context->x + 20.0f + (context->width - 30.0f) * 0.5f,
                42.0f, (context->width - 30.0f) * 0.5f, 38.0f}, NULL).clicked) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
            .data.item_add = {.kind = EDITOR_ITEM_LAYOUT_VIEWPORT}};
        snprintf(command.data.item_add.name, sizeof(command.data.item_add.name),
            "Viewport%u", context->project->next_layout_viewport_id);
        EditorCommandResult result = editor_command_execute(context->project, &command);
        if(result.kind == ERROR_RESULT_VALUE) {
            context->viewport->selected_layout_viewport = result.result.object;
            context->viewport->selection = EDITOR_SELECTION_LAYOUT_VIEWPORT;
            context->viewport->mode = EDITOR_VIEWPORT_LAYOUT;
            editor_project_selection_clear(context->project);
        }
    }
    selected_object = context->viewport->selection == EDITOR_SELECTION_OBJECT ?
        editor_project_selected_get(context->project) : NULL;
    selected_viewport = context->viewport->selection ==
            EDITOR_SELECTION_LAYOUT_VIEWPORT ?
        editor_project_layout_viewport_get(context->project,
            context->viewport->selected_layout_viewport) : NULL;
    if(selected_object != NULL) {
        overview_position = &selected_object->overview_position;
        selected_name = selected_object->name;
    } else if(selected_viewport != NULL) {
        overview_position = &selected_viewport->overview_position;
        selected_name = selected_viewport->name;
    }
    if(overview_position != NULL && selected_name != NULL) {
        char name[EDITOR_OBJECT_NAME_MAX];
        UIFieldResult name_result;
        rohr_ui_label(&editor->name_label,
            (UIRect){context->x + 10.0f, 92.0f, 54.0f, 28.0f});
        snprintf(name, sizeof(name), "%s", selected_name);
        name_result = rohr_ui_field("editor.hierarchy.selected.name",
            (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
                .string_capacity = sizeof(name)}, &editor->name_field,
            (UIRect){context->x + 64.0f, 92.0f,
                context->width - 74.0f, 28.0f}, NULL);
        rohr_ui_label(&editor->x_label,
            (UIRect){context->x + 10.0f, 126.0f, 24.0f, 28.0f});
        (void)rohr_ui_field("editor.hierarchy.selected.x",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &overview_position->x}, &editor->x_field,
            (UIRect){context->x + 34.0f, 126.0f,
                (context->width - 54.0f) * 0.5f, 28.0f}, NULL);
        rohr_ui_label(&editor->y_label,
            (UIRect){context->x + 44.0f +
                (context->width - 54.0f) * 0.5f, 126.0f, 24.0f, 28.0f});
        (void)rohr_ui_field("editor.hierarchy.selected.y",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &overview_position->y}, &editor->y_field,
            (UIRect){context->x + 68.0f +
                (context->width - 54.0f) * 0.5f, 126.0f,
                (context->width - 88.0f) * 0.5f, 28.0f}, NULL);
        if(name_result.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
                .data.item_rename = {.kind = selected_object != NULL ?
                    EDITOR_ITEM_OBJECT : EDITOR_ITEM_LAYOUT_VIEWPORT,
                    .object = selected_object == NULL ? 0 : selected_object->id,
                    .item = selected_viewport == NULL ? 0 : selected_viewport->id}};
            snprintf(command.data.item_rename.name,
                sizeof(command.data.item_rename.name), "%s", name);
            (void)editor_command_execute(context->project, &command);
        }
        list_y = 174.0f;
    }
    (void)rohr_graphics_screen_rect_draw(context->x + 10.0f, list_y - 12.0f,
        context->width - 20.0f, 1.0f, (Color){75, 84, 100, 255});
    editor_project_hierarchy_sync(context->project);
    for(size_t row = 0; row < context->project->hierarchy_count; row += 1) {
        EditorProjectHierarchyItem hierarchy_item =
            context->project->hierarchy[row];
        float y = list_y + (float)row * 34.0f;
        if(hierarchy_item.kind == EDITOR_PROJECT_HIERARCHY_OBJECT) {
        EditorObject *object = editor_object_query_get(context->project,
            hierarchy_item.id);
        size_t i = object == NULL ? context->project->object_count :
            (size_t)(object - context->project->objects);
        char id[64], visibility_id[72];
        if(object == NULL || i >= EDITOR_OBJECT_MAX) continue;
        EditorSelectionRef ref = {EDITOR_SELECTION_OBJECT,
            object->id, 0, 0, object->id};
        if(!editor_mode_named_text_sync(editor->font, object->name,
                &editor->object_names[i], editor->object_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.object.%u", object->id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.object.%u.visibility", object->id);
        if(rohr_ui_button(visibility_id, object->visible ? &editor->visible_label :
                &editor->hidden_label, (UIRect){context->x + 8.0f,
                    y + 1.0f, 26.0f, 26.0f}, NULL).clicked) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_OBJECT,
                    object->id, 0, 0, !object->visible}};
            (void)editor_command_execute(context->project, &command);
        }
        UIButtonStyle style = rohr_ui_button_style_default_get();
        style.idle = (Color){118, 96, 35, 255};
        style.hovered = (Color){145, 119, 45, 255};
        UIRect bounds = {context->x + 40.0f, y,
            context->width - 48.0f, 28.0f};
        UIButtonResult result = rohr_ui_button(id, &editor->object_names[i], bounds,
            editor_viewport_selection_contains(context->viewport, ref) ? &style : NULL);
        if(context->hierarchy_row != NULL)
            context->hierarchy_row(context->hierarchy_context, context->viewport,
                ref, bounds, result,
                row + 1 == context->project->hierarchy_count);
        if(result.clicked || result.focus_changed) {
            editor_viewport_selection_clear(context->viewport);
            (void)editor_project_object_select(context->project, object->id);
            context->viewport->selection = EDITOR_SELECTION_OBJECT;
            if(result.double_clicked)
                (void)editor_navigation_selected_open(context->project,
                    context->viewport);
        }
        } else {
        EditorLayoutViewport *viewport = editor_project_layout_viewport_get(
            context->project, hierarchy_item.id);
        size_t i = viewport == NULL ? context->project->layout_viewport_count :
            (size_t)(viewport - context->project->layout_viewports);
        char id[64], visibility_id[72];
        if(viewport == NULL || i >= EDITOR_LAYOUT_VIEWPORT_MAX) continue;
        if(!editor_mode_named_text_sync(editor->font, viewport->name,
                &editor->viewport_names[i], editor->viewport_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.layout_viewport.%u", viewport->id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.layout_viewport.%u.visibility", viewport->id);
        if(rohr_ui_button(visibility_id, viewport->enabled ? &editor->visible_label :
                &editor->hidden_label, (UIRect){context->x + 8.0f,
                    y + 1.0f, 26.0f, 26.0f},
                NULL).clicked)
            viewport->enabled = !viewport->enabled;
        UIButtonStyle style = rohr_ui_button_style_default_get();
        style.idle = (Color){118, 96, 35, 255};
        style.hovered = (Color){145, 119, 45, 255};
        UIButtonResult result = rohr_ui_button(id, &editor->viewport_names[i],
            (UIRect){context->x + 40.0f, y,
                context->width - 48.0f, 28.0f},
            context->viewport->selection == EDITOR_SELECTION_LAYOUT_VIEWPORT &&
                context->viewport->selected_layout_viewport == viewport->id ?
                &style : NULL);
        if(context->hierarchy_row != NULL) {
            UIRect bounds = {context->x + 40.0f,
                y,
                context->width - 48.0f, 28.0f};
            context->hierarchy_row(context->hierarchy_context, context->viewport,
                (EditorSelectionRef){EDITOR_SELECTION_LAYOUT_VIEWPORT,
                    0, 0, 0, viewport->id}, bounds, result,
                row + 1 == context->project->hierarchy_count);
        }
        if(result.clicked || result.focus_changed) {
            editor_viewport_selection_clear(context->viewport);
            context->viewport->selected_layout_viewport = viewport->id;
            context->viewport->selection = EDITOR_SELECTION_LAYOUT_VIEWPORT;
            editor_project_selection_clear(context->project);
            if(result.double_clicked) context->viewport->mode = EDITOR_VIEWPORT_LAYOUT;
        }
        }
    }
}
