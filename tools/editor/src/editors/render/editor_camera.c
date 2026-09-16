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
    CREATE("Follow Orientation", inherit_label); CREATE("Visible", visible_label);
    CREATE("None", none_label); CREATE("Delete Camera", delete_label);
    CREATE("", x_field); CREATE("", y_field); CREATE("", angle_field);
    CREATE("", width_field); CREATE("", height_field);
    CREATE("", zoom_field);
#undef CREATE
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
    DESTROY(inherit_label); DESTROY(visible_label); DESTROY(none_label);
    DESTROY(delete_label); DESTROY(x_field); DESTROY(y_field); DESTROY(angle_field);
    DESTROY(width_field); DESTROY(height_field);
    DESTROY(zoom_field);
#undef DESTROY
    for(size_t i = 0; i < EDITOR_CAMERA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->name_values[i]);
    for(size_t i = 0; i < 256; i += 1)
        rohr_graphics_text_destroy(&editor->target_names[i]);
    *editor = (EditorCameraEditor){0};
}

static void camera_label_field(TextAsset *label, TextAsset *field, const char *id,
        float x, float y, float width, float *value, UIFieldResult *result) {
    rohr_ui_label(label, (UIRect){x + 8.0f, y, 82.0f, 28.0f});
    *result = rohr_ui_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){x + 94.0f, y, width - 104.0f, 28.0f}, NULL);
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
    float angle, width, height, zoom;
    UIFieldResult name_result, x_result, y_result, angle_result, width_result,
        height_result, zoom_result;
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
    rohr_ui_label(&editor->name_label, (UIRect){context->x + 8, 42, 82, 28});
    name_result = rohr_ui_field("editor.camera.name",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = sizeof(name)}, &editor->name_values[index],
        (UIRect){context->x + 94, 42, context->width - 104, 28}, NULL);
    position = camera->position; angle = camera->rotation;
    width = camera->dimensions.x; height = camera->dimensions.y; zoom = camera->zoom;
    camera_label_field(&editor->x_label, &editor->x_field, "editor.camera.x",
        context->x, 80, context->width, &position.x, &x_result);
    camera_label_field(&editor->y_label, &editor->y_field, "editor.camera.y",
        context->x, 118, context->width, &position.y, &y_result);
    camera_label_field(&editor->angle_label, &editor->angle_field, "editor.camera.angle",
        context->x, 156, context->width, &angle, &angle_result);
    camera_label_field(&editor->width_label, &editor->width_field, "editor.camera.width",
        context->x, 194, context->width, &width, &width_result);
    camera_label_field(&editor->height_label, &editor->height_field, "editor.camera.height",
        context->x, 232, context->width, &height, &height_result);
    camera_label_field(&editor->zoom_label, &editor->zoom_field, "editor.camera.zoom",
        context->x, 270, context->width, &zoom, &zoom_result);
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
    rohr_ui_label(&editor->attachment_label,
        (UIRect){context->x + 8, 308, 82, 28});
    UIDropdownResult attachment = rohr_ui_dropdown("editor.camera.attachment",
        options, option_count, selected,
        (UIRect){context->x + 94, 308, context->width - 104, 28}, NULL);
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
    bool inherit = camera->inherit_orientation, visible = camera->visible;
    bool inherit_changed = editor_mode_checkbox_left("editor.camera.inherit",
        &editor->inherit_label, (UIRect){context->x + 10, 346,
            context->width - 20, 28}, &inherit);
    bool visible_changed = editor_mode_checkbox_left("editor.camera.visible",
        &editor->visible_label, (UIRect){context->x + 10, 384,
            context->width - 20, 28}, &visible);
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
