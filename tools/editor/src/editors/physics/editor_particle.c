/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_particle.h"

#include "editors/editor_mode_controls.h"

#include <math.h>
#include <stdio.h>

bool editor_particle_editor_create(EditorParticleEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorParticleEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Particle", title);
    CREATE("Visibility", visibility_label);
    CREATE("[X]", visible_label);
    CREATE("[ ]", hidden_label);
    CREATE("Delete Particle", delete_label);
    CREATE("Radius", radius_label);
    CREATE("Rigid Vertices", rigid_vertices_label);
    CREATE("Origin X", origin_x_label);
    CREATE("Origin Y", origin_y_label);
    CREATE("Auto Fit", auto_fit_label);
    CREATE("Ring Color", ring_color_label);
    CREATE("Fill Color", fill_color_label);
    CREATE("", radius_field);
    CREATE("", rigid_vertices_field);
    CREATE("", origin_x_field);
    CREATE("", origin_y_field);
    CREATE("Name", name_label);
    CREATE("X", x_label);
    CREATE("Y", y_label);
    CREATE("Velocity X", velocity_x_label);
    CREATE("Velocity Y", velocity_y_label);
    CREATE("Acceleration X", acceleration_x_label);
    CREATE("Acceleration Y", acceleration_y_label);
    CREATE("Mass", mass_label);
    CREATE("Friction", friction_label);
    CREATE("Restitution", restitution_label);
    CREATE("Parent", parent_label);
    CREATE("None", none_label);
    CREATE("Gravity", gravity_label);
    CREATE("Dynamic", dynamic_label);
    CREATE("Static", static_label);
    CREATE("", x_field);
    CREATE("", y_field);
    CREATE("", velocity_x_field);
    CREATE("", velocity_y_field);
    CREATE("", acceleration_x_field);
    CREATE("", acceleration_y_field);
    CREATE("", mass_field);
    CREATE("", friction_field);
    CREATE("", restitution_field);
    CREATE("", name_field);
#undef CREATE
    if(!editor_center_of_mass_editor_create(&editor->center_of_mass, font) ||
            !editor_collision_controls_create(&editor->collision, font)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->initial_motion_section, font,
            "Initial Motion", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->physics_section, font,
            "Physics", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->material_section, font,
            "Material", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->collision_section, font,
            "Collision", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->parenting_section, font,
            "Parenting", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->appearance_section, font,
            "Appearance", false)) goto fail;
    if(!editor_mode_accordion_section_create(&editor->geometry_section, font,
            "Geometry", false)) goto fail;
    return true;
fail:
    editor_particle_editor_destroy(editor);
    return false;
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
    rohr_graphics_text_destroy(&editor->name_label);
    rohr_graphics_text_destroy(&editor->x_label);
    rohr_graphics_text_destroy(&editor->y_label);
    rohr_graphics_text_destroy(&editor->velocity_x_label);
    rohr_graphics_text_destroy(&editor->velocity_y_label);
    rohr_graphics_text_destroy(&editor->acceleration_x_label);
    rohr_graphics_text_destroy(&editor->acceleration_y_label);
    rohr_graphics_text_destroy(&editor->mass_label);
    rohr_graphics_text_destroy(&editor->friction_label);
    rohr_graphics_text_destroy(&editor->restitution_label);
    rohr_graphics_text_destroy(&editor->parent_label);
    rohr_graphics_text_destroy(&editor->none_label);
    rohr_graphics_text_destroy(&editor->gravity_label);
    rohr_graphics_text_destroy(&editor->dynamic_label);
    rohr_graphics_text_destroy(&editor->static_label);
    rohr_graphics_text_destroy(&editor->x_field);
    rohr_graphics_text_destroy(&editor->y_field);
    rohr_graphics_text_destroy(&editor->velocity_x_field);
    rohr_graphics_text_destroy(&editor->velocity_y_field);
    rohr_graphics_text_destroy(&editor->acceleration_x_field);
    rohr_graphics_text_destroy(&editor->acceleration_y_field);
    rohr_graphics_text_destroy(&editor->mass_field);
    rohr_graphics_text_destroy(&editor->friction_field);
    rohr_graphics_text_destroy(&editor->restitution_field);
    rohr_graphics_text_destroy(&editor->name_field);
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->initial_motion_section);
    editor_mode_accordion_section_destroy(&editor->physics_section);
    editor_mode_accordion_section_destroy(&editor->material_section);
    editor_mode_accordion_section_destroy(&editor->collision_section);
    editor_mode_accordion_section_destroy(&editor->parenting_section);
    editor_mode_accordion_section_destroy(&editor->appearance_section);
    editor_mode_accordion_section_destroy(&editor->geometry_section);
    for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->parent_names[i]);
    *editor = (EditorParticleEditor){0};
}

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

static UIFieldResult number_field(const char *id, TextAsset *label, TextAsset *field,
        float *value, const EditorModeContext *context, float y, float label_width) {
    UIButtonStyle style = editor_mode_section_field_style_get();
    rohr_ui_label(label, (UIRect){context->x + 8, y, label_width, 26});
    return editor_mode_field(id, (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value},
        field, (UIRect){context->x + label_width + 10, y,
            context->width - label_width - 20, 26}, &style);
}

bool editor_particle_editor_draw(EditorParticleEditor *editor,
        const EditorModeContext *context) {
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    EditorObject *object = editor_project_selected_get(context->project);
    EditorRigidBody *body = object == NULL ? NULL : editor_project_rigid_body_get(
        object, context->viewport->selected_rigid_body);
    if(body == NULL || !body->particle) return false;
    float x = context->x, width = context->width;
    char name[EDITOR_OBJECT_NAME_MAX];
    UIFieldResult name_result;
    EditorSelectionRef target = {EDITOR_SELECTION_PARTICLE, object->id, 0, 0, body->id};
    editor_collision_controls_selection_set(&editor->collision, &target, 1);
    snprintf(name, sizeof(name), "%s", body->name);
    if(!editor_mode_named_text_sync(editor->font, body->name,
            &editor->name_field, editor->name_cache,
            EDITOR_OBJECT_NAME_MAX)) return false;
    rohr_ui_label(&editor->name_label, (UIRect){x + 40.0f, 42.0f, 48.0f, 30.0f});
    name_result = editor_mode_name_field("editor.particle.name", name,
        sizeof(name), &editor->name_field,
        (UIRect){x + 88.0f, 42.0f, width - 96.0f, 30.0f});
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = object->id, .item = body->id}};
        snprintf(command.data.item_rename.name,
            sizeof(command.data.item_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
        return true;
    }
    {
        bool visible = body->visible;
        if(editor_mode_visibility_field("editor.particle.visibility",
                &editor->visibility_label, &editor->visible_label,
                &editor->hidden_label,
                (UIRect){x + 10.0f, 80.0f, width - 20.0f, 28.0f}, &visible)) {
            EditorCommand command = {.type = EDITOR_COMMAND_VISIBILITY,
                .data.visibility = {EDITOR_VISIBILITY_RIGID_BODY, object->id, 0,
                    body->id, visible}};
            (void)editor_command_execute(context->project, &command);
            return true;
        }
    }
    bool field_active = name_result.active;
    float layer_height = context->layer_control == NULL ? 0.0f :
        body->graphics_layer.layer == 0 || context->layer_control->adding ||
        context->layer_control->edited_layer != 0 ? 86.0f : 48.0f;
    const float transform_rows[] = {26, 26, layer_height};
    const float motion_rows[] = {26, 26, 26, 26};
    const float physics_rows[] = {28, 26, 28, EDITOR_COM_ROW_COUNT * EDITOR_COM_ROW_HEIGHT};
    const float material_rows[] = {26, 26};
    const float collision_rows[] = {editor_collision_controls_height_get(
        &editor->collision, context->project, true)};
    const float parenting_rows[] = {28};
    const float appearance_rows[] = {26, 26};
    const float geometry_rows[] = {26, 26, 26, 26};
    EditorModeAccordionLayoutCursor cursor = editor_mode_accordion_layout_cursor_get(x, width, 118);
    EditorModeAccordionLayoutResult transform_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->transform_section, "editor.particle.section.transform",
        transform_rows, context->layer_control == NULL ? 2 : 3, 6.0f);
    EditorModeAccordionLayoutResult initial_motion_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->initial_motion_section, "editor.particle.section.initial_motion",
        motion_rows, 4, 6.0f);
    EditorModeAccordionLayoutResult physics_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->physics_section, "editor.particle.section.physics",
        physics_rows, 4, 6.0f);
    EditorModeAccordionLayoutResult material_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->material_section, "editor.particle.section.material",
        material_rows, 2, 6.0f);
    EditorModeAccordionLayoutResult collision_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->collision_section, "editor.particle.section.collision",
        collision_rows, 1, 6.0f);
    EditorModeAccordionLayoutResult parenting_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->parenting_section, "editor.particle.section.parenting",
        parenting_rows, 1, 6.0f);
    EditorModeAccordionLayoutResult appearance_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->appearance_section, "editor.particle.section.appearance",
        appearance_rows, 2, 6.0f);
    EditorModeAccordionLayoutResult geometry_layout = editor_mode_accordion_layout_section(
        &cursor, &editor->geometry_section, "editor.particle.section.geometry",
        geometry_rows, 4, 6.0f);
    if(transform_layout.expanded) {
        Position position = body->position;
        UIFieldResult xr = number_field("editor.particle.x", &editor->x_label,
            &editor->x_field, &position.x, context, transform_layout.content_y, 24);
        UIFieldResult yr = number_field("editor.particle.y", &editor->y_label,
            &editor->y_field, &position.y, context, transform_layout.content_y + 32, 24);
        if(xr.changed || yr.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_RIGID_BODY_TRANSFORM,
                .data.rigid_body_transform = {object->id, body->id, position, body->rotation}};
            (void)editor_command_execute(context->project, &command);
            return true;
        }
        field_active |= xr.active || yr.active;
        if(context->layer_control != NULL)
            field_active |= editor_mode_layer_control_draw(context->layer_control,
                "editor.particle", context->project, &body->graphics_layer, NULL,
                x, transform_layout.content_y + 64, width);
    }
    bool initial_motion_open = initial_motion_layout.expanded;
    float initial_motion_row_y[4];
    for(size_t i = 0; i < 4; i += 1)
        initial_motion_row_y[i] = editor_mode_accordion_layout_row_y(
            &initial_motion_layout, motion_rows, i, 6);
    if(initial_motion_open) {
        TextAsset *labels[] = {&editor->velocity_x_label,
            &editor->velocity_y_label, &editor->acceleration_x_label,
            &editor->acceleration_y_label};
        TextAsset *fields[] = {&editor->velocity_x_field,
            &editor->velocity_y_field, &editor->acceleration_x_field,
            &editor->acceleration_y_field};
        const char *ids[] = {"editor.particle.initial_velocity_x",
            "editor.particle.initial_velocity_y",
            "editor.particle.initial_acceleration_x",
            "editor.particle.initial_acceleration_y"};
        EditorPropertyKind properties[] = {EDITOR_PROPERTY_INITIAL_VELOCITY_X,
            EDITOR_PROPERTY_INITIAL_VELOCITY_Y,
            EDITOR_PROPERTY_INITIAL_ACCELERATION_X,
            EDITOR_PROPERTY_INITIAL_ACCELERATION_Y};
        float values[] = {body->initial_velocity.x, body->initial_velocity.y,
            body->initial_acceleration.x, body->initial_acceleration.y};
        UIButtonStyle style = editor_mode_section_field_style_get();
        for(size_t i = 0; i < 4; i += 1) {
            float row = initial_motion_row_y[i];
            UIFieldResult result;
            rohr_ui_label(labels[i], (UIRect){x + 8.0f, row, 112.0f, 26.0f});
            result = editor_mode_field(ids[i],
                (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &values[i]},
                fields[i], (UIRect){x + 122.0f, row, width - 132.0f, 26.0f},
                &style);
            if(result.changed) {
                property_float_set(context->project, object->id, body->id, properties[i], values[i]);
                return true;
            }
            field_active = field_active || result.active;
        }
    }
    if(physics_layout.expanded) {
        float row = physics_layout.content_y;
        const TextAsset *options[] = {&editor->dynamic_label, &editor->static_label};
        UIButtonStyle style = editor_mode_section_field_style_get();
        UIDropdownResult motion = editor_mode_dropdown("editor.particle.motion", options,
            2, body->static_body ? 1 : 0, (UIRect){x + 10, row, width - 20, 28}, &style);
        if(motion.changed) {
            property_bool_set(context->project, object->id, body->id,
                EDITOR_PROPERTY_STATIC, motion.selected_index == 1);
            return true;
        }
        float value = body->mass_value;
        UIFieldResult result = number_field("editor.particle.mass", &editor->mass_label,
            &editor->mass_field, &value, context, row + 34, 76);
        if(result.changed) {
            property_float_set(context->project, object->id, body->id, EDITOR_PROPERTY_MASS, value);
            return true;
        }
        field_active |= result.active;
        bool gravity = body->gravity_enabled;
        if(editor_mode_checkbox_left("editor.particle.gravity", &editor->gravity_label,
                (UIRect){x + 10, row + 66, width - 20, 28}, &gravity)) {
            property_bool_set(context->project, object->id, body->id, EDITOR_PROPERTY_GRAVITY, gravity);
            return true;
        }
        field_active |= editor_center_of_mass_editor_draw(&editor->center_of_mass,
            context, object, body, row + 100);
    }
    if(material_layout.expanded) {
        float values[] = {body->friction, body->restitution};
        TextAsset *labels[] = {&editor->friction_label, &editor->restitution_label};
        TextAsset *fields[] = {&editor->friction_field, &editor->restitution_field};
        const char *ids[] = {"editor.particle.friction", "editor.particle.restitution"};
        EditorPropertyKind properties[] = {EDITOR_PROPERTY_FRICTION, EDITOR_PROPERTY_RESTITUTION};
        for(size_t i = 0; i < 2; i += 1) {
            UIFieldResult result = number_field(ids[i], labels[i], fields[i], &values[i],
                context, material_layout.content_y + i * 32, 96);
            if(result.changed) {
                property_float_set(context->project, object->id, body->id, properties[i], values[i]);
                return true;
            }
            field_active |= result.active;
        }
    }
    if(collision_layout.expanded) {
        EditorCollisionDrawResult result = editor_collision_controls_draw(&editor->collision,
            "editor.particle.collision", context->project, context->history, &target, 1,
            x + 10, collision_layout.content_y, width - 20, true);
        if(result.changed) return true;
        field_active |= result.active;
    }
    bool parenting_open = parenting_layout.expanded;
    float parenting_y = parenting_layout.content_y;
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
        UIDropdownResult result = editor_mode_dropdown("editor.particle.parent",
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
            return true;
        }
    }
    if(appearance_layout.expanded) {
        TextAsset *labels[] = {&editor->ring_color_label, &editor->fill_color_label};
        uint32_t *colors[] = {&body->particle_ring_color, &body->particle_fill_color};
        const char *ids[] = {"editor.particle.ring_color", "editor.particle.fill_color"};
        EditorPropertyKind properties[] = {EDITOR_PROPERTY_PARTICLE_RING_COLOR, EDITOR_PROPERTY_PARTICLE_FILL_COLOR};
        for(size_t i = 0; i < 2; i += 1) {
            float row = appearance_layout.content_y + i * 32;
            rohr_ui_label(labels[i], (UIRect){x + 8, row, 104, 26});
            (void)editor_mode_color_swatch(ids[i], colors[i], false,
                (UIRect){x + 114, row, width - 124, 26}, context,
                EDITOR_ITEM_RIGID_BODY, object->id, 0, body->id, properties[i]);
        }
    }
    if(geometry_layout.expanded) {
        float geometry_y = geometry_layout.content_y;
        UIFieldResult radius = {0}, rigid_vertices = {0}, origin_x = {0}, origin_y = {0};
        rohr_ui_label(&editor->radius_label,
            (UIRect){context->x + 8.0f, geometry_y, 52.0f, 26.0f});
        float radius_value = body->particle_radius;
        if(body->particle_auto_fit) {
            editor_mode_numeric_disabled_draw(&editor->radius_field,
                body->particle_radius, (UIRect){context->x + 62.0f, geometry_y,
                    fmaxf(30.0f, context->width - 166.0f), 26.0f});
        } else {
            radius = editor_mode_field("editor.particle.radius",
                (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                    .number = &radius_value}, &editor->radius_field,
                (UIRect){context->x + 62.0f, geometry_y,
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
            return true;
        }
        bool auto_fit = body->particle_auto_fit;
        if(editor_mode_checkbox_left("editor.particle.auto_fit", &editor->auto_fit_label,
                (UIRect){context->x + context->width - 100.0f, geometry_y, 90.0f, 26.0f},
                &auto_fit)) {
            property_bool_set(context->project, object->id, body->id,
                EDITOR_PROPERTY_PARTICLE_AUTO_FIT, auto_fit);
            return true;
        }
        rohr_ui_label(&editor->rigid_vertices_label,
            (UIRect){context->x + 8.0f, geometry_y + 32.0f, 110.0f, 26.0f});
        float rigid_vertices_value = (float)body->particle_rigid_vertices;
        rigid_vertices = editor_mode_field("editor.particle.rigid_vertices",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &rigid_vertices_value}, &editor->rigid_vertices_field,
            (UIRect){context->x + 120.0f, geometry_y + 32.0f,
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
            return true;
        }
        rohr_ui_label(&editor->origin_x_label,
            (UIRect){context->x + 8.0f, geometry_y + 64.0f, 72.0f, 26.0f});
        float origin_x_value = body->particle_origin.x;
        origin_x = editor_mode_field("editor.particle.origin_x",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &origin_x_value}, &editor->origin_x_field,
            (UIRect){context->x + 82.0f, geometry_y + 64.0f,
                context->width - 92.0f, 26.0f}, NULL);
        if(origin_x.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
                .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                    .object = object->id, .item = body->id,
                    .property = EDITOR_PROPERTY_PARTICLE_ORIGIN_X,
                    .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                    .value.number = origin_x_value}};
            (void)editor_command_execute(context->project, &command);
            return true;
        }
        rohr_ui_label(&editor->origin_y_label,
            (UIRect){context->x + 8.0f, geometry_y + 96.0f, 72.0f, 26.0f});
        float origin_y_value = body->particle_origin.y;
        origin_y = editor_mode_field("editor.particle.origin_y",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &origin_y_value}, &editor->origin_y_field,
            (UIRect){context->x + 82.0f, geometry_y + 96.0f,
                context->width - 92.0f, 26.0f}, NULL);
        if(origin_y.changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
                .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
                    .object = object->id, .item = body->id,
                    .property = EDITOR_PROPERTY_PARTICLE_ORIGIN_Y,
                    .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
                    .value.number = origin_y_value}};
            (void)editor_command_execute(context->project, &command);
            return true;
        }
        field_active |= radius.active || rigid_vertices.active || origin_x.active || origin_y.active;
    }
    return field_active;
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
