/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_layout_viewport.h"
#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

static bool layout_local_swatch(const char *id, uint32_t *color, UIRect bounds,
    const EditorModeContext *context);

bool editor_layout_viewport_editor_create(EditorLayoutViewportEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorLayoutViewportEditor){.font = font};
#define CREATE(text, member) if(!editor_mode_text_create(font, text, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Width", width_label); CREATE("Height", height_label);
    CREATE("Enabled", enabled_label); CREATE("Background", background_color_label);
    CREATE("Graphics Layers", graphics_layers_label);
    CREATE("Add Graphics Layer", add_layer_label);
    CREATE("Screens", cameras_label);
    CREATE("Add Screen", add_label); CREATE("Delete Viewport", delete_label);
    CREATE("Remove", remove_label); CREATE("Layer", layer_label);
    CREATE("Direct value", direct_layer_label);
    CREATE("Visible", visible_label);
    CREATE("Rotation", rotation_label);
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
    CREATE("Mount Existing UI", mount_ui_label);
    CREATE("Button", button_label);
    CREATE("Text", text_label); CREATE("Font File", font_file_label);
    CREATE("Default", default_font_label); CREATE("Load Font", load_font_label);
    CREATE("Add Vertex", add_vertex_label); CREATE("Length", length_label);
    CREATE("Font Color", font_color_label);
    CREATE("Text Width Scale", width_scale_label);
    CREATE("Text Height Scale", height_scale_label);
    CREATE("Text Offset X", text_offset_x_label);
    CREATE("Text Offset Y", text_offset_y_label);
    CREATE("Content X", content_x_label); CREATE("Content Y", content_y_label);
    CREATE("Source Rotation", content_rotation_label);
    CREATE("Content Width Scale", content_width_scale_label);
    CREATE("Content Height Scale", content_height_scale_label);
    CREATE("Source", source_label);
    CREATE("", name_field); CREATE("", x_field); CREATE("", y_field);
    CREATE("", width_field); CREATE("", height_field); CREATE("", layer_field);
    CREATE("", rotation_field);
    CREATE("", text_field); CREATE("", font_file_field);
    CREATE("", width_scale_field); CREATE("", height_scale_field);
    CREATE("", length_field);
    CREATE("", content_x_field); CREATE("", content_y_field);
    CREATE("", content_rotation_field);
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
    DESTROY(height_label); DESTROY(enabled_label); DESTROY(background_color_label);
    DESTROY(graphics_layers_label); DESTROY(add_layer_label);
    DESTROY(cameras_label);
    DESTROY(add_label); DESTROY(delete_label); DESTROY(name_field);
    DESTROY(remove_label); DESTROY(layer_label); DESTROY(direct_layer_label);
    DESTROY(visible_label);
    DESTROY(rotation_label); DESTROY(rotation_field);
    DESTROY(visible_icon); DESTROY(hidden_icon);
    DESTROY(border_label); DESTROY(border_type_label); DESTROY(border_line_label);
    DESTROY(border_hashed_label); DESTROY(border_thickness_label);
    DESTROY(hash_spacing_label); DESTROY(corner_radius_label);
    DESTROY(border_color_label); DESTROY(fill_color_label);
    DESTROY(hover_border_color_label); DESTROY(hover_fill_color_label);
    DESTROY(click_border_color_label); DESTROY(click_fill_color_label);
    DESTROY(border_thickness_field); DESTROY(hash_spacing_field);
    DESTROY(corner_radius_field);
    DESTROY(add_shape_label); DESTROY(mount_ui_label); DESTROY(button_label);
    DESTROY(text_label); DESTROY(font_file_label); DESTROY(font_color_label);
    DESTROY(default_font_label); DESTROY(load_font_label);
    DESTROY(add_vertex_label); DESTROY(length_label); DESTROY(length_field);
    DESTROY(width_scale_label); DESTROY(height_scale_label);
    DESTROY(content_x_label); DESTROY(content_y_label);
    DESTROY(content_rotation_label); DESTROY(source_label);
    DESTROY(content_width_scale_label); DESTROY(content_height_scale_label);
    DESTROY(text_offset_x_label); DESTROY(text_offset_y_label);
    DESTROY(x_field); DESTROY(y_field); DESTROY(width_field); DESTROY(height_field);
    DESTROY(layer_field);
    DESTROY(text_field); DESTROY(font_file_field); DESTROY(width_scale_field);
    DESTROY(height_scale_field);
    DESTROY(content_x_field); DESTROY(content_y_field);
    DESTROY(content_rotation_field);
#undef DESTROY
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->camera_names[i]);
    for(size_t i = 0; i < EDITOR_LAYOUT_VIEWPORT_UI_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->ui_names[i]);
    for(size_t i = 0; i < EDITOR_UI_FONT_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->font_names[i]);
    for(size_t i = 0; i < MAX_GRAPHICS_LAYERS; i += 1)
        rohr_graphics_text_destroy(&editor->layer_names[i]);
    for(size_t i = 0; i < MAX_GRAPHICS_UI_ELEMENTS; i += 1)
        rohr_graphics_text_destroy(&editor->definition_names[i]);
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
    rohr_ui_label(&editor->background_color_label,
        (UIRect){context->x + 8.0f, 270.0f, context->width - 64.0f, 28.0f});
    (void)layout_local_swatch("editor.layout.background_color",
        &viewport->background_color,
        (UIRect){context->x + context->width - 48.0f, 270.0f, 38.0f, 28.0f},
        context);
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_LAYOUT_VIEWPORT,
                .item = viewport->id}};
        snprintf(command.data.item_rename.name, sizeof(command.data.item_rename.name),
            "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->cameras_label,
        (UIRect){context->x + 8.0f, 314.0f, context->width - 16.0f, 28.0f});
    y = 348.0f;
    if(rohr_ui_button("editor.layout.add_shape", &editor->add_shape_label,
            (UIRect){context->x + 8.0f, y, context->width - 16.0f,
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
    y += 40.0f;
    if(context->project->ui_definition_count > 0) {
        const TextAsset *options[MAX_GRAPHICS_UI_ELEMENTS + 1];
        options[0] = &editor->mount_ui_label;
        for(size_t i = 0; i < context->project->ui_definition_count; i += 1) {
            EditorViewportUiDefinition *definition =
                &context->project->ui_definitions[i];
            if(!editor_mode_named_text_sync(editor->font, definition->name,
                    &editor->definition_names[i], editor->definition_cache[i],
                    EDITOR_OBJECT_NAME_MAX)) return false;
            options[i + 1] = &editor->definition_names[i];
        }
        UIDropdownResult mounted = rohr_ui_dropdown("editor.layout.mount_ui",
            options, context->project->ui_definition_count + 1, 0,
            (UIRect){context->x + 8.0f, y, context->width - 16.0f, 30.0f}, NULL);
        if(mounted.changed && mounted.selected_index > 0) {
            EditorViewportUiItem *item = editor_viewport_ui_mount(context->project,
                viewport, context->project->ui_definitions[
                    mounted.selected_index - 1].id);
            if(item != NULL) {
                context->viewport->selected_viewport_ui_item = item->id;
                context->viewport->selected_viewport_camera_item = 0;
                context->viewport->selection = item->kind ==
                    EDITOR_VIEWPORT_UI_SHAPE ? EDITOR_SELECTION_UI_SHAPE :
                    EDITOR_SELECTION_UI_TEXT;
            }
        }
        y += 40.0f;
    }
    if(rohr_ui_button("editor.layout.add_screen", &editor->add_label,
            (UIRect){context->x + 8.0f, y, context->width - 16.0f, 30.0f},
            NULL).clicked) {
        EditorObject *camera_object = NULL;
        EditorCamera *camera = NULL;
        for(size_t object_index = 0;
                object_index < context->project->object_count && camera == NULL;
                object_index += 1)
            if(context->project->objects[object_index].camera_count > 0) {
                camera_object = &context->project->objects[object_index];
                camera = &camera_object->cameras[0];
            }
        if(camera_object != NULL && camera != NULL) {
            EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                .data.item_add = {.kind = EDITOR_ITEM_VIEWPORT_CAMERA,
                    .object = camera_object->id, .parent = viewport->id,
                    .first = camera->id}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    y += 40.0f;
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
        UIButtonStyle selected_style = rohr_ui_button_style_default_get();
        selected_style.idle = (Color){118, 96, 35, 255};
        selected_style.hovered = (Color){145, 119, 45, 255};
        UIButtonResult camera_result = rohr_ui_button(id, &editor->camera_names[i],
                (UIRect){context->x + 46.0f, y, context->width - 54.0f, 28.0f},
                context->viewport->selected_viewport_camera_item == item->id ?
                    &selected_style : NULL);
        if(camera_result.clicked) {
            context->viewport->selected_viewport_camera_item = item->id,
            context->viewport->selected_viewport_ui_item = 0;
            context->viewport->selection = EDITOR_SELECTION_LAYOUT_VIEWPORT;
        }
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
            context->viewport->selection = EDITOR_SELECTION_UI_SHAPE;
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
    y += 12.0f;
    rohr_ui_label(&editor->graphics_layers_label,
        (UIRect){context->x + 8.0f, y, context->width - 16.0f, 28.0f});
    y += 34.0f;
    if(rohr_ui_button("editor.layout.graphics_layer.add", &editor->add_layer_label,
            (UIRect){context->x + 8.0f, y, context->width - 16.0f, 30.0f},
            NULL).clicked) {
        char layer_name[GRAPHICS_LAYER_NAME_MAX];
        snprintf(layer_name, sizeof(layer_name), "layer_%u",
            context->project->next_graphics_layer_id);
        (void)editor_project_graphics_layer_add(context->project, layer_name, 0);
    }
    y += 38.0f;
    for(size_t i = 0; i < context->project->graphics_layer_count; i += 1) {
        EditorGraphicsLayer *named = &context->project->graphics_layers[i];
        char name[GRAPHICS_LAYER_NAME_MAX];
        char name_id[96];
        char value_id[96];
        char remove_id[96];
        float value = (float)named->value;
        snprintf(name, sizeof(name), "%s", named->name);
        snprintf(name_id, sizeof(name_id), "editor.layout.graphics_layer.%u.name",
            named->id);
        snprintf(value_id, sizeof(value_id), "editor.layout.graphics_layer.%u.value",
            named->id);
        snprintf(remove_id, sizeof(remove_id),
            "editor.layout.graphics_layer.%u.remove", named->id);
        UIFieldResult name_changed = rohr_ui_field(name_id,
            (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
                .string_capacity = sizeof(name)}, &editor->name_field,
            (UIRect){context->x + 8.0f, y, context->width * 0.48f, 28.0f}, NULL);
        UIFieldResult value_changed = rohr_ui_field(value_id,
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &value},
            &editor->layer_field,
            (UIRect){context->x + context->width * 0.50f, y,
                context->width * 0.28f, 28.0f}, NULL);
        if(name_changed.changed && name[0] != '\0') {
            bool unique = true;
            for(size_t other = 0; other < context->project->graphics_layer_count;
                    other += 1)
                if(other != i && strcmp(context->project->graphics_layers[other].name,
                        name) == 0) unique = false;
            if(unique) snprintf(named->name, sizeof(named->name), "%s", name);
        }
        if(value_changed.changed) named->value = (int)value;
        if(rohr_ui_button(remove_id, &editor->remove_label,
                (UIRect){context->x + context->width * 0.80f, y,
                    context->width * 0.19f - 8.0f, 28.0f}, NULL).clicked) {
            (void)editor_project_graphics_layer_remove(context->project, named->id);
            break;
        }
        y += 34.0f;
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
    UIFieldResult x_result, y_result, width_result, height_result, layer_result,
        rotation_result, content_x_result, content_y_result,
        content_width_result, content_height_result, content_rotation_result;
    const TextAsset *camera_options[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX];
    EditorObjectId camera_objects[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX] = {0};
    EditorCameraId camera_ids[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX] = {0};
    size_t camera_count = 0;
    size_t selected_camera = 0;
    float layer;
    float rotation_degrees;
    float source_rotation_degrees;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    viewport = editor_project_layout_viewport_get(context->project,
        context->viewport->selected_layout_viewport);
    if(viewport != NULL) for(size_t i = 0; i < viewport->camera_item_count; i += 1)
        if(viewport->camera_items[i].id ==
                context->viewport->selected_viewport_camera_item)
            item = &viewport->camera_items[i];
    if(item == NULL) return false;
    rotation_degrees = item->placement.orientation *
        180.0f / 3.14159265359f;
    source_rotation_degrees = item->content_rotation *
        180.0f / 3.14159265359f;
    if(rohr_ui_button("editor.layout.camera.visibility", item->placement.visible ?
            &editor->visible_icon : &editor->hidden_icon,
            (UIRect){context->x + 10.0f, 42.0f, 34.0f, 28.0f}, NULL).clicked)
        item->placement.visible = !item->placement.visible;
    rohr_ui_label(&editor->visible_label, (UIRect){context->x + 50.0f, 42.0f,
        context->width - 60.0f, 28.0f});
    for(size_t object_index = 0; object_index < context->project->object_count;
            object_index += 1) {
        EditorObject *object = &context->project->objects[object_index];
        for(size_t camera_index = 0; camera_index < object->camera_count &&
                camera_count < EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX; camera_index += 1) {
            EditorCamera *camera = &object->cameras[camera_index];
            if(!editor_mode_named_text_sync(editor->font, camera->name,
                    &editor->camera_names[camera_count],
                    editor->camera_cache[camera_count], EDITOR_OBJECT_NAME_MAX))
                continue;
            camera_options[camera_count] = &editor->camera_names[camera_count];
            camera_objects[camera_count] = object->id;
            camera_ids[camera_count] = camera->id;
            if(item->object == object->id && item->camera == camera->id)
                selected_camera = camera_count;
            camera_count += 1;
        }
    }
    rohr_ui_label(&editor->source_label, (UIRect){context->x + 8.0f, 80.0f,
        70.0f, 28.0f});
    if(camera_count > 0) {
        UIDropdownResult source = rohr_ui_dropdown("editor.layout.screen.source",
            camera_options, camera_count, selected_camera,
            (UIRect){context->x + 82.0f, 80.0f, context->width - 92.0f, 28.0f},
            NULL);
        if(source.button_hovered || source.hovered_index >= 0) {
            size_t preview = source.hovered_index >= 0 ?
                (size_t)source.hovered_index : selected_camera;
            if(preview < camera_count)
                context->viewport->preview_camera = camera_ids[preview];
        }
        if(source.changed && source.selected_index < camera_count) {
            item->object = camera_objects[source.selected_index];
            item->camera = camera_ids[source.selected_index];
        }
    }
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.camera_editor.x", context->x, 118.0f, context->width,
        &item->placement.rectangle.x);
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.camera_editor.y", context->x, 156.0f, context->width,
        &item->placement.rectangle.y);
    width_result = layout_number(&editor->width_label, &editor->width_field,
        "editor.layout.camera_editor.width", context->x, 194.0f, context->width,
        &item->placement.rectangle.width);
    height_result = layout_number(&editor->height_label, &editor->height_field,
        "editor.layout.camera_editor.height", context->x, 232.0f, context->width,
        &item->placement.rectangle.height);
    rotation_result = layout_number(&editor->rotation_label, &editor->rotation_field,
        "editor.layout.screen.rotation", context->x, 270.0f, context->width,
        &rotation_degrees);
    if(rotation_result.changed) {
        rotation_degrees = fmodf(rotation_degrees, 360.0f);
        if(rotation_degrees < 0.0f) rotation_degrees += 360.0f;
        item->placement.orientation = rotation_degrees *
            3.14159265359f / 180.0f;
    }
    layer = (float)item->placement.layer;
    layer_result = layout_number(&editor->layer_label, &editor->layer_field,
        "editor.layout.camera_editor.layer", context->x, 308.0f, context->width,
        &layer);
    if(layer_result.changed) item->placement.layer = (int)layer;
    content_x_result = layout_number(&editor->content_x_label,
        &editor->content_x_field, "editor.layout.screen.content_x", context->x,
        346.0f, context->width, &item->content_offset.x);
    content_y_result = layout_number(&editor->content_y_label,
        &editor->content_y_field, "editor.layout.screen.content_y", context->x,
        384.0f, context->width, &item->content_offset.y);
    content_width_result = layout_number(&editor->content_width_scale_label,
        &editor->width_scale_field, "editor.layout.screen.content_width", context->x,
        422.0f, context->width, &item->content_scale.x);
    content_height_result = layout_number(&editor->content_height_scale_label,
        &editor->height_scale_field, "editor.layout.screen.content_height", context->x,
        460.0f, context->width, &item->content_scale.y);
    content_rotation_result = layout_number(&editor->content_rotation_label,
        &editor->content_rotation_field, "editor.layout.screen.content_rotation",
        context->x, 498.0f, context->width, &source_rotation_degrees);
    if(content_rotation_result.changed) {
        source_rotation_degrees = fmodf(source_rotation_degrees, 360.0f);
        if(source_rotation_degrees < 0.0f) source_rotation_degrees += 360.0f;
        item->content_rotation = source_rotation_degrees *
            3.14159265359f / 180.0f;
    }
    item->content_scale.x = fmaxf(0.01f, item->content_scale.x);
    item->content_scale.y = fmaxf(0.01f, item->content_scale.y);
    return x_result.active || y_result.active || width_result.active ||
        height_result.active || rotation_result.active || layer_result.active ||
        content_x_result.active || content_y_result.active ||
        content_width_result.active || content_height_result.active ||
        content_rotation_result.active;
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
    const TextAsset *layer_options[MAX_GRAPHICS_LAYERS + 1];
    size_t selected_layer = 0;
    bool visible = item->visible;
    UIFieldResult x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.ui.x", context->x, *y, context->width, &item->position.x);
    *y += 38.0f;
    UIFieldResult y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.ui.y", context->x, *y, context->width, &item->position.y);
    *y += 38.0f;
    layer_options[0] = &editor->direct_layer_label;
    for(size_t i = 0; i < context->project->graphics_layer_count; i += 1) {
        EditorGraphicsLayer *named = &context->project->graphics_layers[i];
        if(!editor_mode_named_text_sync(editor->font, named->name,
                &editor->layer_names[i], editor->layer_cache[i],
                GRAPHICS_LAYER_NAME_MAX)) continue;
        layer_options[i + 1] = &editor->layer_names[i];
        if(item->graphics_layer == named->id) selected_layer = i + 1;
    }
    rohr_ui_label(&editor->layer_label,
        (UIRect){context->x + 8.0f, *y, 82.0f, 28.0f});
    UIDropdownResult layer_source = rohr_ui_dropdown("editor.layout.ui.layer_source",
        layer_options, context->project->graphics_layer_count + 1,
        selected_layer, (UIRect){context->x + 94.0f, *y,
            context->width - 104.0f, 28.0f}, NULL);
    if(layer_source.changed) item->graphics_layer = layer_source.selected_index == 0
        ? 0 : context->project->graphics_layers[
            layer_source.selected_index - 1].id;
    *y += 38.0f;
    UIFieldResult layer_result = {0};
    if(item->graphics_layer == 0) {
        layer_result = layout_number(&editor->direct_layer_label,
            &editor->layer_field, "editor.layout.ui.layer", context->x, *y,
            context->width, &layer);
        if(layer_result.changed) item->layer = (int)layer;
        *y += 38.0f;
    }
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
    {
        UIFieldResult rotation = layout_number(&editor->rotation_label,
            &editor->rotation_field, "editor.ui_shape.rotation", context->x, y,
            context->width, &item->value.shape.rotation);
        active = active || rotation.active;
        y += 38.0f;
    }
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
