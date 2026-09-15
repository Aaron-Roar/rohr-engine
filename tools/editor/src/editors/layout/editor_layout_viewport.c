/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_layout_viewport.h"
#include "editors/editor_mode_controls.h"

#include <stdio.h>

static UIFieldResult layout_number(TextAsset *label, TextAsset *field,
        const char *id, float x, float y, float width, float *value) {
    rohr_ui_label(label, (UIRect){x + 8.0f, y, 82.0f, 28.0f});
    return rohr_ui_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){x + 94.0f, y, width - 104.0f, 28.0f}, NULL);
}

bool editor_layout_viewport_editor_create(EditorLayoutViewportEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorLayoutViewportEditor){.font = font};
#define CREATE(text, member) if(!editor_mode_text_create(font, text, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Width", width_label); CREATE("Height", height_label);
    CREATE("Enabled", enabled_label); CREATE("Cameras", cameras_label);
    CREATE("Add", add_label); CREATE("Delete Viewport", delete_label);
    CREATE("Remove", remove_label); CREATE("Layer", layer_label);
    CREATE("Visible", visible_label);
    CREATE("Add UI Shape", add_shape_label);
    CREATE("Add UI Text", add_text_label);
    CREATE("Button", button_label);
    CREATE("", name_field); CREATE("", x_field); CREATE("", y_field);
    CREATE("", width_field); CREATE("", height_field); CREATE("", layer_field);
#undef CREATE
    return true;
fail:
    editor_layout_viewport_editor_destroy(editor);
    return false;
}

void editor_layout_viewport_editor_destroy(EditorLayoutViewportEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(x_label); DESTROY(y_label); DESTROY(width_label);
    DESTROY(height_label); DESTROY(enabled_label); DESTROY(cameras_label);
    DESTROY(add_label); DESTROY(delete_label); DESTROY(name_field);
    DESTROY(remove_label); DESTROY(layer_label); DESTROY(visible_label);
    DESTROY(add_shape_label); DESTROY(add_text_label); DESTROY(button_label);
    DESTROY(x_field); DESTROY(y_field); DESTROY(width_field); DESTROY(height_field);
    DESTROY(layer_field);
#undef DESTROY
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->camera_names[i]);
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_UI_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->ui_names[i]);
    *editor = (EditorLayoutViewportEditor){0};
}

bool editor_layout_viewport_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorLayoutViewport *viewport;
    char name[EDITOR_OBJECT_NAME_MAX];
    UIFieldResult name_result, x_result, y_result, width_result, height_result;
    bool enabled;
    float y;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    viewport = editor_project_layout_viewport_get(context->project,
        context->viewport->selected_layout_viewport);
    if(viewport == NULL) return false;
    snprintf(name, sizeof(name), "%s", viewport->name);
    rohr_ui_label(&editor->name_label, (UIRect){context->x + 8.0f, 42.0f, 82.0f, 28.0f});
    name_result = rohr_ui_field("editor.layout.name",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = sizeof(name)}, &editor->name_field,
        (UIRect){context->x + 94.0f, 42.0f, context->width - 104.0f, 28.0f}, NULL);
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.x", context->x, 80.0f, context->width,
        &viewport->config.rectangle.x);
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.y", context->x, 118.0f, context->width,
        &viewport->config.rectangle.y);
    width_result = layout_number(&editor->width_label, &editor->width_field,
        "editor.layout.width", context->x, 156.0f, context->width,
        &viewport->config.rectangle.width);
    height_result = layout_number(&editor->height_label, &editor->height_field,
        "editor.layout.height", context->x, 194.0f, context->width,
        &viewport->config.rectangle.height);
    enabled = viewport->enabled;
    if(editor_mode_checkbox_left("editor.layout.enabled", &editor->enabled_label,
            (UIRect){context->x + 10.0f, 232.0f, context->width - 20.0f, 28.0f},
            &enabled)) viewport->enabled = enabled;
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_LAYOUT_VIEWPORT,
                .item = viewport->id}};
        snprintf(command.data.item_rename.name, sizeof(command.data.item_rename.name),
            "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->cameras_label,
        (UIRect){context->x + 8.0f, 276.0f, context->width - 16.0f, 28.0f});
    y = 310.0f;
    if(rohr_ui_button("editor.layout.add_shape", &editor->add_shape_label,
            (UIRect){context->x + 8.0f, y, (context->width - 24.0f) * 0.5f,
                30.0f}, NULL).clicked) {
        EditorViewportUiItem *item = editor_viewport_ui_add(context->project,
            viewport, EDITOR_VIEWPORT_UI_SHAPE);
        if(item != NULL) {
            context->viewport->selected_viewport_ui_item = item->id;
            context->viewport->selected_viewport_camera_item = 0;
        }
    }
    if(rohr_ui_button("editor.layout.add_text", &editor->add_text_label,
            (UIRect){context->x + 16.0f + (context->width - 24.0f) * 0.5f,
                y, (context->width - 24.0f) * 0.5f, 30.0f}, NULL).clicked) {
        EditorViewportUiItem *item = editor_viewport_ui_add(context->project,
            viewport, EDITOR_VIEWPORT_UI_TEXT);
        if(item != NULL) {
            context->viewport->selected_viewport_ui_item = item->id;
            context->viewport->selected_viewport_camera_item = 0;
        }
    }
    y += 40.0f;
    for(size_t object_index = 0; object_index < context->project->object_count;
            object_index += 1) {
        EditorObject *object = &context->project->objects[object_index];
        for(size_t camera_index = 0; camera_index < object->camera_count;
                camera_index += 1) {
            EditorCamera *camera = &object->cameras[camera_index];
            char id[96];
            snprintf(id, sizeof(id), "editor.layout.add_camera.%u.%u",
                object->id, camera->id);
            if(rohr_ui_button(id, &editor->add_label,
                    (UIRect){context->x + 8.0f, y, 54.0f, 28.0f}, NULL).clicked) {
                EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                    .data.item_add = {.kind = EDITOR_ITEM_VIEWPORT_CAMERA,
                        .object = object->id, .parent = viewport->id,
                        .first = camera->id}};
                snprintf(command.data.item_add.name, sizeof(command.data.item_add.name),
                    "%s", camera->name);
                (void)editor_command_execute(context->project, &command);
            }
            if(camera_index < EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX &&
                    editor_mode_named_text_sync(editor->font, camera->name,
                        &editor->camera_names[camera_index],
                        editor->camera_cache[camera_index], EDITOR_OBJECT_NAME_MAX))
                rohr_ui_label(&editor->camera_names[camera_index],
                    (UIRect){context->x + 70.0f, y, context->width - 78.0f, 28.0f});
            y += 32.0f;
        }
    }
    y += 8.0f;
    for(size_t i = 0; i < viewport->camera_item_count; i += 1) {
        EditorViewportCameraItem *item = &viewport->camera_items[i];
        char id[80];
        if(i >= EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX ||
                !editor_mode_named_text_sync(editor->font, item->name,
                    &editor->camera_names[i], editor->camera_cache[i],
                    EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.layout.camera.%u", item->id);
        if(rohr_ui_button(id, &editor->camera_names[i],
                (UIRect){context->x + 8.0f, y, context->width - 16.0f, 28.0f},
                NULL).clicked)
            context->viewport->selected_viewport_camera_item = item->id,
            context->viewport->selected_viewport_ui_item = 0;
        y += 32.0f;
    }
    for(size_t i = 0; i < viewport->ui_item_count &&
            i < EDITOR_LAYOUT_VIEWPORT_UI_MAX; i += 1) {
        EditorViewportUiItem *item = &viewport->ui_items[i];
        char id[80];
        if(!editor_mode_named_text_sync(editor->font, item->name,
                &editor->ui_names[i], editor->ui_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.layout.ui.%u", item->id);
        if(rohr_ui_button(id, &editor->ui_names[i],
                (UIRect){context->x + 8.0f, y, context->width - 16.0f, 28.0f},
                NULL).clicked) {
            context->viewport->selected_viewport_ui_item = item->id;
            context->viewport->selected_viewport_camera_item = 0;
        }
        y += 32.0f;
    }
    for(size_t i = 0; i < viewport->camera_item_count; i += 1) {
        EditorViewportCameraItem *item = &viewport->camera_items[i];
        float layer;
        bool visible;
        UIFieldResult item_x, item_y, item_width, item_height, item_layer;
        if(item->id != context->viewport->selected_viewport_camera_item) continue;
        y += 8.0f;
        item_x = layout_number(&editor->x_label, &editor->x_field,
            "editor.layout.camera.x", context->x, y, context->width,
            &item->placement.rectangle.x); y += 38.0f;
        item_y = layout_number(&editor->y_label, &editor->y_field,
            "editor.layout.camera.y", context->x, y, context->width,
            &item->placement.rectangle.y); y += 38.0f;
        item_width = layout_number(&editor->width_label, &editor->width_field,
            "editor.layout.camera.width", context->x, y, context->width,
            &item->placement.rectangle.width); y += 38.0f;
        item_height = layout_number(&editor->height_label, &editor->height_field,
            "editor.layout.camera.height", context->x, y, context->width,
            &item->placement.rectangle.height); y += 38.0f;
        layer = (float)item->placement.layer;
        item_layer = layout_number(&editor->layer_label, &editor->layer_field,
            "editor.layout.camera.layer", context->x, y, context->width, &layer);
        if(item_layer.changed) item->placement.layer = (int)layer;
        y += 38.0f;
        visible = item->placement.visible;
        if(editor_mode_checkbox_left("editor.layout.camera.visible",
                &editor->visible_label, (UIRect){context->x + 10.0f, y,
                    context->width - 20.0f, 28.0f}, &visible))
            item->placement.visible = visible;
        y += 38.0f;
        if(rohr_ui_button("editor.layout.camera.remove", &editor->remove_label,
                (UIRect){context->x + 8.0f, y, context->width - 16.0f, 30.0f},
                NULL).clicked) {
            EditorCommand command = {.type = EDITOR_COMMAND_ITEM_REMOVE,
                .data.item_remove = {.kind = EDITOR_ITEM_VIEWPORT_CAMERA,
                    .parent = viewport->id, .item = item->id}};
            (void)editor_command_execute(context->project, &command);
            context->viewport->selected_viewport_camera_item = 0;
        }
        return name_result.active || x_result.active || y_result.active ||
            width_result.active || height_result.active || item_x.active ||
            item_y.active || item_width.active || item_height.active ||
            item_layer.active;
    }
    for(size_t i = 0; i < viewport->ui_item_count; i += 1) {
        EditorViewportUiItem *item = &viewport->ui_items[i];
        float layer;
        bool visible;
        UIFieldResult text_result, item_x, item_y, item_width, item_height,
            item_layer;
        if(item->id != context->viewport->selected_viewport_ui_item) continue;
        y += 8.0f;
        rohr_ui_label(&editor->name_label,
            (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
        char *text = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
            item->value.shape.text : item->value.text.text;
        text_result = rohr_ui_field("editor.layout.ui.text",
            (UIFieldBinding){.kind = UI_FIELD_STRING, .string = text,
                .string_capacity = UI_LABEL_MAX}, &editor->name_field,
            (UIRect){context->x + 94.0f, y, context->width - 104.0f, 28.0f}, NULL);
        y += 38.0f;
        item_x = layout_number(&editor->x_label, &editor->x_field,
            "editor.layout.ui.x", context->x, y, context->width,
            &item->position.x); y += 38.0f;
        item_y = layout_number(&editor->y_label, &editor->y_field,
            "editor.layout.ui.y", context->x, y, context->width,
            &item->position.y); y += 38.0f;
        item_width = (UIFieldResult){0}; item_height = (UIFieldResult){0};
        if(item->kind == EDITOR_VIEWPORT_UI_SHAPE) {
            bool button = item->value.shape.button_enabled;
            if(editor_mode_checkbox_left("editor.layout.ui.button",
                    &editor->button_label, (UIRect){context->x + 10.0f, y,
                        context->width - 20.0f, 28.0f}, &button))
                item->value.shape.button_enabled = button;
            y += 38.0f;
        }
        layer = (float)item->layer;
        item_layer = layout_number(&editor->layer_label, &editor->layer_field,
            "editor.layout.ui.layer", context->x, y, context->width, &layer);
        if(item_layer.changed) item->layer = (int)layer;
        y += 38.0f;
        visible = item->visible;
        if(editor_mode_checkbox_left("editor.layout.ui.visible",
                &editor->visible_label, (UIRect){context->x + 10.0f, y,
                    context->width - 20.0f, 28.0f}, &visible))
            item->visible = visible;
        y += 38.0f;
        if(rohr_ui_button("editor.layout.ui.remove", &editor->remove_label,
                (UIRect){context->x + 8.0f, y, context->width - 16.0f, 30.0f},
                NULL).clicked) {
            (void)editor_viewport_ui_remove(viewport, item->id);
            context->viewport->selected_viewport_ui_item = 0;
        }
        return name_result.active || x_result.active || y_result.active ||
            width_result.active || height_result.active || text_result.active ||
            item_x.active || item_y.active || item_width.active ||
            item_height.active || item_layer.active;
    }
    return name_result.active || x_result.active || y_result.active ||
        width_result.active || height_result.active;
}
