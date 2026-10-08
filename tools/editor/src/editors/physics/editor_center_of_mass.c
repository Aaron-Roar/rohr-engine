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

/* The dedicated panel and inline body controls share assets and command paths. */
bool editor_center_of_mass_panel_draw(EditorCenterOfMassEditor *editor,
        const EditorModeContext *context) {
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    EditorObject *object = editor_project_selected_get(context->project);
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
