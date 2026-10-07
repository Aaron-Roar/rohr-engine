/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_particle.h"

#include "editors/editor_mode_controls.h"

#include <math.h>

bool editor_particle_editor_create(EditorParticleEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorParticleEditor){0};
    if(!editor_mode_text_create(font, "Particle", &editor->title) ||
            !editor_mode_text_create(font, "Visibility", &editor->visibility_label) ||
            !editor_mode_text_create(font, "[X]", &editor->visible_label) ||
            !editor_mode_text_create(font, "[ ]", &editor->hidden_label) ||
            !editor_mode_text_create(font, "Delete Particle", &editor->delete_label) ||
            !editor_mode_text_create(font, "Radius", &editor->radius_label) ||
            !editor_mode_text_create(font, "Rigid Vertices",
                &editor->rigid_vertices_label) ||
            !editor_mode_text_create(font, "Origin X", &editor->origin_x_label) ||
            !editor_mode_text_create(font, "Origin Y", &editor->origin_y_label) ||
            !editor_mode_text_create(font, "Auto Fit", &editor->auto_fit_label) ||
            !editor_mode_text_create(font, "Ring Color",
                &editor->ring_color_label) ||
            !editor_mode_text_create(font, "Fill Color",
                &editor->fill_color_label) ||
            !editor_mode_text_create(font, "", &editor->radius_field) ||
            !editor_mode_text_create(font, "", &editor->rigid_vertices_field) ||
            !editor_mode_text_create(font, "", &editor->origin_x_field) ||
            !editor_mode_text_create(font, "", &editor->origin_y_field)) {
        editor_particle_editor_destroy(editor);
        return false;
    }
    if(!editor_center_of_mass_editor_create(&editor->center_of_mass, font)) {
        editor_particle_editor_destroy(editor);
        return false;
    }
    if(!editor_collision_controls_create(&editor->collision, font)) {
        editor_particle_editor_destroy(editor); return false;
    }
    return true;
}

void editor_particle_editor_destroy(EditorParticleEditor *editor) {
    if(editor == NULL) return;
    editor_collision_controls_destroy(&editor->collision);
    editor_center_of_mass_editor_destroy(&editor->center_of_mass);
    rohr_graphics_text_destroy(&editor->title);
    rohr_graphics_text_destroy(&editor->visibility_label);
    rohr_graphics_text_destroy(&editor->visible_label);
    rohr_graphics_text_destroy(&editor->hidden_label);
    rohr_graphics_text_destroy(&editor->delete_label);
    rohr_graphics_text_destroy(&editor->radius_label);
    rohr_graphics_text_destroy(&editor->rigid_vertices_label);
    rohr_graphics_text_destroy(&editor->origin_x_label);
    rohr_graphics_text_destroy(&editor->origin_y_label);
    rohr_graphics_text_destroy(&editor->auto_fit_label);
    rohr_graphics_text_destroy(&editor->ring_color_label);
    rohr_graphics_text_destroy(&editor->fill_color_label);
    rohr_graphics_text_destroy(&editor->radius_field);
    rohr_graphics_text_destroy(&editor->rigid_vertices_field);
    rohr_graphics_text_destroy(&editor->origin_x_field);
    rohr_graphics_text_destroy(&editor->origin_y_field);
    *editor = (EditorParticleEditor){0};
}

bool editor_particle_editor_draw(EditorParticleEditor *editor,
        const EditorModeContext *context) {
    EditorObject *object;
    EditorRigidBody *body;
    UIFieldResult radius = {0};
    UIFieldResult rigid_vertices = {0};
    UIFieldResult origin_x = {0};
    UIFieldResult origin_y = {0};
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    body = object == NULL ? NULL : editor_project_rigid_body_get(object,
        context->viewport->selected_rigid_body);
    if(body == NULL || !body->particle) return false;
    EditorSelectionRef collision_target = {EDITOR_SELECTION_PARTICLE, object->id, 0, 0, body->id};
    editor_collision_controls_selection_set(&editor->collision, &collision_target, 1);
    rohr_ui_label(&editor->title,
        (UIRect){context->x + 10.0f, 42.0f, context->width - 20.0f, 30.0f});
    {
        bool visible = body->visible;
        if(editor_mode_visibility_field("editor.particle.visibility",
                &editor->visibility_label, &editor->visible_label,
                &editor->hidden_label,
                (UIRect){context->x + 10.0f, 80.0f,
                    context->width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_RIGID_BODY,
                    object->id, 0, body->id, visible}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    rohr_ui_label(&editor->radius_label,
        (UIRect){context->x + 8.0f, 120.0f, 52.0f, 26.0f});
    float radius_value = body->particle_radius;
    if(body->particle_auto_fit) {
        editor_mode_numeric_disabled_draw(&editor->radius_field,
            body->particle_radius, (UIRect){context->x + 62.0f, 120.0f,
                fmaxf(30.0f, context->width - 166.0f), 26.0f});
    } else {
        radius = editor_mode_field("editor.particle.radius",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &radius_value}, &editor->radius_field,
            (UIRect){context->x + 62.0f, 120.0f,
                fmaxf(30.0f, context->width - 166.0f), 26.0f}, NULL);
    }
    if(radius.changed && radius_value > 0.0f) {
        EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id,
                .property = EDITOR_PROPERTY_PARTICLE_RADIUS,
                .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                .value.number = radius_value}};
        (void)editor_command_execute(context->project, &command);
    }
    if(editor_mode_checkbox_left("editor.particle.auto_fit",
            &editor->auto_fit_label,
            (UIRect){context->x + context->width - 100.0f,
                120.0f, 90.0f, 26.0f}, &body->particle_auto_fit) &&
            body->particle_auto_fit)
        body->particle_radius = editor_project_particle_auto_radius_get(body);
    rohr_ui_label(&editor->rigid_vertices_label,
        (UIRect){context->x + 8.0f, 156.0f, 110.0f, 26.0f});
    float rigid_vertices_value = (float)body->particle_rigid_vertices;
    rigid_vertices = editor_mode_field("editor.particle.rigid_vertices",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT,
            .number = &rigid_vertices_value}, &editor->rigid_vertices_field,
        (UIRect){context->x + 120.0f, 156.0f,
            context->width - 130.0f, 26.0f}, NULL);
    if(rigid_vertices.changed) {
        uint32_t count = (uint32_t)fminf((float)EDITOR_HITBOX_VERTEX_MAX,
            fmaxf(3.0f, roundf(rigid_vertices_value)));
        EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id,
                .property = EDITOR_PROPERTY_PARTICLE_RIGID_VERTICES,
                .value_kind = EDITOR_PROPERTY_VALUE_UINT,
                .value.integer = count}};
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->origin_x_label,
        (UIRect){context->x + 8.0f, 192.0f, 72.0f, 26.0f});
    float origin_x_value = body->particle_origin.x;
    origin_x = editor_mode_field("editor.particle.origin_x",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT,
            .number = &origin_x_value}, &editor->origin_x_field,
        (UIRect){context->x + 82.0f, 192.0f,
            context->width - 92.0f, 26.0f}, NULL);
    if(origin_x.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id,
                .property = EDITOR_PROPERTY_PARTICLE_ORIGIN_X,
                .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                .value.number = origin_x_value}};
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->origin_y_label,
        (UIRect){context->x + 8.0f, 228.0f, 72.0f, 26.0f});
    float origin_y_value = body->particle_origin.y;
    origin_y = editor_mode_field("editor.particle.origin_y",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT,
            .number = &origin_y_value}, &editor->origin_y_field,
        (UIRect){context->x + 82.0f, 228.0f,
            context->width - 92.0f, 26.0f}, NULL);
    if(origin_y.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id,
                .property = EDITOR_PROPERTY_PARTICLE_ORIGIN_Y,
                .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                .value.number = origin_y_value}};
        (void)editor_command_execute(context->project, &command);
    }
    rohr_ui_label(&editor->ring_color_label,
        (UIRect){context->x + 8.0f, 264.0f, 104.0f, 26.0f});
    (void)editor_mode_color_swatch("editor.particle.ring_color",
        &body->particle_ring_color, false,
        (UIRect){context->x + 114.0f, 264.0f,
            context->width - 124.0f, 26.0f}, context,
        EDITOR_ITEM_RIGID_BODY, object->id, 0, body->id,
        EDITOR_PROPERTY_PARTICLE_RING_COLOR);
    rohr_ui_label(&editor->fill_color_label,
        (UIRect){context->x + 8.0f, 300.0f, 104.0f, 26.0f});
    (void)editor_mode_color_swatch("editor.particle.fill_color",
        &body->particle_fill_color, false,
        (UIRect){context->x + 114.0f, 300.0f,
            context->width - 124.0f, 26.0f}, context,
        EDITOR_ITEM_RIGID_BODY, object->id, 0, body->id,
        EDITOR_PROPERTY_PARTICLE_FILL_COLOR);
    bool com_active = editor_center_of_mass_editor_draw(&editor->center_of_mass,
        context, object, body, 336.0f);
    float layer_y = 336.0f + EDITOR_COM_ROW_COUNT * EDITOR_COM_ROW_HEIGHT + 6.0f;
    bool layer_active = context->layer_control != NULL &&
        editor_mode_layer_control_draw(context->layer_control,
            "editor.particle", context->project, &body->graphics_layer, NULL,
            context->x, layer_y, context->width);
    float layer_height = context->layer_control == NULL ? 0.0f :
        body->graphics_layer.layer == 0 || context->layer_control->adding ||
        context->layer_control->edited_layer != 0 ? 86.0f : 48.0f;
    EditorSelectionRef target = {EDITOR_SELECTION_PARTICLE, object->id, 0, 0, body->id};
    EditorCollisionDrawResult collision = editor_collision_controls_draw(&editor->collision,
        "editor.particle.collision", context->project, context->history, &target, 1,
        context->x + 10, layer_y + layer_height + 6, context->width - 20, true);
    if(collision.changed) return true;
    editor_mode_accordion_layout_measure_include(layer_y + layer_height);
    return collision.active || com_active || radius.active || rigid_vertices.active || origin_x.active ||
        origin_y.active || layer_active;
}

bool editor_particle_radius_editor_draw(EditorParticleEditor *editor,
        const EditorModeContext *context) {
    EditorObject *object;
    EditorRigidBody *body;
    UIFieldResult radius;
    float radius_value;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    body = object == NULL ? NULL : editor_project_rigid_body_get(object,
        context->viewport->selected_rigid_body);
    if(body == NULL || !body->particle) return false;
    rohr_ui_label(&editor->title,
        (UIRect){context->x + 10.0f, 42.0f, context->width - 20.0f, 30.0f});
    rohr_ui_label(&editor->radius_label,
        (UIRect){context->x + 8.0f, 84.0f, 72.0f, 26.0f});
    radius_value = body->particle_radius;
    radius = editor_mode_field("editor.particle.resize.radius",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &radius_value},
        &editor->radius_field, (UIRect){context->x + 82.0f, 84.0f,
            context->width - 92.0f, 26.0f}, NULL);
    if(radius.changed && radius_value > 0.0f) {
        EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id,
                .property = EDITOR_PROPERTY_PARTICLE_RADIUS,
                .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                .value.number = radius_value}};
        (void)editor_command_execute(context->project, &command);
    }
    return radius.active;
}
