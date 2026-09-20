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
    return editor_mode_field(id,
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
    CREATE("Elements", cameras_label);
    CREATE("Add Screen", add_label); CREATE("Delete Viewport", delete_label);
    CREATE("Remove", remove_label); CREATE("Layer", layer_label);
    CREATE("Direct value", direct_layer_label);
    CREATE("Visibility", visible_label);
    CREATE("Draggable", draggable_label); CREATE("Drag Axis", drag_axis_label);
    CREATE("Horizontal", drag_x_label); CREATE("Vertical", drag_y_label);
    CREATE("Horizontal + Vertical", drag_xy_label);
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
    CREATE("Add Slider", add_slider_label);
    CREATE("Button", button_label);
    CREATE("Text", text_label); CREATE("Font File", font_file_label);
    CREATE("Default", default_font_label); CREATE("Load Font", load_font_label);
    CREATE("Add Vertex", add_vertex_label); CREATE("Length", length_label);
    CREATE("Font Color", font_color_label);
    CREATE("Minimum", minimum_label); CREATE("Maximum", maximum_label);
    CREATE("Value", value_label); CREATE("Step", step_label);
    CREATE("Orientation", orientation_label); CREATE("Horizontal", horizontal_label);
    CREATE("Vertical", vertical_label); CREATE("Track Thickness", track_thickness_label);
    CREATE("Thumb Size", thumb_size_label);
    CREATE("Track Color", track_color_label); CREATE("Filled Track", filled_track_color_label);
    CREATE("Thumb Color", thumb_color_label); CREATE("Hover Thumb", hover_thumb_color_label);
    CREATE("Pressed Thumb", pressed_thumb_color_label);
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
    CREATE("", minimum_field); CREATE("", maximum_field); CREATE("", value_field);
    CREATE("", step_field); CREATE("", track_thickness_field); CREATE("", thumb_size_field);
    CREATE("", content_x_field); CREATE("", content_y_field);
    CREATE("", content_rotation_field);
    CREATE("", border_thickness_field); CREATE("", hash_spacing_field);
    CREATE("", corner_radius_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true) ||
            !editor_mode_accordion_section_create(&editor->appearance_section, font,
                "Appearance", false) ||
            !editor_mode_accordion_section_create(&editor->source_section, font,
                "Source", true) ||
            !editor_mode_accordion_section_create(&editor->placement_section, font,
                "Placement", false) ||
            !editor_mode_accordion_section_create(&editor->interaction_section, font,
                "Interaction", false) ||
            !editor_mode_accordion_section_create(&editor->content_section, font,
                "Content Transform", false) ||
            !editor_mode_accordion_section_create(&editor->ui_transform_section, font,
                "Transform", true) ||
            !editor_mode_accordion_section_create(&editor->ui_interaction_section, font,
                "Interaction", false) ||
            !editor_mode_accordion_section_create(&editor->ui_appearance_section, font,
                "Appearance", false) ||
            !editor_mode_accordion_section_create(&editor->ui_content_section, font,
                "Content", false)) goto fail;
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
    DESTROY(cameras_label);
    DESTROY(add_label); DESTROY(delete_label); DESTROY(name_field);
    DESTROY(remove_label); DESTROY(layer_label); DESTROY(direct_layer_label);
    DESTROY(visible_label);
    DESTROY(draggable_label); DESTROY(drag_axis_label);
    DESTROY(drag_x_label); DESTROY(drag_y_label); DESTROY(drag_xy_label);
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
    DESTROY(add_shape_label); DESTROY(add_slider_label); DESTROY(button_label);
    DESTROY(minimum_label); DESTROY(maximum_label); DESTROY(value_label);
    DESTROY(step_label); DESTROY(orientation_label); DESTROY(horizontal_label);
    DESTROY(vertical_label); DESTROY(track_thickness_label); DESTROY(thumb_size_label);
    DESTROY(track_color_label); DESTROY(filled_track_color_label);
    DESTROY(thumb_color_label); DESTROY(hover_thumb_color_label);
    DESTROY(pressed_thumb_color_label);
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
    DESTROY(minimum_field); DESTROY(maximum_field); DESTROY(value_field);
    DESTROY(step_field); DESTROY(track_thickness_field); DESTROY(thumb_size_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    editor_mode_accordion_section_destroy(&editor->source_section);
    editor_mode_accordion_section_destroy(&editor->placement_section);
    editor_mode_accordion_section_destroy(&editor->interaction_section);
    editor_mode_accordion_section_destroy(&editor->content_section);
    editor_mode_accordion_section_destroy(&editor->ui_transform_section);
    editor_mode_accordion_section_destroy(&editor->ui_interaction_section);
    editor_mode_accordion_section_destroy(&editor->ui_appearance_section);
    editor_mode_accordion_section_destroy(&editor->ui_content_section);
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
    float y, transform_y, appearance_y, contents_y;
    bool transform_open, appearance_open;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    viewport = editor_project_layout_viewport_get(context->project,
        context->viewport->selected_layout_viewport);
    if(viewport == NULL) return false;
    snprintf(name, sizeof(name), "%s", viewport->name);
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 8.0f, 42.0f, 82.0f, 28.0f});
    name_result = editor_mode_field("editor.layout.name",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = sizeof(name)}, &editor->name_field,
        (UIRect){context->x + 94.0f, 42.0f, context->width - 104.0f, 28.0f}, NULL);
    bool enabled = viewport->enabled;
    if(editor_mode_checkbox_left("editor.layout.visibility",
            &editor->visible_label, (UIRect){context->x + 10.0f, 80.0f,
                context->width - 20.0f, 28.0f}, &enabled))
        viewport->enabled = enabled;
    y = 118.0f;
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
    y += 36.0f;
    if(rohr_ui_button("editor.layout.add_slider", &editor->add_slider_label,
            (UIRect){context->x + 8.0f, y, context->width - 16.0f,
                30.0f}, NULL).clicked) {
        EditorViewportUiItem *item = editor_viewport_ui_add(context->project,
            viewport, EDITOR_VIEWPORT_UI_SLIDER);
        if(item != NULL) {
            context->viewport->selected_viewport_ui_item = item->id;
            context->viewport->selected_viewport_camera_item = 0;
            context->viewport->mode = EDITOR_VIEWPORT_UI_SLIDER_EDITOR;
            context->viewport->selection = EDITOR_SELECTION_UI_SLIDER;
        }
    }
    y += 36.0f;
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
    y += 36.0f;
    size_t content_items = viewport->camera_item_count + viewport->ui_item_count;
    contents_y = y;
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width,
            y + 34.0f + (float)content_items * 34.0f);
    const float transform_rows[] = {26.0f, 26.0f, 26.0f, 28.0f};
    const float appearance_rows[] = {28.0f};
    EditorModeAccordionLayoutResult transform =
        editor_mode_accordion_layout_section(&accordion,
            &editor->transform_section, "editor.layout.section.transform",
            transform_rows, 4, 6.0f);
    EditorModeAccordionLayoutResult appearance =
        editor_mode_accordion_layout_section(&accordion,
            &editor->appearance_section, "editor.layout.section.appearance",
            appearance_rows, 1, 6.0f);
    transform_open = transform.expanded; transform_y = transform.content_y;
    appearance_open = appearance.expanded; appearance_y = appearance.content_y;
    x_result = (UIFieldResult){0}; y_result = (UIFieldResult){0};
    width_result = (UIFieldResult){0}; height_result = (UIFieldResult){0};
    if(transform_open) {
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.x", context->x, transform_y, context->width,
        &viewport->config.rectangle.x);
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.y", context->x, transform_y + 32.0f, context->width,
        &viewport->config.rectangle.y);
    width_result = layout_number(&editor->width_label, &editor->width_field,
        "editor.layout.width", context->x, transform_y + 64.0f, context->width,
        &viewport->config.rectangle.width);
    height_result = layout_number(&editor->height_label, &editor->height_field,
        "editor.layout.height", context->x, transform_y + 96.0f, context->width,
        &viewport->config.rectangle.height);
    }
    if(appearance_open) {
    rohr_ui_label(&editor->background_color_label,
        (UIRect){context->x + 8.0f, appearance_y,
            context->width - 64.0f, 28.0f});
    (void)layout_local_swatch("editor.layout.background_color",
        &viewport->background_color,
        (UIRect){context->x + context->width - 48.0f, appearance_y,
            38.0f, 28.0f},
        context);
    }
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_LAYOUT_VIEWPORT,
                .item = viewport->id}};
        snprintf(command.data.item_rename.name, sizeof(command.data.item_rename.name),
            "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->cameras_label,
        (UIRect){context->x + 8.0f, contents_y,
            context->width - 16.0f, 28.0f});
    y = contents_y + 34.0f;
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
        y += 34.0f;
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
            context->viewport->selection = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                EDITOR_SELECTION_UI_SHAPE : item->kind == EDITOR_VIEWPORT_UI_TEXT ?
                    EDITOR_SELECTION_UI_TEXT : EDITOR_SELECTION_UI_SLIDER;
        }
        if(ui_result.double_clicked) {
            context->viewport->mode = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                EDITOR_VIEWPORT_UI_SHAPE_EDITOR : item->kind == EDITOR_VIEWPORT_UI_TEXT ?
                    EDITOR_VIEWPORT_UI_TEXT_EDITOR : EDITOR_VIEWPORT_UI_SLIDER_EDITOR;
            context->viewport->selection = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                EDITOR_SELECTION_UI_SHAPE : item->kind == EDITOR_VIEWPORT_UI_TEXT ?
                    EDITOR_SELECTION_UI_TEXT : EDITOR_SELECTION_UI_SLIDER;
        }
        y += 34.0f;
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
        text_result = (UIFieldResult){0};
        if(item->kind != EDITOR_VIEWPORT_UI_SLIDER) {
            char *text = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
                item->value.shape.text.text : item->value.text.text;
            text_result = editor_mode_field("editor.layout.ui.text",
                (UIFieldBinding){.kind = UI_FIELD_STRING, .string = text,
                    .string_capacity = UI_LABEL_MAX}, &editor->name_field,
                (UIRect){context->x + 94.0f, y, context->width - 104.0f, 28.0f}, NULL);
            y += 38.0f;
        }
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
    UIFieldResult x_result = {0}, y_result = {0}, width_result = {0};
    UIFieldResult height_result = {0}, rotation_result = {0};
    UIFieldResult content_x_result = {0}, content_y_result = {0};
    UIFieldResult content_width_result = {0}, content_height_result = {0};
    UIFieldResult content_rotation_result = {0};
    const TextAsset *camera_options[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX];
    EditorObjectId camera_objects[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX] = {0};
    EditorCameraId camera_ids[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX] = {0};
    size_t camera_count = 0;
    size_t selected_camera = 0;
    float y = 42.0f;
    float rotation_degrees;
    float source_rotation_degrees;
    bool layer_active = false;
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
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
    UIFieldResult name_result = editor_mode_name_field(
        "editor.layout.camera.name", item->name,
        sizeof(item->name), &editor->name_field,
        (UIRect){context->x + 94.0f, y,
            context->width - 104.0f, 28.0f});
    y += 38.0f;
    bool visible = item->placement.visible;
    if(editor_mode_checkbox_left("editor.layout.camera.visibility",
            &editor->visible_label, (UIRect){context->x + 10.0f, y,
                context->width - 20.0f, 28.0f}, &visible))
        item->placement.visible = visible;
    y += 38.0f;
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
    float layer_height = item->graphics_layer == 0 ||
        (context->layer_control != NULL &&
            (context->layer_control->adding ||
                context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
    const float source_rows[] = {28.0f};
    const float placement_rows[] = {
        28.0f, 28.0f, 28.0f, 28.0f, 28.0f, layer_height};
    const float interaction_rows[] = {28.0f, 28.0f};
    const float content_rows[] = {
        28.0f, 28.0f, 28.0f, 28.0f, 28.0f};
    size_t interaction_count = item->placement.drag_mode ==
        VIEWPORT_ITEM_DRAG_NONE ? 1 : 2;
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    EditorModeAccordionLayoutResult source_section =
        editor_mode_accordion_layout_section(&accordion,
            &editor->source_section, "editor.layout.screen.section.source",
            source_rows, 1, 10.0f);
    EditorModeAccordionLayoutResult placement_section =
        editor_mode_accordion_layout_section(&accordion,
            &editor->placement_section,
            "editor.layout.screen.section.placement", placement_rows, 6, 10.0f);
    EditorModeAccordionLayoutResult interaction_section =
        editor_mode_accordion_layout_section(&accordion,
            &editor->interaction_section,
            "editor.layout.screen.section.interaction", interaction_rows,
            interaction_count, 10.0f);
    EditorModeAccordionLayoutResult content_section =
        editor_mode_accordion_layout_section(&accordion,
            &editor->content_section, "editor.layout.screen.section.content",
            content_rows, 5, 10.0f);

    if(source_section.expanded) {
        y = source_section.content_y;
        rohr_ui_label(&editor->source_label,
            (UIRect){context->x + 8.0f, y, 70.0f, 28.0f});
    }
    if(source_section.expanded && camera_count > 0) {
        UIDropdownResult source = editor_mode_dropdown("editor.layout.screen.source",
            camera_options, camera_count, selected_camera,
            (UIRect){context->x + 82.0f, y, context->width - 92.0f, 28.0f},
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
    if(placement_section.expanded) {
        y = placement_section.content_y;
        x_result = layout_number(&editor->x_label, &editor->x_field,
            "editor.layout.camera_editor.x", context->x, y, context->width,
            &item->placement.rectangle.x);
        y += 38.0f;
        y_result = layout_number(&editor->y_label, &editor->y_field,
            "editor.layout.camera_editor.y", context->x, y, context->width,
            &item->placement.rectangle.y);
        y += 38.0f;
        width_result = layout_number(&editor->width_label, &editor->width_field,
            "editor.layout.camera_editor.width", context->x, y, context->width,
            &item->placement.rectangle.width);
        y += 38.0f;
        height_result = layout_number(&editor->height_label,
            &editor->height_field, "editor.layout.camera_editor.height",
            context->x, y, context->width, &item->placement.rectangle.height);
        y += 38.0f;
        rotation_result = layout_number(&editor->rotation_label,
            &editor->rotation_field, "editor.layout.screen.rotation",
            context->x, y, context->width, &rotation_degrees);
        if(rotation_result.changed) {
            rotation_degrees = fmodf(rotation_degrees, 360.0f);
            if(rotation_degrees < 0.0f) rotation_degrees += 360.0f;
            item->placement.orientation = rotation_degrees *
                3.14159265359f / 180.0f;
        }
        y += 38.0f;
        EditorGraphicsLayerBinding layer_binding = {
            .value = item->placement.layer, .layer = item->graphics_layer};
        layer_active = context->layer_control != NULL &&
            editor_mode_layer_control_draw(context->layer_control,
                "editor.layout.screen", context->project, &layer_binding, NULL,
                context->x, y, context->width);
        item->placement.layer = layer_binding.value;
        item->graphics_layer = layer_binding.layer;
    }
    if(interaction_section.expanded) {
        y = interaction_section.content_y;
        bool draggable = item->placement.drag_mode != VIEWPORT_ITEM_DRAG_NONE;
        if(editor_mode_checkbox_left("editor.layout.screen.draggable",
                &editor->draggable_label,
                (UIRect){context->x + 10.0f, y,
                    context->width - 20.0f, 28.0f}, &draggable))
            item->placement.drag_mode = draggable ? VIEWPORT_ITEM_DRAG_XY :
                VIEWPORT_ITEM_DRAG_NONE;
        y += 38.0f;
    }
    if(interaction_section.expanded &&
            item->placement.drag_mode != VIEWPORT_ITEM_DRAG_NONE) {
        const TextAsset *axes[] = {&editor->drag_x_label, &editor->drag_y_label,
            &editor->drag_xy_label};
        rohr_ui_label(&editor->drag_axis_label,
            (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
        UIDropdownResult axis = editor_mode_dropdown(
            "editor.layout.screen.drag_axis", axes, 3,
            (size_t)item->placement.drag_mode - 1,
            (UIRect){context->x + 94.0f, y,
                context->width - 104.0f, 28.0f}, NULL);
        if(axis.changed) item->placement.drag_mode =
            (ViewportItemDragMode)(axis.selected_index + 1);
    }
    if(content_section.expanded) {
        y = content_section.content_y;
        content_x_result = layout_number(&editor->content_x_label,
            &editor->content_x_field, "editor.layout.screen.content_x",
            context->x, y, context->width, &item->content_offset.x);
        y += 38.0f;
        content_y_result = layout_number(&editor->content_y_label,
            &editor->content_y_field, "editor.layout.screen.content_y",
            context->x, y, context->width, &item->content_offset.y);
        y += 38.0f;
        content_width_result = layout_number(&editor->content_width_scale_label,
            &editor->width_scale_field, "editor.layout.screen.content_width",
            context->x, y, context->width, &item->content_scale.x);
        y += 38.0f;
        content_height_result = layout_number(
            &editor->content_height_scale_label, &editor->height_scale_field,
            "editor.layout.screen.content_height", context->x, y,
            context->width, &item->content_scale.y);
        y += 38.0f;
        content_rotation_result = layout_number(&editor->content_rotation_label,
            &editor->content_rotation_field,
            "editor.layout.screen.content_rotation", context->x, y,
            context->width, &source_rotation_degrees);
        if(content_rotation_result.changed) {
            source_rotation_degrees = fmodf(source_rotation_degrees, 360.0f);
            if(source_rotation_degrees < 0.0f) source_rotation_degrees += 360.0f;
            item->content_rotation = source_rotation_degrees *
                3.14159265359f / 180.0f;
        }
    }
    item->content_scale.x = fmaxf(0.01f, item->content_scale.x);
    item->content_scale.y = fmaxf(0.01f, item->content_scale.y);
    return name_result.active || x_result.active || y_result.active || width_result.active ||
        height_result.active || rotation_result.active || layer_active ||
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
    bool visible = item->visible;
    UIFieldResult x_result = {0}, y_result = {0}, rotation_result = {0};
    UIFieldResult thickness = {0}, spacing = {0}, radius = {0};
    bool layer_active = false;
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 8.0f, *y, 82.0f, 28.0f});
    UIFieldResult name_result = editor_mode_name_field("editor.layout.ui.name",
        item->name, sizeof(item->name), &editor->name_field,
        (UIRect){context->x + 94.0f, *y,
            context->width - 104.0f, 28.0f});
    *y += 38.0f;
    if(editor_mode_checkbox_left("editor.layout.ui.visibility",
            &editor->visible_label, (UIRect){context->x + 10.0f, *y,
                context->width - 20.0f, 28.0f}, &visible))
        item->visible = visible;
    *y += 38.0f;
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(
            context->x, context->width, *y);
    float layer_height = item->graphics_layer == 0 ||
        (context->layer_control != NULL &&
            (context->layer_control->adding ||
                context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
    const float transform_rows[] = {28.0f, 28.0f, 28.0f, layer_height};
    float interaction_rows[] = {28.0f, 28.0f};
    size_t interaction_count = item->drag_mode == VIEWPORT_ITEM_DRAG_NONE ? 1 : 2;
    float appearance_rows[11];
    size_t appearance_count = 1;
    appearance_rows[0] = 28.0f;
    if(item->border_enabled) {
        appearance_count = 6 +
            (item->border_type == EDITOR_VIEWPORT_UI_BORDER_HASHED ? 1 : 0) +
            (item->kind == EDITOR_VIEWPORT_UI_SHAPE &&
                item->value.shape.button_enabled ? 4 : 0);
        for(size_t i = 1; i < appearance_count; i += 1)
            appearance_rows[i] = 28.0f;
    }
    EditorModeAccordionLayoutResult transform =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_transform_section, "editor.layout.ui.section.transform",
            transform_rows, 4, 10.0f);
    EditorModeAccordionLayoutResult interaction =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_interaction_section,
            "editor.layout.ui.section.interaction",
            interaction_rows, interaction_count, 10.0f);
    EditorModeAccordionLayoutResult appearance =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_appearance_section,
            "editor.layout.ui.section.appearance",
            appearance_rows, appearance_count, 10.0f);
    if(transform.expanded) {
    *y = transform.content_y;
    x_result = layout_number(&editor->x_label, &editor->x_field,
        "editor.layout.ui.x", context->x, *y, context->width, &item->position.x);
    *y += 38.0f;
    y_result = layout_number(&editor->y_label, &editor->y_field,
        "editor.layout.ui.y", context->x, *y, context->width, &item->position.y);
    *y += 38.0f;
    float *rotation = item->kind == EDITOR_VIEWPORT_UI_SHAPE ?
        &item->value.shape.rotation : &item->rotation;
    rotation_result = layout_number(&editor->rotation_label,
        &editor->rotation_field, "editor.layout.ui.rotation", context->x, *y,
        context->width, rotation);
    *y += 38.0f;
    EditorGraphicsLayerBinding layer_binding = {
        .value = item->layer, .layer = item->graphics_layer};
    layer_active = context->layer_control != NULL &&
        editor_mode_layer_control_draw(context->layer_control,
            "editor.layout.ui", context->project, &layer_binding, NULL,
            context->x, *y, context->width);
    item->layer = layer_binding.value;
    item->graphics_layer = layer_binding.layer;
    *y += layer_height;
    }
    if(interaction.expanded) {
    *y = interaction.content_y;
    bool draggable = item->drag_mode != VIEWPORT_ITEM_DRAG_NONE;
    if(editor_mode_checkbox_left("editor.layout.ui.draggable",
            &editor->draggable_label,
            (UIRect){context->x + 10.0f, *y, context->width - 20.0f, 28.0f},
            &draggable))
        item->drag_mode = draggable ? VIEWPORT_ITEM_DRAG_XY :
            VIEWPORT_ITEM_DRAG_NONE;
    *y += 38.0f;
    if(item->drag_mode != VIEWPORT_ITEM_DRAG_NONE) {
        const TextAsset *axes[] = {&editor->drag_x_label, &editor->drag_y_label,
            &editor->drag_xy_label};
        rohr_ui_label(&editor->drag_axis_label,
            (UIRect){context->x + 8.0f, *y, 82.0f, 28.0f});
        UIDropdownResult axis = editor_mode_dropdown("editor.layout.ui.drag_axis",
            axes, 3, (size_t)item->drag_mode - 1,
            (UIRect){context->x + 94.0f, *y,
                context->width - 104.0f, 28.0f}, NULL);
        if(axis.changed) item->drag_mode =
            (ViewportItemDragMode)(axis.selected_index + 1);
        *y += 38.0f;
    }
    }
    if(appearance.expanded) {
    *y = appearance.content_y;
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
        UIDropdownResult type = editor_mode_dropdown("editor.layout.ui.border_type",
            types, 2, item->border_type, (UIRect){context->x + 94.0f, *y,
                context->width - 104.0f, 28.0f}, NULL);
        if(type.changed) item->border_type =
            (EditorViewportUiBorderType)type.selected_index;
        *y += 38.0f;
        thickness = layout_number(&editor->border_thickness_label,
            &editor->border_thickness_field, "editor.layout.ui.border_thickness",
            context->x, *y, context->width, &item->border_thickness);
        item->border_thickness = fmaxf(0.1f, item->border_thickness);
        *y += 38.0f;
        if(item->border_type == EDITOR_VIEWPORT_UI_BORDER_HASHED) {
            spacing = layout_number(&editor->hash_spacing_label,
                &editor->hash_spacing_field, "editor.layout.ui.hash_spacing",
                context->x, *y, context->width, &item->border_hash_spacing);
            item->border_hash_spacing = fmaxf(0.1f, item->border_hash_spacing);
            *y += 38.0f;
        }
        radius = layout_number(&editor->corner_radius_label,
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
    }
    }
    *y = accordion.y;
    return name_result.active || x_result.active || y_result.active ||
        rotation_result.active || layer_active || thickness.active ||
        spacing.active || radius.active;
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
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    const float content_rows[] = {28.0f, 30.0f};
    EditorModeAccordionLayoutResult content =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_content_section, "editor.ui_shape.section.content",
            content_rows, 2, 10.0f);
    if(!content.expanded) return active;
    y = content.content_y;
    button = item->value.shape.button_enabled;
    if(editor_mode_checkbox_left("editor.ui_shape.button", &editor->button_label,
            (UIRect){context->x + 10.0f, y, context->width - 20.0f, 28.0f},
            &button)) item->value.shape.button_enabled = button;
    y += 38.0f;
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
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    const float content_rows[] = {28.0f, 28.0f, 28.0f, 28.0f, 28.0f,
        28.0f, 28.0f, 28.0f, 28.0f};
    EditorModeAccordionLayoutResult content =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_content_section,
            item->kind == EDITOR_VIEWPORT_UI_TEXT ?
                "editor.ui_text.section.content" :
                "editor.ui_text_child.section.content",
            content_rows, 9, 10.0f);
    if(!content.expanded) return active;
    y = content.content_y;
    rohr_ui_label(&editor->text_label,
        (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
    text_result = editor_mode_field("editor.ui_text.text",
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
    UIDropdownResult font_result = editor_mode_dropdown("editor.ui_text.font",
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

bool editor_ui_slider_editor_draw(EditorLayoutViewportEditor *editor,
        const EditorModeContext *context) {
    EditorViewportUiItem *item = layout_ui_item_get(context,
        EDITOR_VIEWPORT_UI_SLIDER);
    EditorViewportUiSlider *slider;
    UIFieldResult minimum, maximum, value, step, length, track, thumb;
    float y = 42.0f;
    bool active;
    if(editor == NULL || item == NULL) return false;
    slider = &item->value.slider;
    active = layout_ui_common_draw(editor, context, item, &y);
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    const float content_rows[] = {28.0f, 28.0f, 28.0f, 28.0f, 28.0f, 28.0f};
    const float appearance_rows[] = {28.0f, 28.0f, 28.0f, 28.0f, 28.0f,
        28.0f, 28.0f, 28.0f};
    EditorModeAccordionLayoutResult content =
        editor_mode_accordion_layout_section(&accordion,
            &editor->ui_content_section, "editor.ui_slider.section.content",
            content_rows, 6, 10.0f);
    EditorModeAccordionLayoutResult appearance =
        editor_mode_accordion_layout_section(&accordion,
            &editor->appearance_section, "editor.ui_slider.section.appearance",
            appearance_rows, 8, 10.0f);
    minimum = maximum = value = step = length = track = thumb =
        (UIFieldResult){0};
    if(content.expanded) {
        const TextAsset *orientation_options[] = {&editor->horizontal_label,
            &editor->vertical_label};
        y = content.content_y;
        minimum = layout_number(&editor->minimum_label, &editor->minimum_field,
            "editor.ui_slider.minimum", context->x, y, context->width,
            &slider->minimum); y += 38.0f;
        maximum = layout_number(&editor->maximum_label, &editor->maximum_field,
            "editor.ui_slider.maximum", context->x, y, context->width,
            &slider->maximum); y += 38.0f;
        value = layout_number(&editor->value_label, &editor->value_field,
            "editor.ui_slider.value", context->x, y, context->width,
            &slider->value); y += 38.0f;
        step = layout_number(&editor->step_label, &editor->step_field,
            "editor.ui_slider.step", context->x, y, context->width,
            &slider->step); y += 38.0f;
        rohr_ui_label(&editor->orientation_label,
            (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
        UIDropdownResult orientation = editor_mode_dropdown(
            "editor.ui_slider.orientation", orientation_options, 2,
            (size_t)slider->orientation, (UIRect){context->x + 94.0f, y,
                context->width - 104.0f, 28.0f}, NULL);
        if(orientation.changed) slider->orientation =
            (ViewportUiSliderOrientation)orientation.selected_index;
        y += 38.0f;
        (void)editor_mode_checkbox_left("editor.ui_slider.enabled",
            &editor->enabled_label, (UIRect){context->x + 10.0f, y,
                context->width - 20.0f, 28.0f}, &slider->enabled);
        if(slider->maximum <= slider->minimum) slider->maximum = slider->minimum + 1.0f;
        slider->value = fminf(slider->maximum, fmaxf(slider->minimum, slider->value));
        slider->step = fmaxf(0.0f, slider->step);
    }
    if(appearance.expanded) {
        y = appearance.content_y;
        length = layout_number(&editor->length_label, &editor->length_field,
            "editor.ui_slider.length", context->x, y, context->width,
            &slider->length); y += 38.0f;
        track = layout_number(&editor->track_thickness_label,
            &editor->track_thickness_field, "editor.ui_slider.track_thickness",
            context->x, y, context->width, &slider->track_thickness); y += 38.0f;
        thumb = layout_number(&editor->thumb_size_label, &editor->thumb_size_field,
            "editor.ui_slider.thumb_size", context->x, y, context->width,
            &slider->thumb_size); y += 38.0f;
#define SLIDER_SWATCH(id, label, member) \
        rohr_ui_label(&(label), (UIRect){context->x + 8.0f, y, 120.0f, 28.0f}); \
        (void)layout_local_swatch((id), &(member), (UIRect){context->x + \
            context->width - 46.0f, y, 36.0f, 28.0f}, context); y += 38.0f
        SLIDER_SWATCH("editor.ui_slider.track_color", editor->track_color_label,
            slider->track_color);
        SLIDER_SWATCH("editor.ui_slider.filled_track_color",
            editor->filled_track_color_label, slider->filled_track_color);
        SLIDER_SWATCH("editor.ui_slider.thumb_color", editor->thumb_color_label,
            slider->thumb_color);
        SLIDER_SWATCH("editor.ui_slider.hover_thumb_color",
            editor->hover_thumb_color_label, slider->hover_thumb_color);
        SLIDER_SWATCH("editor.ui_slider.pressed_thumb_color",
            editor->pressed_thumb_color_label, slider->pressed_thumb_color);
#undef SLIDER_SWATCH
        slider->length = fmaxf(1.0f, slider->length);
        slider->track_thickness = fmaxf(1.0f, slider->track_thickness);
        slider->thumb_size = fmaxf(1.0f, slider->thumb_size);
    }
    return active || minimum.active || maximum.active || value.active ||
        step.active || length.active || track.active || thumb.active;
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
