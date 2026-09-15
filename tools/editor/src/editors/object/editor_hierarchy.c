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
            !editor_mode_text_create(font, "[ ]", &editor->hidden_label)) {
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
    for(size_t i = 0; i < EDITOR_OBJECT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->object_names[i]);
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->viewport_names[i]);
    *editor = (EditorHierarchyEditor){0};
}

void editor_hierarchy_editor_draw(EditorHierarchyEditor *editor,
        const EditorModeContext *context) {
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
    (void)rohr_graphics_screen_rect_draw(context->x + 10.0f, 132.0f,
        context->width - 20.0f, 1.0f, (Color){75, 84, 100, 255});
    for(size_t i = 0; i < context->project->object_count &&
            i < EDITOR_OBJECT_MAX; i += 1) {
        EditorObject *object = &context->project->objects[i];
        float y = 144.0f + (float)i * 34.0f;
        char id[64], visibility_id[72];
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
                ref, bounds, result, i + 1 == context->project->object_count);
        if(result.clicked || result.focus_changed) {
            (void)editor_project_object_select(context->project, object->id);
            context->viewport->selection = EDITOR_SELECTION_OBJECT;
            if(result.double_clicked)
                (void)editor_navigation_selected_open(context->project,
                    context->viewport);
        }
    }
    float viewport_y = 154.0f + (float)context->project->object_count * 34.0f;
    for(size_t i = 0; i < context->project->layout_viewport_count &&
            i < EDITOR_LAYOUT_VIEWPORT_MAX; i += 1) {
        EditorLayoutViewport *viewport = &context->project->layout_viewports[i];
        char id[64];
        if(!editor_mode_named_text_sync(editor->font, viewport->name,
                &editor->viewport_names[i], editor->viewport_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.layout_viewport.%u", viewport->id);
        UIButtonResult result = rohr_ui_button(id, &editor->viewport_names[i],
            (UIRect){context->x + 40.0f, viewport_y + (float)i * 34.0f,
                context->width - 48.0f, 28.0f}, NULL);
        if(result.clicked || result.focus_changed) {
            context->viewport->selected_layout_viewport = viewport->id;
            context->viewport->selection = EDITOR_SELECTION_LAYOUT_VIEWPORT;
            editor_project_selection_clear(context->project);
            if(result.double_clicked) context->viewport->mode = EDITOR_VIEWPORT_LAYOUT;
        }
    }
}
