/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_center_of_mass.h"
#include "editor_mass_properties.h"
#include "editors/editor_mode_controls.h"

#include <stdio.h>

bool editor_center_of_mass_editor_create(EditorCenterOfMassEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorCenterOfMassEditor){0};
#define CREATE(member, value) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE(mode_label, "COM");
    CREATE(mass_label, "Total Mass"); CREATE(mass_field, "");
    CREATE(automatic_label, "Automatic");
    CREATE(explicit_label, "Explicit");
    CREATE(centroid_label, "Centroid");
    CREATE(x_label, "COM Local X");
    CREATE(y_label, "COM Local Y");
    CREATE(inertia_label, "Inertia");
    CREATE(response_label, "");
    CREATE(x_field, "");
    CREATE(y_field, "");
    CREATE(inertia_field, "");
#undef CREATE
    return true;
fail:
    editor_center_of_mass_editor_destroy(editor);
    return false;
}

void editor_center_of_mass_editor_destroy(EditorCenterOfMassEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(mass_label); DESTROY(mass_field);
    DESTROY(mode_label); DESTROY(automatic_label); DESTROY(explicit_label);
    DESTROY(centroid_label); DESTROY(x_label); DESTROY(y_label);
    DESTROY(inertia_label); DESTROY(response_label);
    DESTROY(x_field); DESTROY(y_field); DESTROY(inertia_field);
#undef DESTROY
    *editor = (EditorCenterOfMassEditor){0};
}

static void readout_draw(TextAsset *field, bool available, float value,
        UIRect bounds) {
    char text[48];
    if(available) snprintf(text, sizeof(text), "%.6g", (double)value);
    else snprintf(text, sizeof(text), "Unavailable");
    (void)rohr_graphics_text_value_set(field, text);
    rohr_ui_button_disabled(bounds, NULL);
    rohr_ui_label(field, bounds);
}

bool editor_center_of_mass_editor_draw(EditorCenterOfMassEditor *editor,
        const EditorModeContext *context, EditorObject *object,
        EditorRigidBody *body, float y) {
    if(editor == NULL || context == NULL || object == NULL || body == NULL)
        return false;
    float x = context->x + 10.0f, width = context->width - 20.0f;
    float label_width = 112.0f;
    bool active = false;
    rohr_ui_label(&editor->mode_label, (UIRect){x, y, label_width, 26});
    UIRect field = {x + label_width, y, width - label_width, 26};
    if(body->standalone_particle) rohr_ui_label(&editor->centroid_label, field);
    else {
        const TextAsset *options[] = {&editor->automatic_label, &editor->explicit_label};
        UIButtonStyle style = editor_mode_section_field_style_get();
        UIDropdownResult mode = editor_mode_dropdown("editor.com.mode", options,
            2, body->center_of_mass_explicit ? 1 : 0, field, &style);
        if(mode.changed)
            (void)editor_center_of_mass_mode_set(context->project,
                object->id, body->id, mode.selected_index == 1);
    }
    EditorMassProperties value = editor_mass_properties_get(body);
    TextAsset *labels[] = {&editor->x_label, &editor->y_label};
    TextAsset *fields[] = {&editor->x_field, &editor->y_field};
    const char *ids[] = {"editor.com.x", "editor.com.y"};
    for(size_t i = 0; i < 2; i += 1) {
        field.y += EDITOR_COM_ROW_HEIGHT;
        rohr_ui_label(labels[i], (UIRect){x, field.y, label_width, 26});
        float number = i == 0 ? value.center.x : value.center.y;
        if(body->center_of_mass_explicit && !body->standalone_particle) {
            UIFieldResult changed = editor_mode_field(ids[i],
                (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &number},
                fields[i], field, NULL);
            active = changed.active || active;
            if(changed.changed) {
                Position offset = body->center_of_mass_offset;
                if(i == 0) offset.x = number;
                else offset.y = number;
                (void)editor_center_of_mass_set(context->project,
                    object->id, body->id, true, offset);
            }
        } else readout_draw(fields[i], value.center_available, number, field);
    }
    value = editor_mass_properties_get(body);
    field.y += EDITOR_COM_ROW_HEIGHT;
    rohr_ui_label(&editor->inertia_label, (UIRect){x, field.y, label_width, 26});
    readout_draw(&editor->inertia_field, value.inertia_available, value.inertia, field);
    (void)rohr_graphics_text_value_set(&editor->response_label,
        value.angular_response ? "Angular response enabled" : "Angular response disabled");
    rohr_ui_label(&editor->response_label,
        (UIRect){x, field.y + EDITOR_COM_ROW_HEIGHT, width, 26});
    return active;
}

bool editor_soft_center_of_mass_editor_draw(EditorCenterOfMassEditor *editor,
        const EditorModeContext *context, EditorObject *object, EditorSoftBody *body, float y) {
    if(editor == NULL || context == NULL || object == NULL || body == NULL) return false;
    if(editor->soft_body != body->id || editor->soft_object != object->id) {
        editor->soft_error = false; editor->soft_body = body->id; editor->soft_object = object->id;
    }
    bool active = false;
    TextAsset *labels[] = {&editor->mass_label, &editor->x_label, &editor->y_label};
    TextAsset *fields[] = {&editor->mass_field, &editor->x_field, &editor->y_field};
    const char *ids[] = {"editor.soft_body.mass", "editor.com.x", "editor.com.y"};
    for(size_t i = 0; i < 3; i++) {
        EditorSoftMassProperties value = editor_soft_mass_properties_get(body);
        UIRect bounds = {context->x + 122, y + i * EDITOR_COM_ROW_HEIGHT,
            context->width - 132, 26};
        rohr_ui_label(labels[i], (UIRect){context->x + 10, bounds.y, 112, 26});
        float number = i == 0 ? value.total : i == 1 ? value.center.x : value.center.y;
        bool available = i == 0 ? value.mass_available && body->node_count > 0 : value.center_available;
        if(!available) {
            readout_draw(fields[i], i == 0 && value.mass_available, number, bounds);
            continue;
        }
        UIFieldResult changed = editor_mode_field(ids[i],
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &number}, fields[i], bounds, NULL);
        active = changed.active || active;
        if(changed.changed) {
            EditorCommandResult result;
            if(i == 0) {
                EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
                    .data.property_set = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id,
                        .item = body->id, .property = EDITOR_PROPERTY_MASS,
                        .value_kind = EDITOR_PROPERTY_VALUE_FLOAT, .value.number = number}};
                result = editor_command_execute(context->project, &command);
            } else {
                Position center = value.center;
                if(i == 1) center.x = number; else center.y = number;
                result = editor_soft_center_of_mass_set(context->project, object->id, body->id, center);
            }
            editor->soft_error = result.kind == ERROR_RESULT_ERROR;
        }
    }
    (void)rohr_graphics_text_value_set(&editor->response_label,
        editor->soft_error ? "Invalid mass or unreachable COM" : "Calculated from node masses");
    rohr_ui_label(&editor->response_label,
        (UIRect){context->x + 10, y + 3 * EDITOR_COM_ROW_HEIGHT, context->width - 20, 26});
    return active;
}

/* The dedicated panel and inline body controls share assets and command paths. */
bool editor_center_of_mass_panel_draw(EditorCenterOfMassEditor *editor,
        const EditorModeContext *context) {
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    EditorObject *object = editor_project_selected_get(context->project);
    if(context->viewport->selected_rigid_body == 0 && object != NULL) {
        EditorSoftBody *soft = NULL;
        for(size_t i = 0; i < object->soft_body_count; i++)
            if(object->soft_body_items[i].id == context->viewport->selected_soft_body)
                soft = &object->soft_body_items[i];
        if(soft != NULL) {
            editor_mode_accordion_layout_measure_include(42 + 4 * EDITOR_COM_ROW_HEIGHT);
            return editor_soft_center_of_mass_editor_draw(editor, context, object, soft, 42);
        }
    }
    EditorRigidBody *body = editor_project_rigid_body_get(object,
        context->viewport->selected_rigid_body);
    if(body == NULL || body->standalone_particle) {
        editor_viewport_transform_cancel(context->viewport);
        editor_viewport_object_editor_enter(context->viewport);
        if(object == NULL) context->viewport->mode = EDITOR_VIEWPORT_HIERARCHY;
        return false;
    }
    editor_mode_accordion_layout_measure_include(42 + EDITOR_COM_ROW_COUNT * EDITOR_COM_ROW_HEIGHT);
    return editor_center_of_mass_editor_draw(editor, context, object, body, 42);
}
