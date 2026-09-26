/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_soft_body.h"

#include "editor_navigation.h"
#include "editors/editor_mode_controls.h"

#include <stdio.h>

static UIButtonStyle selected_style_get(void) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    style.idle = (Color){118, 96, 35, 255};
    style.hovered = (Color){145, 119, 45, 255};
    return style;
}

static UIButtonStyle delete_style_get(void) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    style.idle = (Color){145, 42, 48, 255};
    style.hovered = (Color){181, 53, 60, 255};
    style.pressed = (Color){112, 31, 37, 255};
    style.disabled = (Color){75, 35, 38, 210};
    return style;
}

static EditorSoftBody *body_get(EditorObject *object, EditorSoftBodyId id) {
    if(object == NULL) return NULL;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}

static void initial_motion_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorPropertyKind property, float value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_SOFT_BODY, object, 0, body, 0,
            property, EDITOR_PROPERTY_VALUE_FLOAT, {.number = value}}};
    (void)editor_command_execute(project, &command);
}

bool editor_soft_body_editor_create(EditorSoftBodyEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorSoftBodyEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Rotation (deg)", rotation_label); CREATE("Node Color", node_color_label);
    CREATE("Velocity X", velocity_x_label); CREATE("Velocity Y", velocity_y_label);
    CREATE("Acceleration X", acceleration_x_label);
    CREATE("Acceleration Y", acceleration_y_label);
    CREATE("Angular Velocity (deg/s)", angular_velocity_label);
    CREATE("Beam Color", beam_color_label); CREATE("Area Color", area_color_label);
    CREATE("Origin", origin_label); CREATE("Auto Shape", auto_shape_label);
    CREATE("Add Node", add_node_label); CREATE("Add Beam", add_beam_label);
    CREATE("Visibility", visibility_label); CREATE("[X]", visible_label);
    CREATE("[ ]", hidden_label);
    CREATE("Delete Soft Body", delete_label); CREATE("", x_field);
    CREATE("", y_field); CREATE("", rotation_field);
    CREATE("", velocity_x_field); CREATE("", velocity_y_field);
    CREATE("", acceleration_x_field); CREATE("", acceleration_y_field);
    CREATE("", angular_velocity_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true) ||
            !editor_mode_accordion_section_create(
                &editor->initial_motion_section, font, "Initial Motion", false) ||
            !editor_mode_accordion_section_create(&editor->appearance_section, font,
                "Appearance", false) ||
            !editor_mode_accordion_section_create(&editor->topology_section, font,
                "Topology", false)) goto fail;
    return true;
fail:
    editor_soft_body_editor_destroy(editor);
    return false;
}

void editor_soft_body_editor_destroy(EditorSoftBodyEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(x_label); DESTROY(y_label);
    DESTROY(rotation_label); DESTROY(node_color_label); DESTROY(beam_color_label);
    DESTROY(velocity_x_label); DESTROY(velocity_y_label);
    DESTROY(acceleration_x_label); DESTROY(acceleration_y_label);
    DESTROY(angular_velocity_label);
    DESTROY(area_color_label); DESTROY(origin_label); DESTROY(auto_shape_label);
    DESTROY(add_node_label); DESTROY(add_beam_label); DESTROY(visibility_label);
    DESTROY(visible_label);
    DESTROY(hidden_label); DESTROY(delete_label); DESTROY(x_field);
    DESTROY(y_field); DESTROY(rotation_field);
    DESTROY(velocity_x_field); DESTROY(velocity_y_field);
    DESTROY(acceleration_x_field); DESTROY(acceleration_y_field);
    DESTROY(angular_velocity_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->initial_motion_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    editor_mode_accordion_section_destroy(&editor->topology_section);
    for(size_t i = 0; i < EDITOR_SOFT_BODY_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->body_names[i]);
    for(size_t i = 0; i < EDITOR_SOFT_NODE_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->node_names[i]);
    for(size_t i = 0; i < EDITOR_SOFT_BEAM_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->beam_names[i]);
    for(size_t i = 0; i < EDITOR_SOFT_AREA_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->area_names[i]);
    *editor = (EditorSoftBodyEditor){0};
}

static bool hierarchy_item_draw(EditorSoftBodyEditor *editor,
        const EditorModeContext *context, EditorObject *object,
        EditorSoftBody *body, EditorSoftHierarchyItem item,
        size_t hierarchy_index, float base_y) {
    EditorHierarchySelection selection;
    EditorVisibilityKind visibility;
    TextAsset *label = NULL;
    char *cache = NULL;
    const char *name = NULL;
    bool shown = false, selected = false;
    char id[64], visibility_id[72];
    float y = base_y + (float)hierarchy_index * 28.0f;
    if(item.kind == EDITOR_SOFT_HIERARCHY_NODE) {
        for(size_t i = 0; i < body->node_count; i += 1)
            if(body->nodes[i].id == item.id) {
                name = body->nodes[i].name; shown = body->nodes[i].visible;
                label = &editor->node_names[i]; cache = editor->node_cache[i];
            }
        selection = EDITOR_SELECTION_SOFT_NODE;
        visibility = EDITOR_VISIBILITY_SOFT_NODE;
        selected = context->viewport->selection == selection &&
            context->viewport->selected_soft_node == item.id;
        snprintf(id, sizeof(id), "editor.soft_node.%u", item.id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.soft_node.%u.visibility", item.id);
    } else if(item.kind == EDITOR_SOFT_HIERARCHY_BEAM) {
        for(size_t i = 0; i < body->beam_count; i += 1)
            if(body->beams[i].id == item.id) {
                name = body->beams[i].name; shown = body->beams[i].visible;
                label = &editor->beam_names[i]; cache = editor->beam_cache[i];
            }
        selection = EDITOR_SELECTION_SOFT_BEAM;
        visibility = EDITOR_VISIBILITY_SOFT_BEAM;
        selected = context->viewport->selection == selection &&
            context->viewport->selected_soft_beam == item.id;
        snprintf(id, sizeof(id), "editor.soft_beam.%u", item.id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.soft_beam.%u.visibility", item.id);
    } else {
        for(size_t i = 0; i < body->area_count; i += 1)
            if(body->areas[i].id == item.id) {
                name = body->areas[i].name; shown = body->areas[i].visible;
                label = &editor->area_names[i]; cache = editor->area_cache[i];
            }
        selection = EDITOR_SELECTION_SOFT_AREA;
        visibility = EDITOR_VISIBILITY_SOFT_AREA;
        selected = context->viewport->selection == selection &&
            context->viewport->selected_soft_area == item.id;
        snprintf(id, sizeof(id), "editor.soft_area.%u", item.id);
        snprintf(visibility_id, sizeof(visibility_id),
            "editor.soft_area.%u.visibility", item.id);
    }
    if(name == NULL || label == NULL || cache == NULL) return true;
    if(!editor_mode_named_text_sync(editor->font, name, label, cache,
            EDITOR_OBJECT_NAME_MAX)) return false;
    if(editor_mode_visibility_button(visibility_id, shown, false,
            (UIRect){context->x + 10.0f, y, 24.0f, 24.0f}).clicked) {
        EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
            .data.visibility = {visibility, object->id, body->id, item.id, !shown}};
        (void)editor_command_execute(context->project, &command);
    }
    {
        UIButtonStyle style = selected_style_get();
        UIRect bounds = {context->x + 40.0f, y, context->width - 48.0f, 24.0f};
        UIButtonResult result = rohr_ui_button(id, label, bounds,
            selected || editor_viewport_selection_contains(context->viewport,
                (EditorSelectionRef){selection, object->id, body->id, 0, item.id}) ?
                &style : NULL);
        editor_mode_element_icon_draw(selection,
            (UIRect){bounds.x + 2.0f, bounds.y + 2.0f, 20.0f, 20.0f});
        if(context->hierarchy_row != NULL)
            context->hierarchy_row(context->hierarchy_context, context->viewport,
                (EditorSelectionRef){selection, object->id, body->id, 0, item.id},
                bounds, result, hierarchy_index + 1 == body->hierarchy_count);
        if(result.clicked || result.focus_changed) {
            context->viewport->selection = selection;
            if(selection == EDITOR_SELECTION_SOFT_NODE)
                context->viewport->selected_soft_node = item.id;
            else if(selection == EDITOR_SELECTION_SOFT_BEAM)
                context->viewport->selected_soft_beam = item.id;
            else {
                context->viewport->selected_soft_area = item.id;
                context->viewport->soft_area_candidates[0] = item.id;
                context->viewport->soft_area_candidate_count = 1;
            }
            if(result.double_clicked)
                (void)editor_navigation_selected_open(context->project,
                    context->viewport);
        }
    }
    return true;
}

bool editor_soft_body_editor_draw(EditorSoftBodyEditor *editor,
        EditorAutoShapeEditor *auto_shape, const EditorModeContext *context) {
    EditorObject *object;
    EditorSoftBody *body;
    size_t body_index;
    char name[EDITOR_OBJECT_NAME_MAX];
    Position position;
    float rotation;
    UIFieldResult name_result, x_result = {0}, y_result = {0},
        rotation_result = {0};
    bool field_active = false, transform_open, initial_motion_open, appearance_open,
        topology_open;
    float section_y = 118.0f;
    float transform_row_y[4] = {0};
    float initial_motion_row_y[5] = {0};
    float appearance_row_y[3] = {0};
    float topology_origin_y = 0.0f, topology_auto_shape_y = 0.0f;
    float topology_add_node_y = 0.0f, topology_add_beam_y = 0.0f;
    float topology_list_y = 0.0f, topology_picker_y = 0.0f;
    if(editor == NULL || auto_shape == NULL || context == NULL ||
            context->project == NULL || context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    body = body_get(object, context->viewport->selected_soft_body);
    if(body == NULL) return false;
    body_index = (size_t)(body - object->soft_body_items);
    if(body_index >= EDITOR_SOFT_BODY_MAX) return false;
    snprintf(name, sizeof(name), "%s", body->name);
    if(!editor_mode_named_text_sync(editor->font, body->name,
            &editor->body_names[body_index], editor->body_cache[body_index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 40.0f, 42.0f, 48.0f, 30.0f});
    name_result = editor_mode_name_field("editor.soft_body.name", name,
        sizeof(name), &editor->body_names[body_index],
        (UIRect){context->x + 88.0f, 42.0f, context->width - 96.0f, 30.0f});
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_SOFT_BODY,
                .object = object->id, .item = body->id}};
        snprintf(command.data.item_rename.name,
            sizeof(command.data.item_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    {
        bool visible = body->visible;
        if(editor_mode_visibility_field("editor.soft_body.visibility",
                &editor->visibility_label, &editor->visible_label,
                &editor->hidden_label,
                (UIRect){context->x + 10.0f, 80.0f,
                    context->width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_SOFT_BODY, object->id,
                    0, body->id, visible}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    {
        float layer_height = body->graphics_layer.layer == 0 ||
            (context->layer_control != NULL &&
                (context->layer_control->adding ||
                    context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
        const EditorModeAccordionLayoutGroup transform_groups[] = {
            {.row_count = 3, .row_height = 26.0f, .row_gap = 10.0f},
            {.row_count = 1, .row_height = layer_height,
                .gap_before = 10.0f}};
        const float appearance_rows[] = {26.0f, 26.0f, 26.0f};
        const float initial_motion_rows[] = {
            26.0f, 26.0f, 26.0f, 26.0f, 26.0f};
        EditorModeAccordionLayoutGroup topology_groups[3] = {
            {.row_count = 1, .row_height = 28.0f},
            {.row_count = 1, .row_height = 30.0f, .gap_before = 6.0f}};
        size_t topology_group_count = 2;
        if(editor->auto_shape_picker_open) {
            topology_groups[topology_group_count++] =
                (EditorModeAccordionLayoutGroup){.row_count = 1,
                    .row_height = 62.0f, .gap_before = 6.0f};
        } else {
            topology_groups[1].row_count = 3;
            topology_groups[1].row_gap = 6.0f;
            topology_groups[topology_group_count++] =
                (EditorModeAccordionLayoutGroup){
                    .row_count = body->hierarchy_count,
                    .row_height = 28.0f, .gap_before = 6.0f};
        }
        EditorModeAccordionLayoutCursor accordion =
            editor_mode_accordion_layout_cursor_get(
                context->x, context->width, section_y);
        EditorModeAccordionLayoutResult transform =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->transform_section,
                "editor.soft_body.section.transform", transform_groups, 2);
        EditorModeAccordionLayoutResult appearance =
            editor_mode_accordion_layout_section(&accordion,
                &editor->initial_motion_section,
                "editor.soft_body.section.initial_motion",
                initial_motion_rows, 5, 6.0f);
        initial_motion_open = appearance.expanded;
        for(size_t row = 0; row < 5; row += 1)
            initial_motion_row_y[row] = editor_mode_accordion_layout_row_y(
                &appearance, initial_motion_rows, row, 6.0f);
        appearance =
            editor_mode_accordion_layout_section(&accordion,
                &editor->appearance_section,
                "editor.soft_body.section.appearance", appearance_rows, 3,
                6.0f);
        EditorModeAccordionLayoutResult topology =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->topology_section,
                "editor.soft_body.section.topology", topology_groups,
                topology_group_count);
        transform_open = transform.expanded;
        for(size_t row = 0; row < 3; row += 1)
            transform_row_y[row] =
                editor_mode_accordion_layout_group_row_y(&transform,
                    transform_groups, 2, 0, row);
        transform_row_y[3] = editor_mode_accordion_layout_group_row_y(
            &transform, transform_groups, 2, 1, 0);
        appearance_open = appearance.expanded;
        for(size_t row = 0; row < 3; row += 1)
            appearance_row_y[row] = editor_mode_accordion_layout_row_y(
                &appearance, appearance_rows, row, 6.0f);
        topology_open = topology.expanded;
        topology_origin_y = editor_mode_accordion_layout_group_row_y(
            &topology, topology_groups, topology_group_count, 0, 0);
        topology_auto_shape_y = editor_mode_accordion_layout_group_row_y(
            &topology, topology_groups, topology_group_count, 1, 0);
        if(editor->auto_shape_picker_open) {
            topology_picker_y = editor_mode_accordion_layout_group_row_y(
                &topology, topology_groups, topology_group_count, 2, 0);
        } else {
            topology_add_node_y = editor_mode_accordion_layout_group_row_y(
                &topology, topology_groups, topology_group_count, 1, 1);
            topology_add_beam_y = editor_mode_accordion_layout_group_row_y(
                &topology, topology_groups, topology_group_count, 1, 2);
            if(body->hierarchy_count > 0)
                topology_list_y = editor_mode_accordion_layout_group_row_y(
                    &topology, topology_groups, topology_group_count, 2, 0);
        }
    }
    position = body->position; rotation = body->rotation;
    bool layer_active = false;
    if(transform_open) {
        UIButtonStyle style = editor_mode_section_field_style_get();
        rohr_ui_label(&editor->x_label,
            (UIRect){context->x + 8.0f, transform_row_y[0], 50.0f, 26.0f});
        x_result = editor_mode_field("editor.soft_body.x",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.x},
            &editor->x_field, (UIRect){context->x + 60.0f, transform_row_y[0],
                context->width - 70.0f, 26.0f}, &style);
        rohr_ui_label(&editor->y_label,
            (UIRect){context->x + 8.0f, transform_row_y[1], 50.0f, 26.0f});
        y_result = editor_mode_field("editor.soft_body.y",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.y},
            &editor->y_field, (UIRect){context->x + 60.0f,
                transform_row_y[1], context->width - 70.0f, 26.0f}, &style);
        rohr_ui_label(&editor->rotation_label,
            (UIRect){context->x + 8.0f, transform_row_y[2], 82.0f, 26.0f});
        rotation_result = editor_mode_field("editor.soft_body.rotation",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &rotation},
            &editor->rotation_field, (UIRect){context->x + 92.0f,
                transform_row_y[2], context->width - 102.0f, 26.0f}, &style);
        if(context->layer_control != NULL)
            layer_active = editor_mode_layer_control_draw(context->layer_control,
                "editor.soft_body", context->project, &body->graphics_layer, NULL,
                context->x, transform_row_y[3], context->width);
    }
    if(x_result.changed || y_result.changed || rotation_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_SOFT_BODY_TRANSFORM,
            .data.soft_body_transform = {object->id, body->id, position, rotation}};
        (void)editor_command_execute(context->project, &command);
    }
    if(initial_motion_open) {
        TextAsset *labels[] = {&editor->velocity_x_label,
            &editor->velocity_y_label, &editor->acceleration_x_label,
            &editor->acceleration_y_label, &editor->angular_velocity_label};
        TextAsset *fields[] = {&editor->velocity_x_field,
            &editor->velocity_y_field, &editor->acceleration_x_field,
            &editor->acceleration_y_field, &editor->angular_velocity_field};
        const char *ids[] = {"editor.soft_body.initial_velocity_x",
            "editor.soft_body.initial_velocity_y",
            "editor.soft_body.initial_acceleration_x",
            "editor.soft_body.initial_acceleration_y",
            "editor.soft_body.initial_angular_velocity"};
        EditorPropertyKind properties[] = {EDITOR_PROPERTY_INITIAL_VELOCITY_X,
            EDITOR_PROPERTY_INITIAL_VELOCITY_Y,
            EDITOR_PROPERTY_INITIAL_ACCELERATION_X,
            EDITOR_PROPERTY_INITIAL_ACCELERATION_Y,
            EDITOR_PROPERTY_INITIAL_ANGULAR_VELOCITY};
        float values[] = {body->initial_velocity.x, body->initial_velocity.y,
            body->initial_acceleration.x, body->initial_acceleration.y,
            body->initial_angular_velocity};
        UIButtonStyle style = editor_mode_section_field_style_get();
        for(size_t i = 0; i < 5; i += 1) {
            UIFieldResult result;
            rohr_ui_label(labels[i], (UIRect){context->x + 8.0f,
                initial_motion_row_y[i], 112.0f, 26.0f});
            result = editor_mode_field(ids[i],
                (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &values[i]},
                fields[i], (UIRect){context->x + 122.0f,
                    initial_motion_row_y[i], context->width - 132.0f, 26.0f},
                &style);
            if(result.changed) initial_motion_set(context->project, object->id,
                body->id, properties[i], values[i]);
            field_active = field_active || result.active;
        }
    }
    if(appearance_open) {
    (void)editor_mode_color_swatch("editor.soft_body.node_color",
        &body->node_color, false, (UIRect){context->x + 100.0f,
            appearance_row_y[0],
            context->width - 110.0f, 26.0f}, context, EDITOR_ITEM_SOFT_BODY,
        object->id, 0, body->id, EDITOR_PROPERTY_NODE_COLOR);
    rohr_ui_label(&editor->node_color_label,
        (UIRect){context->x + 8.0f, appearance_row_y[0], 90.0f, 26.0f});
    rohr_ui_label(&editor->beam_color_label,
        (UIRect){context->x + 8.0f, appearance_row_y[1], 90.0f, 26.0f});
    (void)editor_mode_color_swatch("editor.soft_body.beam_color",
        &body->beam_color, false, (UIRect){context->x + 100.0f,
            appearance_row_y[1],
            context->width - 110.0f, 26.0f}, context, EDITOR_ITEM_SOFT_BODY,
        object->id, 0, body->id, EDITOR_PROPERTY_BEAM_COLOR);
    rohr_ui_label(&editor->area_color_label,
        (UIRect){context->x + 8.0f, appearance_row_y[2], 90.0f, 26.0f});
    (void)editor_mode_color_swatch("editor.soft_body.area_color",
        &body->area_color, false, (UIRect){context->x + 100.0f,
            appearance_row_y[2],
            context->width - 110.0f, 26.0f}, context, EDITOR_ITEM_SOFT_BODY,
        object->id, 0, body->id, EDITOR_PROPERTY_AREA_COLOR);
    }
    if(topology_open) {
    {
        UIButtonStyle style = selected_style_get();
        UIButtonResult result = rohr_ui_button("editor.soft_body.origin",
            &editor->origin_label, (UIRect){context->x + 10.0f, topology_origin_y,
                context->width - 20.0f, 28.0f},
            context->viewport->selection == EDITOR_SELECTION_ORIGIN &&
                context->viewport->selected_origin_kind == EDITOR_ORIGIN_SOFT_BODY ?
                &style : NULL);
        if(result.clicked || result.focus_changed) {
            context->viewport->selection = EDITOR_SELECTION_ORIGIN;
            context->viewport->selected_origin_kind = EDITOR_ORIGIN_SOFT_BODY;
            if(result.double_clicked) context->viewport->mode = EDITOR_VIEWPORT_ORIGIN;
        }
    }
    if(rohr_ui_button("editor.soft_body.auto_shape", &editor->auto_shape_label,
            (UIRect){context->x + 10.0f, topology_auto_shape_y,
                context->width - 20.0f, 30.0f}, NULL).clicked)
        editor->auto_shape_picker_open = !editor->auto_shape_picker_open;
    if(!editor->auto_shape_picker_open) {
        if(rohr_ui_button("editor.soft_body.add_node", &editor->add_node_label,
                (UIRect){context->x + 10.0f, topology_add_node_y,
                    context->width - 20.0f, 30.0f}, NULL).clicked) {
            EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                .data.item_add = {.kind = EDITOR_ITEM_SOFT_NODE,
                    .object = object->id, .parent = body->id,
                    .position = {(float)body->node_count * 24.0f, 0.0f}}};
            EditorCommandResult result = editor_command_execute(context->project, &command);
            if(result.kind == ERROR_RESULT_VALUE) {
                context->viewport->selection = EDITOR_SELECTION_SOFT_NODE;
                context->viewport->selected_soft_node = result.result.object;
                if(editor_navigation_selected_open(context->project,
                        context->viewport))
                    (void)editor_mode_name_focus_request(context->viewport);
            }
        }
        if(rohr_ui_button("editor.soft_body.add_beam", &editor->add_beam_label,
                (UIRect){context->x + 10.0f, topology_add_beam_y,
                    context->width - 20.0f, 30.0f}, NULL).clicked) {
            EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                .data.item_add = {.kind = EDITOR_ITEM_SOFT_BEAM,
                    .object = object->id, .parent = body->id}};
            EditorCommandResult result = editor_command_execute(context->project, &command);
            if(result.kind == ERROR_RESULT_VALUE) {
                context->viewport->selection = EDITOR_SELECTION_SOFT_BEAM;
                context->viewport->selected_soft_beam = result.result.object;
                if(editor_navigation_selected_open(context->project,
                        context->viewport))
                    (void)editor_mode_name_focus_request(context->viewport);
            }
        }
        editor_project_soft_body_hierarchy_sync(body);
        for(size_t i = 0; i < body->hierarchy_count; i += 1)
            if(!hierarchy_item_draw(editor, context, object, body,
                    body->hierarchy[i], i, topology_list_y)) return false;
    } else {
        size_t count = editor_auto_shape_soft_body_points_capture(
            context->viewport, object, body);
        int shape = editor_auto_shape_picker_draw(auto_shape,
            "editor.soft_body.auto_shape.option",
            (UIRect){context->x + 10.0f, topology_picker_y,
                context->width - 20.0f, 62.0f}, count > 0 ? count : body->node_count);
        if(shape >= 0) {
            auto_shape->config.kind = (EditorAutoShapeKind)shape;
            context->viewport->auto_shape_parent_mode = EDITOR_VIEWPORT_SOFT_BODY;
            (void)editor_auto_shape_editor_apply(auto_shape, context->project,
                context->viewport, EDITOR_VIEWPORT_SOFT_BODY);
            context->viewport->mode = EDITOR_VIEWPORT_AUTO_SHAPE;
            context->viewport->selection = EDITOR_SELECTION_SOFT_BODY;
            auto_shape->first_was_active = false;
            auto_shape->second_was_active = false;
            auto_shape->third_was_active = false;
            editor->auto_shape_picker_open = false;
        }
    }
    }
    if(context->delete_open_item != NULL && !context->delete_footer) {
        UIButtonStyle delete_style = delete_style_get();
        if(rohr_ui_button(
            "editor.soft_body.delete", &editor->delete_label,
            (UIRect){context->x + 10.0f,
                context->delete_y_get != NULL ?
                    context->delete_y_get(context->delete_context) : 650.0f,
                context->width - 20.0f, 34.0f}, &delete_style).clicked)
            (void)context->delete_open_item(context->delete_context);
    }
    field_active = field_active || name_result.active || x_result.active || y_result.active ||
        rotation_result.active || layer_active;
    return field_active;
}
