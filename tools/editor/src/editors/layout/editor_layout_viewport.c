/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_layout_viewport.h"
#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>

static UIFieldResult layout_number(TextAsset *label, TextAsset *field,
        const char *id, float x, float y, float width, float *value) {
    float field_width = fminf(116.0f, fmaxf(72.0f, width * 0.42f));
    float field_x = x + width - field_width - 10.0f;
    rohr_ui_label(label, (UIRect){x + 8.0f, y,
        fmaxf(1.0f, field_x - x - 12.0f), 28.0f});
    return rohr_ui_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){field_x, y, field_width, 28.0f}, NULL);
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
    CREATE("[X]", visible_icon); CREATE("[ ]", hidden_icon);
    CREATE("Border", border_label); CREATE("Border Type", border_type_label);
    CREATE("Line", border_line_label); CREATE("Hashed", border_hashed_label);
    CREATE("Border Thickness", border_thickness_label);
    CREATE("Hash Spacing", hash_spacing_label);
    CREATE("Corner Radius", corner_radius_label);
    CREATE("Border Color", border_color_label); CREATE("Fill Color", fill_color_label);
    CREATE("Hover Border", hover_border_color_label);
    CREATE("Hover Fill", hover_fill_color_label);
    CREATE("Click Border", click_border_color_label);
    CREATE("Click Fill", click_fill_color_label);
    CREATE("Add UI Shape", add_shape_label);
    CREATE("Add UI Text", add_text_label);
    CREATE("Button", button_label);
    CREATE("Text", text_label); CREATE("Font File", font_file_label);
    CREATE("Default", default_font_label); CREATE("Load Font", load_font_label);
    CREATE("Add Vertex", add_vertex_label); CREATE("Length", length_label);
    CREATE("Font Color", font_color_label);
    CREATE("Text Width Scale", width_scale_label);
    CREATE("Text Height Scale", height_scale_label);
    CREATE("Text Offset X", text_offset_x_label);
    CREATE("Text Offset Y", text_offset_y_label);
    CREATE("", name_field); CREATE("", x_field); CREATE("", y_field);
    CREATE("", width_field); CREATE("", height_field); CREATE("", layer_field);
    CREATE("", text_field); CREATE("", font_file_field);
    CREATE("", width_scale_field); CREATE("", height_scale_field);
    CREATE("", length_field);
    CREATE("", border_thickness_field); CREATE("", hash_spacing_field);
    CREATE("", corner_radius_field);
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
    DESTROY(visible_icon); DESTROY(hidden_icon);
    DESTROY(border_label); DESTROY(border_type_label); DESTROY(border_line_label);
    DESTROY(border_hashed_label); DESTROY(border_thickness_label);
    DESTROY(hash_spacing_label); DESTROY(corner_radius_label);
    DESTROY(border_color_label); DESTROY(fill_color_label);
    DESTROY(hover_border_color_label); DESTROY(hover_fill_color_label);
    DESTROY(click_border_color_label); DESTROY(click_fill_color_label);
    DESTROY(border_thickness_field); DESTROY(hash_spacing_field);
    DESTROY(corner_radius_field);
    DESTROY(add_shape_label); DESTROY(add_text_label); DESTROY(button_label);
    DESTROY(text_label); DESTROY(font_file_label); DESTROY(font_color_label);
    DESTROY(default_font_label); DESTROY(load_font_label);
    DESTROY(add_vertex_label); DESTROY(length_label); DESTROY(length_field);
    DESTROY(width_scale_label); DESTROY(height_scale_label);
    DESTROY(text_offset_x_label); DESTROY(text_offset_y_label);
    DESTROY(x_field); DESTROY(y_field); DESTROY(width_field); DESTROY(height_field);
    DESTROY(layer_field);
    DESTROY(text_field); DESTROY(font_file_field); DESTROY(width_scale_field);
    DESTROY(height_scale_field);
#undef DESTROY
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->camera_names[i]);
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_UI_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->ui_names[i]);
    for(size_t i = 0; i < EDITOR_UI_FONT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->font_names[i]);
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
            context->viewport->mode = EDITOR_VIEWPORT_UI_SHAPE_EDITOR;
            context->viewport->selection = EDITOR_SELECTION_UI_SHAPE;
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
            context->viewport->mode = EDITOR_VIEWPORT_UI_TEXT_EDITOR;
            context->viewport->selection = EDITOR_SELECTION_UI_TEXT;
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
        char visibility_id[88];
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.layout.camera.%u.visibility", item->id);
        if(rohr_ui_button(visibility_id, item->placement.visible ?
                &editor->visible_icon : &editor->hidden_icon,
                (UIRect){context->x + 8.0f, y, 34.0f, 28.0f}, NULL).clicked)
            item->placement.visible = !item->placement.visible;
        UIButtonResult camera_result = rohr_ui_button(id, &editor->camera_names[i],
                (UIRect){context->x + 46.0f, y, context->width - 54.0f, 28.0f},
                NULL);
        if(camera_result.clicked)
            context->viewport->selected_viewport_camera_item = item->id,
            context->viewport->selected_viewport_ui_item = 0;
        if(camera_result.double_clicked)
            context->viewport->mode = EDITOR_VIEWPORT_LAYOUT_CAMERA_EDITOR;
        y += 32.0f;
    }
    for(size_t i = 0; i < viewport->ui_item_count &&
            i < EDITOR_LAYOUT_VIEWPORT_UI_MAX; i += 1) {
        EditorViewportUiItem *item = &viewport->ui_items[i];
        char id[80];
        char visibility_id[88];
        if(!editor_mode_named_text_sync(editor->font, item->name,
                &editor->ui_names[i], editor->ui_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        snprintf(id, sizeof(id), "editor.layout.ui.%u", item->id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.layout.ui.%u.visibility", item->id);
        if(rohr_ui_button(visibility_id, item->visible ? &editor->visible_icon :
                &editor->hidden_icon, (UIRect){context->x + 8.0f, y,
                    34.0f, 28.0f}, NULL).clicked) item->visible = !item->visible;
        UIButtonResult ui_result = rohr_ui_button(id, &editor->ui_names[i],
                (UIRect){context->x + 46.0f, y, context->width - 54.0f, 28.0f},
                NULL);
        if(ui_result.clicked) {
            context->viewport->selected_viewport_ui_item = item->id;
            context->viewport->selected_viewport_camera_item = 0;
        }
        if(ui_result.double_clicked) {
            context->viewport->mode = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                EDITOR_VIEWPORT_UI_SHAPE_EDITOR : EDITOR_VIEWPORT_UI_TEXT_EDITOR;
            context->viewport->selection = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                EDITOR_SELECTION_UI_SHAPE : EDITOR_SELECTION_UI_TEXT;
        }
        y += 32.0f;
    }
    for(size_t i = 0; i < viewport->camera_item_count; i += 1) {
        EditorViewportCameraItem *item = &viewport->camera_items[i];
        float layer;
        bool visible;
        UIFieldResult item_x, item_y, item_width, item_height, item_layer;
        if(item->id != context->viewport->selected_viewport_camera_item ||
                context->viewport->mode == EDITOR_VIEWPORT_LAYOUT) continue;
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
        if(item->id != context->viewport->selected_viewport_ui_item ||
                context->viewport->mode == EDITOR_VIEWPORT_LAYOUT) continue;
        y += 8.0f;
        rohr_ui_label(&editor->name_label,
            (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
        char *text = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
            item->value.shape.text.text : item->value.text.text;
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

static EditorViewportUiItem *layout_ui_item_get(const EditorModeContext *context,
        EditorViewportUiKind kind) {
    EditorLayoutViewport *viewport;
    if(context == NULL || context->project == NULL || context->viewport == NULL)
        return NULL;
    viewport = editor_project_layout_viewport_get(context->project,
        context->viewport->selected_layout_viewport);
    if(viewport == NULL) return NULL;
    for(size_t i = 0; i < viewport->ui_item_count; i += 1)
        if(viewport->ui_items[i].id == context->viewport->selected_viewport_ui_item &&
                viewport->ui_items[i].kind == kind) return &viewport->ui_items[i];
    return NULL;
}

bool editor_layout_camera_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorLayoutViewport *viewport;
    EditorViewportCameraItem *item = NULL;
    UIFieldResult x_result, y_result, width_result, height_result, layer_result;
    float layer;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    viewport = editor_project_layout_viewport_get(context->project,
        context->viewport->selected_layout_viewport);
    if(viewport != NULL) for(size_t i = 0; i < viewport->camera_item_count; i += 1)
        if(viewport->camera_items[i].id ==
                context->viewport->selected_viewport_camera_item)
            item = &viewport->camera_items[i];
    if(item == NULL) return false;
    if(rohr_ui_button("editor.layout.camera.visibility", item->placement.visible ?
            &editor->visible_icon : &editor->hidden_icon,
            (UIRect){context->x + 10.0f, 42.0f, 34.0f, 28.0f}, NULL).clicked)
        item->placement.visible = !item->placement.visible;
    rohr_ui_label(&editor->visible_label, (UIRect){context->x + 50.0f, 42.0f,
        context->width - 60.0f, 28.0f});
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.camera_editor.x", context->x, 80.0f, context->width,
        &item->placement.rectangle.x);
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.camera_editor.y", context->x, 118.0f, context->width,
        &item->placement.rectangle.y);
    width_result = layout_number(&editor->width_label, &editor->width_field,
        "editor.layout.camera_editor.width", context->x, 156.0f, context->width,
        &item->placement.rectangle.width);
    height_result = layout_number(&editor->height_label, &editor->height_field,
        "editor.layout.camera_editor.height", context->x, 194.0f, context->width,
        &item->placement.rectangle.height);
    layer = (float)item->placement.layer;
    layer_result = layout_number(&editor->layer_label, &editor->layer_field,
        "editor.layout.camera_editor.layer", context->x, 232.0f, context->width,
        &layer);
    if(layer_result.changed) item->placement.layer = (int)layer;
    return x_result.active || y_result.active || width_result.active ||
        height_result.active || layer_result.active;
}

static bool layout_local_swatch(const char *id, uint32_t *color, UIRect bounds,
        const EditorModeContext *context) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    Color displayed = rohr_graphics_color_hex_create(*color);
    UIButtonResult result;
    style.idle = displayed; style.pressed = displayed;
    style.hovered = (Color){displayed.red, displayed.green, displayed.blue, 220};
    result = rohr_ui_button(id, NULL, bounds, &style);
    rohr_ui_border(bounds, 2.0f, (Color){8, 9, 12, 255});
    if(result.clicked && context->local_color_open != NULL)
        context->local_color_open(context->color_context, color);
    return result.clicked;
}

static bool layout_ui_common_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context, EditorViewportUiItem *item, float *y) {
    float layer = (float)item->layer;
    bool visible = item->visible;
    UIFieldResult x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.ui.x", context->x, *y, context->width, &item->position.x);
    *y += 38.0f;
    UIFieldResult y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.ui.y", context->x, *y, context->width, &item->position.y);
    *y += 38.0f;
    UIFieldResult layer_result = layout_number(&editor->layer_label,
        &editor->layer_field, "editor.layout.ui.layer", context->x, *y,
        context->width, &layer);
    if(layer_result.changed) item->layer = (int)layer;
    *y += 38.0f;
    if(rohr_ui_button("editor.layout.ui.visibility", visible ?
            &editor->visible_icon : &editor->hidden_icon,
            (UIRect){context->x + 10.0f, *y, 34.0f, 28.0f}, NULL).clicked)
        item->visible = !item->visible;
    rohr_ui_label(&editor->visible_label, (UIRect){context->x + 50.0f, *y,
        context->width - 60.0f, 28.0f});
    *y += 42.0f;
    bool border_enabled = item->border_enabled;
    if(editor_mode_checkbox_left("editor.layout.ui.border", &editor->border_label,
            (UIRect){context->x + 10.0f, *y, context->width - 20.0f, 28.0f},
            &border_enabled)) item->border_enabled = border_enabled;
    *y += 38.0f;
    if(item->border_enabled) {
        const TextAsset *types[] = {&editor->border_line_label,
            &editor->border_hashed_label};
        rohr_ui_label(&editor->border_type_label,
            (UIRect){context->x + 8.0f, *y, 82.0f, 28.0f});
        UIDropdownResult type = rohr_ui_dropdown("editor.layout.ui.border_type",
            types, 2, item->border_type, (UIRect){context->x + 94.0f, *y,
                context->width - 104.0f, 28.0f}, NULL);
        if(type.changed) item->border_type =
            (EditorViewportUiBorderType)type.selected_index;
        *y += 38.0f;
        UIFieldResult thickness = layout_number(&editor->border_thickness_label,
            &editor->border_thickness_field, "editor.layout.ui.border_thickness",
            context->x, *y, context->width, &item->border_thickness);
        item->border_thickness = fmaxf(0.1f, item->border_thickness);
        *y += 38.0f;
        UIFieldResult spacing = {0};
        if(item->border_type == EDITOR_VIEWPORT_UI_BORDER_HASHED) {
            spacing = layout_number(&editor->hash_spacing_label,
                &editor->hash_spacing_field, "editor.layout.ui.hash_spacing",
                context->x, *y, context->width, &item->border_hash_spacing);
            item->border_hash_spacing = fmaxf(0.1f, item->border_hash_spacing);
            *y += 38.0f;
        }
        UIFieldResult radius = layout_number(&editor->corner_radius_label,
            &editor->corner_radius_field, "editor.layout.ui.corner_radius",
            context->x, *y, context->width, &item->border_corner_radius);
        item->border_corner_radius = fmaxf(0.0f, item->border_corner_radius);
        *y += 38.0f;
        rohr_ui_label(&editor->border_color_label,
            (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
        (void)layout_local_swatch("editor.layout.ui.border_color",
            &item->border_color, (UIRect){context->x + context->width - 46.0f,
                *y, 36.0f, 28.0f}, context);
        *y += 38.0f;
        rohr_ui_label(&editor->fill_color_label,
            (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
        (void)layout_local_swatch("editor.layout.ui.fill_color", &item->fill_color,
            (UIRect){context->x + context->width - 46.0f, *y, 36.0f, 28.0f}, context);
        *y += 42.0f;
        if(item->kind == EDITOR_VIEWPORT_UI_SHAPE &&
                item->value.shape.button_enabled) {
            rohr_ui_label(&editor->hover_border_color_label,
                (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
            (void)layout_local_swatch("editor.layout.ui.hover_border",
                &item->hover_border_color, (UIRect){context->x + context->width -
                    46.0f, *y, 36.0f, 28.0f}, context);
            *y += 38.0f;
            rohr_ui_label(&editor->hover_fill_color_label,
                (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
            (void)layout_local_swatch("editor.layout.ui.hover_fill",
                &item->hover_fill_color, (UIRect){context->x + context->width -
                    46.0f, *y, 36.0f, 28.0f}, context);
            *y += 38.0f;
            rohr_ui_label(&editor->click_border_color_label,
                (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
            (void)layout_local_swatch("editor.layout.ui.click_border",
                &item->click_border_color, (UIRect){context->x + context->width -
                    46.0f, *y, 36.0f, 28.0f}, context);
            *y += 38.0f;
            rohr_ui_label(&editor->click_fill_color_label,
                (UIRect){context->x + 8.0f, *y, 120.0f, 28.0f});
            (void)layout_local_swatch("editor.layout.ui.click_fill",
                &item->click_fill_color, (UIRect){context->x + context->width -
                    46.0f, *y, 36.0f, 28.0f}, context);
            *y += 42.0f;
        }
        return x_result.active || y_result.active || layer_result.active ||
            thickness.active || spacing.active || radius.active;
    }
    return x_result.active || y_result.active || layer_result.active;
}

bool editor_ui_shape_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorViewportUiItem *item = layout_ui_item_get(context,
        EDITOR_VIEWPORT_UI_SHAPE);
    bool button;
    float y = 42.0f;
    bool active;
    if(editor == NULL || item == NULL) return false;
    active = layout_ui_common_draw(editor, context, item, &y);
    button = item->value.shape.button_enabled;
    if(editor_mode_checkbox_left("editor.ui_shape.button", &editor->button_label,
            (UIRect){context->x + 10.0f, y, context->width - 20.0f, 28.0f},
            &button)) item->value.shape.button_enabled = button;
    y += 42.0f;
    if(rohr_ui_button("editor.ui_shape.text_child", &editor->text_label,
            (UIRect){context->x + 10.0f, y, context->width - 20.0f, 30.0f},
            NULL).clicked) {
        context->viewport->selected_viewport_ui_text_child = true;
        context->viewport->mode = EDITOR_VIEWPORT_UI_TEXT_EDITOR;
        context->viewport->selection = EDITOR_SELECTION_UI_TEXT;
    }
    return active;
}

bool editor_ui_text_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorViewportUiItem *item = layout_ui_item_get(context,
        EDITOR_VIEWPORT_UI_TEXT);
    EditorViewportUiText *text;
    UIFieldResult text_result, box_width_result, box_height_result,
        width_result, height_result;
    const TextAsset *font_options[EDITOR_UI_FONT_MAX + 2];
    size_t selected_font = 0;
    float y = 42.0f;
    bool active;
    if(editor == NULL) return false;
    if(item == NULL && context != NULL && context->viewport != NULL &&
            context->viewport->selected_viewport_ui_text_child) {
        EditorLayoutViewport *layout = editor_project_layout_viewport_get(
            context->project, context->viewport->selected_layout_viewport);
        if(layout != NULL) for(size_t i = 0; i < layout->ui_item_count; i += 1)
            if(layout->ui_items[i].id ==
                    context->viewport->selected_viewport_ui_item &&
                    layout->ui_items[i].kind == EDITOR_VIEWPORT_UI_SHAPE)
                item = &layout->ui_items[i];
    }
    if(item == NULL) return false;
    text = item->kind == EDITOR_VIEWPORT_UI_SHAPE ? &item->value.shape.text :
        &item->value.text;
    active = item->kind == EDITOR_VIEWPORT_UI_TEXT ?
        layout_ui_common_draw(editor, context, item, &y) : false;
    rohr_ui_label(&editor->text_label,
        (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
    text_result = rohr_ui_field("editor.ui_text.text",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = text->text,
            .string_capacity = sizeof(text->text)}, &editor->text_field,
        (UIRect){context->x + 94.0f, y, context->width - 104.0f, 28.0f}, NULL);
    y += 38.0f;
    UIFieldResult offset_x_result = layout_number(&editor->text_offset_x_label,
        &editor->x_field, "editor.ui_text.offset_x", context->x, y,
        context->width, &text->offset.x);
    y += 38.0f;
    UIFieldResult offset_y_result = layout_number(&editor->text_offset_y_label,
        &editor->y_field, "editor.ui_text.offset_y", context->x, y,
        context->width, &text->offset.y);
    y += 38.0f;
    rohr_ui_label(&editor->font_file_label,
        (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
    font_options[0] = &editor->default_font_label;
    font_options[1] = &editor->load_font_label;
    for(size_t i = 0; i < context->project->ui_font_count; i += 1) {
        EditorUiFont *font = &context->project->ui_fonts[i];
        if(!editor_mode_named_text_sync(editor->font, font->name,
                &editor->font_names[i], editor->font_cache[i],
                EDITOR_OBJECT_NAME_MAX)) continue;
        font_options[i + 2] = &editor->font_names[i];
        if(text->font == font->id) selected_font = i + 2;
    }
    UIDropdownResult font_result = rohr_ui_dropdown("editor.ui_text.font",
        font_options, context->project->ui_font_count + 2, selected_font,
        (UIRect){context->x + 94.0f, y, context->width - 104.0f, 28.0f}, NULL);
    if(font_result.changed) {
        if(font_result.selected_index == 0) text->font = 0;
        else if(font_result.selected_index == 1 && context->font_browser_open != NULL)
            context->font_browser_open(context->font_browser_context);
        else if(font_result.selected_index >= 2)
            text->font = context->project->ui_fonts[
                font_result.selected_index - 2].id;
    }
    y += 38.0f;
    rohr_ui_label(&editor->font_color_label,
        (UIRect){context->x + 8.0f, y, 120.0f, 28.0f});
    (void)layout_local_swatch("editor.ui_text.color", &text->color,
        (UIRect){context->x + context->width - 46.0f, y, 36.0f, 28.0f}, context);
    y += 38.0f;
    box_width_result = layout_number(&editor->width_label, &editor->width_field,
        "editor.ui_text.box_width", context->x, y, context->width,
        &text->box_width);
    if(text->box_width <= 0.0f) text->box_width = 1.0f;
    y += 38.0f;
    box_height_result = layout_number(&editor->height_label, &editor->height_field,
        "editor.ui_text.box_height", context->x, y, context->width,
        &text->box_height);
    if(text->box_height <= 0.0f) text->box_height = 1.0f;
    y += 38.0f;
    width_result = layout_number(&editor->width_scale_label,
        &editor->width_scale_field, "editor.ui_text.width_scale", context->x,
        y, context->width, &text->width_scale);
    if(text->width_scale <= 0.0f) text->width_scale = 0.01f;
    y += 38.0f;
    height_result = layout_number(&editor->height_scale_label,
        &editor->height_scale_field, "editor.ui_text.height_scale", context->x,
        y, context->width, &text->height_scale);
    if(text->height_scale <= 0.0f) text->height_scale = 0.01f;
    y += 42.0f;
    return active || text_result.active ||
        box_width_result.active || box_height_result.active ||
        width_result.active || height_result.active || offset_x_result.active ||
        offset_y_result.active;
}

bool editor_ui_vertex_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorViewportUiItem *item = layout_ui_item_get(context,
        EDITOR_VIEWPORT_UI_SHAPE);
    UIFieldResult x_result, y_result;
    Position edited;
    if(editor == NULL || item == NULL ||
            context->viewport->selected_vertex >= item->value.shape.vertex_count)
        return false;
    edited = item->value.shape.vertices[context->viewport->selected_vertex];
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.ui_vertex.x", context->x, 42.0f, context->width, &edited.x);
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.ui_vertex.y", context->x, 80.0f, context->width, &edited.y);
    if(x_result.changed || y_result.changed)
        item->value.shape.vertices[context->viewport->selected_vertex] = edited;
    return x_result.active || y_result.active;
}

bool editor_ui_line_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorViewportUiItem *item = layout_ui_item_get(context,
        EDITOR_VIEWPORT_UI_SHAPE);
    size_t line;
    size_t next;
    Position *first;
    Position *second;
    float length;
    UIFieldResult length_result;
    if(editor == NULL || item == NULL || item->value.shape.vertex_count < 2)
        return false;
    line = context->viewport->selected_line;
    if(line >= item->value.shape.vertex_count) return false;
    next = (line + 1) % item->value.shape.vertex_count;
    first = &item->value.shape.vertices[line];
    second = &item->value.shape.vertices[next];
    length = sqrtf((second->x - first->x) * (second->x - first->x) +
        (second->y - first->y) * (second->y - first->y));
    if(item->value.shape.vertex_count < EDITOR_HITBOX_VERTEX_MAX &&
            rohr_ui_button("editor.ui_line.add_vertex", &editor->add_vertex_label,
                (UIRect){context->x + 10.0f, 42.0f,
                    context->width - 20.0f, 34.0f}, NULL).clicked) {
        Position inserted = {(first->x + second->x) * 0.5f,
            (first->y + second->y) * 0.5f};
        size_t insertion = line + 1;
        memmove(&item->value.shape.vertices[insertion + 1],
            &item->value.shape.vertices[insertion],
            (item->value.shape.vertex_count - insertion) *
                sizeof(item->value.shape.vertices[0]));
        item->value.shape.vertices[insertion] = inserted;
        item->value.shape.vertex_count += 1;
        context->viewport->selected_vertex = (uint32_t)insertion;
        context->viewport->mode = EDITOR_VIEWPORT_UI_VERTEX_EDITOR;
        context->viewport->selection = EDITOR_SELECTION_UI_VERTEX;
        return false;
    }
    length_result = layout_number(&editor->length_label, &editor->length_field,
        "editor.ui_line.length", context->x, 86.0f, context->width, &length);
    if(length_result.changed) {
        Vec2D direction = {second->x - first->x, second->y - first->y};
        float prior_length = sqrtf(direction.x * direction.x +
            direction.y * direction.y);
        length = fmaxf(0.001f, length);
        if(prior_length <= 0.0001f) direction = (Vec2D){1.0f, 0.0f};
        else {
            direction.x /= prior_length;
            direction.y /= prior_length;
        }
        second->x = first->x + direction.x * length;
        second->y = first->y + direction.y * length;
    }
    return length_result.active;
}
