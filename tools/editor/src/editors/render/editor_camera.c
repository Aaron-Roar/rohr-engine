/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_camera.h"
#include "editors/editor_mode_controls.h"

#include <stdio.h>

bool editor_camera_editor_create(EditorCameraEditor *editor, FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorCameraEditor){.font = font};
#define CREATE(value, member) if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Angle", angle_label); CREATE("Width", width_label);
    CREATE("Height", height_label); CREATE("Attachment", attachment_label);
    CREATE("Zoom", zoom_label);
    CREATE("Follow Orientation", inherit_label);
    CREATE("Visibility", visibility_label);
    CREATE("None", none_label); CREATE("Delete Camera", delete_label);
    CREATE("", x_field); CREATE("", y_field); CREATE("", angle_field);
    CREATE("", width_field); CREATE("", height_field);
    CREATE("", zoom_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true) ||
            !editor_mode_accordion_section_create(&editor->view_section, font,
                "View", false) ||
            !editor_mode_accordion_section_create(&editor->attachment_section, font,
                "Attachment", false)) goto fail;
    return true;
fail:
    editor_camera_editor_destroy(editor);
    return false;
}

void editor_camera_editor_destroy(EditorCameraEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(x_label); DESTROY(y_label); DESTROY(angle_label);
    DESTROY(width_label); DESTROY(height_label); DESTROY(attachment_label);
    DESTROY(zoom_label);
    DESTROY(inherit_label); DESTROY(visibility_label);
    DESTROY(none_label);
    DESTROY(delete_label); DESTROY(x_field); DESTROY(y_field); DESTROY(angle_field);
    DESTROY(width_field); DESTROY(height_field);
    DESTROY(zoom_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->view_section);
    editor_mode_accordion_section_destroy(&editor->attachment_section);
    for(size_t i = 0; i < EDITOR_CAMERA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->name_values[i]);
    for(size_t i = 0; i < 256; i += 1)
        rohr_graphics_text_destroy(&editor->target_names[i]);
    *editor = (EditorCameraEditor){0};
}

static void camera_label_field(TextAsset *label, TextAsset *field, const char *id,
        float x, float y, float width, float *value, UIFieldResult *result) {
    UIButtonStyle style = editor_mode_section_field_style_get();
    rohr_ui_label(label, (UIRect){x + 8.0f, y, 82.0f, 28.0f});
    *result = editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){x + 94.0f, y, width - 104.0f, 28.0f}, &style);
}

bool editor_camera_editor_draw(EditorCameraEditor *editor,
        const EditorModeContext *context) {
    EditorObject *object;
    EditorCamera *camera;
    size_t index, option_count = 1, selected = 0;
    const TextAsset *options[257];
    EditorCameraAttachmentKind kinds[257] = {EDITOR_CAMERA_ATTACHMENT_NONE};
    uint32_t target_ids[257] = {0};
    EditorSoftBodyId parent_ids[257] = {0};
    char name[EDITOR_OBJECT_NAME_MAX];
    Position position;
    float angle, width, height, zoom, y;
    UIFieldResult name_result, x_result = {0}, y_result = {0}, angle_result = {0},
        width_result = {0}, height_result = {0}, zoom_result = {0};
    UIDropdownResult attachment = {0};
    bool inherit_changed = false, visible_changed = false;
    bool inherit, visible;
    UIButtonStyle section_field_style = editor_mode_section_field_style_get();
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    camera = editor_project_camera_get(object, context->viewport->selected_camera_entity);
    if(object == NULL || camera == NULL) return false;
    index = (size_t)(camera - object->cameras);
    if(index >= EDITOR_CAMERA_MAX || !editor_mode_named_text_sync(editor->font,
            camera->name, &editor->name_values[index], editor->name_cache[index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    snprintf(name, sizeof(name), "%s", camera->name);
    visible = camera->visible;
    rohr_ui_label(&editor->name_label, (UIRect){context->x + 8, 42, 82, 28});
    name_result = editor_mode_field("editor.camera.name",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = sizeof(name)}, &editor->name_values[index],
        (UIRect){context->x + 94, 42, context->width - 104, 28}, NULL);
    position = camera->position; angle = camera->rotation;
    width = camera->dimensions.x; height = camera->dimensions.y; zoom = camera->zoom;
    visible_changed = editor_mode_checkbox_left("editor.camera.visible",
        &editor->visibility_label, (UIRect){context->x + 10.0f, 80.0f,
            context->width - 20.0f, 28.0f}, &visible);
    y = 118.0f;
    const float transform_rows[] = {
        28.0f, 28.0f, 28.0f, 28.0f, 28.0f};
    const float view_rows[] = {28.0f};
    const float attachment_rows[] = {28.0f, 28.0f};
    EditorModeAccordionLayoutCursor accordion =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    EditorModeAccordionLayoutResult transform =
        editor_mode_accordion_layout_section(&accordion,
            &editor->transform_section, "editor.camera.section.transform",
            transform_rows, 5, 10.0f);
    EditorModeAccordionLayoutResult view =
        editor_mode_accordion_layout_section(&accordion,
            &editor->view_section, "editor.camera.section.view",
            view_rows, 1, 10.0f);
    EditorModeAccordionLayoutResult attachment_section =
        editor_mode_accordion_layout_section(&accordion,
            &editor->attachment_section, "editor.camera.section.attachment",
            attachment_rows, 2, 10.0f);
    if(transform.expanded) {
        y = transform.content_y;
        camera_label_field(&editor->x_label, &editor->x_field, "editor.camera.x",
            context->x, y, context->width, &position.x, &x_result); y += 38.0f;
        camera_label_field(&editor->y_label, &editor->y_field, "editor.camera.y",
            context->x, y, context->width, &position.y, &y_result); y += 38.0f;
        camera_label_field(&editor->angle_label, &editor->angle_field,
            "editor.camera.angle", context->x, y, context->width, &angle,
            &angle_result); y += 38.0f;
        camera_label_field(&editor->width_label, &editor->width_field,
            "editor.camera.width", context->x, y, context->width, &width,
            &width_result); y += 38.0f;
        camera_label_field(&editor->height_label, &editor->height_field,
            "editor.camera.height", context->x, y, context->width, &height,
            &height_result); y += 38.0f;
    }
    if(view.expanded) {
        y = view.content_y;
        camera_label_field(&editor->zoom_label, &editor->zoom_field,
            "editor.camera.zoom", context->x, y, context->width, &zoom,
            &zoom_result); y += 38.0f;
    }
    options[0] = &editor->none_label;
#define ADD_TARGET(kind_value, id_value, parent_value, source_name) do { \
    if(option_count < 257 && editor_mode_named_text_sync(editor->font, source_name, \
            &editor->target_names[option_count - 1], editor->target_cache[option_count - 1], \
            EDITOR_OBJECT_NAME_MAX)) { options[option_count] = &editor->target_names[option_count - 1]; \
        kinds[option_count] = kind_value; target_ids[option_count] = id_value; \
        parent_ids[option_count] = parent_value; if(camera->attachment_kind == kind_value && \
            camera->attachment == id_value && camera->attachment_soft_body == parent_value) \
            selected = option_count; option_count += 1; } } while(0)
    for(size_t i = 0; i < object->rigid_body_count; i += 1)
        ADD_TARGET(EDITOR_CAMERA_ATTACHMENT_RIGID_BODY, object->rigid_bodies[i].id, 0,
            object->rigid_bodies[i].name);
    for(size_t i = 0; i < object->soft_body_count; i += 1) {
        EditorSoftBody *body = &object->soft_body_items[i];
        ADD_TARGET(EDITOR_CAMERA_ATTACHMENT_SOFT_BODY, body->id, 0, body->name);
        for(size_t n = 0; n < body->node_count; n += 1)
            ADD_TARGET(EDITOR_CAMERA_ATTACHMENT_SOFT_NODE, body->nodes[n].id,
                body->id, body->nodes[n].name);
    }
    for(size_t i = 0; i < object->anchor_count; i += 1)
        ADD_TARGET(EDITOR_CAMERA_ATTACHMENT_ANCHOR, object->anchors[i].id, 0,
            object->anchors[i].name);
#undef ADD_TARGET
    attachment.selected_index = selected;
    inherit = camera->inherit_orientation;
    if(attachment_section.expanded) {
        y = attachment_section.content_y;
        rohr_ui_label(&editor->attachment_label,
            (UIRect){context->x + 8.0f, y, 82.0f, 28.0f});
        attachment = editor_mode_dropdown("editor.camera.attachment", options,
            option_count, selected,
            (UIRect){context->x + 94.0f, y, context->width - 104.0f, 28.0f},
            &section_field_style);
        if(attachment.button_hovered || attachment.hovered_index >= 0) {
            size_t preview = attachment.hovered_index >= 0 ?
                (size_t)attachment.hovered_index : selected;
            if(preview < option_count) {
                if(kinds[preview] == EDITOR_CAMERA_ATTACHMENT_RIGID_BODY)
                    context->viewport->preview_rigid_body = target_ids[preview];
                else if(kinds[preview] == EDITOR_CAMERA_ATTACHMENT_SOFT_BODY)
                    context->viewport->preview_soft_body = target_ids[preview];
                else if(kinds[preview] == EDITOR_CAMERA_ATTACHMENT_SOFT_NODE)
                    context->viewport->preview_soft_node = target_ids[preview];
                else if(kinds[preview] == EDITOR_CAMERA_ATTACHMENT_ANCHOR)
                    context->viewport->preview_anchor = target_ids[preview];
            }
        }
        y += 38.0f;
        inherit_changed = editor_mode_checkbox_left("editor.camera.inherit",
            &editor->inherit_label, (UIRect){context->x + 10.0f, y,
                context->width - 20.0f, 28.0f}, &inherit);
    }
    if(name_result.changed) { EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
        .data.item_rename = {.kind = EDITOR_ITEM_CAMERA, .object = object->id,
            .item = camera->id}}; snprintf(command.data.item_rename.name,
        sizeof(command.data.item_rename.name), "%s", name); (void)editor_command_execute(context->project, &command); }
    if(x_result.changed || y_result.changed || angle_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_CAMERA_TRANSFORM,
            .data.camera_transform = {object->id, camera->id, position, angle}};
        (void)editor_command_execute(context->project, &command);
    }
    if(width_result.changed || height_result.changed) { EditorCommand command = {
        .type = EDITOR_COMMAND_CAMERA_DIMENSIONS_SET,
        .data.camera_dimensions_set = {object->id, camera->id, {width, height}}};
        (void)editor_command_execute(context->project, &command); }
    if(zoom_result.changed) { EditorCommand command = {
        .type = EDITOR_COMMAND_CAMERA_ZOOM_SET,
        .data.camera_zoom_set = {object->id, camera->id, zoom}};
        (void)editor_command_execute(context->project, &command); }
    if(attachment.changed || inherit_changed) { size_t choice = attachment.changed ?
        attachment.selected_index : selected; EditorCommand command = {
        .type = EDITOR_COMMAND_CAMERA_ATTACHMENT_SET,
        .data.camera_attachment_set = {object->id, camera->id, kinds[choice],
            target_ids[choice], parent_ids[choice], inherit}};
        (void)editor_command_execute(context->project, &command); }
    if(visible_changed) { EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
        .data.visibility = {EDITOR_VISIBILITY_CAMERA, object->id, 0,
            camera->id, visible}}; (void)editor_command_execute(context->project,
        &command); }
    return name_result.active || x_result.active || y_result.active ||
        angle_result.active || width_result.active || height_result.active ||
        zoom_result.active;
}
