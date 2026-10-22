/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_soft_beam.h"

#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>

static EditorSoftBody *body_get(EditorObject *object, EditorSoftBodyId id) {
    if(object == NULL) return NULL;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}

static EditorSoftBeam *beam_get(EditorSoftBody *body, EditorSoftBeamId id) {
    if(body == NULL) return NULL;
    for(size_t i = 0; i < body->beam_count; i += 1)
        if(body->beams[i].id == id) return &body->beams[i];
    return NULL;
}

static void float_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorSoftBeamId beam,
        EditorPropertyKind property, float value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_SOFT_BEAM, object, body, beam, 0,
            property, EDITOR_PROPERTY_VALUE_FLOAT, {.number = value}}};
    (void)editor_command_execute(project, &command);
}

static void bool_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorSoftBeamId beam,
        EditorPropertyKind property, bool value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_SOFT_BEAM, object, body, beam, 0,
            property, EDITOR_PROPERTY_VALUE_BOOL, {.boolean = value}}};
    (void)editor_command_execute(project, &command);
}

static void relationship_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorSoftBeamId beam, uint32_t endpoint,
        EditorSoftNodeId node) {
    EditorCommand command = {.type = EDITOR_COMMAND_RELATIONSHIP_SET,
        .data.relationship_set = {EDITOR_RELATIONSHIP_SOFT_BEAM_NODE,
            object, body, beam, endpoint, node}};
    (void)editor_command_execute(project, &command);
}

bool editor_soft_beam_editor_create(EditorSoftBeamEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorSoftBeamEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("Node A", node_a_label);
    CREATE("Node B", node_b_label); CREATE("Stiffness", stiffness_label);
    CREATE("Damping", damping_label);
    CREATE("Rest Length", rest_length_label); CREATE("", rest_length_field);
    CREATE("Explicit Rest Length", rest_explicit_label);
    CREATE("Thickness", thickness_label);
    CREATE("Friction", friction_label); CREATE("Restitution", restitution_label);
    CREATE("", friction_field); CREATE("", restitution_field);

    CREATE("Beam Color", color_label);
    CREATE("Inherit", inherit_label); CREATE("None", none_label);
    CREATE("Visible in App", visibility_label); CREATE("[X]", visible_label);
    CREATE("[ ]", hidden_label);
    CREATE("Delete Beam", delete_label); CREATE("", stiffness_field);
    CREATE("", damping_field); CREATE("", thickness_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->appearance_section, font, "Appearance", false) ||
            !editor_mode_accordion_section_create(&editor->material_section, font,
            "Material", false)) goto fail;
    if(!editor_collision_controls_create(&editor->collision, font)) {
        editor_soft_beam_editor_destroy(editor); return false;
    }
    return true;
fail:
    editor_soft_beam_editor_destroy(editor);
    return false;
}

void editor_soft_beam_editor_destroy(EditorSoftBeamEditor *editor) {
    if(editor == NULL) return;
    editor_collision_controls_destroy(&editor->collision);
    editor_mode_accordion_section_destroy(&editor->material_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    rohr_graphics_text_destroy(&editor->friction_label);
    rohr_graphics_text_destroy(&editor->restitution_label);
    rohr_graphics_text_destroy(&editor->friction_field);
    rohr_graphics_text_destroy(&editor->restitution_field);
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(node_a_label); DESTROY(node_b_label);
    DESTROY(stiffness_label); DESTROY(damping_label);
    DESTROY(rest_length_label); DESTROY(rest_length_field); DESTROY(rest_explicit_label);
    DESTROY(thickness_label);

    DESTROY(color_label);
    DESTROY(inherit_label); DESTROY(none_label); DESTROY(visibility_label);
    DESTROY(visible_label);
    DESTROY(hidden_label); DESTROY(delete_label); DESTROY(stiffness_field);
    DESTROY(damping_field); DESTROY(thickness_field);
#undef DESTROY
    for(size_t i = 0; i < EDITOR_SOFT_BEAM_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->beam_names[i]);
    for(size_t i = 0; i < EDITOR_SOFT_NODE_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->node_names[i]);
    *editor = (EditorSoftBeamEditor){0};
}

bool editor_soft_beam_editor_draw(EditorSoftBeamEditor *editor,
        const EditorModeContext *context) {
    EditorObject *object;
    EditorSoftBody *body;
    EditorSoftBeam *beam;
    size_t beam_index;
    char name[EDITOR_OBJECT_NAME_MAX];
    float stiffness, damping, thickness;
    float color_y;
    float layer_y;
    bool field_active = false;
    UIFieldResult name_result, stiffness_result, damping_result, thickness_result;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    body = body_get(object, context->viewport->selected_soft_body);
    beam = beam_get(body, context->viewport->selected_soft_beam);
    if(beam == NULL) return false;
    EditorSelectionRef collision_target = {EDITOR_SELECTION_SOFT_BEAM, object->id, body->id, 0, beam->id};
    editor_collision_controls_selection_set(&editor->collision, &collision_target, 1);
    beam_index = (size_t)(beam - body->beams);
    if(beam_index >= EDITOR_SOFT_BEAM_MAX) return false;
    snprintf(name, sizeof(name), "%s", beam->name);
    if(!editor_mode_named_text_sync(editor->font, beam->name,
            &editor->beam_names[beam_index], editor->beam_cache[beam_index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 40.0f, 40.0f, 48.0f, 30.0f});
    name_result = editor_mode_name_field("editor.soft_beam.name", name,
        sizeof(name), &editor->beam_names[beam_index],
        (UIRect){context->x + 88.0f, 40.0f,
            context->width - 96.0f, 30.0f});
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_SOFT_BEAM,
                .object = object->id, .parent = body->id, .item = beam->id}};
        snprintf(command.data.item_rename.name,
            sizeof(command.data.item_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    {
        bool visible = !beam->app_hidden;
        if(editor_mode_checkbox_left("editor.soft_beam.visibility",
                &editor->visibility_label,
                (UIRect){context->x + 10.0f, 80.0f,
                    context->width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_APP_VISIBILITY,
                .data.app_visibility = {EDITOR_ITEM_SOFT_BEAM, object->id,
                    body->id, beam->id, visible}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    {
        const TextAsset *options[EDITOR_SOFT_NODE_MAX + 1];
        EditorSelectionRef references[EDITOR_SOFT_NODE_MAX + 1] = {0};
        size_t selected_a = 0, selected_b = 0;
        options[0] = &editor->none_label;
        for(size_t i = 0; i < body->node_count && i < EDITOR_SOFT_NODE_MAX; i += 1) {
            if(!editor_mode_named_text_sync(editor->font, body->nodes[i].name,
                    &editor->node_names[i], editor->node_cache[i],
                    EDITOR_OBJECT_NAME_MAX)) return name_result.active;
            options[i + 1] = &editor->node_names[i];
            references[i + 1] = (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE,
                object->id, body->id, 0, body->nodes[i].id};
            if(body->nodes[i].id == beam->node_a) selected_a = i + 1;
            if(body->nodes[i].id == beam->node_b) selected_b = i + 1;
        }
        rohr_ui_label(&editor->node_a_label,
            (UIRect){context->x + 8.0f, 122.0f, 70.0f, 28.0f});
        rohr_ui_label(&editor->node_b_label,
            (UIRect){context->x + 8.0f, 158.0f, 70.0f, 28.0f});
        UIDropdownResult a = editor_mode_reference_dropdown("editor.soft_beam.node_a", options,
            (body->node_count < EDITOR_SOFT_NODE_MAX ? body->node_count : EDITOR_SOFT_NODE_MAX) + 1, selected_a,
            (UIRect){context->x + 80.0f, 122.0f,
                context->width - 90.0f, 28.0f}, NULL, context, references);
        UIDropdownResult b = editor_mode_reference_dropdown("editor.soft_beam.node_b", options,
            (body->node_count < EDITOR_SOFT_NODE_MAX ? body->node_count : EDITOR_SOFT_NODE_MAX) + 1, selected_b,
            (UIRect){context->x + 80.0f, 158.0f,
                context->width - 90.0f, 28.0f}, NULL, context, references);
        if(a.button_hovered) context->viewport->preview_soft_node = beam->node_a;
        else if(a.hovered_index > 0) context->viewport->preview_soft_node =
            body->nodes[a.hovered_index - 1].id;
        if(b.button_hovered) context->viewport->preview_soft_node = beam->node_b;
        else if(b.hovered_index > 0) context->viewport->preview_soft_node =
            body->nodes[b.hovered_index - 1].id;
        if(a.changed) relationship_set(context->project, object->id, body->id,
            beam->id, 0, a.selected_index == 0 ? 0 :
                body->nodes[a.selected_index - 1].id);
        if(b.changed) relationship_set(context->project, object->id, body->id,
            beam->id, 1, b.selected_index == 0 ? 0 :
                body->nodes[b.selected_index - 1].id);
    }
    stiffness = beam->stiffness;
    rohr_ui_label(&editor->stiffness_label,
        (UIRect){context->x + 8.0f, 196.0f, 90.0f, 26.0f});
    stiffness_result = editor_mode_field("editor.soft_beam.stiffness",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &stiffness},
        &editor->stiffness_field, (UIRect){context->x + 100.0f, 196.0f,
            context->width - 110.0f, 26.0f}, NULL);
    if(stiffness_result.changed) float_set(context->project, object->id,
        body->id, beam->id, EDITOR_PROPERTY_STIFFNESS, fmaxf(0.0f, stiffness));
    damping = beam->damping;
    rohr_ui_label(&editor->damping_label,
        (UIRect){context->x + 8.0f, 232.0f, 90.0f, 26.0f});
    damping_result = editor_mode_field("editor.soft_beam.damping",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &damping},
        &editor->damping_field, (UIRect){context->x + 100.0f, 232.0f,
            context->width - 110.0f, 26.0f}, NULL);
    if(damping_result.changed) float_set(context->project, object->id,
        body->id, beam->id, EDITOR_PROPERTY_DAMPING, fmaxf(0.0f, damping));
    thickness = beam->collision_thickness;
    rohr_ui_label(&editor->thickness_label,
        (UIRect){context->x + 8.0f, 268.0f, 90.0f, 26.0f});
    thickness_result = editor_mode_field("editor.soft_beam.thickness",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &thickness},
        &editor->thickness_field, (UIRect){context->x + 100.0f, 268.0f,
            context->width - 110.0f, 26.0f}, NULL);
    if(thickness_result.changed) float_set(context->project, object->id,
        body->id, beam->id, EDITOR_PROPERTY_BEAM_COLLISION_THICKNESS,
        fmaxf(ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN, thickness));
    bool rest_explicit = beam->rest_length_explicit;
    if(editor_mode_checkbox_left("editor.soft_beam.rest_explicit", &editor->rest_explicit_label,
            (UIRect){context->x + 10, 304, context->width - 20, 28}, &rest_explicit))
        bool_set(context->project, object->id, body->id, beam->id,
            EDITOR_PROPERTY_REST_LENGTH_EXPLICIT, rest_explicit);
    float rest_bottom = 340;
    if(rest_explicit) {
        float length = beam->rest_length;
        rohr_ui_label(&editor->rest_length_label,
            (UIRect){context->x + 8, 340, 90, 26});
        UIFieldResult rest = editor_mode_field("editor.soft_beam.rest_length",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &length},
            &editor->rest_length_field,
            (UIRect){context->x + 100, 340, context->width - 110, 26}, NULL);
        if(rest.changed) float_set(context->project, object->id, body->id,
            beam->id, EDITOR_PROPERTY_REST_LENGTH, length);
        field_active |= rest.active;
        rest_bottom = 376;
    }
    const float material_rows[] = {26, 26};
    EditorModeAccordionLayoutCursor cursor = editor_mode_accordion_layout_cursor_get(
        context->x, context->width, rest_bottom);
    EditorModeAccordionLayoutResult material = editor_mode_accordion_layout_section(
        &cursor, &editor->material_section, "editor.soft_beam.section.material",
        material_rows, 2, 6);
    if(material.expanded) {
        float values[] = {beam->friction, beam->restitution};
        TextAsset *labels[] = {&editor->friction_label, &editor->restitution_label};
        TextAsset *fields[] = {&editor->friction_field, &editor->restitution_field};
        const char *ids[] = {"editor.soft_beam.friction", "editor.soft_beam.restitution"};
        EditorPropertyKind properties[] = {EDITOR_PROPERTY_FRICTION, EDITOR_PROPERTY_RESTITUTION};
        UIButtonStyle style = editor_mode_section_field_style_get();
        for(size_t i = 0; i < 2; i += 1) {
            float y = editor_mode_accordion_layout_row_y(&material, material_rows, i, 6);
            rohr_ui_label(labels[i], (UIRect){context->x + 8, y, 96, 26});
            UIFieldResult result = editor_mode_field(ids[i],
                (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &values[i]}, fields[i],
                (UIRect){context->x + 106, y, context->width - 116, 26}, &style);
            if(result.changed) {
                float_set(context->project, object->id, body->id, beam->id, properties[i], values[i]);
                return true;
            }
            field_active |= result.active;
        }
    }
    EditorSelectionRef target = {EDITOR_SELECTION_SOFT_BEAM, object->id, body->id, 0, beam->id};
    EditorCollisionDrawResult collision = editor_collision_controls_draw(&editor->collision,
        "editor.soft_beam.collision", context->project, context->history, &target, 1,
        context->x + 10, cursor.y, context->width - 20, true);
    if(collision.changed) return true;
    field_active |= collision.active;
    cursor.y = collision.bottom + 4;
    float layer_height = editor_mode_layer_control_height_get(context->layer_control,
        beam->graphics_layer, &beam->graphics_layer_inherited);
    float appearance_rows[] = {26, layer_height};
    EditorModeAccordionLayoutResult appearance = editor_mode_accordion_layout_section(
        &cursor, &editor->appearance_section, "editor.soft_beam.section.appearance",
        appearance_rows, 2, 10);
    bool layer_active = false;
    if(appearance.expanded) {
    color_y = appearance.content_y;
    layer_y = color_y + 36.0f;
    {
        bool inherit = !beam->color_overridden;
        float field_width = fmaxf(34.0f, context->width - 196.0f);
        if(inherit) beam->color = body->beam_color;
        rohr_ui_label(&editor->color_label,
            (UIRect){context->x + 8.0f, color_y, 90.0f, 26.0f});
        if(editor_mode_checkbox_left("editor.soft_beam.color_inherit",
                &editor->inherit_label,
                (UIRect){context->x + context->width - 92.0f,
                    color_y, 82.0f, 26.0f}, &inherit)) {
            beam->color_overridden = !inherit;
            beam->color = body->beam_color;
        }
        (void)editor_mode_color_swatch("editor.soft_beam.color", &beam->color,
            inherit, (UIRect){context->x + 100.0f, color_y,
                field_width, 26.0f}, context, EDITOR_ITEM_SOFT_BEAM,
            object->id, body->id, beam->id, EDITOR_PROPERTY_COLOR);
    }
    layer_active = context->layer_control != NULL &&
        editor_mode_layer_control_draw(context->layer_control,
            "editor.soft_beam", context->project, &beam->graphics_layer,
            &beam->graphics_layer_inherited, context->x, layer_y,
            context->width);
    }
    if(context->delete_y_get != NULL && context->delete_open_item != NULL &&
            !context->delete_footer) {
        UIButtonStyle style = editor_mode_delete_style_get();
        if(rohr_ui_button("editor.soft_beam.delete", &editor->delete_label,
                (UIRect){context->x + 10.0f,
                    context->delete_y_get(context->delete_context),
                    context->width - 20.0f, 34.0f}, &style).clicked)
            (void)context->delete_open_item(context->delete_context);
    }
    return name_result.active || stiffness_result.active || damping_result.active ||
        thickness_result.active || layer_active || field_active;
}
