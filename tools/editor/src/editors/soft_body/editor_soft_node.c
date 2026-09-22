/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_soft_node.h"

#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>

static EditorSoftBody *body_get(EditorObject *object, EditorSoftBodyId id) {
    if(object == NULL) return NULL;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}

static EditorSoftNode *node_get(EditorSoftBody *body, EditorSoftNodeId id) {
    if(body == NULL) return NULL;
    for(size_t i = 0; i < body->node_count; i += 1)
        if(body->nodes[i].id == id) return &body->nodes[i];
    return NULL;
}

static void float_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorSoftNodeId node,
        EditorPropertyKind property, float value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_SOFT_NODE, object, body, node, 0,
            property, EDITOR_PROPERTY_VALUE_FLOAT, {.number = value}}};
    (void)editor_command_execute(project, &command);
}

static void bool_set(EditorProject *project, EditorObjectId object,
        EditorSoftBodyId body, EditorSoftNodeId node,
        EditorPropertyKind property, bool value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_SOFT_NODE, object, body, node, 0,
            property, EDITOR_PROPERTY_VALUE_BOOL, {.boolean = value}}};
    (void)editor_command_execute(project, &command);
}

static bool checkbox(const char *id, const TextAsset *label,
        UIRect bounds, bool *checked) {
    UIButtonResult result = rohr_ui_interaction(id, bounds);
    UIRect box = {bounds.x + 4.0f, bounds.y + 4.0f,
        bounds.height - 8.0f, bounds.height - 8.0f};
    Color background = result.pressed ? (Color){58, 65, 78, 255} :
        result.hovered || result.focused ? (Color){67, 75, 90, 255} :
        (Color){48, 54, 66, 255};
    if(result.clicked) *checked = !*checked;
    rohr_ui_surface(bounds, background);
    rohr_ui_surface(box, (Color){22, 25, 31, 255});
    rohr_ui_border(box, 2.0f, (Color){8, 9, 12, 255});
    if(*checked) rohr_ui_surface((UIRect){box.x + 5.0f, box.y + 5.0f,
        box.width - 10.0f, box.height - 10.0f}, (Color){225, 230, 240, 255});
    rohr_ui_label(label, (UIRect){box.x + box.width + 8.0f, bounds.y,
        bounds.width - box.width - 12.0f, bounds.height});
    return result.clicked;
}

bool editor_soft_node_editor_create(EditorSoftNodeEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorSoftNodeEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Mass", mass_label); CREATE("Radius", radius_label);
    CREATE("Friction", friction_label); CREATE("Restitution", restitution_label);
    CREATE("Gravity", gravity_label); CREATE("Collision", collision_label);
    CREATE("Collision Category", collision_category_label);
    CREATE("Collide With", collide_with_label); CREATE("Node Color", color_label);
    CREATE("Inherit", inherit_label); CREATE("Visibility", visibility_label);
    CREATE("Inherit From Soft Body", motion_inherit_label);
    CREATE("Velocity X", velocity_x_label); CREATE("Velocity Y", velocity_y_label);
    CREATE("Acceleration X", acceleration_x_label);
    CREATE("Acceleration Y", acceleration_y_label);
    CREATE("[X]", visible_label);
    CREATE("[ ]", hidden_label); CREATE("Delete Node", delete_label);
    CREATE("", x_field); CREATE("", y_field); CREATE("", mass_field);
    CREATE("", radius_field); CREATE("", friction_field);
    CREATE("", restitution_field);
    CREATE("", velocity_x_field); CREATE("", velocity_y_field);
    CREATE("", acceleration_x_field); CREATE("", acceleration_y_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true) ||
            !editor_mode_accordion_section_create(
                &editor->initial_motion_section, font, "Initial Motion", false) ||
            !editor_mode_accordion_section_create(&editor->physics_section, font,
                "Physics", false) ||
            !editor_mode_accordion_section_create(&editor->material_section, font,
                "Material", false) ||
            !editor_mode_accordion_section_create(&editor->collision_section, font,
                "Collision", false) ||
            !editor_mode_accordion_section_create(&editor->appearance_section, font,
                "Appearance", false)) goto fail;
    for(size_t i = 0; i < EDITOR_SOFT_NODE_MAX; i += 1) {
        char name[32]; snprintf(name, sizeof(name), "node_%zu", i + 1);
        if(!editor_mode_text_create(font, name, &editor->node_names[i])) goto fail;
    }
    return true;
fail:
    editor_soft_node_editor_destroy(editor);
    return false;
}

void editor_soft_node_editor_destroy(EditorSoftNodeEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(x_label); DESTROY(y_label); DESTROY(mass_label);
    DESTROY(radius_label); DESTROY(friction_label); DESTROY(restitution_label);
    DESTROY(gravity_label); DESTROY(collision_label);
    DESTROY(collision_category_label); DESTROY(collide_with_label);
    DESTROY(color_label); DESTROY(inherit_label); DESTROY(visibility_label);
    DESTROY(motion_inherit_label); DESTROY(velocity_x_label);
    DESTROY(velocity_y_label); DESTROY(acceleration_x_label);
    DESTROY(acceleration_y_label);
    DESTROY(visible_label);
    DESTROY(hidden_label); DESTROY(delete_label); DESTROY(x_field);
    DESTROY(y_field); DESTROY(mass_field); DESTROY(radius_field);
    DESTROY(friction_field); DESTROY(restitution_field);
    DESTROY(velocity_x_field); DESTROY(velocity_y_field);
    DESTROY(acceleration_x_field); DESTROY(acceleration_y_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->initial_motion_section);
    editor_mode_accordion_section_destroy(&editor->physics_section);
    editor_mode_accordion_section_destroy(&editor->material_section);
    editor_mode_accordion_section_destroy(&editor->collision_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    for(size_t i = 0; i < EDITOR_SOFT_NODE_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->node_names[i]);
    *editor = (EditorSoftNodeEditor){0};
}

bool editor_soft_node_editor_draw(EditorSoftNodeEditor *editor,
        const EditorModeContext *context,
        EditorSoftNodeCollisionMenuFunction collision_menu,
        void *collision_context) {
    EditorObject *object;
    EditorSoftBody *body;
    EditorSoftNode *node;
    size_t node_index;
    char name[EDITOR_OBJECT_NAME_MAX];
    Position position;
    float mass_value, radius_value, friction_value, restitution_value;
    UIFieldResult name_result, x_result = {0}, y_result = {0}, mass_result = {0};
    UIFieldResult radius_result = {0}, friction_result = {0},
        restitution_result = {0};
    bool field_active = false, layer_active = false;
    bool transform_open, initial_motion_open, physics_open, material_open, collision_open,
        appearance_open;
    float collision_y, appearance_y;
    float transform_row_y[3] = {0}, physics_row_y[3] = {0};
    float initial_motion_row_y[5] = {0};
    float material_row_y[2] = {0};
    float collision_category_y = 0.0f, collision_category_list_y = 0.0f;
    float collide_with_y = 0.0f, collide_with_list_y = 0.0f;
    float collision_bottom = 0.0f;
    float section_y = 118.0f;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    body = body_get(object, context->viewport->selected_soft_body);
    node = node_get(body, context->viewport->selected_soft_node);
    if(node == NULL) return false;
    node_index = (size_t)(node - body->nodes);
    if(node_index >= EDITOR_SOFT_NODE_MAX) return false;
    snprintf(name, sizeof(name), "%s", node->name);
    if(!editor_mode_named_text_sync(editor->font, node->name,
            &editor->node_names[node_index], editor->node_cache[node_index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 40.0f, 40.0f, 48.0f, 30.0f});
    name_result = editor_mode_name_field("editor.soft_node.name", name,
        sizeof(name), &editor->node_names[node_index],
        (UIRect){context->x + 88.0f, 40.0f,
            context->width - 96.0f, 30.0f});
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_SOFT_NODE,
                .object = object->id, .parent = body->id, .item = node->id}};
        snprintf(command.data.item_rename.name,
            sizeof(command.data.item_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    {
        bool visible = node->visible;
        if(editor_mode_checkbox_left("editor.soft_node.visibility",
                &editor->visibility_label,
                (UIRect){context->x + 10.0f, 80.0f,
                    context->width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_SOFT_NODE, object->id,
                    body->id, node->id, visible}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    {
        float layer_height = node->graphics_layer.layer == 0 ||
            (context->layer_control != NULL &&
                (context->layer_control->adding ||
                    context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
        const EditorModeAccordionLayoutGroup transform_groups[] = {
            {.row_count = 2, .row_height = 26.0f, .row_gap = 6.0f},
            {.row_count = 1, .row_height = layer_height,
                .gap_before = 6.0f}};
        const float physics_rows[] = {26.0f, 26.0f, 28.0f};
        const float initial_motion_rows[] = {
            28.0f, 26.0f, 26.0f, 26.0f, 26.0f};
        size_t initial_motion_row_count =
            node->initial_motion_inherited ? 1 : 5;
        const float material_rows[] = {26.0f, 26.0f};
        const float appearance_rows[] = {26.0f};
        EditorModeAccordionLayoutGroup collision_groups[5] = {
            {.row_count = 1, .row_height = 28.0f}};
        size_t collision_group_count = 1;
        size_t category_group = SIZE_MAX, category_list_group = SIZE_MAX;
        size_t collide_group = SIZE_MAX, collide_list_group = SIZE_MAX;
        if(node->collision_enabled) {
            category_group = collision_group_count;
            collision_groups[collision_group_count++] =
                (EditorModeAccordionLayoutGroup){.row_count = 1,
                    .row_height = 28.0f, .gap_before = 4.0f};
            if(editor->collision_category_open) {
                category_list_group = collision_group_count;
                collision_groups[collision_group_count++] =
                    (EditorModeAccordionLayoutGroup){
                        .row_count = context->project->collision_mask_count + 1,
                        .row_height = 26.0f, .row_gap = 4.0f,
                        .gap_before = 4.0f};
            }
            collide_group = collision_group_count;
            collision_groups[collision_group_count++] =
                (EditorModeAccordionLayoutGroup){.row_count = 1,
                    .row_height = 28.0f, .gap_before = 4.0f};
            if(editor->collide_with_open) {
                collide_list_group = collision_group_count;
                collision_groups[collision_group_count++] =
                    (EditorModeAccordionLayoutGroup){
                        .row_count = context->project->collision_mask_count + 1,
                        .row_height = 26.0f, .row_gap = 4.0f,
                        .gap_before = 4.0f};
            }
        }
        EditorModeAccordionLayoutCursor accordion =
            editor_mode_accordion_layout_cursor_get(
                context->x, context->width, section_y);
        EditorModeAccordionLayoutResult transform =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->transform_section,
                "editor.soft_node.section.transform", transform_groups, 2);
        EditorModeAccordionLayoutResult physics =
            editor_mode_accordion_layout_section(&accordion,
                &editor->initial_motion_section,
                "editor.soft_node.section.initial_motion",
                initial_motion_rows, initial_motion_row_count, 6.0f);
        initial_motion_open = physics.expanded;
        for(size_t row = 0; row < initial_motion_row_count; row += 1)
            initial_motion_row_y[row] = editor_mode_accordion_layout_row_y(
                &physics, initial_motion_rows, row, 6.0f);
        physics =
            editor_mode_accordion_layout_section(&accordion,
                &editor->physics_section, "editor.soft_node.section.physics",
                physics_rows, 3, 6.0f);
        EditorModeAccordionLayoutResult material =
            editor_mode_accordion_layout_section(&accordion,
                &editor->material_section, "editor.soft_node.section.material",
                material_rows, 2, 6.0f);
        EditorModeAccordionLayoutResult collision =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->collision_section,
                "editor.soft_node.section.collision", collision_groups,
                collision_group_count);
        EditorModeAccordionLayoutResult appearance =
            editor_mode_accordion_layout_section(&accordion,
                &editor->appearance_section,
                "editor.soft_node.section.appearance", appearance_rows, 1,
                0.0f);
        transform_open = transform.expanded;
        physics_open = physics.expanded;
        material_open = material.expanded;
        collision_open = collision.expanded;
        collision_y = collision.content_y;
        if(category_group != SIZE_MAX)
            collision_category_y = editor_mode_accordion_layout_group_row_y(
                &collision, collision_groups, collision_group_count,
                category_group, 0);
        if(category_list_group != SIZE_MAX)
            collision_category_list_y =
                editor_mode_accordion_layout_group_row_y(&collision,
                    collision_groups, collision_group_count,
                    category_list_group, 0);
        if(collide_group != SIZE_MAX)
            collide_with_y = editor_mode_accordion_layout_group_row_y(
                &collision, collision_groups, collision_group_count,
                collide_group, 0);
        if(collide_list_group != SIZE_MAX)
            collide_with_list_y = editor_mode_accordion_layout_group_row_y(
                &collision, collision_groups, collision_group_count,
                collide_list_group, 0);
        collision_bottom = collision.content_y +
            editor_mode_accordion_layout_groups_height_get(
                collision_groups, collision_group_count);
        appearance_open = appearance.expanded;
        appearance_y = appearance.content_y;
        transform_row_y[0] = editor_mode_accordion_layout_group_row_y(
            &transform, transform_groups, 2, 0, 0);
        transform_row_y[1] = editor_mode_accordion_layout_group_row_y(
            &transform, transform_groups, 2, 0, 1);
        transform_row_y[2] = editor_mode_accordion_layout_group_row_y(
            &transform, transform_groups, 2, 1, 0);
        for(size_t row = 0; row < 3; row += 1)
            physics_row_y[row] = editor_mode_accordion_layout_row_y(
                &physics, physics_rows, row, 6.0f);
        for(size_t row = 0; row < 2; row += 1)
            material_row_y[row] = editor_mode_accordion_layout_row_y(
                &material, material_rows, row, 6.0f);
    }
    position = node->position;
    if(transform_open) {
    UIButtonStyle field_style = editor_mode_section_field_style_get();
    rohr_ui_label(&editor->x_label,
        (UIRect){context->x + 8.0f, transform_row_y[0], 50.0f, 26.0f});
    x_result = editor_mode_field("editor.soft_node.x",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.x},
        &editor->x_field, (UIRect){context->x + 60.0f, transform_row_y[0],
            context->width - 70.0f, 26.0f}, &field_style);
    rohr_ui_label(&editor->y_label,
        (UIRect){context->x + 8.0f, transform_row_y[1], 50.0f, 26.0f});
    y_result = editor_mode_field("editor.soft_node.y",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.y},
        &editor->y_field, (UIRect){context->x + 60.0f, transform_row_y[1],
            context->width - 70.0f, 26.0f}, &field_style);
    if(x_result.changed || y_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_SOFT_NODE_POSITION,
            .data.soft_node_position = {object->id, body->id, node->id, position}};
        (void)editor_command_execute(context->project, &command);
    }
    if(context->layer_control != NULL)
        layer_active = editor_mode_layer_control_draw(context->layer_control,
            "editor.soft_node", context->project, &node->graphics_layer,
            &node->graphics_layer_inherited, context->x, transform_row_y[2],
            context->width);
    }
    if(initial_motion_open) {
        bool inherited = node->initial_motion_inherited;
        if(editor_mode_checkbox_left("editor.soft_node.initial_motion.inherit",
                &editor->motion_inherit_label,
                (UIRect){context->x + 10.0f, initial_motion_row_y[0],
                    context->width - 20.0f, 28.0f}, &inherited))
            bool_set(context->project, object->id, body->id, node->id,
                EDITOR_PROPERTY_INITIAL_MOTION_INHERITED, inherited);
        if(!node->initial_motion_inherited) {
            TextAsset *labels[] = {&editor->velocity_x_label,
                &editor->velocity_y_label, &editor->acceleration_x_label,
                &editor->acceleration_y_label};
            TextAsset *fields[] = {&editor->velocity_x_field,
                &editor->velocity_y_field, &editor->acceleration_x_field,
                &editor->acceleration_y_field};
            const char *ids[] = {"editor.soft_node.initial_velocity_x",
                "editor.soft_node.initial_velocity_y",
                "editor.soft_node.initial_acceleration_x",
                "editor.soft_node.initial_acceleration_y"};
            EditorPropertyKind properties[] = {
                EDITOR_PROPERTY_INITIAL_VELOCITY_X,
                EDITOR_PROPERTY_INITIAL_VELOCITY_Y,
                EDITOR_PROPERTY_INITIAL_ACCELERATION_X,
                EDITOR_PROPERTY_INITIAL_ACCELERATION_Y};
            float values[] = {node->initial_velocity.x,
                node->initial_velocity.y, node->initial_acceleration.x,
                node->initial_acceleration.y};
            UIButtonStyle style = editor_mode_section_field_style_get();
            for(size_t i = 0; i < 4; i += 1) {
                UIFieldResult result;
                rohr_ui_label(labels[i], (UIRect){context->x + 8.0f,
                    initial_motion_row_y[i + 1], 112.0f, 26.0f});
                result = editor_mode_field(ids[i], (UIFieldBinding){
                    .kind = UI_FIELD_FLOAT, .number = &values[i]}, fields[i],
                    (UIRect){context->x + 122.0f,
                        initial_motion_row_y[i + 1],
                        context->width - 132.0f, 26.0f}, &style);
                if(result.changed) float_set(context->project, object->id,
                    body->id, node->id, properties[i], values[i]);
                field_active = field_active || result.active;
            }
        }
    }
    mass_value = node->node_mass;
    if(physics_open) {
    UIButtonStyle field_style = editor_mode_section_field_style_get();
    rohr_ui_label(&editor->mass_label,
        (UIRect){context->x + 8.0f, physics_row_y[0], 68.0f, 26.0f});
    mass_result = editor_mode_field("editor.soft_node.mass",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &mass_value},
        &editor->mass_field, (UIRect){context->x + 78.0f, physics_row_y[0],
            context->width - 88.0f, 26.0f}, &field_style);
    if(mass_result.changed) float_set(context->project, object->id, body->id,
        node->id, EDITOR_PROPERTY_MASS, fmaxf(0.0f, mass_value));
    radius_value = node->radius;
    rohr_ui_label(&editor->radius_label,
        (UIRect){context->x + 8.0f, physics_row_y[1], 68.0f, 26.0f});
    radius_result = editor_mode_field("editor.soft_node.radius",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &radius_value},
        &editor->radius_field, (UIRect){context->x + 78.0f,
            physics_row_y[1], context->width - 88.0f, 26.0f}, &field_style);
    if(radius_result.changed) float_set(context->project, object->id,
        body->id, node->id, EDITOR_PROPERTY_NODE_RADIUS,
        radius_value <= 0.0f ? 0.1f : radius_value);
    {
        bool gravity = node->gravity_enabled;
        if(checkbox("editor.soft_node.gravity", &editor->gravity_label,
                (UIRect){context->x + 10.0f, physics_row_y[2],
                    context->width - 20.0f, 28.0f}, &gravity))
            bool_set(context->project, object->id, body->id, node->id,
                EDITOR_PROPERTY_GRAVITY, gravity);
    }
    }
    friction_value = node->friction;
    if(material_open) {
    UIButtonStyle field_style = editor_mode_section_field_style_get();
    rohr_ui_label(&editor->friction_label,
        (UIRect){context->x + 8.0f, material_row_y[0], 68.0f, 26.0f});
    friction_result = editor_mode_field("editor.soft_node.friction",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &friction_value},
        &editor->friction_field, (UIRect){context->x + 78.0f, material_row_y[0],
            context->width - 88.0f, 26.0f}, &field_style);
    if(friction_result.changed) float_set(context->project, object->id, body->id,
        node->id, EDITOR_PROPERTY_FRICTION, fmaxf(0.0f, friction_value));
    restitution_value = node->restitution;
    rohr_ui_label(&editor->restitution_label,
        (UIRect){context->x + 8.0f, material_row_y[1], 96.0f, 26.0f});
    restitution_result = editor_mode_field("editor.soft_node.restitution",
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &restitution_value},
        &editor->restitution_field, (UIRect){context->x + 106.0f,
            material_row_y[1], context->width - 116.0f, 26.0f}, &field_style);
    if(restitution_result.changed) float_set(context->project, object->id, body->id,
        node->id, EDITOR_PROPERTY_RESTITUTION,
        fminf(1.0f, fmaxf(0.0f, restitution_value)));
    }
    field_active = field_active || name_result.active || x_result.active || y_result.active ||
        mass_result.active || radius_result.active || friction_result.active ||
        restitution_result.active || layer_active;
    {
        float row_x = context->x + 10.0f, row_width = context->width - 20.0f;
        bool collision = node->collision_enabled;
        if(collision_open && checkbox("editor.soft_node.collision",
                &editor->collision_label,
                (UIRect){row_x, collision_y, row_width, 28.0f}, &collision)) {
            bool_set(context->project, object->id, body->id, node->id,
                EDITOR_PROPERTY_COLLISION, collision);
            if(!collision) editor->collision_category_open =
                editor->collide_with_open = false;
        }
        if(collision_open && node->collision_enabled) {
            if(rohr_ui_button("editor.soft_node.collision_category",
                    &editor->collision_category_label,
                    (UIRect){row_x, collision_category_y,
                        row_width, 28.0f}, NULL).clicked) {
                editor->collision_category_open = !editor->collision_category_open;
                editor->collide_with_open = false;
            }
            rohr_ui_border((UIRect){row_x, collision_category_y,
                    row_width, 28.0f},
                2.0f, (Color){0, 0, 0, 255});
            if(editor->collision_category_open && collision_menu != NULL) {
                size_t rows = 0;
                if(!collision_menu(collision_context,
                        "editor.soft_node.collision_category.mask", context->project,
                        &node->collision_category, object->id, body->id, node->id,
                        EDITOR_COLLISION_FILTER_CATEGORY, row_x,
                        collision_category_list_y, row_width,
                        &field_active, &rows)) return field_active;
            }
            if(rohr_ui_button("editor.soft_node.collide_with",
                    &editor->collide_with_label,
                    (UIRect){row_x, collide_with_y,
                        row_width, 28.0f}, NULL).clicked) {
                editor->collide_with_open = !editor->collide_with_open;
                editor->collision_category_open = false;
            }
            rohr_ui_border((UIRect){row_x, collide_with_y,
                    row_width, 28.0f},
                2.0f, (Color){0, 0, 0, 255});
            if(editor->collide_with_open && collision_menu != NULL) {
                size_t rows = 0;
                if(!collision_menu(collision_context,
                        "editor.soft_node.collide_with.mask", context->project,
                        &node->collision_with, object->id, body->id, node->id,
                        EDITOR_COLLISION_FILTER_COLLIDE_WITH, row_x,
                        collide_with_list_y, row_width,
                        &field_active, &rows)) return field_active;
            }
            if((editor->collision_category_open || editor->collide_with_open) &&
                    context->primary_button == MOUSE_BUTTON_STATE_PRESSED) {
                Position pointer = rohr_graphics_mouse_screen_position_get();
                if(pointer.x < row_x || pointer.x > row_x + row_width ||
                        pointer.y < collision_y ||
                        pointer.y > collision_bottom)
                    editor->collision_category_open = editor->collide_with_open = false;
            }
        }
    }
    if(appearance_open) {
        bool inherit = !node->color_overridden;
        float field_width = fmaxf(34.0f, context->width - 196.0f);
        if(inherit) node->color = body->node_color;
        rohr_ui_label(&editor->color_label,
            (UIRect){context->x + 8.0f, appearance_y, 90.0f, 26.0f});
        if(editor_mode_checkbox_left("editor.soft_node.color_inherit",
                &editor->inherit_label,
                (UIRect){context->x + context->width - 92.0f,
                    appearance_y, 82.0f, 26.0f}, &inherit)) {
            node->color_overridden = !inherit;
            node->color = body->node_color;
        }
        (void)editor_mode_color_swatch("editor.soft_node.color", &node->color,
            inherit, (UIRect){context->x + 100.0f, appearance_y,
                field_width, 26.0f}, context, EDITOR_ITEM_SOFT_NODE,
            object->id, body->id, node->id, EDITOR_PROPERTY_COLOR);
    }
    if(context->delete_y_get != NULL && context->delete_open_item != NULL &&
            !context->delete_footer) {
        UIButtonStyle style = editor_mode_delete_style_get();
        if(rohr_ui_button("editor.soft_node.delete", &editor->delete_label,
                (UIRect){context->x + 10.0f,
                    context->delete_y_get(context->delete_context),
                    context->width - 20.0f, 34.0f}, &style).clicked)
            (void)context->delete_open_item(context->delete_context);
    }
    return field_active;
}
