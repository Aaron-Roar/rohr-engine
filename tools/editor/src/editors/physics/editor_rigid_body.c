/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_rigid_body.h"

#include "editor_navigation.h"
#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>

static void property_float_set(EditorProject *project, EditorObjectId object,
        EditorRigidBodyId body, EditorPropertyKind property, float value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_RIGID_BODY, object, 0, body, 0,
            property, EDITOR_PROPERTY_VALUE_FLOAT, {.number = value}}};
    (void)editor_command_execute(project, &command);
}

static void property_bool_set(EditorProject *project, EditorObjectId object,
        EditorRigidBodyId body, EditorPropertyKind property, bool value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_RIGID_BODY, object, 0, body, 0,
            property, EDITOR_PROPERTY_VALUE_BOOL, {.boolean = value}}};
    (void)editor_command_execute(project, &command);
}

static void property_uint_set(EditorProject *project, EditorObjectId object,
        EditorRigidBodyId body, EditorPropertyKind property, uint32_t value) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_RIGID_BODY, object, 0, body, 0,
            property, EDITOR_PROPERTY_VALUE_UINT, {.integer = value}}};
    (void)editor_command_execute(project, &command);
}

static bool checkbox(const char *id, const TextAsset *label, UIRect bounds,
        bool *checked, bool label_left, bool *hovered) {
    UIButtonResult result = rohr_ui_interaction(id, bounds);
    UIRect box = label_left ?
        (UIRect){bounds.x + bounds.width - bounds.height + 4.0f,
            bounds.y + 4.0f, bounds.height - 8.0f, bounds.height - 8.0f} :
        (UIRect){bounds.x + 4.0f, bounds.y + 4.0f,
            bounds.height - 8.0f, bounds.height - 8.0f};
    Color background = result.pressed ? (Color){58, 65, 78, 255} :
        result.hovered || result.focused ? (Color){67, 75, 90, 255} :
        (Color){48, 54, 66, 255};
    if(result.clicked) *checked = !*checked;
    if(hovered != NULL) *hovered = result.hovered;
    rohr_ui_surface(bounds, background);
    rohr_ui_surface(box, (Color){22, 25, 31, 255});
    rohr_ui_border(box, 2.0f, (Color){8, 9, 12, 255});
    if(*checked) rohr_ui_surface((UIRect){box.x + 5.0f, box.y + 5.0f,
        box.width - 10.0f, box.height - 10.0f}, (Color){225, 230, 240, 255});
    rohr_ui_label(label, label_left ?
        (UIRect){bounds.x, bounds.y, bounds.width - bounds.height, bounds.height} :
        (UIRect){box.x + box.width + 8.0f, bounds.y,
            bounds.width - box.width - 12.0f, bounds.height});
    return result.clicked;
}

static UIButtonStyle selected_style_get(void) {
    UIButtonStyle style = rohr_ui_button_style_default_get();
    style.idle = (Color){118, 96, 35, 255};
    style.hovered = (Color){145, 119, 45, 255};
    return style;
}

bool editor_rigid_body_editor_create(EditorRigidBodyEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorRigidBodyEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("X", x_label); CREATE("Y", y_label);
    CREATE("Rotation", rotation_label); CREATE("Mass", mass_label);
    CREATE("Velocity X", velocity_x_label); CREATE("Velocity Y", velocity_y_label);
    CREATE("Acceleration X", acceleration_x_label);
    CREATE("Acceleration Y", acceleration_y_label);
    CREATE("Angular Velocity", angular_velocity_label);
    CREATE("Friction", friction_label); CREATE("Restitution", restitution_label);
    CREATE("Border Color", border_color_label); CREATE("Surface Color", surface_color_label);
    CREATE("Parent", parent_label); CREATE("None", none_label);
    CREATE("Gravity", gravity_label); CREATE("Dynamic", dynamic_label);
    CREATE("Static", static_label); CREATE("Rotation Unlocked", rotation_unlocked_label);
    CREATE("Rotation Locked", rotation_locked_label); CREATE("Collision", collision_label);
    CREATE("Particle", particle_label); CREATE("Collision Category", collision_category_label);
    CREATE("Collide With", collide_with_label); CREATE("Origin", origin_label);
    CREATE("Initial Active Hitbox", active_hitbox_label);
    CREATE("Add Hitbox Variant", add_hitbox_label);
    CREATE("Bind Frames", bind_frames_label);
    CREATE("Delete Rigid Body", delete_label);
    CREATE("Visibility", visibility_label); CREATE("[X]", visible_label);
    CREATE("[ ]", hidden_label);
    CREATE("", x_field); CREATE("", y_field); CREATE("", rotation_field);
    CREATE("", velocity_x_field); CREATE("", velocity_y_field);
    CREATE("", acceleration_x_field); CREATE("", acceleration_y_field);
    CREATE("", angular_velocity_field);
    CREATE("", mass_field); CREATE("", friction_field);
    CREATE("", restitution_field);
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
            !editor_mode_accordion_section_create(&editor->parenting_section, font,
                "Parenting", false) ||
            !editor_mode_accordion_section_create(&editor->appearance_section, font,
                "Appearance", false) ||
            !editor_mode_accordion_section_create(&editor->geometry_section, font,
                "Geometry", false)) goto fail;
    for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX; i += 1) {
        char name[32]; snprintf(name, sizeof(name), "body_%zu", i + 1);
        if(!editor_mode_text_create(font, name, &editor->body_names[i])) goto fail;
        if(!editor_mode_text_create(font, name, &editor->parent_names[i])) goto fail;
    }
    for(size_t i = 0; i < EDITOR_BODY_HITBOX_MAX; i += 1) {
        char name[32]; snprintf(name, sizeof(name), "hitbox_%zu", i + 1);
        if(!editor_mode_text_create(font, name, &editor->hitbox_names[i])) goto fail;
    }
    for(size_t i = 0; i < MAX_ANIMATIONS_FRAMES; i += 1) {
        char name[32]; snprintf(name, sizeof(name), "frame_%zu", i + 1);
        if(!editor_mode_text_create(font, name, &editor->frame_names[i])) goto fail;
    }
    return true;
fail:
    editor_rigid_body_editor_destroy(editor);
    return false;
}

void editor_rigid_body_editor_destroy(EditorRigidBodyEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(x_label); DESTROY(y_label); DESTROY(rotation_label);
    DESTROY(velocity_x_label); DESTROY(velocity_y_label);
    DESTROY(acceleration_x_label); DESTROY(acceleration_y_label);
    DESTROY(angular_velocity_label);
    DESTROY(mass_label); DESTROY(friction_label); DESTROY(restitution_label);
    DESTROY(border_color_label); DESTROY(surface_color_label); DESTROY(gravity_label);
    DESTROY(parent_label); DESTROY(none_label);
    DESTROY(dynamic_label); DESTROY(static_label); DESTROY(rotation_unlocked_label);
    DESTROY(rotation_locked_label); DESTROY(collision_label); DESTROY(particle_label);
    DESTROY(collision_category_label); DESTROY(collide_with_label); DESTROY(origin_label);
    DESTROY(active_hitbox_label); DESTROY(add_hitbox_label);
    DESTROY(bind_frames_label);
    DESTROY(delete_label); DESTROY(visibility_label); DESTROY(visible_label);
    DESTROY(hidden_label); DESTROY(x_field); DESTROY(y_field);
    DESTROY(rotation_field); DESTROY(mass_field); DESTROY(friction_field);
    DESTROY(velocity_x_field); DESTROY(velocity_y_field);
    DESTROY(acceleration_x_field); DESTROY(acceleration_y_field);
    DESTROY(angular_velocity_field);
    DESTROY(restitution_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->initial_motion_section);
    editor_mode_accordion_section_destroy(&editor->physics_section);
    editor_mode_accordion_section_destroy(&editor->material_section);
    editor_mode_accordion_section_destroy(&editor->collision_section);
    editor_mode_accordion_section_destroy(&editor->parenting_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    editor_mode_accordion_section_destroy(&editor->geometry_section);
    for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->body_names[i]);
    for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->parent_names[i]);
    for(size_t i = 0; i < EDITOR_BODY_HITBOX_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->hitbox_names[i]);
    for(size_t i = 0; i < MAX_ANIMATIONS_FRAMES; i += 1)
        rohr_graphics_text_destroy(&editor->frame_names[i]);
    *editor = (EditorRigidBodyEditor){0};
}

bool editor_rigid_body_editor_draw(EditorRigidBodyEditor *editor,
        const EditorModeContext *context,
        EditorRigidBodyCollisionMenuFunction collision_menu,
        void *collision_context) {
    EditorObject *object;
    EditorRigidBody *body;
    size_t body_index;
    char name[EDITOR_OBJECT_NAME_MAX];
    Position position;
    float rotation;
    UIFieldResult name_result, x_result = {0}, y_result = {0},
        rotation_result = {0};
    bool field_active;
    bool binding_pointer_inside = false;
    bool binding_click_handled = false;
    float x, width, delete_y = 650.0f;
    float material_y = 0.0f;
    float collision_y = 0.0f, parenting_y = 0.0f, appearance_y = 0.0f;
    float section_y = 118.0f;
    float collision_category_y = 0.0f, collision_category_list_y = 0.0f;
    float collide_with_y = 0.0f, collide_with_list_y = 0.0f;
    float collision_bottom = 0.0f;
    float geometry_origin_y = 0.0f, geometry_particle_y = 0.0f;
    float geometry_active_label_y = 0.0f, geometry_active_field_y = 0.0f;
    float geometry_add_y = 0.0f, geometry_list_y = 0.0f;
    float geometry_list_stride = 0.0f;
    float transform_row_y[5] = {0}, physics_row_y[3] = {0};
    float initial_motion_row_y[5] = {0};
    float material_row_y[2] = {0}, appearance_row_y[2] = {0};
    bool transform_open, initial_motion_open, physics_open, material_open,
        collision_open;
    bool parenting_open, appearance_open, geometry_open;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    x = context->x; width = context->width;
    object = editor_project_selected_get(context->project);
    body = object == NULL ? NULL : editor_project_rigid_body_get(object,
        context->viewport->selected_rigid_body);
    if(body == NULL) return false;
    body_index = (size_t)(body - object->rigid_bodies);
    if(body_index >= EDITOR_RIGID_BODY_MAX) return false;
    snprintf(name, sizeof(name), "%s", body->name);
    if(!editor_mode_named_text_sync(editor->font, body->name,
            &editor->body_names[body_index], editor->body_cache[body_index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    rohr_ui_label(&editor->name_label, (UIRect){x + 40.0f, 42.0f, 48.0f, 30.0f});
    name_result = editor_mode_name_field("editor.rigid_body.name", name,
        sizeof(name), &editor->body_names[body_index],
        (UIRect){x + 88.0f, 42.0f, width - 96.0f, 30.0f});
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id}};
        snprintf(command.data.item_rename.name,
            sizeof(command.data.item_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    {
        bool visible = body->visible;
        if(editor_mode_checkbox_left("editor.rigid_body.visibility",
                &editor->visibility_label,
                (UIRect){x + 10.0f, 80.0f, width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_RIGID_BODY, object->id, 0,
                    body->id, visible}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    {
        float layer_height = body->graphics_layer.layer == 0 ||
            (context->layer_control != NULL &&
                (context->layer_control->adding ||
                    context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
        size_t binding_frame_count = 0;
        if(editor->binding_hitbox_open != 0) {
            for(size_t animation = 0; animation < object->animated_sprite_count;
                    animation += 1) {
                if(object->animated_sprite_items[animation].rigid_body != body->id)
                    continue;
                binding_frame_count = object->animated_sprite_items[
                    animation].frame_count < MAX_ANIMATIONS_FRAMES ?
                    object->animated_sprite_items[animation].frame_count :
                    MAX_ANIMATIONS_FRAMES;
                break;
            }
        }
        const EditorModeAccordionLayoutGroup transform_groups[] = {
            {.row_count = 3, .row_height = 26.0f, .row_gap = 6.0f},
            {.row_count = 1, .row_height = 28.0f, .gap_before = 6.0f},
            {.row_count = 1, .row_height = layer_height, .gap_before = 4.0f}};
        const EditorModeAccordionLayoutGroup physics_groups[] = {
            {.row_count = 1, .row_height = 28.0f},
            {.row_count = 1, .row_height = 26.0f, .gap_before = 4.0f},
            {.row_count = 1, .row_height = 28.0f, .gap_before = 6.0f}};
        const float initial_motion_rows[] = {
            26.0f, 26.0f, 26.0f, 26.0f, 26.0f};
        const float material_rows[] = {26.0f, 26.0f};
        const float parenting_rows[] = {26.0f};
        const float appearance_rows[] = {26.0f, 26.0f};
        EditorModeAccordionLayoutGroup collision_groups[5] = {
            {.row_count = 1, .row_height = 28.0f}};
        size_t collision_group_count = 1;
        size_t category_group = SIZE_MAX, category_list_group = SIZE_MAX;
        size_t collide_group = SIZE_MAX, collide_list_group = SIZE_MAX;
        if(body->collision_enabled) {
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
        EditorModeAccordionLayoutGroup geometry_groups[6] = {
            {.row_count = 1, .row_height = 28.0f}};
        size_t geometry_group_count = 1;
        size_t particle_group = SIZE_MAX, active_group = SIZE_MAX;
        size_t add_group, list_group = SIZE_MAX;
        if(body->particle)
            particle_group = geometry_group_count,
            geometry_groups[geometry_group_count++] =
                (EditorModeAccordionLayoutGroup){.row_count = 1,
                    .row_height = 28.0f, .gap_before = 6.0f};
        if(body->hitbox_count > 0) {
            active_group = geometry_group_count;
            geometry_groups[geometry_group_count++] =
                (EditorModeAccordionLayoutGroup){.row_count = 2,
                    .row_height = 28.0f, .row_gap = 2.0f,
                    .gap_before = 6.0f};
        }
        add_group = geometry_group_count;
        geometry_groups[geometry_group_count++] =
            (EditorModeAccordionLayoutGroup){.row_count = 1,
                .row_height = 32.0f, .gap_before = 6.0f};
        if(body->hitbox_count + binding_frame_count > 0) {
            list_group = geometry_group_count;
            geometry_groups[geometry_group_count++] =
                (EditorModeAccordionLayoutGroup){
                    .row_count = body->hitbox_count + binding_frame_count,
                    .row_height = 26.0f, .row_gap = 4.0f,
                    .gap_before = 10.0f};
        }
        EditorModeAccordionLayoutCursor accordion =
            editor_mode_accordion_layout_cursor_get(x, width, section_y);
#define SECTION_LAYOUT(section, id, rows, row_count, row_gap, open_value, output_y) do { \
    EditorModeAccordionLayoutResult layout = \
        editor_mode_accordion_layout_section(&accordion, &(section), (id), \
            (rows), (row_count), (row_gap)); \
    (open_value) = layout.expanded; \
    (output_y) = layout.content_y; \
} while(0)
        EditorModeAccordionLayoutResult transform_layout =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->transform_section,
                "editor.rigid_body.section.transform", transform_groups, 3);
        transform_open = transform_layout.expanded;
        EditorModeAccordionLayoutResult initial_motion_layout =
            editor_mode_accordion_layout_section(&accordion,
                &editor->initial_motion_section,
                "editor.rigid_body.section.initial_motion",
                initial_motion_rows, 5, 6.0f);
        initial_motion_open = initial_motion_layout.expanded;
        for(size_t row = 0; row < 5; row += 1)
            initial_motion_row_y[row] = editor_mode_accordion_layout_row_y(
                &initial_motion_layout, initial_motion_rows, row, 6.0f);
        EditorModeAccordionLayoutResult physics_layout =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->physics_section,
                "editor.rigid_body.section.physics", physics_groups, 3);
        physics_open = physics_layout.expanded;
        SECTION_LAYOUT(editor->material_section,
            "editor.rigid_body.section.material", material_rows, 2, 6.0f,
            material_open, material_y);
        EditorModeAccordionLayoutResult collision_layout =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->collision_section,
                "editor.rigid_body.section.collision", collision_groups,
                collision_group_count);
        collision_open = collision_layout.expanded;
        collision_y = collision_layout.content_y;
        if(category_group != SIZE_MAX)
            collision_category_y = editor_mode_accordion_layout_group_row_y(
                &collision_layout, collision_groups, collision_group_count,
                category_group, 0);
        if(category_list_group != SIZE_MAX)
            collision_category_list_y =
                editor_mode_accordion_layout_group_row_y(&collision_layout,
                    collision_groups, collision_group_count,
                    category_list_group, 0);
        if(collide_group != SIZE_MAX)
            collide_with_y = editor_mode_accordion_layout_group_row_y(
                &collision_layout, collision_groups, collision_group_count,
                collide_group, 0);
        if(collide_list_group != SIZE_MAX)
            collide_with_list_y = editor_mode_accordion_layout_group_row_y(
                &collision_layout, collision_groups, collision_group_count,
                collide_list_group, 0);
        collision_bottom = collision_layout.content_y +
            editor_mode_accordion_layout_groups_height_get(
                collision_groups, collision_group_count);
        SECTION_LAYOUT(editor->parenting_section,
            "editor.rigid_body.section.parenting", parenting_rows, 1, 0.0f,
            parenting_open, parenting_y);
        SECTION_LAYOUT(editor->appearance_section,
            "editor.rigid_body.section.appearance", appearance_rows, 2, 6.0f,
            appearance_open, appearance_y);
        EditorModeAccordionLayoutResult geometry_layout =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->geometry_section,
                "editor.rigid_body.section.geometry", geometry_groups,
                geometry_group_count);
        geometry_open = geometry_layout.expanded;
        for(size_t row = 0; row < 3; row += 1)
            transform_row_y[row] =
                editor_mode_accordion_layout_group_row_y(&transform_layout,
                    transform_groups, 3, 0, row);
        transform_row_y[3] = editor_mode_accordion_layout_group_row_y(
            &transform_layout, transform_groups, 3, 1, 0);
        transform_row_y[4] = editor_mode_accordion_layout_group_row_y(
            &transform_layout, transform_groups, 3, 2, 0);
        for(size_t row = 0; row < 3; row += 1)
            physics_row_y[row] = editor_mode_accordion_layout_group_row_y(
                &physics_layout, physics_groups, 3, row, 0);
        for(size_t row = 0; row < 2; row += 1) {
            material_row_y[row] = editor_mode_accordion_layout_row_y(
                &(EditorModeAccordionLayoutResult){
                    .content_y = material_y}, material_rows, row, 6.0f);
            appearance_row_y[row] = editor_mode_accordion_layout_row_y(
                &(EditorModeAccordionLayoutResult){
                    .content_y = appearance_y}, appearance_rows, row, 6.0f);
        }
        geometry_origin_y = editor_mode_accordion_layout_group_row_y(
            &geometry_layout, geometry_groups, geometry_group_count, 0, 0);
        if(particle_group != SIZE_MAX)
            geometry_particle_y = editor_mode_accordion_layout_group_row_y(
                &geometry_layout, geometry_groups, geometry_group_count,
                particle_group, 0);
        if(active_group != SIZE_MAX) {
            geometry_active_label_y = editor_mode_accordion_layout_group_row_y(
                &geometry_layout, geometry_groups, geometry_group_count,
                active_group, 0);
            geometry_active_field_y = editor_mode_accordion_layout_group_row_y(
                &geometry_layout, geometry_groups, geometry_group_count,
                active_group, 1);
        }
        geometry_add_y = editor_mode_accordion_layout_group_row_y(
            &geometry_layout, geometry_groups, geometry_group_count,
            add_group, 0);
        if(list_group != SIZE_MAX) {
            geometry_list_y = editor_mode_accordion_layout_group_row_y(
                &geometry_layout, geometry_groups, geometry_group_count,
                list_group, 0);
            geometry_list_stride = geometry_groups[list_group].row_height +
                geometry_groups[list_group].row_gap;
        }
#undef SECTION_LAYOUT
    }
    position = body->position; rotation = body->rotation;
    field_active = name_result.active;
    if(transform_open) {
        UIButtonStyle field_style = editor_mode_section_field_style_get();
        rohr_ui_label(&editor->x_label,
            (UIRect){x + 8.0f, transform_row_y[0], 24.0f, 26.0f});
        x_result = editor_mode_field("editor.rigid_body.x",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.x},
            &editor->x_field, (UIRect){x + 34.0f, transform_row_y[0],
                width - 44.0f, 26.0f}, &field_style);
        rohr_ui_label(&editor->y_label,
            (UIRect){x + 8.0f, transform_row_y[1], 24.0f, 26.0f});
        y_result = editor_mode_field("editor.rigid_body.y",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &position.y},
            &editor->y_field, (UIRect){x + 34.0f, transform_row_y[1],
                width - 44.0f, 26.0f}, &field_style);
        rohr_ui_label(&editor->rotation_label,
            (UIRect){x + 8.0f, transform_row_y[2], 76.0f, 26.0f});
        rotation_result = editor_mode_field("editor.rigid_body.rotation",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &rotation},
            &editor->rotation_field, (UIRect){x + 86.0f,
                transform_row_y[2], width - 96.0f, 26.0f}, &field_style);
        if(x_result.changed || y_result.changed || rotation_result.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_RIGID_BODY_TRANSFORM,
                .data.rigid_body_transform = {
                    object->id, body->id, position, rotation}};
            (void)editor_command_execute(context->project, &command);
        }
        field_active = field_active || x_result.active || y_result.active ||
            rotation_result.active;
        const TextAsset *options[] = {&editor->rotation_unlocked_label,
            &editor->rotation_locked_label};
        UIDropdownResult result = editor_mode_dropdown("editor.rigid_body.rotation_lock",
            options, 2, body->rotation_locked ? 1 : 0,
            (UIRect){x + 10.0f, transform_row_y[3], width - 20.0f, 28.0f},
            &field_style);
        if(result.changed) property_bool_set(context->project, object->id, body->id,
            EDITOR_PROPERTY_ROTATION_LOCKED, result.selected_index == 1);
        if(context->layer_control != NULL)
            field_active = editor_mode_layer_control_draw(context->layer_control,
                "editor.rigid_body", context->project, &body->graphics_layer,
                NULL, x, transform_row_y[4], width) || field_active;
    }
    if(initial_motion_open) {
        TextAsset *labels[] = {&editor->velocity_x_label,
            &editor->velocity_y_label, &editor->acceleration_x_label,
            &editor->acceleration_y_label, &editor->angular_velocity_label};
        TextAsset *fields[] = {&editor->velocity_x_field,
            &editor->velocity_y_field, &editor->acceleration_x_field,
            &editor->acceleration_y_field, &editor->angular_velocity_field};
        const char *ids[] = {"editor.rigid_body.initial_velocity_x",
            "editor.rigid_body.initial_velocity_y",
            "editor.rigid_body.initial_acceleration_x",
            "editor.rigid_body.initial_acceleration_y",
            "editor.rigid_body.initial_angular_velocity"};
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
            float row = initial_motion_row_y[i];
            UIFieldResult result;
            rohr_ui_label(labels[i], (UIRect){x + 8.0f, row, 112.0f, 26.0f});
            result = editor_mode_field(ids[i],
                (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &values[i]},
                fields[i], (UIRect){x + 122.0f, row, width - 132.0f, 26.0f},
                &style);
            if(result.changed) property_float_set(context->project, object->id,
                body->id, properties[i], values[i]);
            field_active = field_active || result.active;
        }
    }
#define FLOAT_FIELD(field_id, label, field, field_y, label_width, property, source, min_value, clamp_max) do { \
    float value = (source); UIFieldResult result; \
    UIButtonStyle field_style = editor_mode_section_field_style_get(); \
    rohr_ui_label(&(label), (UIRect){x + 8.0f, (field_y), (label_width), 26.0f}); \
    result = editor_mode_field((field_id), (UIFieldBinding){.kind = UI_FIELD_FLOAT, \
        .number = &value}, &(field), \
        (UIRect){x + (label_width) + 10.0f, (field_y), \
            width - (label_width) - 20.0f, 26.0f}, &field_style); \
    if(result.changed) value = fmaxf((min_value), value); \
    if(result.changed && (clamp_max) >= 0.0f) value = fminf((clamp_max), value); \
    if(result.changed) property_float_set(context->project, object->id, body->id, \
        (property), value); field_active = field_active || result.active; \
} while(0)
    if(physics_open)
        FLOAT_FIELD("editor.rigid_body.mass", editor->mass_label,
            editor->mass_field, physics_row_y[1], 76.0f,
            EDITOR_PROPERTY_MASS, body->mass_value, 0.0f, -1.0f);
    if(material_open) {
        FLOAT_FIELD("editor.rigid_body.friction", editor->friction_label,
            editor->friction_field, material_row_y[0], 76.0f,
            EDITOR_PROPERTY_FRICTION, body->friction, 0.0f, -1.0f);
        FLOAT_FIELD("editor.rigid_body.restitution", editor->restitution_label,
            editor->restitution_field, material_row_y[1], 96.0f,
            EDITOR_PROPERTY_RESTITUTION, body->restitution, 0.0f, 1.0f);
    }
#undef FLOAT_FIELD
    if(appearance_open) {
        rohr_ui_label(&editor->border_color_label,
            (UIRect){x + 8.0f, appearance_row_y[0], 104.0f, 26.0f});
        if(context->color_open != NULL) editor_mode_color_swatch(
            "editor.rigid_body.border_color", &body->border_color, false,
            (UIRect){x + 114.0f, appearance_row_y[0], width - 124.0f, 26.0f},
            context, EDITOR_ITEM_RIGID_BODY, object->id, 0, body->id,
            EDITOR_PROPERTY_OUTLINE_COLOR);
        rohr_ui_label(&editor->surface_color_label,
            (UIRect){x + 8.0f, appearance_row_y[1], 104.0f, 26.0f});
        if(context->color_open != NULL) editor_mode_color_swatch(
            "editor.rigid_body.surface_color", &body->surface_color, false,
            (UIRect){x + 114.0f, appearance_row_y[1],
                width - 124.0f, 26.0f}, context, EDITOR_ITEM_RIGID_BODY,
            object->id, 0, body->id, EDITOR_PROPERTY_SURFACE_COLOR);
    }
    if(parenting_open) {
        const TextAsset *options[EDITOR_RIGID_BODY_MAX + 1];
        EditorRigidBodyId ids[EDITOR_RIGID_BODY_MAX + 1] = {0};
        size_t count = 1, selected = 0;
        options[0] = &editor->none_label;
        for(size_t i = 0; i < object->rigid_body_count &&
                count <= EDITOR_RIGID_BODY_MAX; i += 1) {
            EditorRigidBody *candidate = &object->rigid_bodies[i];
            if(candidate->id == body->id) continue;
            if(!editor_mode_named_text_sync(editor->font, candidate->name,
                    &editor->parent_names[count - 1],
                    editor->parent_cache[count - 1], EDITOR_OBJECT_NAME_MAX))
                return field_active;
            options[count] = &editor->parent_names[count - 1];
            ids[count] = candidate->id;
            if(candidate->id == body->parent) selected = count;
            count += 1;
        }
        rohr_ui_label(&editor->parent_label,
            (UIRect){x + 8.0f, parenting_y, 70.0f, 28.0f});
        UIButtonStyle field_style = editor_mode_section_field_style_get();
        UIDropdownResult result = editor_mode_dropdown("editor.rigid_body.parent",
            options, count, selected,
            (UIRect){x + 80.0f, parenting_y, width - 90.0f, 28.0f},
            &field_style);
        if(result.button_hovered || result.hovered_index >= 0) {
            size_t preview = result.hovered_index >= 0 ?
                (size_t)result.hovered_index : selected;
            if(preview < count) context->viewport->preview_rigid_body = ids[preview];
        }
        if(result.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_RELATIONSHIP_SET,
                .data.relationship_set = {EDITOR_RELATIONSHIP_ENTITY_PARENT,
                    object->id, 0, body->id, 0, ids[result.selected_index]}};
            (void)editor_command_execute(context->project, &command);
        }
    }
    if(physics_open) {
        bool value = body->gravity_enabled;
        if(checkbox("editor.rigid_body.gravity", &editor->gravity_label,
                (UIRect){x + 10.0f, physics_row_y[2],
                    width - 20.0f, 28.0f}, &value,
                false, NULL))
            property_bool_set(context->project, object->id, body->id,
                EDITOR_PROPERTY_GRAVITY, value);
    }
    if(physics_open) {
        const TextAsset *options[] = {&editor->dynamic_label, &editor->static_label};
        UIButtonStyle field_style = editor_mode_section_field_style_get();
        UIDropdownResult result = editor_mode_dropdown("editor.rigid_body.motion", options,
            2, body->static_body ? 1 : 0,
            (UIRect){x + 10.0f, physics_row_y[0], width - 20.0f, 28.0f},
            &field_style);
        if(result.changed) property_bool_set(context->project, object->id, body->id,
            EDITOR_PROPERTY_STATIC, result.selected_index == 1);
    }
    {
        float row_x = x + 10.0f, row_width = width - 20.0f;
        bool collision = body->collision_enabled;
        if(collision_open && checkbox("editor.rigid_body.collision",
                &editor->collision_label,
                (UIRect){row_x, collision_y, row_width * 0.52f, 28.0f},
                &collision, false, NULL)) {
            property_bool_set(context->project, object->id, body->id,
                EDITOR_PROPERTY_COLLISION, collision);
            if(!collision) editor->collision_category_open =
                editor->collide_with_open = false;
        }
        if(collision_open && body->collision_enabled) {
            bool particle = body->particle;
            if(checkbox("editor.rigid_body.particle", &editor->particle_label,
                    (UIRect){row_x + row_width * 0.54f, collision_y,
                        row_width * 0.46f, 28.0f}, &particle, true, NULL))
                property_bool_set(context->project, object->id, body->id,
                    EDITOR_PROPERTY_PARTICLE, particle);
            if(!body->particle && context->viewport->selection ==
                    EDITOR_SELECTION_PARTICLE)
                context->viewport->selection = EDITOR_SELECTION_RIGID_BODY;
            if(rohr_ui_button("editor.rigid_body.collision_category",
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
                        "editor.rigid_body.collision_category.mask", context->project,
                        &body->collision_category, object->id, body->id,
                        EDITOR_COLLISION_FILTER_CATEGORY, row_x,
                        collision_category_list_y, row_width,
                        &field_active, &rows)) return field_active;
            }
            if(rohr_ui_button("editor.rigid_body.collide_with",
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
                        "editor.rigid_body.collide_with.mask", context->project,
                        &body->collision_with, object->id, body->id,
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
        if(geometry_open) {
            UIButtonStyle selected_style = selected_style_get();
            UIButtonResult origin = rohr_ui_button("editor.rigid_body.origin",
                &editor->origin_label,
                (UIRect){row_x, geometry_origin_y, row_width, 28.0f},
                context->viewport->selection == EDITOR_SELECTION_ORIGIN &&
                    context->viewport->selected_origin_kind ==
                        EDITOR_ORIGIN_RIGID_BODY ? &selected_style : NULL);
            if(origin.clicked || origin.focus_changed) {
                context->viewport->selection = EDITOR_SELECTION_ORIGIN;
                context->viewport->selected_origin_kind = EDITOR_ORIGIN_RIGID_BODY;
                if(origin.double_clicked) context->viewport->mode = EDITOR_VIEWPORT_ORIGIN;
            }
            if(body->particle) {
                UIButtonResult particle = rohr_ui_button("editor.rigid_body.particle_item",
                    &editor->particle_label,
                    (UIRect){row_x, geometry_particle_y, row_width, 28.0f},
                    context->viewport->selection == EDITOR_SELECTION_PARTICLE ?
                        &selected_style : NULL);
                if(particle.clicked || particle.focus_changed) {
                    context->viewport->selection = EDITOR_SELECTION_PARTICLE;
                    if(particle.double_clicked)
                        context->viewport->mode = EDITOR_VIEWPORT_PARTICLE;
                }
            }
            if(body->hitbox_count > 0) {
                const TextAsset *options[EDITOR_BODY_HITBOX_MAX];
                size_t option_count = body->hitbox_count < EDITOR_BODY_HITBOX_MAX ?
                    body->hitbox_count : EDITOR_BODY_HITBOX_MAX;
                rohr_ui_label(&editor->active_hitbox_label,
                    (UIRect){row_x, geometry_active_label_y,
                        row_width, 26.0f});
                for(size_t i = 0; i < option_count; i += 1) {
                    if(!editor_mode_named_text_sync(editor->font,
                            body->hitboxes[i].name, &editor->hitbox_names[i],
                            editor->hitbox_cache[i], EDITOR_OBJECT_NAME_MAX))
                        return field_active;
                    options[i] = &editor->hitbox_names[i];
                }
                {
                    UIDropdownResult active = editor_mode_dropdown(
                        "editor.rigid_body.active_hitbox", options, option_count,
                        body->active_hitbox_index < option_count ?
                            body->active_hitbox_index : 0,
                        (UIRect){row_x, geometry_active_field_y,
                            row_width, 28.0f}, NULL);
                    if(active.changed) property_uint_set(context->project,
                        object->id, body->id, EDITOR_PROPERTY_ACTIVE_HITBOX,
                        (uint32_t)active.selected_index);
                }
            }
            if(rohr_ui_button("editor.rigid_body.add_hitbox", &editor->add_hitbox_label,
                    (UIRect){row_x, geometry_add_y,
                        row_width, 32.0f}, NULL).clicked) {
                EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
                    .data.item_add = {.kind = EDITOR_ITEM_HITBOX,
                        .object = object->id, .parent = body->id}};
                EditorCommandResult added = editor_command_execute(context->project, &command);
                if(added.kind == ERROR_RESULT_VALUE) {
                    context->viewport->selection = EDITOR_SELECTION_HITBOX;
                    context->viewport->selected_hitbox = added.result.object;
                    if(editor_navigation_selected_open(context->project,
                            context->viewport))
                        (void)editor_mode_name_focus_request(context->viewport);
                }
            }
            {
            float hitbox_y = geometry_list_y;
            EditorAnimatedSprite *animation = NULL;
            for(size_t animation_index = 0;
                    animation_index < object->animated_sprite_count;
                    animation_index += 1) {
                if(object->animated_sprite_items[animation_index].rigid_body ==
                        body->id) {
                    animation = &object->animated_sprite_items[animation_index];
                    break;
                }
            }
            for(size_t i = 0; i < body->hitbox_count &&
                    i < EDITOR_BODY_HITBOX_MAX; i += 1) {
                EditorHitbox *hitbox = &body->hitboxes[i];
                char id[64], visibility_id[72], binding_id[80];
                float y = hitbox_y;
                UIButtonResult result;
                if(!editor_mode_named_text_sync(editor->font, hitbox->name,
                        &editor->hitbox_names[i], editor->hitbox_cache[i],
                        EDITOR_OBJECT_NAME_MAX)) return field_active;
                snprintf(id, sizeof(id), "editor.hitbox.%u", hitbox->id);
                snprintf(visibility_id, sizeof(visibility_id),
                    "editor.hitbox.%u.visibility", hitbox->id);
                snprintf(binding_id, sizeof(binding_id),
                    "editor.hitbox.%u.bind_frames", hitbox->id);
                if(rohr_ui_button(visibility_id, hitbox->visible ?
                        &editor->visible_label : &editor->hidden_label,
                        (UIRect){row_x, y, 26.0f, 26.0f}, NULL).clicked) {
                    EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                        .data.visibility = {EDITOR_VISIBILITY_HITBOX, object->id,
                            body->id, hitbox->id, !hitbox->visible}};
                    (void)editor_command_execute(context->project, &command);
                }
                result = rohr_ui_button(id, &editor->hitbox_names[i],
                    (UIRect){row_x + 32.0f, y, row_width * 0.52f - 32.0f, 26.0f},
                    ((context->viewport->selection == EDITOR_SELECTION_HITBOX &&
                        context->viewport->selected_hitbox == hitbox->id) ||
                        editor_viewport_selection_contains(context->viewport,
                            (EditorSelectionRef){EDITOR_SELECTION_HITBOX,
                                object->id, body->id, 0, hitbox->id})) ?
                        &selected_style : NULL);
                if(context->hierarchy_row != NULL)
                    context->hierarchy_row(context->hierarchy_context,
                        context->viewport,
                        (EditorSelectionRef){EDITOR_SELECTION_HITBOX,
                            object->id, body->id, 0, hitbox->id},
                        (UIRect){row_x + 32.0f, y,
                            row_width * 0.52f - 32.0f, 26.0f},
                        result, i + 1 == body->hitbox_count);
                if(result.clicked || result.focus_changed) {
                    context->viewport->selection = EDITOR_SELECTION_HITBOX;
                    context->viewport->selected_hitbox = hitbox->id;
                    if(result.double_clicked)
                        (void)editor_navigation_selected_open(context->project,
                            context->viewport);
                }
                {
                    UIButtonStyle disabled = rohr_ui_button_style_default_get();
                    UIRect binding_bounds = {row_x + row_width * 0.54f, y,
                        row_width * 0.46f, 26.0f};
                    UIButtonResult binding_button;
                    disabled.idle = disabled.hovered = (Color){38, 41, 48, 255};
                    binding_button = rohr_ui_button(binding_id,
                        &editor->bind_frames_label, binding_bounds,
                        animation == NULL ? &disabled : NULL);
                    if(editor->binding_hitbox_open == hitbox->id &&
                            binding_button.hovered)
                        binding_pointer_inside = true;
                    if(binding_button.clicked && animation != NULL) {
                        binding_click_handled = true;
                        editor->binding_hitbox_open =
                            editor->binding_hitbox_open == hitbox->id ? 0 : hitbox->id;
                    }
                }
                hitbox_y += geometry_list_stride;
                if(editor->binding_hitbox_open == hitbox->id && animation != NULL) {
                    size_t frame_count = animation->frame_count < MAX_ANIMATIONS_FRAMES ?
                        animation->frame_count : MAX_ANIMATIONS_FRAMES;
                    for(size_t frame = 0; frame < frame_count; frame += 1) {
                        char frame_id[96];
                        UIRect frame_bounds = {row_x + 18.0f, hitbox_y,
                            row_width - 18.0f, 26.0f};
                        bool checked = editor_project_hitbox_animation_binding_check(
                            body, animation->id, animation->frames[frame].id,
                            hitbox->id);
                        if(!editor_mode_named_text_sync(editor->font,
                                animation->frames[frame].name,
                                &editor->frame_names[frame],
                                editor->frame_cache[frame], EDITOR_OBJECT_NAME_MAX))
                            return field_active;
                        snprintf(frame_id, sizeof(frame_id),
                            "editor.hitbox.%u.frame.%u", hitbox->id,
                            animation->frames[frame].id);
                        bool frame_hovered = false;
                        if(checkbox(frame_id, &editor->frame_names[frame],
                                frame_bounds, &checked, false,
                                &frame_hovered)) {
                            binding_click_handled = true;
                            EditorCommand command = {.type =
                                EDITOR_COMMAND_PROPERTY_SET,
                                .data.property_set = {
                                    .kind = EDITOR_ITEM_RIGID_BODY,
                                    .object = object->id,
                                    .parent = hitbox->id,
                                    .item = body->id,
                                    .index = animation->frames[frame].id,
                                    .property =
                                        EDITOR_PROPERTY_HITBOX_FRAME_BINDING,
                                    .value_kind = EDITOR_PROPERTY_VALUE_BOOL,
                                    .value.boolean = checked}};
                            (void)editor_command_execute(context->project,
                                &command);
                        }
                        binding_pointer_inside = binding_pointer_inside ||
                            frame_hovered;
                        hitbox_y += geometry_list_stride;
                    }
                }
            }
            delete_y = hitbox_y + 8.0f;
            }
        }
    }
    if(editor->binding_hitbox_open != 0 &&
            context->primary_button == MOUSE_BUTTON_STATE_PRESSED &&
            !binding_pointer_inside && !binding_click_handled)
        editor->binding_hitbox_open = 0;
    if(context->delete_y_get != NULL && context->delete_open_item != NULL &&
            !context->delete_footer) {
        UIButtonStyle style = editor_mode_delete_style_get();
        float panel_delete_y = context->delete_y_get(context->delete_context);
        if(delete_y < panel_delete_y) delete_y = panel_delete_y;
        if(rohr_ui_button("editor.rigid_body.delete", &editor->delete_label,
                (UIRect){x + 10.0f, delete_y,
                    width - 20.0f, 34.0f}, &style).clicked)
            (void)context->delete_open_item(context->delete_context);
    }
    return field_active;
}
