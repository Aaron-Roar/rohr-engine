/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_project.h"
#include "editor_array.h"

#include "yyjson.h"

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static yyjson_mut_val *editor_json_position_write(yyjson_mut_doc *document,
    Position position) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_obj_add_real(document, value, "x", position.x);
    yyjson_mut_obj_add_real(document, value, "y", position.y);
    return value;
}

static bool editor_json_position_read(yyjson_val *value, Position *position) {
    yyjson_val *x;
    yyjson_val *y;
    if(!yyjson_is_obj(value) || position == NULL) return false;
    x = yyjson_obj_get(value, "x");
    y = yyjson_obj_get(value, "y");
    if(!yyjson_is_num(x) || !yyjson_is_num(y)) return false;
    *position = (Position){(float)yyjson_get_num(x), (float)yyjson_get_num(y)};
    return true;
}

static bool editor_json_uint(yyjson_val *object, const char *key, uint32_t *value) {
    if(!yyjson_is_obj(object) || key == NULL || value == NULL) return false;
    yyjson_val *item = yyjson_obj_get(object, key);
    if(!yyjson_is_uint(item) || yyjson_get_uint(item) > UINT32_MAX) {
        return false;
    }
    *value = (uint32_t)yyjson_get_uint(item);
    return true;
}

static bool editor_json_uint64(yyjson_val *object, const char *key, uint64_t *value) {
    yyjson_val *item = yyjson_obj_get(object, key);
    if(!yyjson_is_uint(item) || value == NULL) return false;
    *value = yyjson_get_uint(item);
    return true;
}

static bool editor_json_real(yyjson_val *object, const char *key, float *value) {
    yyjson_val *item = yyjson_obj_get(object, key);
    if(!yyjson_is_num(item) || value == NULL) return false;
    *value = (float)yyjson_get_num(item);
    return true;
}

static bool editor_json_int(yyjson_val *object, const char *key, int *value) {
    yyjson_val *item = yyjson_obj_get(object, key);
    int64_t number;
    if(!yyjson_is_int(item) || value == NULL) return false;
    number = yyjson_get_sint(item);
    if(number < INT_MIN || number > INT_MAX) return false;
    *value = (int)number;
    return true;
}

static bool editor_json_optional_real(yyjson_val *object, const char *key,
        float *value, float fallback) {
    yyjson_val *item;
    if(object == NULL || key == NULL || value == NULL) return false;
    item = yyjson_obj_get(object, key);
    if(item == NULL) {
        *value = fallback;
        return true;
    }
    if(!yyjson_is_num(item)) return false;
    *value = (float)yyjson_get_num(item);
    return true;
}

static bool editor_json_bool(yyjson_val *object, const char *key, bool *value) {
    yyjson_val *item = yyjson_obj_get(object, key);
    if(!yyjson_is_bool(item) || value == NULL) return false;
    *value = yyjson_get_bool(item);
    return true;
}

static bool editor_json_name(yyjson_val *object, char name[EDITOR_OBJECT_NAME_MAX]) {
    yyjson_val *item = yyjson_obj_get(object, "name");
    size_t length;
    if(!yyjson_is_str(item) || name == NULL) return false;
    length = yyjson_get_len(item);
    if(length == 0 || length >= EDITOR_OBJECT_NAME_MAX) return false;
    memcpy(name, yyjson_get_str(item), length + 1);
    return true;
}

static void editor_json_graphics_layer_binding_write(yyjson_mut_doc *document,
        yyjson_mut_val *value, EditorGraphicsLayerBinding binding) {
    yyjson_mut_obj_add_sint(document, value, "graphics_layer_value", binding.value);
    yyjson_mut_obj_add_uint(document, value, "graphics_layer", binding.layer);
}

static bool editor_json_graphics_layer_binding_read(yyjson_val *value,
        EditorGraphicsLayerBinding *binding) {
    yyjson_val *direct;
    yyjson_val *layer;
    if(value == NULL || binding == NULL) return false;
    direct = yyjson_obj_get(value, "graphics_layer_value");
    layer = yyjson_obj_get(value, "graphics_layer");
    if((direct != NULL && (!yyjson_is_int(direct) ||
                yyjson_get_sint(direct) < INT_MIN ||
                yyjson_get_sint(direct) > INT_MAX)) ||
            (layer != NULL && (!yyjson_is_uint(layer) ||
                yyjson_get_uint(layer) > UINT32_MAX))) return false;
    binding->value = direct == NULL ? 0 : (int)yyjson_get_sint(direct);
    binding->layer = layer == NULL ? 0 :
        (EditorGraphicsLayerId)yyjson_get_uint(layer);
    return true;
}

static yyjson_mut_val *editor_json_hitbox_write(yyjson_mut_doc *document,
    const EditorHitbox *hitbox) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_val *vertices = yyjson_mut_arr(document);
    yyjson_mut_val *lines = yyjson_mut_arr(document);
    yyjson_mut_obj_add_uint(document, value, "id", hitbox->id);
    yyjson_mut_obj_add_strcpy(document, value, "name", hitbox->name);
    yyjson_mut_obj_add_bool(document, value, "visible", hitbox->visible);
    for(size_t i = 0; i < hitbox->vertex_count; i += 1) {
        yyjson_mut_val *vertex = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, vertex, "id", hitbox->vertices[i].id);
        yyjson_mut_obj_add_strcpy(document, vertex, "name", hitbox->vertices[i].name);
        yyjson_mut_obj_add_val(document, vertex, "position",
            editor_json_position_write(document, hitbox->vertices[i].position));
        yyjson_mut_obj_add_bool(document, vertex, "position_locked",
            hitbox->vertices[i].position_locked);
        yyjson_mut_arr_add_val(vertices, vertex);
        yyjson_mut_arr_add_strcpy(document, lines, hitbox->line_names[i]);
    }
    yyjson_mut_obj_add_val(document, value, "vertices", vertices);
    yyjson_mut_obj_add_val(document, value, "lines", lines);
    return value;
}

static yyjson_mut_val *editor_json_body_write(yyjson_mut_doc *document,
    const EditorRigidBody *body) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_val *hitboxes = yyjson_mut_arr(document);
    yyjson_mut_val *bindings = yyjson_mut_arr(document);
    float particle_radius = body->particle_auto_fit ?
        editor_project_particle_auto_radius_get(body) : body->particle_radius;
    yyjson_mut_obj_add_uint(document, value, "id", body->id);
    yyjson_mut_obj_add_uint(document, value, "parent", body->parent);
    yyjson_mut_obj_add_strcpy(document, value, "name", body->name);
    editor_json_graphics_layer_binding_write(document, value,
        body->graphics_layer);
    yyjson_mut_obj_add_val(document, value, "position",
        editor_json_position_write(document, body->position));
    yyjson_mut_obj_add_real(document, value, "rotation", body->rotation);
    yyjson_mut_obj_add_real(document, value, "mass", body->mass_value);
    yyjson_mut_obj_add_real(document, value, "friction", body->friction);
    yyjson_mut_obj_add_real(document, value, "restitution", body->restitution);
    yyjson_mut_obj_add_bool(document, value, "static", body->static_body);
    yyjson_mut_obj_add_bool(document, value, "rotation_locked", body->rotation_locked);
    yyjson_mut_obj_add_bool(document, value, "gravity_enabled", body->gravity_enabled);
    yyjson_mut_obj_add_bool(document, value, "collision_enabled", body->collision_enabled);
    yyjson_mut_obj_add_bool(document, value, "particle", body->particle);
    yyjson_mut_obj_add_bool(document, value, "particle_auto_fit",
        body->particle_auto_fit);
    yyjson_mut_obj_add_real(document, value, "particle_radius", particle_radius);
    yyjson_mut_obj_add_val(document, value, "particle_origin",
        editor_json_position_write(document, body->particle_origin));
    yyjson_mut_obj_add_uint(document, value, "particle_ring_color",
        body->particle_ring_color);
    yyjson_mut_obj_add_uint(document, value, "particle_fill_color",
        body->particle_fill_color);
    yyjson_mut_obj_add_uint(document, value, "collision_category",
        body->collision_category);
    yyjson_mut_obj_add_uint(document, value, "collision_with", body->collision_with);
    yyjson_mut_obj_add_bool(document, value, "visible", body->visible);
    yyjson_mut_obj_add_uint(document, value, "border_color", body->border_color);
    yyjson_mut_obj_add_uint(document, value, "surface_color", body->surface_color);
    yyjson_mut_obj_add_uint(document, value, "active_hitbox_index",
        body->active_hitbox_index);
    for(size_t i = 0; i < body->hitbox_count; i += 1) {
        yyjson_mut_arr_add_val(hitboxes, editor_json_hitbox_write(document,
            &body->hitboxes[i]));
    }
    yyjson_mut_obj_add_val(document, value, "hitboxes", hitboxes);
    for(size_t i = 0; i < body->hitbox_animation_binding_count; i += 1) {
        const EditorHitboxAnimationBinding *binding =
            &body->hitbox_animation_bindings[i];
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "animation", binding->animation);
        yyjson_mut_obj_add_uint(document, item, "frame", binding->frame);
        yyjson_mut_obj_add_uint(document, item, "hitbox", binding->hitbox);
        yyjson_mut_arr_add_val(bindings, item);
    }
    yyjson_mut_obj_add_val(document, value, "hitbox_animation_bindings", bindings);
    return value;
}

static yyjson_mut_val *editor_json_anchor_write(yyjson_mut_doc *document,
    const EditorAnchor *anchor) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_obj_add_uint(document, value, "id", anchor->id);
    yyjson_mut_obj_add_strcpy(document, value, "name", anchor->name);
    yyjson_mut_obj_add_val(document, value, "position",
        editor_json_position_write(document, anchor->position));
    yyjson_mut_obj_add_real(document, value, "rotation", anchor->rotation);
    yyjson_mut_obj_add_uint(document, value, "rigid_body", anchor->rigid_body);
    yyjson_mut_obj_add_uint(document, value, "attachment_kind",
        anchor->attachment_kind);
    yyjson_mut_obj_add_uint(document, value, "attachment_soft_body",
        anchor->attachment_soft_body);
    yyjson_mut_obj_add_uint(document, value, "attachment_soft_node",
        anchor->attachment_soft_node);
    yyjson_mut_obj_add_bool(document, value, "position_follows_body",
        anchor->position_follows_body);
    yyjson_mut_obj_add_bool(document, value, "rotation_follows_body",
        anchor->rotation_follows_body);
    yyjson_mut_obj_add_bool(document, value, "visible", anchor->visible);
    return value;
}

static yyjson_mut_val *editor_json_joint_write(yyjson_mut_doc *document,
    const EditorJoint *joint) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_obj_add_uint(document, value, "id", joint->id);
    yyjson_mut_obj_add_strcpy(document, value, "name", joint->name);
    editor_json_graphics_layer_binding_write(document, value,
        joint->graphics_layer);
    yyjson_mut_obj_add_uint(document, value, "kind", (uint32_t)joint->kind);
    yyjson_mut_obj_add_uint(document, value, "anchor_a", joint->anchor_a);
    yyjson_mut_obj_add_uint(document, value, "anchor_b", joint->anchor_b);
    yyjson_mut_obj_add_real(document, value, "rest_length", joint->rest_length);
    yyjson_mut_obj_add_real(document, value, "stiffness", joint->stiffness);
    yyjson_mut_obj_add_real(document, value, "damping", joint->damping);
    yyjson_mut_obj_add_real(document, value, "rest_angle", joint->rest_angle);
    yyjson_mut_obj_add_real(document, value, "visual_size", joint->visual_size);
    yyjson_mut_obj_add_bool(document, value, "visible", joint->visible);
    return value;
}

static yyjson_mut_val *editor_json_soft_body_write(yyjson_mut_doc *document,
    const EditorSoftBody *body) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_val *nodes = yyjson_mut_arr(document);
    yyjson_mut_val *beams = yyjson_mut_arr(document);
    yyjson_mut_val *areas = yyjson_mut_arr(document);
    yyjson_mut_val *hierarchy = yyjson_mut_arr(document);
    yyjson_mut_obj_add_uint(document, value, "id", body->id);
    yyjson_mut_obj_add_strcpy(document, value, "name", body->name);
    editor_json_graphics_layer_binding_write(document, value,
        body->graphics_layer);
    yyjson_mut_obj_add_val(document, value, "position",
        editor_json_position_write(document, body->position));
    yyjson_mut_obj_add_real(document, value, "rotation", body->rotation);
    yyjson_mut_obj_add_bool(document, value, "visible", body->visible);
    yyjson_mut_obj_add_uint(document, value, "node_color", body->node_color);
    yyjson_mut_obj_add_uint(document, value, "beam_color", body->beam_color);
    yyjson_mut_obj_add_uint(document, value, "area_color", body->area_color);
    for(size_t i = 0; i < body->node_count; i += 1) {
        const EditorSoftNode *node = &body->nodes[i];
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "id", node->id);
        yyjson_mut_obj_add_strcpy(document, item, "name", node->name);
        editor_json_graphics_layer_binding_write(document, item,
            node->graphics_layer);
        yyjson_mut_obj_add_bool(document, item, "graphics_layer_inherited",
            node->graphics_layer_inherited);
        yyjson_mut_obj_add_val(document, item, "position",
            editor_json_position_write(document, node->position));
        yyjson_mut_obj_add_real(document, item, "mass", node->node_mass);
        yyjson_mut_obj_add_real(document, item, "radius", node->radius);
        yyjson_mut_obj_add_real(document, item, "friction", node->friction);
        yyjson_mut_obj_add_real(document, item, "restitution", node->restitution);
        yyjson_mut_obj_add_bool(document, item, "gravity_enabled", node->gravity_enabled);
        yyjson_mut_obj_add_bool(document, item, "collision_enabled", node->collision_enabled);
        yyjson_mut_obj_add_uint(document, item, "collision_category", node->collision_category);
        yyjson_mut_obj_add_uint(document, item, "collision_with", node->collision_with);
        yyjson_mut_obj_add_bool(document, item, "visible", node->visible);
        yyjson_mut_obj_add_uint(document, item, "color", node->color);
        yyjson_mut_obj_add_bool(document, item, "color_overridden", node->color_overridden);
        yyjson_mut_arr_add_val(nodes, item);
    }
    for(size_t i = 0; i < body->beam_count; i += 1) {
        const EditorSoftBeam *beam = &body->beams[i];
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "id", beam->id);
        yyjson_mut_obj_add_strcpy(document, item, "name", beam->name);
        editor_json_graphics_layer_binding_write(document, item,
            beam->graphics_layer);
        yyjson_mut_obj_add_bool(document, item, "graphics_layer_inherited",
            beam->graphics_layer_inherited);
        yyjson_mut_obj_add_uint(document, item, "node_a", beam->node_a);
        yyjson_mut_obj_add_uint(document, item, "node_b", beam->node_b);
        yyjson_mut_obj_add_real(document, item, "stiffness", beam->stiffness);
        yyjson_mut_obj_add_real(document, item, "damping", beam->damping);
        yyjson_mut_obj_add_bool(document, item, "visible", beam->visible);
        yyjson_mut_obj_add_uint(document, item, "color", beam->color);
        yyjson_mut_obj_add_bool(document, item, "color_overridden", beam->color_overridden);
        yyjson_mut_arr_add_val(beams, item);
    }
    for(size_t i = 0; i < body->area_count; i += 1) {
        const EditorSoftArea *area = &body->areas[i];
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_val *area_nodes = yyjson_mut_arr(document);
        yyjson_mut_obj_add_uint(document, item, "id", area->id);
        yyjson_mut_obj_add_strcpy(document, item, "name", area->name);
        editor_json_graphics_layer_binding_write(document, item,
            area->graphics_layer);
        yyjson_mut_obj_add_bool(document, item, "graphics_layer_inherited",
            area->graphics_layer_inherited);
        for(size_t node_index = 0; node_index < area->node_count; node_index += 1)
            yyjson_mut_arr_add_uint(document, area_nodes, area->nodes[node_index]);
        yyjson_mut_obj_add_val(document, item, "nodes", area_nodes);
        yyjson_mut_obj_add_uint(document, item, "color", area->color);
        yyjson_mut_obj_add_bool(document, item, "color_overridden", area->color_overridden);
        yyjson_mut_obj_add_bool(document, item, "visible", area->visible);
        yyjson_mut_arr_add_val(areas, item);
    }
    for(size_t i = 0; i < body->hierarchy_count; i += 1) {
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "kind", body->hierarchy[i].kind);
        yyjson_mut_obj_add_uint(document, item, "id", body->hierarchy[i].id);
        yyjson_mut_arr_add_val(hierarchy, item);
    }
    yyjson_mut_obj_add_val(document, value, "nodes", nodes);
    yyjson_mut_obj_add_val(document, value, "beams", beams);
    yyjson_mut_obj_add_val(document, value, "areas", areas);
    yyjson_mut_obj_add_val(document, value, "hierarchy", hierarchy);
    return value;
}

static yyjson_mut_val *editor_json_animated_sprite_write(yyjson_mut_doc *document,
        const EditorAnimatedSprite *sprite) {
    yyjson_mut_val *value = yyjson_mut_obj(document);
    yyjson_mut_val *frames = yyjson_mut_arr(document);
    yyjson_mut_obj_add_uint(document, value, "id", sprite->id);
    yyjson_mut_obj_add_strcpy(document, value, "name", sprite->name);
    editor_json_graphics_layer_binding_write(document, value,
        sprite->graphics_layer);
    yyjson_mut_obj_add_uint(document, value, "rigid_body", sprite->rigid_body);
    yyjson_mut_obj_add_val(document, value, "editor_position",
        editor_json_position_write(document, sprite->editor_position));
    yyjson_mut_obj_add_real(document, value, "editor_rotation",
        sprite->editor_rotation);
    for(size_t i = 0; i < sprite->frame_count; i += 1) {
        const EditorAnimationFrame *frame = &sprite->frames[i];
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "id", frame->id);
        yyjson_mut_obj_add_strcpy(document, item, "name", frame->name);
        yyjson_mut_obj_add_strcpy(document, item, "path", frame->path);
        yyjson_mut_obj_add_real(document, item, "width", frame->size.x);
        yyjson_mut_obj_add_real(document, item, "height", frame->size.y);
        yyjson_mut_arr_add_val(frames, item);
    }
    yyjson_mut_obj_add_val(document, value, "frames", frames);
    yyjson_mut_obj_add_uint(document, value, "ticks_per_frame",
        sprite->ticks_per_frame);
    yyjson_mut_obj_add_real(document, value, "time_per_frame",
        sprite->time_per_frame);
    yyjson_mut_obj_add_uint(document, value, "starting_frame",
        sprite->starting_frame);
    yyjson_mut_obj_add_real(document, value, "scale_x", sprite->scale.x);
    yyjson_mut_obj_add_real(document, value, "scale_y", sprite->scale.y);
    yyjson_mut_obj_add_uint(document, value, "direction", sprite->direction);
    yyjson_mut_obj_add_bool(document, value, "follow_body_rotation",
        sprite->follow_body_rotation);
    yyjson_mut_obj_add_bool(document, value, "visible", sprite->visible);
    yyjson_mut_obj_add_bool(document, value, "playing", sprite->playing);
    return value;
}

static yyjson_mut_val *editor_json_ui_definition_write(yyjson_mut_doc *document,
        const EditorViewportUiDefinition *ui) {
    yyjson_mut_val *item = yyjson_mut_obj(document);
    yyjson_mut_obj_add_uint(document, item, "id", ui->id);
    yyjson_mut_obj_add_uint(document, item, "kind", ui->kind);
    yyjson_mut_obj_add_strcpy(document, item, "name", ui->name);
    yyjson_mut_obj_add_bool(document, item, "border_enabled", ui->border_enabled);
    yyjson_mut_obj_add_uint(document, item, "border_type", ui->border_type);
    yyjson_mut_obj_add_real(document, item, "border_thickness", ui->border_thickness);
    yyjson_mut_obj_add_real(document, item, "border_hash_spacing",
        ui->border_hash_spacing);
    yyjson_mut_obj_add_real(document, item, "border_corner_radius",
        ui->border_corner_radius);
    yyjson_mut_obj_add_uint(document, item, "border_color", ui->border_color);
    yyjson_mut_obj_add_uint(document, item, "ui_fill_color", ui->fill_color);
    yyjson_mut_obj_add_uint(document, item, "hover_border_color",
        ui->hover_border_color);
    yyjson_mut_obj_add_uint(document, item, "hover_fill_color", ui->hover_fill_color);
    yyjson_mut_obj_add_uint(document, item, "click_border_color",
        ui->click_border_color);
    yyjson_mut_obj_add_uint(document, item, "click_fill_color", ui->click_fill_color);
    if(ui->kind == EDITOR_VIEWPORT_UI_SHAPE) {
        yyjson_mut_val *vertices = yyjson_mut_arr(document);
        for(size_t vertex = 0; vertex < ui->value.shape.vertex_count; vertex += 1)
            yyjson_mut_arr_add_val(vertices, editor_json_position_write(document,
                ui->value.shape.vertices[vertex]));
        yyjson_mut_obj_add_val(document, item, "vertices", vertices);
        yyjson_mut_obj_add_real(document, item, "ui_rotation",
            ui->value.shape.rotation);
        yyjson_mut_obj_add_uint(document, item, "outline_color",
            ui->value.shape.outline_color);
        yyjson_mut_obj_add_uint(document, item, "fill_color",
            ui->value.shape.fill_color);
        yyjson_mut_obj_add_bool(document, item, "button_enabled",
            ui->value.shape.button_enabled);
        yyjson_mut_obj_add_strcpy(document, item, "text",
            ui->value.shape.text.text);
        yyjson_mut_obj_add_uint(document, item, "font", ui->value.shape.text.font);
        yyjson_mut_obj_add_real(document, item, "text_offset_x",
            ui->value.shape.text.offset.x);
        yyjson_mut_obj_add_real(document, item, "text_offset_y",
            ui->value.shape.text.offset.y);
        yyjson_mut_obj_add_uint(document, item, "color", ui->value.shape.text.color);
        yyjson_mut_obj_add_real(document, item, "box_width",
            ui->value.shape.text.box_width);
        yyjson_mut_obj_add_real(document, item, "box_height",
            ui->value.shape.text.box_height);
        yyjson_mut_obj_add_real(document, item, "width_scale",
            ui->value.shape.text.width_scale);
        yyjson_mut_obj_add_real(document, item, "height_scale",
            ui->value.shape.text.height_scale);
    } else {
        yyjson_mut_obj_add_strcpy(document, item, "text", ui->value.text.text);
        yyjson_mut_obj_add_uint(document, item, "font", ui->value.text.font);
        yyjson_mut_obj_add_real(document, item, "text_offset_x",
            ui->value.text.offset.x);
        yyjson_mut_obj_add_real(document, item, "text_offset_y",
            ui->value.text.offset.y);
        yyjson_mut_obj_add_uint(document, item, "color", ui->value.text.color);
        yyjson_mut_obj_add_real(document, item, "box_width", ui->value.text.box_width);
        yyjson_mut_obj_add_real(document, item, "box_height", ui->value.text.box_height);
        yyjson_mut_obj_add_real(document, item, "width_scale",
            ui->value.text.width_scale);
        yyjson_mut_obj_add_real(document, item, "height_scale",
            ui->value.text.height_scale);
    }
    return item;
}

bool editor_project_save(const EditorProject *project, const char *path) {
    yyjson_mut_doc *document;
    yyjson_mut_val *root;
    yyjson_mut_val *objects;
    yyjson_mut_val *layout_viewports;
    yyjson_mut_val *hierarchy;
    yyjson_mut_val *collision_masks;
    yyjson_mut_val *ui_fonts;
    yyjson_mut_val *graphics_layers;
    yyjson_mut_val *ui_definitions;
    bool success;
    if(project == NULL || path == NULL || path[0] == '\0') return false;
    document = yyjson_mut_doc_new(NULL);
    if(document == NULL) return false;
    root = yyjson_mut_obj(document);
    objects = yyjson_mut_arr(document);
    layout_viewports = yyjson_mut_arr(document);
    hierarchy = yyjson_mut_arr(document);
    collision_masks = yyjson_mut_arr(document);
    ui_fonts = yyjson_mut_arr(document);
    graphics_layers = yyjson_mut_arr(document);
    ui_definitions = yyjson_mut_arr(document);
    yyjson_mut_doc_set_root(document, root);
    yyjson_mut_obj_add_uint(document, root, "format_version", EDITOR_PROJECT_FORMAT_VERSION);
    yyjson_mut_obj_add_val(document, root, "viewport_camera_offset",
        editor_json_position_write(document,
            (Position){project->viewport_camera_offset.x,
                project->viewport_camera_offset.y}));
    yyjson_mut_obj_add_real(document, root, "viewport_camera_zoom",
        project->viewport_camera_zoom);
    yyjson_mut_obj_add_bool(document, root, "viewport_local_view",
        project->viewport_local_view);
    {
        const EditorNavigationState *navigation = &project->navigation;
        yyjson_mut_val *value = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, value, "mode", navigation->mode);
        yyjson_mut_obj_add_uint(document, value, "selection", navigation->selection);
        yyjson_mut_obj_add_uint(document, value, "object", navigation->object);
        yyjson_mut_obj_add_uint(document, value, "line", navigation->selected_line);
        yyjson_mut_obj_add_uint(document, value, "vertex", navigation->selected_vertex);
        yyjson_mut_obj_add_uint(document, value, "rigid_body", navigation->rigid_body);
        yyjson_mut_obj_add_uint(document, value, "hitbox", navigation->hitbox);
        yyjson_mut_obj_add_uint(document, value, "joint", navigation->joint);
        yyjson_mut_obj_add_uint(document, value, "anchor", navigation->anchor);
        yyjson_mut_obj_add_uint(document, value, "soft_body", navigation->soft_body);
        yyjson_mut_obj_add_uint(document, value, "soft_node", navigation->soft_node);
        yyjson_mut_obj_add_uint(document, value, "soft_beam", navigation->soft_beam);
        yyjson_mut_obj_add_uint(document, value, "sprite", navigation->sprite);
        yyjson_mut_obj_add_uint(document, value, "animated_sprite",
            navigation->animated_sprite);
        yyjson_mut_obj_add_uint(document, value, "animation_frame",
            navigation->animation_frame);
        yyjson_mut_obj_add_uint(document, value, "camera", navigation->camera);
        yyjson_mut_obj_add_uint(document, value, "origin_kind", navigation->origin_kind);
        yyjson_mut_obj_add_val(document, root, "navigation", value);
    }
    yyjson_mut_obj_add_uint(document, root, "selected", project->selected);
    yyjson_mut_obj_add_uint(document, root, "next_object_id", project->next_id);
    yyjson_mut_obj_add_uint(document, root, "next_vertex_id", project->next_vertex_id);
    yyjson_mut_obj_add_uint(document, root, "next_rigid_body_id", project->next_rigid_body_id);
    yyjson_mut_obj_add_uint(document, root, "next_hitbox_id", project->next_hitbox_id);
    yyjson_mut_obj_add_uint(document, root, "next_joint_id", project->next_joint_id);
    yyjson_mut_obj_add_uint(document, root, "next_anchor_id", project->next_anchor_id);
    yyjson_mut_obj_add_uint(document, root, "next_soft_body_id", project->next_soft_body_id);
    yyjson_mut_obj_add_uint(document, root, "next_soft_node_id", project->next_soft_node_id);
    yyjson_mut_obj_add_uint(document, root, "next_soft_beam_id", project->next_soft_beam_id);
    yyjson_mut_obj_add_uint(document, root, "next_soft_area_id", project->next_soft_area_id);
    yyjson_mut_obj_add_uint(document, root, "next_sprite_id", project->next_sprite_id);
    yyjson_mut_obj_add_uint(document, root, "next_animated_sprite_id",
        project->next_animated_sprite_id);
    yyjson_mut_obj_add_uint(document, root, "next_camera_id",
        project->next_camera_id);
    yyjson_mut_obj_add_uint(document, root, "next_layout_viewport_id",
        project->next_layout_viewport_id);
    yyjson_mut_obj_add_uint(document, root, "next_viewport_camera_item_id",
        project->next_viewport_camera_item_id);
    yyjson_mut_obj_add_uint(document, root, "next_viewport_ui_item_id",
        project->next_viewport_ui_item_id);
    yyjson_mut_obj_add_uint(document, root, "next_ui_font_id",
        project->next_ui_font_id);
    yyjson_mut_obj_add_uint(document, root, "next_graphics_layer_id",
        project->next_graphics_layer_id);
    yyjson_mut_obj_add_uint(document, root, "next_ui_definition_id",
        project->next_ui_definition_id);
    for(size_t i = 0; i < project->ui_font_count; i += 1) {
        yyjson_mut_val *font = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, font, "id", project->ui_fonts[i].id);
        yyjson_mut_obj_add_strcpy(document, font, "name", project->ui_fonts[i].name);
        yyjson_mut_obj_add_strcpy(document, font, "path", project->ui_fonts[i].path);
        yyjson_mut_arr_add_val(ui_fonts, font);
    }
    yyjson_mut_obj_add_val(document, root, "ui_fonts", ui_fonts);
    for(size_t i = 0; i < project->graphics_layer_count; i += 1) {
        yyjson_mut_val *layer = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, layer, "id", project->graphics_layers[i].id);
        yyjson_mut_obj_add_strcpy(document, layer, "name",
            project->graphics_layers[i].name);
        yyjson_mut_obj_add_sint(document, layer, "value",
            project->graphics_layers[i].value);
        yyjson_mut_arr_add_val(graphics_layers, layer);
    }
    yyjson_mut_obj_add_val(document, root, "graphics_layers", graphics_layers);
    for(size_t i = 0; i < project->ui_definition_count; i += 1)
        yyjson_mut_arr_add_val(ui_definitions,
            editor_json_ui_definition_write(document, &project->ui_definitions[i]));
    yyjson_mut_obj_add_val(document, root, "ui_definitions", ui_definitions);
    for(size_t i = 0; i < project->collision_mask_count; i += 1) {
        yyjson_mut_arr_add_strcpy(document, collision_masks,
            project->collision_masks[i].name);
    }
    yyjson_mut_obj_add_val(document, root, "collision_masks", collision_masks);
    for(size_t i = 0; i < project->object_count; i += 1) {
        const EditorObject *object = &project->objects[i];
        yyjson_mut_val *value = yyjson_mut_obj(document);
        yyjson_mut_val *bodies = yyjson_mut_arr(document);
        yyjson_mut_val *anchors = yyjson_mut_arr(document);
        yyjson_mut_val *joint_values = yyjson_mut_arr(document);
        yyjson_mut_val *soft_body_values = yyjson_mut_arr(document);
        yyjson_mut_val *sprites = yyjson_mut_arr(document);
        yyjson_mut_val *animated_sprite_values = yyjson_mut_arr(document);
        yyjson_mut_val *camera_values = yyjson_mut_arr(document);
        yyjson_mut_val *hierarchy = yyjson_mut_arr(document);
        yyjson_mut_obj_add_uint(document, value, "id", object->id);
        yyjson_mut_obj_add_strcpy(document, value, "name", object->name);
        yyjson_mut_obj_add_val(document, value, "position",
            editor_json_position_write(document, object->position));
        yyjson_mut_obj_add_val(document, value, "overview_position",
            editor_json_position_write(document, object->overview_position));
        yyjson_mut_obj_add_bool(document, value, "visible", object->visible);
        for(size_t j = 0; j < object->rigid_body_count; j += 1)
            yyjson_mut_arr_add_val(bodies, editor_json_body_write(document,
                &object->rigid_bodies[j]));
        for(size_t j = 0; j < object->anchor_count; j += 1)
            yyjson_mut_arr_add_val(anchors, editor_json_anchor_write(document,
                &object->anchors[j]));
        for(size_t j = 0; j < object->joint_count; j += 1)
            yyjson_mut_arr_add_val(joint_values, editor_json_joint_write(document,
                &object->joint_items[j]));
        for(size_t j = 0; j < object->soft_body_count; j += 1)
            yyjson_mut_arr_add_val(soft_body_values, editor_json_soft_body_write(document,
                &object->soft_body_items[j]));
        for(size_t j = 0; j < object->sprite_count; j += 1) {
            yyjson_mut_val *sprite = yyjson_mut_obj(document);
            yyjson_mut_obj_add_uint(document, sprite, "id", object->sprites[j].id);
            yyjson_mut_obj_add_strcpy(document, sprite, "name", object->sprites[j].name);
            editor_json_graphics_layer_binding_write(document, sprite,
                object->sprites[j].graphics_layer);
            yyjson_mut_obj_add_strcpy(document, sprite, "path", object->sprites[j].path);
            yyjson_mut_obj_add_val(document, sprite, "position",
                editor_json_position_write(document, object->sprites[j].position));
            yyjson_mut_obj_add_real(document, sprite, "rotation",
                object->sprites[j].rotation);
            yyjson_mut_obj_add_uint(document, sprite, "rigid_body",
                object->sprites[j].rigid_body);
            yyjson_mut_obj_add_real(document, sprite, "width", object->sprites[j].size.x);
            yyjson_mut_obj_add_real(document, sprite, "height", object->sprites[j].size.y);
            yyjson_mut_obj_add_bool(document, sprite, "follow_body_rotation",
                object->sprites[j].follow_body_rotation);
            yyjson_mut_obj_add_bool(document, sprite, "visible",
                object->sprites[j].visible);
            yyjson_mut_arr_add_val(sprites, sprite);
        }
        for(size_t j = 0; j < object->animated_sprite_count; j += 1)
            yyjson_mut_arr_add_val(animated_sprite_values,
                editor_json_animated_sprite_write(document,
                    &object->animated_sprite_items[j]));
        for(size_t j = 0; j < object->camera_count; j += 1) {
            const EditorCamera *camera = &object->cameras[j];
            yyjson_mut_val *item = yyjson_mut_obj(document);
            yyjson_mut_obj_add_uint(document, item, "id", camera->id);
            yyjson_mut_obj_add_strcpy(document, item, "name", camera->name);
            yyjson_mut_obj_add_val(document, item, "position",
                editor_json_position_write(document, camera->position));
            yyjson_mut_obj_add_real(document, item, "rotation", camera->rotation);
            yyjson_mut_obj_add_real(document, item, "width", camera->dimensions.x);
            yyjson_mut_obj_add_real(document, item, "height", camera->dimensions.y);
            yyjson_mut_obj_add_real(document, item, "zoom", camera->zoom);
            yyjson_mut_obj_add_uint(document, item, "attachment_kind",
                camera->attachment_kind);
            yyjson_mut_obj_add_uint(document, item, "attachment", camera->attachment);
            yyjson_mut_obj_add_uint(document, item, "attachment_soft_body",
                camera->attachment_soft_body);
            yyjson_mut_obj_add_bool(document, item, "inherit_orientation",
                camera->inherit_orientation);
            yyjson_mut_obj_add_bool(document, item, "visible", camera->visible);
            yyjson_mut_arr_add_val(camera_values, item);
        }
        for(size_t j = 0; j < object->hierarchy_count; j += 1) {
            yyjson_mut_val *item = yyjson_mut_obj(document);
            yyjson_mut_obj_add_uint(document, item, "kind", object->hierarchy[j].kind);
            yyjson_mut_obj_add_uint(document, item, "id", object->hierarchy[j].id);
            yyjson_mut_arr_add_val(hierarchy, item);
        }
        yyjson_mut_obj_add_val(document, value, "rigid_bodies", bodies);
        yyjson_mut_obj_add_val(document, value, "anchors", anchors);
        yyjson_mut_obj_add_val(document, value, "joints", joint_values);
        yyjson_mut_obj_add_val(document, value, "soft_bodies", soft_body_values);
        yyjson_mut_obj_add_val(document, value, "sprites", sprites);
        yyjson_mut_obj_add_val(document, value, "animated_sprites",
            animated_sprite_values);
        yyjson_mut_obj_add_val(document, value, "cameras", camera_values);
        yyjson_mut_obj_add_val(document, value, "hierarchy", hierarchy);
        yyjson_mut_arr_add_val(objects, value);
    }
    yyjson_mut_obj_add_val(document, root, "objects", objects);
    for(size_t i = 0; i < project->layout_viewport_count; i += 1) {
        const EditorLayoutViewport *viewport = &project->layout_viewports[i];
        yyjson_mut_val *value = yyjson_mut_obj(document);
        yyjson_mut_val *camera_items = yyjson_mut_arr(document);
        yyjson_mut_val *ui_items = yyjson_mut_arr(document);
        yyjson_mut_obj_add_uint(document, value, "id", viewport->id);
        yyjson_mut_obj_add_strcpy(document, value, "name", viewport->name);
        yyjson_mut_obj_add_val(document, value, "overview_position",
            editor_json_position_write(document, viewport->overview_position));
        yyjson_mut_obj_add_real(document, value, "x", viewport->config.rectangle.x);
        yyjson_mut_obj_add_real(document, value, "y", viewport->config.rectangle.y);
        yyjson_mut_obj_add_real(document, value, "width",
            viewport->config.rectangle.width);
        yyjson_mut_obj_add_real(document, value, "height",
            viewport->config.rectangle.height);
        yyjson_mut_obj_add_uint(document, value, "fit", viewport->config.fit);
        yyjson_mut_obj_add_uint(document, value, "background_color",
            viewport->background_color);
        yyjson_mut_obj_add_bool(document, value, "enabled", viewport->enabled);
        for(size_t j = 0; j < viewport->camera_item_count; j += 1) {
            const EditorViewportCameraItem *camera = &viewport->camera_items[j];
            yyjson_mut_val *item = yyjson_mut_obj(document);
            yyjson_mut_obj_add_uint(document, item, "id", camera->id);
            yyjson_mut_obj_add_strcpy(document, item, "name", camera->name);
            yyjson_mut_obj_add_uint(document, item, "object", camera->object);
            yyjson_mut_obj_add_uint(document, item, "camera", camera->camera);
            yyjson_mut_obj_add_real(document, item, "x", camera->placement.rectangle.x);
            yyjson_mut_obj_add_real(document, item, "y", camera->placement.rectangle.y);
            yyjson_mut_obj_add_real(document, item, "width",
                camera->placement.rectangle.width);
            yyjson_mut_obj_add_real(document, item, "height",
                camera->placement.rectangle.height);
            yyjson_mut_obj_add_uint(document, item, "fit", camera->placement.fit);
            yyjson_mut_obj_add_real(document, item, "orientation",
                camera->placement.orientation);
            yyjson_mut_obj_add_sint(document, item, "layer", camera->placement.layer);
            yyjson_mut_obj_add_uint(document, item, "graphics_layer",
                camera->graphics_layer);
            yyjson_mut_obj_add_bool(document, item, "visible",
                camera->placement.visible);
            yyjson_mut_obj_add_uint(document, item, "drag_mode",
                camera->placement.drag_mode);
            yyjson_mut_obj_add_real(document, item, "content_x",
                camera->content_offset.x);
            yyjson_mut_obj_add_real(document, item, "content_y",
                camera->content_offset.y);
            yyjson_mut_obj_add_real(document, item, "content_width_scale",
                camera->content_scale.x);
            yyjson_mut_obj_add_real(document, item, "content_height_scale",
                camera->content_scale.y);
            yyjson_mut_obj_add_real(document, item, "content_rotation",
                camera->content_rotation);
            yyjson_mut_arr_add_val(camera_items, item);
        }
        yyjson_mut_obj_add_val(document, value, "screen_items", camera_items);
        for(size_t j = 0; j < viewport->ui_item_count; j += 1) {
            const EditorViewportUiItem *ui = &viewport->ui_items[j];
            yyjson_mut_val *item = yyjson_mut_obj(document);
            yyjson_mut_obj_add_uint(document, item, "id", ui->id);
            yyjson_mut_obj_add_uint(document, item, "definition", ui->definition);
            yyjson_mut_obj_add_strcpy(document, item, "name", ui->name);
            yyjson_mut_obj_add_real(document, item, "x", ui->position.x);
            yyjson_mut_obj_add_real(document, item, "y", ui->position.y);
            yyjson_mut_obj_add_real(document, item, "scale_x", ui->scale.x);
            yyjson_mut_obj_add_real(document, item, "scale_y", ui->scale.y);
            yyjson_mut_obj_add_real(document, item, "rotation", ui->rotation);
            yyjson_mut_obj_add_bool(document, item, "clip_enabled", ui->clip_enabled);
            yyjson_mut_obj_add_real(document, item, "clip_x", ui->clip_rectangle.x);
            yyjson_mut_obj_add_real(document, item, "clip_y", ui->clip_rectangle.y);
            yyjson_mut_obj_add_real(document, item, "clip_width",
                ui->clip_rectangle.width);
            yyjson_mut_obj_add_real(document, item, "clip_height",
                ui->clip_rectangle.height);
            yyjson_mut_obj_add_sint(document, item, "layer", ui->layer);
            yyjson_mut_obj_add_uint(document, item, "graphics_layer",
                ui->graphics_layer);
            yyjson_mut_obj_add_bool(document, item, "visible", ui->visible);
            yyjson_mut_obj_add_uint(document, item, "drag_mode", ui->drag_mode);
            yyjson_mut_arr_add_val(ui_items, item);
        }
        yyjson_mut_obj_add_val(document, value, "ui_items", ui_items);
        yyjson_mut_arr_add_val(layout_viewports, value);
    }
    yyjson_mut_obj_add_val(document, root, "layout_viewports", layout_viewports);
    for(size_t i = 0; i < project->hierarchy_count; i += 1) {
        yyjson_mut_val *item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_uint(document, item, "kind", project->hierarchy[i].kind);
        yyjson_mut_obj_add_uint(document, item, "id", project->hierarchy[i].id);
        yyjson_mut_arr_add_val(hierarchy, item);
    }
    yyjson_mut_obj_add_val(document, root, "hierarchy", hierarchy);
    success = yyjson_mut_write_file(path, document, YYJSON_WRITE_PRETTY, NULL, NULL);
    yyjson_mut_doc_free(document);
    return success;
}

static bool editor_json_hitbox_read(yyjson_val *value, EditorHitbox *hitbox,
    EditorProject *project) {
    yyjson_val *vertices = yyjson_obj_get(value, "vertices");
    yyjson_val *lines = yyjson_obj_get(value, "lines");
    size_t count;
    if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &hitbox->id) ||
            hitbox->id == 0 || !editor_json_name(value, hitbox->name) ||
            !editor_json_bool(value, "visible", &hitbox->visible) ||
            !yyjson_is_arr(vertices) || (lines != NULL && !yyjson_is_arr(lines))) return false;
    editor_project_property_name_format(hitbox->name, sizeof(hitbox->name), hitbox->name);
    count = yyjson_arr_size(vertices);
    if(count < EDITOR_HITBOX_VERTEX_MIN ||
            (lines != NULL && yyjson_arr_size(lines) != count)) return false;
    if(!EDITOR_ARRAY_RESERVE(hitbox->vertices, hitbox->vertex_capacity, count))
        return false;
    hitbox->line_names = calloc(hitbox->vertex_capacity,
        sizeof(*hitbox->line_names));
    if(hitbox->line_names == NULL) return false;
    hitbox->vertex_count = (uint32_t)count;
    for(size_t i = 0; i < count; i += 1) {
        yyjson_val *item = yyjson_arr_get(vertices, i);
        yyjson_val *line = lines == NULL ? NULL : yyjson_arr_get(lines, i);
        yyjson_val *name = yyjson_obj_get(item, "name");
        if(!yyjson_is_obj(item) || !editor_json_uint(item, "id", &hitbox->vertices[i].id) ||
                hitbox->vertices[i].id == 0 || !editor_json_position_read(
                    yyjson_obj_get(item, "position"), &hitbox->vertices[i].position) ||
                !editor_json_bool(item, "position_locked",
                    &hitbox->vertices[i].position_locked) ||
                (name != NULL && (!yyjson_is_str(name) || yyjson_get_len(name) == 0 ||
                    yyjson_get_len(name) >= EDITOR_OBJECT_NAME_MAX)) ||
                (line != NULL && (!yyjson_is_str(line) || yyjson_get_len(line) == 0 ||
                    yyjson_get_len(line) >= EDITOR_OBJECT_NAME_MAX))) {
            return false;
        }
        if(name == NULL) snprintf(hitbox->vertices[i].name,
            sizeof(hitbox->vertices[i].name), "vertex_%zu", i + 1);
        else memcpy(hitbox->vertices[i].name, yyjson_get_str(name),
            yyjson_get_len(name) + 1);
        if(line == NULL) snprintf(hitbox->line_names[i],
            sizeof(hitbox->line_names[i]), "line_%zu", i + 1);
        else memcpy(hitbox->line_names[i], yyjson_get_str(line), yyjson_get_len(line) + 1);
        editor_project_property_name_format(hitbox->vertices[i].name,
            sizeof(hitbox->vertices[i].name), hitbox->vertices[i].name);
        editor_project_property_name_format(hitbox->line_names[i],
            sizeof(hitbox->line_names[i]), hitbox->line_names[i]);
        if(project->next_vertex_id <= hitbox->vertices[i].id)
            project->next_vertex_id = hitbox->vertices[i].id + 1;
    }
    if(project->next_hitbox_id <= hitbox->id) project->next_hitbox_id = hitbox->id + 1;
    return true;
}

static bool editor_json_body_read(yyjson_val *value, EditorRigidBody *body,
    EditorProject *project) {
    yyjson_val *hitboxes = yyjson_obj_get(value, "hitboxes");
    yyjson_val *collision_enabled = yyjson_obj_get(value, "collision_enabled");
    yyjson_val *collision_category = yyjson_obj_get(value, "collision_category");
    yyjson_val *collision_with = yyjson_obj_get(value, "collision_with");
    yyjson_val *particle = yyjson_obj_get(value, "particle");
    yyjson_val *particle_auto_fit = yyjson_obj_get(value, "particle_auto_fit");
    yyjson_val *particle_radius = yyjson_obj_get(value, "particle_radius");
    yyjson_val *particle_origin = yyjson_obj_get(value, "particle_origin");
    yyjson_val *particle_ring_color = yyjson_obj_get(value, "particle_ring_color");
    yyjson_val *particle_fill_color = yyjson_obj_get(value, "particle_fill_color");
    yyjson_val *border_color = yyjson_obj_get(value, "border_color");
    yyjson_val *surface_color = yyjson_obj_get(value, "surface_color");
    yyjson_val *active_hitbox_index = yyjson_obj_get(value, "active_hitbox_index");
    yyjson_val *bindings = yyjson_obj_get(value, "hitbox_animation_bindings");
    yyjson_val *parent = yyjson_obj_get(value, "parent");
    uint32_t count;
    *body = editor_project_rigid_body_default_get();
    if(parent != NULL && !editor_json_uint(value, "parent", &body->parent))
        return false;
    if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &body->id) || body->id == 0 ||
            !editor_json_name(value, body->name) || !editor_json_position_read(
                yyjson_obj_get(value, "position"), &body->position) ||
            !editor_json_real(value, "rotation", &body->rotation) ||
            !editor_json_real(value, "mass", &body->mass_value) ||
            !editor_json_real(value, "friction", &body->friction) ||
            !editor_json_real(value, "restitution", &body->restitution) ||
            !editor_json_bool(value, "static", &body->static_body) ||
            !editor_json_bool(value, "rotation_locked", &body->rotation_locked) ||
            !editor_json_bool(value, "gravity_enabled", &body->gravity_enabled) ||
            !editor_json_bool(value, "visible", &body->visible) || !yyjson_is_arr(hitboxes)) {
        return false;
    }
    if(collision_enabled != NULL && (!editor_json_bool(value, "collision_enabled",
                &body->collision_enabled) ||
            !editor_json_uint64(value, "collision_category", &body->collision_category) ||
            !editor_json_uint64(value, "collision_with", &body->collision_with))) return false;
    if(particle != NULL && !editor_json_bool(value, "particle", &body->particle)) return false;
    if(particle_auto_fit != NULL && !editor_json_bool(
            value, "particle_auto_fit", &body->particle_auto_fit)) return false;
    if((particle_radius != NULL && !editor_json_real(
                value, "particle_radius", &body->particle_radius)) ||
            (particle_origin != NULL && !editor_json_position_read(
                particle_origin, &body->particle_origin)) ||
            (particle_ring_color != NULL && !editor_json_uint(
                value, "particle_ring_color", &body->particle_ring_color)) ||
            (particle_fill_color != NULL && !editor_json_uint(
                value, "particle_fill_color", &body->particle_fill_color))) return false;
    body->particle_radius = fmaxf(0.0f, body->particle_radius);
    if((border_color != NULL && !editor_json_uint(value, "border_color", &body->border_color)) ||
            (surface_color != NULL && !editor_json_uint(
                value, "surface_color", &body->surface_color))) return false;
    if(!body->collision_enabled) body->particle = false;
    if(collision_enabled == NULL &&
            (collision_category != NULL || collision_with != NULL)) return false;
    editor_project_property_name_format(body->name, sizeof(body->name), body->name);
    count = (uint32_t)yyjson_arr_size(hitboxes);
    if(!EDITOR_ARRAY_RESERVE(body->hitboxes, body->hitbox_capacity, count))
        return false;
    if(count > 0) memset(body->hitboxes, 0,
        count * sizeof(*body->hitboxes));
    body->hitbox_count = count;
    for(size_t i = 0; i < count; i += 1)
        if(!editor_json_hitbox_read(yyjson_arr_get(hitboxes, i), &body->hitboxes[i], project))
            return false;
    if(active_hitbox_index != NULL) {
        uint32_t active;
        if(!editor_json_uint(value, "active_hitbox_index", &active) ||
                (count == 0 ? active != 0 : active >= count)) return false;
        body->active_hitbox_index = active;
    }
    if(bindings != NULL) {
        if(!yyjson_is_arr(bindings) ||
                !EDITOR_ARRAY_RESERVE(body->hitbox_animation_bindings,
                    body->hitbox_animation_binding_capacity,
                    yyjson_arr_size(bindings))) return false;
        for(size_t i = 0; i < yyjson_arr_size(bindings); i += 1) {
            yyjson_val *item = yyjson_arr_get(bindings, i);
            EditorHitboxAnimationBinding binding = {0};
            if(!yyjson_is_obj(item) ||
                    !editor_json_uint(item, "animation", &binding.animation) ||
                    !editor_json_uint(item, "frame", &binding.frame) ||
                    !editor_json_uint(item, "hitbox", &binding.hitbox) ||
                    binding.animation == 0 || binding.frame == 0 ||
                    editor_project_hitbox_get(body, binding.hitbox) == NULL)
                return false;
            body->hitbox_animation_bindings[
                body->hitbox_animation_binding_count++] = binding;
        }
    }
    if(!editor_json_graphics_layer_binding_read(value, &body->graphics_layer))
        return false;
    if(project->next_rigid_body_id <= body->id) project->next_rigid_body_id = body->id + 1;
    return true;
}

static bool editor_json_anchor_read(yyjson_val *value, EditorAnchor *anchor,
    EditorProject *project) {
    yyjson_val *attachment_kind = yyjson_obj_get(value, "attachment_kind");
    if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &anchor->id) ||
            anchor->id == 0 || !editor_json_name(value, anchor->name) ||
            !editor_json_position_read(yyjson_obj_get(value, "position"), &anchor->position) ||
            !editor_json_real(value, "rotation", &anchor->rotation) ||
            !editor_json_uint(value, "rigid_body", &anchor->rigid_body) ||
            !editor_json_bool(value, "position_follows_body", &anchor->position_follows_body) ||
            !editor_json_bool(value, "rotation_follows_body", &anchor->rotation_follows_body) ||
            !editor_json_bool(value, "visible", &anchor->visible)) return false;
    if(attachment_kind == NULL) {
        anchor->attachment_kind = anchor->rigid_body != 0 ?
            EDITOR_ANCHOR_ATTACHMENT_RIGID_BODY : EDITOR_ANCHOR_ATTACHMENT_NONE;
    } else {
        uint32_t kind;
        if(!editor_json_uint(value, "attachment_kind", &kind) ||
                kind > EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE ||
                !editor_json_uint(value, "attachment_soft_body",
                    &anchor->attachment_soft_body) ||
                !editor_json_uint(value, "attachment_soft_node",
                    &anchor->attachment_soft_node)) return false;
        anchor->attachment_kind = (EditorAnchorAttachmentKind)kind;
    }
    editor_project_property_name_format(anchor->name, sizeof(anchor->name), anchor->name);
    if(project->next_anchor_id <= anchor->id) project->next_anchor_id = anchor->id + 1;
    return true;
}

static bool editor_json_joint_read(yyjson_val *value, EditorJoint *joint,
    EditorProject *project) {
    uint32_t kind;
    if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &joint->id) ||
            joint->id == 0 || !editor_json_name(value, joint->name) ||
            !editor_json_uint(value, "kind", &kind) || kind > EDITOR_JOINT_SPRING ||
            !editor_json_uint(value, "anchor_a", &joint->anchor_a) ||
            !editor_json_uint(value, "anchor_b", &joint->anchor_b) ||
            !editor_json_real(value, "rest_length", &joint->rest_length) ||
            !editor_json_real(value, "stiffness", &joint->stiffness) ||
            !editor_json_real(value, "damping", &joint->damping) ||
            !editor_json_real(value, "rest_angle", &joint->rest_angle) ||
            !editor_json_real(value, "visual_size", &joint->visual_size) ||
            !editor_json_bool(value, "visible", &joint->visible) ||
            !editor_json_graphics_layer_binding_read(value,
                &joint->graphics_layer)) return false;
    editor_project_property_name_format(joint->name, sizeof(joint->name), joint->name);
    joint->kind = (EditorJointKind)kind;
    if(project->next_joint_id <= joint->id) project->next_joint_id = joint->id + 1;
    return true;
}

static bool editor_json_soft_body_read(yyjson_val *value, EditorSoftBody *body,
    EditorProject *project) {
    yyjson_val *nodes = yyjson_obj_get(value, "nodes");
    yyjson_val *beams = yyjson_obj_get(value, "beams");
    yyjson_val *areas = yyjson_obj_get(value, "areas");
    yyjson_val *hierarchy = yyjson_obj_get(value, "hierarchy");
    yyjson_val *rotation = yyjson_obj_get(value, "rotation");
    *body = (EditorSoftBody){
        .node_color = UINT32_C(0xffaa46ff),
        .beam_color = UINT32_C(0xebf0f5ff),
        .area_color = UINT32_C(0x505a78ff)
    };
    if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &body->id) || body->id == 0 ||
            !editor_json_name(value, body->name) || !editor_json_position_read(
                yyjson_obj_get(value, "position"), &body->position) ||
            !editor_json_bool(value, "visible", &body->visible) || !yyjson_is_arr(nodes) ||
            !yyjson_is_arr(beams)) return false;
    if(areas != NULL && !yyjson_is_arr(areas)) return false;
    if(hierarchy != NULL && !yyjson_is_arr(hierarchy)) return false;
    if(yyjson_obj_get(value, "node_color") != NULL &&
            (!editor_json_uint(value, "node_color", &body->node_color) ||
            !editor_json_uint(value, "beam_color", &body->beam_color) ||
            !editor_json_uint(value, "area_color", &body->area_color))) return false;
    if(rotation != NULL && !editor_json_real(value, "rotation", &body->rotation)) {
        return false;
    }
    editor_project_property_name_format(body->name, sizeof(body->name), body->name);
    body->node_count = yyjson_arr_size(nodes);
    body->beam_count = yyjson_arr_size(beams);
    if(!EDITOR_ARRAY_RESERVE(body->nodes, body->node_capacity,
            body->node_count) || !EDITOR_ARRAY_RESERVE(body->beams,
                body->beam_capacity, body->beam_count)) return false;
    for(size_t i = 0; i < body->node_count; i += 1) {
        yyjson_val *item = yyjson_arr_get(nodes, i);
        EditorSoftNode *node = &body->nodes[i];
        yyjson_val *collision_enabled = yyjson_obj_get(item, "collision_enabled");
        yyjson_val *friction = yyjson_obj_get(item, "friction");
        yyjson_val *restitution = yyjson_obj_get(item, "restitution");
        yyjson_val *radius = yyjson_obj_get(item, "radius");
        *node = (EditorSoftNode){
            .graphics_layer_inherited = true,
            .radius = 4.0f,
            .friction = 0.0f,
            .restitution = 0.25f,
            .collision_enabled = true,
            .collision_category = UINT64_C(1),
            .collision_with = UINT64_C(1),
            .color = body->node_color
        };
        if(!yyjson_is_obj(item) || !editor_json_uint(item, "id", &node->id) || node->id == 0 ||
                !editor_json_name(item, node->name) || !editor_json_position_read(
                    yyjson_obj_get(item, "position"), &node->position) ||
                !editor_json_real(item, "mass", &node->node_mass) ||
                !editor_json_bool(item, "gravity_enabled", &node->gravity_enabled) ||
                !editor_json_bool(item, "visible", &node->visible)) return false;
        if(yyjson_obj_get(item, "graphics_layer_inherited") != NULL &&
                (!editor_json_bool(item, "graphics_layer_inherited",
                    &node->graphics_layer_inherited) ||
                !editor_json_graphics_layer_binding_read(item,
                    &node->graphics_layer))) return false;
        if((friction != NULL && !editor_json_real(item, "friction", &node->friction)) ||
                (restitution != NULL && !editor_json_real(
                    item, "restitution", &node->restitution))) return false;
        if(radius != NULL && !editor_json_real(item, "radius", &node->radius)) return false;
        if(yyjson_obj_get(item, "color") != NULL &&
                (!editor_json_uint(item, "color", &node->color) ||
                !editor_json_bool(item, "color_overridden", &node->color_overridden))) {
            return false;
        }
        if(node->radius <= 0.0f) return false;
        if(collision_enabled != NULL &&
                (!editor_json_bool(item, "collision_enabled", &node->collision_enabled) ||
                !editor_json_uint64(item, "collision_category", &node->collision_category) ||
                !editor_json_uint64(item, "collision_with", &node->collision_with))) return false;
        editor_project_property_name_format(node->name, sizeof(node->name), node->name);
        if(project->next_soft_node_id <= node->id) project->next_soft_node_id = node->id + 1;
    }
    for(size_t i = 0; i < body->beam_count; i += 1) {
        yyjson_val *item = yyjson_arr_get(beams, i);
        EditorSoftBeam *beam = &body->beams[i];
        yyjson_val *damping = yyjson_obj_get(item, "damping");
        *beam = (EditorSoftBeam){.graphics_layer_inherited = true,
            .damping = 0.0f, .color = body->beam_color};
        if(!yyjson_is_obj(item) || !editor_json_uint(item, "id", &beam->id) || beam->id == 0 ||
                !editor_json_name(item, beam->name) ||
                !editor_json_uint(item, "node_a", &beam->node_a) ||
                !editor_json_uint(item, "node_b", &beam->node_b) ||
                !editor_json_real(item, "stiffness", &beam->stiffness) ||
                !editor_json_bool(item, "visible", &beam->visible)) return false;
        if(yyjson_obj_get(item, "graphics_layer_inherited") != NULL &&
                (!editor_json_bool(item, "graphics_layer_inherited",
                    &beam->graphics_layer_inherited) ||
                !editor_json_graphics_layer_binding_read(item,
                    &beam->graphics_layer))) return false;
        if(damping != NULL && !editor_json_real(item, "damping", &beam->damping)) {
            return false;
        }
        if(yyjson_obj_get(item, "color") != NULL &&
                (!editor_json_uint(item, "color", &beam->color) ||
                !editor_json_bool(item, "color_overridden", &beam->color_overridden))) {
            return false;
        }
        editor_project_property_name_format(beam->name, sizeof(beam->name), beam->name);
        if(project->next_soft_beam_id <= beam->id) project->next_soft_beam_id = beam->id + 1;
    }
    if(areas != NULL) {
        body->area_count = yyjson_arr_size(areas);
        if(!EDITOR_ARRAY_RESERVE(body->areas, body->area_capacity,
                body->area_count)) return false;
        if(body->area_count > 0) memset(body->areas, 0,
            body->area_count * sizeof(*body->areas));
        for(size_t i = 0; i < body->area_count; i += 1) {
            yyjson_val *item = yyjson_arr_get(areas, i);
            yyjson_val *area_nodes = yyjson_obj_get(item, "nodes");
            EditorSoftArea *area = &body->areas[i];
            area->graphics_layer_inherited = true;
            if(!yyjson_is_obj(item) || !editor_json_uint(item, "id", &area->id) ||
                    area->id == 0 || !editor_json_name(item, area->name) ||
                    !editor_json_uint(item, "color", &area->color) ||
                    !editor_json_bool(item, "color_overridden", &area->color_overridden) ||
                    !editor_json_bool(item, "visible", &area->visible)) return false;
            if(yyjson_obj_get(item, "graphics_layer_inherited") != NULL &&
                    (!editor_json_bool(item, "graphics_layer_inherited",
                        &area->graphics_layer_inherited) ||
                    !editor_json_graphics_layer_binding_read(item,
                        &area->graphics_layer))) return false;
            if(area_nodes != NULL) {
                if(!yyjson_is_arr(area_nodes) || yyjson_arr_size(area_nodes) < 3)
                    return false;
                area->node_count = yyjson_arr_size(area_nodes);
                if(!EDITOR_ARRAY_RESERVE(area->nodes, area->node_capacity,
                        area->node_count)) return false;
                for(size_t node_index = 0; node_index < area->node_count; node_index += 1) {
                    yyjson_val *node = yyjson_arr_get(area_nodes, node_index);
                    if(!yyjson_is_uint(node) || yyjson_get_uint(node) > UINT32_MAX) return false;
                    area->nodes[node_index] = (EditorSoftNodeId)yyjson_get_uint(node);
                }
            } else {
                area->node_count = 3;
                if(!EDITOR_ARRAY_RESERVE(area->nodes, area->node_capacity, 3))
                    return false;
                if(!editor_json_uint(item, "node_a", &area->nodes[0]) ||
                        !editor_json_uint(item, "node_b", &area->nodes[1]) ||
                        !editor_json_uint(item, "node_c", &area->nodes[2])) return false;
            }
            editor_project_property_name_format(area->name, sizeof(area->name), area->name);
            if(project->next_soft_area_id <= area->id) {
                project->next_soft_area_id = area->id + 1;
            }
        }
    }
    editor_project_soft_areas_sync(project, body);
    if(hierarchy != NULL) {
        body->hierarchy_count = yyjson_arr_size(hierarchy);
        if(!EDITOR_ARRAY_RESERVE(body->hierarchy, body->hierarchy_capacity,
                body->hierarchy_count)) return false;
        for(size_t i = 0; i < body->hierarchy_count; i += 1) {
            yyjson_val *item = yyjson_arr_get(hierarchy, i);
            uint32_t kind;
            if(!yyjson_is_obj(item) || !editor_json_uint(item, "kind", &kind) ||
                    kind > EDITOR_SOFT_HIERARCHY_AREA ||
                    !editor_json_uint(item, "id", &body->hierarchy[i].id) ||
                    body->hierarchy[i].id == 0) return false;
            body->hierarchy[i].kind = (EditorSoftHierarchyItemKind)kind;
        }
    }
    {
        size_t serialized_count = body->hierarchy_count;
        size_t expected_count = body->node_count + body->beam_count + body->area_count;
        editor_project_soft_body_hierarchy_sync(body);
        if(hierarchy != NULL && (body->hierarchy_count != serialized_count ||
                body->hierarchy_count != expected_count)) return false;
    }
    if(!editor_json_graphics_layer_binding_read(value, &body->graphics_layer))
        return false;
    if(project->next_soft_body_id <= body->id) project->next_soft_body_id = body->id + 1;
    return true;
}

static bool editor_json_animated_sprite_read(yyjson_val *value,
        EditorAnimatedSprite *sprite, EditorProject *project) {
    yyjson_val *frames;
    yyjson_val *time;
    uint32_t direction;
    uint64_t ticks;
    if(!yyjson_is_obj(value) || sprite == NULL || project == NULL ||
            !editor_json_uint(value, "id", &sprite->id) || sprite->id == 0 ||
            !editor_json_name(value, sprite->name) ||
            !editor_json_uint(value, "rigid_body", &sprite->rigid_body) ||
            !editor_json_position_read(yyjson_obj_get(value, "editor_position"),
                &sprite->editor_position) ||
            !editor_json_optional_real(value, "editor_rotation",
                &sprite->editor_rotation, 0.0f) ||
            !editor_json_uint64(value, "ticks_per_frame", &ticks) ||
            !editor_json_uint(value, "starting_frame", &sprite->starting_frame) ||
            !editor_json_real(value, "scale_x", &sprite->scale.x) ||
            !editor_json_real(value, "scale_y", &sprite->scale.y) ||
            !editor_json_uint(value, "direction", &direction) ||
            direction > DIRECTION_RIGHT ||
            !editor_json_bool(value, "follow_body_rotation",
                &sprite->follow_body_rotation) ||
            !editor_json_bool(value, "visible", &sprite->visible) ||
            !editor_json_bool(value, "playing", &sprite->playing)) return false;
    time = yyjson_obj_get(value, "time_per_frame");
    frames = yyjson_obj_get(value, "frames");
    if(!yyjson_is_num(time) || yyjson_get_real(time) < 0.0 ||
            !yyjson_is_arr(frames) || yyjson_arr_size(frames) > MAX_ANIMATIONS_FRAMES ||
            sprite->scale.x <= 0.0f || sprite->scale.y <= 0.0f) return false;
    editor_project_property_name_format(sprite->name, sizeof(sprite->name),
        sprite->name);
    sprite->ticks_per_frame = (Tick)ticks;
    sprite->time_per_frame = (Time)yyjson_get_real(time);
    sprite->direction = (Direction)direction;
    sprite->frame_count = yyjson_arr_size(frames);
    if(!EDITOR_ARRAY_RESERVE(sprite->frames, sprite->frame_capacity,
            sprite->frame_count)) return false;
    for(size_t i = 0; i < sprite->frame_count; i += 1) {
        yyjson_val *frame = yyjson_arr_get(frames, i);
        uint32_t id;
        yyjson_val *path = yyjson_obj_get(frame, "path");
        if(!yyjson_is_obj(frame) || !editor_json_uint(frame, "id", &id) || id == 0 ||
                !editor_json_name(frame, sprite->frames[i].name) ||
                !yyjson_is_str(path) || yyjson_get_len(path) == 0 ||
                yyjson_get_len(path) >= sizeof(sprite->frames[i].path) ||
                !editor_json_real(frame, "width", &sprite->frames[i].size.x) ||
                !editor_json_real(frame, "height", &sprite->frames[i].size.y) ||
                sprite->frames[i].name[0] == '\0' ||
                sprite->frames[i].size.x <= 0.0f || sprite->frames[i].size.y <= 0.0f)
            return false;
        memcpy(sprite->frames[i].path, yyjson_get_str(path), yyjson_get_len(path) + 1);
        sprite->frames[i].id = (EditorSpriteId)id;
        editor_project_property_name_format(sprite->frames[i].name,
            sizeof(sprite->frames[i].name), sprite->frames[i].name);
        if(project->next_sprite_id <= sprite->frames[i].id)
            project->next_sprite_id = sprite->frames[i].id + 1;
    }
    if(!editor_json_graphics_layer_binding_read(value, &sprite->graphics_layer))
        return false;
    if(project->next_animated_sprite_id <= sprite->id)
        project->next_animated_sprite_id = sprite->id + 1;
    return true;
}

static bool editor_json_ui_definition_read(yyjson_val *value,
        EditorViewportUiDefinition *ui) {
    uint32_t kind;
    uint32_t border_type = EDITOR_VIEWPORT_UI_BORDER_LINE;
    yyjson_val *vertices;
    yyjson_val *text;
    if(value == NULL || ui == NULL || !yyjson_is_obj(value)) return false;
    *ui = (EditorViewportUiDefinition){
        .border_thickness = 2.0f,
        .border_hash_spacing = 6.0f,
        .border_color = 0xFFFFFFFFu,
        .fill_color = 0x394052FFu,
        .hover_border_color = 0xD8E6FFFFu,
        .hover_fill_color = 0x4A5870FFu,
        .click_border_color = 0xAFC8F0FFu,
        .click_fill_color = 0x283246FFu,
    };
    if(!editor_json_uint(value, "id", &ui->id) || ui->id == 0 ||
            !editor_json_uint(value, "kind", &kind) ||
            kind > EDITOR_VIEWPORT_UI_TEXT ||
            !editor_json_name(value, ui->name)) return false;
    ui->kind = (EditorViewportUiKind)kind;
#define OPTIONAL_BOOL(Key, Target) do { yyjson_val *v = yyjson_obj_get(value, Key); \
    if(v != NULL && !editor_json_bool(value, Key, Target)) return false; } while(0)
#define OPTIONAL_UINT(Key, Target) do { yyjson_val *v = yyjson_obj_get(value, Key); \
    if(v != NULL && !editor_json_uint(value, Key, Target)) return false; } while(0)
#define OPTIONAL_REAL(Key, Target) do { yyjson_val *v = yyjson_obj_get(value, Key); \
    if(v != NULL && !editor_json_real(value, Key, Target)) return false; } while(0)
    OPTIONAL_BOOL("border_enabled", &ui->border_enabled);
    OPTIONAL_UINT("border_type", &border_type);
    if(border_type > EDITOR_VIEWPORT_UI_BORDER_HASHED) return false;
    ui->border_type = (EditorViewportUiBorderType)border_type;
    OPTIONAL_REAL("border_thickness", &ui->border_thickness);
    OPTIONAL_REAL("border_hash_spacing", &ui->border_hash_spacing);
    OPTIONAL_REAL("border_corner_radius", &ui->border_corner_radius);
    OPTIONAL_UINT("border_color", &ui->border_color);
    OPTIONAL_UINT("ui_fill_color", &ui->fill_color);
    OPTIONAL_UINT("hover_border_color", &ui->hover_border_color);
    OPTIONAL_UINT("hover_fill_color", &ui->hover_fill_color);
    OPTIONAL_UINT("click_border_color", &ui->click_border_color);
    OPTIONAL_UINT("click_fill_color", &ui->click_fill_color);
    if(ui->border_thickness <= 0.0f || ui->border_hash_spacing <= 0.0f ||
            ui->border_corner_radius < 0.0f) return false;
    text = yyjson_obj_get(value, "text");
    if(!yyjson_is_str(text) || yyjson_get_len(text) >= UI_LABEL_MAX) return false;
    if(ui->kind == EDITOR_VIEWPORT_UI_SHAPE) {
        EditorViewportUiShape *shape = &ui->value.shape;
        vertices = yyjson_obj_get(value, "vertices");
        if(!yyjson_is_arr(vertices) || yyjson_arr_size(vertices) < 3 ||
                yyjson_arr_size(vertices) > EDITOR_HITBOX_VERTEX_MAX) return false;
        shape->vertex_count = yyjson_arr_size(vertices);
        for(size_t i = 0; i < shape->vertex_count; i += 1)
            if(!editor_json_position_read(yyjson_arr_get(vertices, i),
                    &shape->vertices[i])) return false;
        OPTIONAL_REAL("ui_rotation", &shape->rotation);
        OPTIONAL_UINT("outline_color", &shape->outline_color);
        OPTIONAL_UINT("fill_color", &shape->fill_color);
        OPTIONAL_BOOL("button_enabled", &shape->button_enabled);
        memcpy(shape->text.text, yyjson_get_str(text), yyjson_get_len(text) + 1);
        OPTIONAL_UINT("font", &shape->text.font);
        OPTIONAL_REAL("text_offset_x", &shape->text.offset.x);
        OPTIONAL_REAL("text_offset_y", &shape->text.offset.y);
        shape->text.color = 0xFFFFFFFFu;
        shape->text.box_width = 160.0f;
        shape->text.box_height = 28.0f;
        shape->text.width_scale = 1.0f;
        shape->text.height_scale = 1.0f;
        OPTIONAL_UINT("color", &shape->text.color);
        OPTIONAL_REAL("box_width", &shape->text.box_width);
        OPTIONAL_REAL("box_height", &shape->text.box_height);
        OPTIONAL_REAL("width_scale", &shape->text.width_scale);
        OPTIONAL_REAL("height_scale", &shape->text.height_scale);
        if(shape->text.box_width <= 0.0f || shape->text.box_height <= 0.0f ||
                shape->text.width_scale <= 0.0f ||
                shape->text.height_scale <= 0.0f) return false;
    } else {
        EditorViewportUiText *text_value = &ui->value.text;
        memcpy(text_value->text, yyjson_get_str(text), yyjson_get_len(text) + 1);
        text_value->color = 0xFFFFFFFFu;
        text_value->box_width = 160.0f;
        text_value->box_height = 28.0f;
        text_value->width_scale = 1.0f;
        text_value->height_scale = 1.0f;
        OPTIONAL_UINT("font", &text_value->font);
        OPTIONAL_REAL("text_offset_x", &text_value->offset.x);
        OPTIONAL_REAL("text_offset_y", &text_value->offset.y);
        OPTIONAL_UINT("color", &text_value->color);
        OPTIONAL_REAL("box_width", &text_value->box_width);
        OPTIONAL_REAL("box_height", &text_value->box_height);
        OPTIONAL_REAL("width_scale", &text_value->width_scale);
        OPTIONAL_REAL("height_scale", &text_value->height_scale);
        if(text_value->box_width <= 0.0f || text_value->box_height <= 0.0f ||
                text_value->width_scale <= 0.0f ||
                text_value->height_scale <= 0.0f) return false;
    }
#undef OPTIONAL_BOOL
#undef OPTIONAL_UINT
#undef OPTIONAL_REAL
    return true;
}

static bool editor_json_references_valid(EditorProject *project) {
    uint64_t valid_masks = project->collision_mask_count == 64 ? UINT64_MAX :
        (UINT64_C(1) << project->collision_mask_count) - 1;
    for(size_t i = 0; i < project->object_count; i += 1) {
        EditorObject *object = &project->objects[i];
        for(size_t j = 0; j < object->rigid_body_count; j += 1)
            if(object->rigid_bodies[j].graphics_layer.layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        object->rigid_bodies[j].graphics_layer.layer) == NULL)
                return false;
        for(size_t j = 0; j < object->joint_count; j += 1)
            if(object->joint_items[j].graphics_layer.layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        object->joint_items[j].graphics_layer.layer) == NULL)
                return false;
        for(size_t j = 0; j < object->soft_body_count; j += 1) {
            EditorSoftBody *body = &object->soft_body_items[j];
            if(body->graphics_layer.layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        body->graphics_layer.layer) == NULL)
                return false;
            for(size_t child = 0; child < body->node_count; child += 1)
                if(body->nodes[child].graphics_layer.layer != 0 &&
                        editor_project_graphics_layer_get(project,
                            body->nodes[child].graphics_layer.layer) == NULL)
                    return false;
            for(size_t child = 0; child < body->beam_count; child += 1)
                if(body->beams[child].graphics_layer.layer != 0 &&
                        editor_project_graphics_layer_get(project,
                            body->beams[child].graphics_layer.layer) == NULL)
                    return false;
            for(size_t child = 0; child < body->area_count; child += 1)
                if(body->areas[child].graphics_layer.layer != 0 &&
                        editor_project_graphics_layer_get(project,
                            body->areas[child].graphics_layer.layer) == NULL)
                    return false;
        }
        for(size_t j = 0; j < object->sprite_count; j += 1)
            if(object->sprites[j].graphics_layer.layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        object->sprites[j].graphics_layer.layer) == NULL)
                return false;
        for(size_t j = 0; j < object->animated_sprite_count; j += 1)
            if(object->animated_sprite_items[j].graphics_layer.layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        object->animated_sprite_items[j].graphics_layer.layer) == NULL)
                return false;
        for(size_t j = 0; j < object->rigid_body_count; j += 1) {
            EditorRigidBody *body = &object->rigid_bodies[j];
            if((body->collision_category & ~valid_masks) != 0 ||
                    (body->collision_with & ~valid_masks) != 0) return false;
        }
        for(size_t j = 0; j < object->anchor_count; j += 1) {
            EditorAnchor *anchor = &object->anchors[j];
            if(anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_RIGID_BODY &&
                    (anchor->rigid_body == 0 || editor_project_rigid_body_get(
                        object, anchor->rigid_body) == NULL)) return false;
            if(anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE) {
                EditorSoftBody *body = NULL;
                bool found = false;
                for(size_t b = 0; b < object->soft_body_count; b += 1)
                    if(object->soft_body_items[b].id == anchor->attachment_soft_body)
                        body = &object->soft_body_items[b];
                if(body != NULL) for(size_t n = 0; n < body->node_count; n += 1)
                    if(body->nodes[n].id == anchor->attachment_soft_node) found = true;
                if(!found) return false;
            }
        }
        for(size_t j = 0; j < object->joint_count; j += 1) {
            EditorJoint *joint = &object->joint_items[j];
            if((joint->anchor_a != 0 && editor_project_anchor_get(object, joint->anchor_a) == NULL) ||
                    (joint->anchor_b != 0 && editor_project_anchor_get(
                        object, joint->anchor_b) == NULL)) return false;
        }
        for(size_t j = 0; j < object->soft_body_count; j += 1) {
            EditorSoftBody *body = &object->soft_body_items[j];
            for(size_t k = 0; k < body->node_count; k += 1) {
                EditorSoftNode *node = &body->nodes[k];
                if((node->collision_category & ~valid_masks) != 0 ||
                        (node->collision_with & ~valid_masks) != 0) return false;
            }
            for(size_t k = 0; k < body->beam_count; k += 1) {
                bool found_a = body->beams[k].node_a == 0;
                bool found_b = body->beams[k].node_b == 0;
                for(size_t n = 0; n < body->node_count; n += 1) {
                    found_a = found_a || body->nodes[n].id == body->beams[k].node_a;
                    found_b = found_b || body->nodes[n].id == body->beams[k].node_b;
                }
                if(!found_a || !found_b) return false;
            }
        }
        for(size_t j = 0; j < object->sprite_count; j += 1) {
            EditorSprite *sprite = &object->sprites[j];
            if(sprite->rigid_body != 0 &&
                    editor_project_rigid_body_get(object, sprite->rigid_body) == NULL)
                return false;
            for(size_t other = 0; other < j; other += 1)
                if(sprite->rigid_body != 0 &&
                        object->sprites[other].rigid_body == sprite->rigid_body)
                    return false;
        }
        for(size_t j = 0; j < object->animated_sprite_count; j += 1) {
            EditorAnimatedSprite *sprite = &object->animated_sprite_items[j];
            if(sprite->rigid_body != 0 &&
                    editor_project_rigid_body_get(object, sprite->rigid_body) == NULL)
                return false;
            if(sprite->frame_count > MAX_ANIMATIONS_FRAMES ||
                    (sprite->frame_count > 0 &&
                        sprite->starting_frame >= sprite->frame_count))
                return false;
            for(size_t other = 0; other < j; other += 1)
                if(sprite->rigid_body != 0 &&
                        object->animated_sprite_items[other].rigid_body ==
                            sprite->rigid_body) return false;
        }
        for(size_t j = 0; j < object->camera_count; j += 1) {
            EditorCamera *camera = &object->cameras[j];
            bool valid = camera->attachment_kind == EDITOR_CAMERA_ATTACHMENT_NONE;
            if(camera->attachment_kind == EDITOR_CAMERA_ATTACHMENT_RIGID_BODY)
                valid = editor_project_rigid_body_get(object,
                    camera->attachment) != NULL;
            else if(camera->attachment_kind == EDITOR_CAMERA_ATTACHMENT_SOFT_BODY) {
                for(size_t b = 0; b < object->soft_body_count; b += 1)
                    valid = valid || object->soft_body_items[b].id == camera->attachment;
            } else if(camera->attachment_kind == EDITOR_CAMERA_ATTACHMENT_ANCHOR)
                valid = editor_project_anchor_get(object, camera->attachment) != NULL;
            else if(camera->attachment_kind == EDITOR_CAMERA_ATTACHMENT_SOFT_NODE) {
                for(size_t b = 0; b < object->soft_body_count; b += 1) {
                    EditorSoftBody *body = &object->soft_body_items[b];
                    if(body->id != camera->attachment_soft_body) continue;
                    for(size_t n = 0; n < body->node_count; n += 1)
                        valid = valid || body->nodes[n].id == camera->attachment;
                }
            }
            if(!valid) return false;
        }
        for(size_t j = 0; j < object->rigid_body_count; j += 1) {
            EditorRigidBody *body = &object->rigid_bodies[j];
            for(size_t k = 0; k < body->hitbox_animation_binding_count; k += 1) {
                EditorHitboxAnimationBinding *binding =
                    &body->hitbox_animation_bindings[k];
                EditorAnimatedSprite *animation =
                    editor_project_animated_sprite_get(object,
                        binding->animation);
                bool frame_found = false;
                if(animation == NULL || animation->rigid_body != body->id ||
                        editor_project_hitbox_get(body, binding->hitbox) == NULL)
                    return false;
                for(size_t frame = 0; frame < animation->frame_count; frame += 1)
                    frame_found = frame_found ||
                        animation->frames[frame].id == binding->frame;
                if(!frame_found) return false;
                for(size_t other = 0; other < k; other += 1)
                    if(body->hitbox_animation_bindings[other].animation ==
                            binding->animation &&
                            body->hitbox_animation_bindings[other].frame ==
                                binding->frame) return false;
            }
        }
    }
    for(size_t i = 0; i < project->layout_viewport_count; i += 1) {
        EditorLayoutViewport *viewport = &project->layout_viewports[i];
        for(size_t j = 0; j < viewport->camera_item_count; j += 1) {
            EditorViewportCameraItem *item = &viewport->camera_items[j];
            EditorObject *object = NULL;
            for(size_t object_index = 0; object_index < project->object_count;
                    object_index += 1) {
                if(project->objects[object_index].id == item->object)
                    object = &project->objects[object_index];
            }
            if(object == NULL || editor_project_camera_get(object, item->camera) == NULL)
                return false;
            if(item->graphics_layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        item->graphics_layer) == NULL) return false;
        }
        for(size_t j = 0; j < viewport->ui_item_count; j += 1) {
            EditorViewportUiItem *item = &viewport->ui_items[j];
            EditorViewportUiDefinition *definition =
                editor_project_ui_definition_get(project, item->definition);
            if(definition == NULL) return false;
            for(size_t earlier_viewport = 0; earlier_viewport <= i;
                    earlier_viewport += 1) {
                size_t earlier_count = earlier_viewport == i ? j :
                    project->layout_viewports[earlier_viewport].ui_item_count;
                for(size_t earlier_item = 0; earlier_item < earlier_count;
                        earlier_item += 1)
                    if(project->layout_viewports[earlier_viewport]
                            .ui_items[earlier_item].id == item->id) return false;
            }
            EditorUiFontId font = definition->kind == EDITOR_VIEWPORT_UI_TEXT ?
                definition->value.text.font : definition->value.shape.text.font;
            if(font != 0 && editor_project_ui_font_get(project, font) == NULL)
                return false;
            if(item->graphics_layer != 0 &&
                    editor_project_graphics_layer_get(project,
                        item->graphics_layer) == NULL) return false;
        }
    }
    return project->selected == 0 || editor_project_selected_get(project) != NULL;
}

EditorResult editor_project_load(EditorProject *project, const char *path) {
    static EditorProject loaded;
    yyjson_doc *document;
    yyjson_read_err read_error = {0};
    yyjson_val *root;
    yyjson_val *objects;
    yyjson_val *layout_viewports;
    yyjson_val *hierarchy;
    yyjson_val *collision_masks;
    yyjson_val *ui_fonts;
    yyjson_val *graphics_layers;
    yyjson_val *ui_definitions;
    uint32_t version;
    EditorResult result = editor_result_error(EDITOR_ERROR_SCHEMA_INVALID,
        "Project editor state does not match the current schema: %s",
        path == NULL ? "(null)" : path);
    if(project == NULL || path == NULL || path[0] == '\0')
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Project editor-state load received an invalid argument");
    document = yyjson_read_file(path, 0, NULL, &read_error);
    if(document == NULL) {
        return editor_result_error(
            read_error.code == YYJSON_READ_ERROR_FILE_OPEN ||
                    read_error.code == YYJSON_READ_ERROR_FILE_READ ?
                EDITOR_ERROR_FILE_IO : EDITOR_ERROR_JSON_PARSE,
            "Could not parse project editor state '%s': %s at byte %zu",
            path, read_error.msg == NULL ? "unknown JSON error" : read_error.msg,
            read_error.pos);
    }
    root = yyjson_doc_get_root(document);
    objects = yyjson_obj_get(root, "objects");
    layout_viewports = yyjson_obj_get(root, "layout_viewports");
    hierarchy = yyjson_obj_get(root, "hierarchy");
    collision_masks = yyjson_obj_get(root, "collision_masks");
    ui_fonts = yyjson_obj_get(root, "ui_fonts");
    graphics_layers = yyjson_obj_get(root, "graphics_layers");
    ui_definitions = yyjson_obj_get(root, "ui_definitions");
    editor_project_destroy(&loaded);
    editor_project_init(&loaded);
    if(!yyjson_is_obj(root)) goto done;
    if(!editor_json_uint(root, "format_version", &version)) {
        result = editor_result_error(EDITOR_ERROR_SCHEMA_VERSION,
            "Project editor state '%s' is missing integer format_version; expected %u",
            path, EDITOR_PROJECT_FORMAT_VERSION);
        goto done;
    }
    if(version != EDITOR_PROJECT_FORMAT_VERSION) {
        result = editor_result_error(EDITOR_ERROR_SCHEMA_VERSION,
            "Project editor state '%s' uses format_version %u; this editor requires %u",
            path, version, EDITOR_PROJECT_FORMAT_VERSION);
        goto done;
    }
    {
        yyjson_val *camera_offset = yyjson_obj_get(root, "viewport_camera_offset");
        yyjson_val *camera_zoom = yyjson_obj_get(root, "viewport_camera_zoom");
        yyjson_val *local_view = yyjson_obj_get(root, "viewport_local_view");
        Position offset;
        if((camera_offset != NULL &&
                !editor_json_position_read(camera_offset, &offset)) ||
                (camera_zoom != NULL && (!yyjson_is_num(camera_zoom) ||
                    yyjson_get_real(camera_zoom) < 0.1 ||
                    yyjson_get_real(camera_zoom) > 8.0)) ||
                (local_view != NULL && !yyjson_is_bool(local_view))) goto done;
        if(camera_offset != NULL)
            loaded.viewport_camera_offset = (Vec2D){offset.x, offset.y};
        if(camera_zoom != NULL)
            loaded.viewport_camera_zoom = (float)yyjson_get_real(camera_zoom);
        if(local_view != NULL)
            loaded.viewport_local_view = yyjson_get_bool(local_view);
    }
    {
        yyjson_val *navigation = yyjson_obj_get(root, "navigation");
        if(navigation != NULL && (!yyjson_is_obj(navigation) ||
                !editor_json_uint(navigation, "mode", &loaded.navigation.mode) ||
                !editor_json_uint(navigation, "selection", &loaded.navigation.selection) ||
                !editor_json_uint(navigation, "object", &loaded.navigation.object) ||
                !editor_json_uint(navigation, "line", &loaded.navigation.selected_line) ||
                !editor_json_uint(navigation, "vertex", &loaded.navigation.selected_vertex) ||
                !editor_json_uint(navigation, "rigid_body", &loaded.navigation.rigid_body) ||
                !editor_json_uint(navigation, "hitbox", &loaded.navigation.hitbox) ||
                !editor_json_uint(navigation, "joint", &loaded.navigation.joint) ||
                !editor_json_uint(navigation, "anchor", &loaded.navigation.anchor) ||
                !editor_json_uint(navigation, "soft_body", &loaded.navigation.soft_body) ||
                !editor_json_uint(navigation, "soft_node", &loaded.navigation.soft_node) ||
                !editor_json_uint(navigation, "soft_beam", &loaded.navigation.soft_beam) ||
                !editor_json_uint(navigation, "origin_kind", &loaded.navigation.origin_kind) ||
                loaded.navigation.mode > EDITOR_NAVIGATION_MODE_MAX ||
                loaded.navigation.selection > EDITOR_NAVIGATION_SELECTION_MAX ||
                loaded.navigation.origin_kind > 2)) goto done;
        if(navigation != NULL) {
            yyjson_val *sprite = yyjson_obj_get(navigation, "sprite");
            yyjson_val *animated = yyjson_obj_get(navigation, "animated_sprite");
            yyjson_val *frame = yyjson_obj_get(navigation, "animation_frame");
            yyjson_val *camera = yyjson_obj_get(navigation, "camera");
            if((sprite != NULL && (!yyjson_is_uint(sprite) ||
                        yyjson_get_uint(sprite) > UINT32_MAX)) ||
                    (animated != NULL && (!yyjson_is_uint(animated) ||
                        yyjson_get_uint(animated) > UINT32_MAX)) ||
                    (frame != NULL && (!yyjson_is_uint(frame) ||
                        yyjson_get_uint(frame) > UINT32_MAX)) ||
                    (camera != NULL && (!yyjson_is_uint(camera) ||
                        yyjson_get_uint(camera) > UINT32_MAX))) goto done;
            if(sprite != NULL)
                loaded.navigation.sprite = (uint32_t)yyjson_get_uint(sprite);
            if(animated != NULL)
                loaded.navigation.animated_sprite =
                    (uint32_t)yyjson_get_uint(animated);
            if(frame != NULL)
                loaded.navigation.animation_frame =
                    (uint32_t)yyjson_get_uint(frame);
            if(camera != NULL)
                loaded.navigation.camera = (uint32_t)yyjson_get_uint(camera);
        }
    }
    if(!editor_json_uint(root, "selected", &loaded.selected) || !yyjson_is_arr(objects) ||
            !editor_json_uint(root, "next_object_id", &loaded.next_id) ||
            !editor_json_uint(root, "next_vertex_id", &loaded.next_vertex_id) ||
            !editor_json_uint(root, "next_rigid_body_id", &loaded.next_rigid_body_id) ||
            !editor_json_uint(root, "next_hitbox_id", &loaded.next_hitbox_id) ||
            !editor_json_uint(root, "next_joint_id", &loaded.next_joint_id) ||
            !editor_json_uint(root, "next_anchor_id", &loaded.next_anchor_id) ||
            !editor_json_uint(root, "next_soft_body_id", &loaded.next_soft_body_id) ||
            !editor_json_uint(root, "next_soft_node_id", &loaded.next_soft_node_id) ||
            !editor_json_uint(root, "next_soft_beam_id", &loaded.next_soft_beam_id) ||
            !editor_json_uint(root, "next_soft_area_id", &loaded.next_soft_area_id) ||
            loaded.next_id == 0 || loaded.next_vertex_id == 0 ||
            loaded.next_rigid_body_id == 0 || loaded.next_hitbox_id == 0 ||
            loaded.next_joint_id == 0 || loaded.next_anchor_id == 0 ||
            loaded.next_soft_body_id == 0 || loaded.next_soft_node_id == 0 ||
            loaded.next_soft_beam_id == 0 || loaded.next_soft_area_id == 0 ||
            !yyjson_is_arr(collision_masks) || yyjson_arr_size(collision_masks) == 0 ||
            yyjson_arr_size(collision_masks) > EDITOR_COLLISION_MASK_MAX)
        goto done;
    {
        yyjson_val *next_sprite = yyjson_obj_get(root, "next_sprite_id");
        yyjson_val *next_animated = yyjson_obj_get(root, "next_animated_sprite_id");
        yyjson_val *next_camera = yyjson_obj_get(root, "next_camera_id");
        yyjson_val *next_layout_viewport = yyjson_obj_get(root,
            "next_layout_viewport_id");
        yyjson_val *next_viewport_camera_item = yyjson_obj_get(root,
            "next_viewport_camera_item_id");
        yyjson_val *next_ui_font = yyjson_obj_get(root, "next_ui_font_id");
        yyjson_val *next_graphics_layer = yyjson_obj_get(root,
            "next_graphics_layer_id");
        yyjson_val *next_ui_definition = yyjson_obj_get(root,
            "next_ui_definition_id");
        if((next_sprite != NULL && !editor_json_uint(root, "next_sprite_id",
                    &loaded.next_sprite_id)) ||
                (next_animated != NULL && !editor_json_uint(root,
                    "next_animated_sprite_id", &loaded.next_animated_sprite_id)) ||
                (next_camera != NULL && !editor_json_uint(root,
                    "next_camera_id", &loaded.next_camera_id)) ||
                (next_layout_viewport != NULL && !editor_json_uint(root,
                    "next_layout_viewport_id", &loaded.next_layout_viewport_id)) ||
                (next_viewport_camera_item != NULL && !editor_json_uint(root,
                    "next_viewport_camera_item_id",
                    &loaded.next_viewport_camera_item_id)) ||
                (next_ui_font != NULL && !editor_json_uint(root,
                    "next_ui_font_id", &loaded.next_ui_font_id)) ||
                (next_graphics_layer != NULL && !editor_json_uint(root,
                    "next_graphics_layer_id", &loaded.next_graphics_layer_id)) ||
                (next_ui_definition != NULL && !editor_json_uint(root,
                    "next_ui_definition_id", &loaded.next_ui_definition_id)) ||
                loaded.next_sprite_id == 0 || loaded.next_animated_sprite_id == 0 ||
                loaded.next_camera_id == 0 || loaded.next_layout_viewport_id == 0 ||
                loaded.next_viewport_camera_item_id == 0 ||
                loaded.next_graphics_layer_id == 0 ||
                loaded.next_ui_definition_id == 0 ||
                (layout_viewports != NULL && !yyjson_is_arr(layout_viewports)) ||
                (hierarchy != NULL && !yyjson_is_arr(hierarchy)))
            goto done;
    }
    if(graphics_layers != NULL) {
        if(!yyjson_is_arr(graphics_layers) ||
                yyjson_arr_size(graphics_layers) > MAX_GRAPHICS_LAYERS ||
                !EDITOR_ARRAY_RESERVE(loaded.graphics_layers,
                    loaded.graphics_layer_capacity,
                    yyjson_arr_size(graphics_layers))) goto done;
        loaded.graphics_layer_count = yyjson_arr_size(graphics_layers);
        for(size_t i = 0; i < loaded.graphics_layer_count; i += 1) {
            yyjson_val *value = yyjson_arr_get(graphics_layers, i);
            EditorGraphicsLayer *layer = &loaded.graphics_layers[i];
            if(!yyjson_is_obj(value) ||
                    !editor_json_uint(value, "id", &layer->id) || layer->id == 0 ||
                    !editor_json_name(value, layer->name) ||
                    !editor_json_int(value, "value", &layer->value)) goto done;
            for(size_t previous = 0; previous < i; previous += 1)
                if(strcmp(loaded.graphics_layers[previous].name, layer->name) == 0 ||
                        loaded.graphics_layers[previous].id == layer->id) goto done;
            if(loaded.next_graphics_layer_id <= layer->id)
                loaded.next_graphics_layer_id = layer->id + 1;
        }
    }
    if(ui_definitions != NULL) {
        if(!yyjson_is_arr(ui_definitions) ||
                yyjson_arr_size(ui_definitions) > MAX_GRAPHICS_UI_ELEMENTS ||
                !EDITOR_ARRAY_RESERVE(loaded.ui_definitions,
                    loaded.ui_definition_capacity,
                    yyjson_arr_size(ui_definitions))) goto done;
        loaded.ui_definition_count = yyjson_arr_size(ui_definitions);
        for(size_t i = 0; i < loaded.ui_definition_count; i += 1) {
            if(!editor_json_ui_definition_read(yyjson_arr_get(ui_definitions, i),
                    &loaded.ui_definitions[i])) goto done;
            for(size_t previous = 0; previous < i; previous += 1)
                if(loaded.ui_definitions[previous].id == loaded.ui_definitions[i].id ||
                        strcmp(loaded.ui_definitions[previous].name,
                            loaded.ui_definitions[i].name) == 0)
                    goto done;
            if(loaded.next_ui_definition_id <= loaded.ui_definitions[i].id)
                loaded.next_ui_definition_id = loaded.ui_definitions[i].id + 1;
        }
    }
    if(ui_fonts != NULL) {
        if(!yyjson_is_arr(ui_fonts) || yyjson_arr_size(ui_fonts) > EDITOR_UI_FONT_MAX ||
                !EDITOR_ARRAY_RESERVE(loaded.ui_fonts, loaded.ui_font_capacity,
                    yyjson_arr_size(ui_fonts))) goto done;
        loaded.ui_font_count = yyjson_arr_size(ui_fonts);
        for(size_t i = 0; i < loaded.ui_font_count; i += 1) {
            yyjson_val *value = yyjson_arr_get(ui_fonts, i);
            yyjson_val *path_value = yyjson_obj_get(value, "path");
            EditorUiFont *font = &loaded.ui_fonts[i];
            if(!yyjson_is_obj(value) ||
                    !editor_json_uint(value, "id", &font->id) || font->id == 0 ||
                    !editor_json_name(value, font->name) ||
                    !yyjson_is_str(path_value) ||
                    yyjson_get_len(path_value) >= sizeof(font->path)) goto done;
            memcpy(font->path, yyjson_get_str(path_value),
                yyjson_get_len(path_value) + 1);
            if(loaded.next_ui_font_id <= font->id)
                loaded.next_ui_font_id = font->id + 1;
        }
    }
    loaded.collision_mask_count = yyjson_arr_size(collision_masks);
    if(!EDITOR_ARRAY_RESERVE(loaded.collision_masks,
            loaded.collision_mask_capacity, loaded.collision_mask_count)) goto done;
    for(size_t i = 0; i < loaded.collision_mask_count; i += 1) {
        yyjson_val *name = yyjson_arr_get(collision_masks, i);
        if(!yyjson_is_str(name) || yyjson_get_len(name) == 0 ||
                yyjson_get_len(name) >= EDITOR_OBJECT_NAME_MAX) goto done;
        memcpy(loaded.collision_masks[i].name, yyjson_get_str(name),
            yyjson_get_len(name) + 1);
        editor_project_property_name_format(loaded.collision_masks[i].name,
            sizeof(loaded.collision_masks[i].name), loaded.collision_masks[i].name);
    }
    loaded.object_count = yyjson_arr_size(objects);
    if(!EDITOR_ARRAY_RESERVE(loaded.objects, loaded.object_capacity,
            loaded.object_count)) goto done;
    if(loaded.object_count > 0) memset(loaded.objects, 0,
        loaded.object_count * sizeof(*loaded.objects));
    for(size_t i = 0; i < loaded.object_count; i += 1) {
        yyjson_val *value = yyjson_arr_get(objects, i);
        EditorObject *object = &loaded.objects[i];
        yyjson_val *bodies = yyjson_obj_get(value, "rigid_bodies");
        yyjson_val *anchors = yyjson_obj_get(value, "anchors");
        yyjson_val *joint_values = yyjson_obj_get(value, "joints");
        yyjson_val *soft_body_values = yyjson_obj_get(value, "soft_bodies");
        yyjson_val *sprites = yyjson_obj_get(value, "sprites");
        yyjson_val *animated_sprite_values = yyjson_obj_get(value,
            "animated_sprites");
        yyjson_val *camera_values = yyjson_obj_get(value, "cameras");
        yyjson_val *hierarchy = yyjson_obj_get(value, "hierarchy");
        yyjson_val *overview_position = yyjson_obj_get(value, "overview_position");
        if(!yyjson_is_obj(value) || !editor_json_uint(value, "id", &object->id) ||
                object->id == 0 || !editor_json_name(value, object->name) ||
                !editor_json_position_read(yyjson_obj_get(value, "position"), &object->position) ||
                !editor_json_bool(value, "visible", &object->visible) ||
                !yyjson_is_arr(bodies) || !yyjson_is_arr(anchors) ||
                !yyjson_is_arr(joint_values) || !yyjson_is_arr(soft_body_values) ||
                (sprites != NULL && !yyjson_is_arr(sprites)) ||
                (animated_sprite_values != NULL &&
                    !yyjson_is_arr(animated_sprite_values)) ||
                (camera_values != NULL && !yyjson_is_arr(camera_values)) ||
                (hierarchy != NULL && !yyjson_is_arr(hierarchy))) goto done;
        object->overview_position = object->position;
        if(overview_position != NULL && !editor_json_position_read(
                overview_position, &object->overview_position)) goto done;
        editor_project_object_name_format(object->name, sizeof(object->name), object->name);
        object->rigid_body_count = yyjson_arr_size(bodies);
        object->anchor_count = yyjson_arr_size(anchors);
        object->joint_count = yyjson_arr_size(joint_values);
        object->soft_body_count = yyjson_arr_size(soft_body_values);
        object->sprite_count = sprites == NULL ? 0 : yyjson_arr_size(sprites);
        object->animated_sprite_count = animated_sprite_values == NULL ? 0 :
            yyjson_arr_size(animated_sprite_values);
        object->camera_count = camera_values == NULL ? 0 :
            yyjson_arr_size(camera_values);
        if(object->camera_count > EDITOR_CAMERA_MAX) goto done;
        if(!EDITOR_ARRAY_RESERVE(object->rigid_bodies,
                object->rigid_body_capacity, object->rigid_body_count) ||
                !EDITOR_ARRAY_RESERVE(object->anchors, object->anchor_capacity,
                    object->anchor_count) ||
                !EDITOR_ARRAY_RESERVE(object->joint_items, object->joint_capacity,
                    object->joint_count) ||
                !EDITOR_ARRAY_RESERVE(object->soft_body_items,
                    object->soft_body_capacity, object->soft_body_count) ||
                !EDITOR_ARRAY_RESERVE(object->sprites, object->sprite_capacity,
                    object->sprite_count) ||
                !EDITOR_ARRAY_RESERVE(object->animated_sprite_items,
                    object->animated_sprite_capacity,
                    object->animated_sprite_count) ||
                !EDITOR_ARRAY_RESERVE(object->cameras, object->camera_capacity,
                    object->camera_count)) goto done;
        if(object->rigid_body_count > 0) memset(object->rigid_bodies, 0,
            object->rigid_body_count * sizeof(*object->rigid_bodies));
        if(object->anchor_count > 0) memset(object->anchors, 0,
            object->anchor_count * sizeof(*object->anchors));
        if(object->joint_count > 0) memset(object->joint_items, 0,
            object->joint_count * sizeof(*object->joint_items));
        if(object->soft_body_count > 0) memset(object->soft_body_items, 0,
            object->soft_body_count * sizeof(*object->soft_body_items));
        for(size_t j = 0; j < object->sprite_count; j += 1) {
            yyjson_val *sprite_value = yyjson_arr_get(sprites, j);
            yyjson_val *path_value = yyjson_obj_get(sprite_value, "path");
            EditorSprite *sprite = &object->sprites[j];
            if(!yyjson_is_obj(sprite_value) ||
                    !editor_json_uint(sprite_value, "id", &sprite->id) ||
                    sprite->id == 0 || !editor_json_name(sprite_value, sprite->name) ||
                    !yyjson_is_str(path_value) || yyjson_get_len(path_value) == 0 ||
                    yyjson_get_len(path_value) >= EDITOR_ASSET_PATH_MAX ||
                    !editor_json_position_read(yyjson_obj_get(sprite_value, "position"),
                        &sprite->position) ||
                    !editor_json_optional_real(sprite_value, "rotation",
                        &sprite->rotation, 0.0f) ||
                    !editor_json_uint(sprite_value, "rigid_body", &sprite->rigid_body) ||
                    !editor_json_real(sprite_value, "width", &sprite->size.x) ||
                    !editor_json_real(sprite_value, "height", &sprite->size.y) ||
                    !editor_json_bool(sprite_value, "follow_body_rotation",
                        &sprite->follow_body_rotation) ||
                    !editor_json_bool(sprite_value, "visible", &sprite->visible) ||
                    !editor_json_graphics_layer_binding_read(sprite_value,
                        &sprite->graphics_layer) ||
                    sprite->size.x <= 0.0f || sprite->size.y <= 0.0f) goto done;
            editor_project_property_name_format(sprite->name, sizeof(sprite->name),
                sprite->name);
            memcpy(sprite->path, yyjson_get_str(path_value),
                yyjson_get_len(path_value) + 1);
            if(loaded.next_sprite_id <= sprite->id)
                loaded.next_sprite_id = sprite->id + 1;
        }
        if(object->animated_sprite_count > 0) memset(object->animated_sprite_items, 0,
            object->animated_sprite_count * sizeof(*object->animated_sprite_items));
        for(size_t j = 0; j < object->rigid_body_count; j += 1)
            if(!editor_json_body_read(yyjson_arr_get(bodies, j),
                    &object->rigid_bodies[j], &loaded)) goto done;
        for(size_t j = 0; j < object->anchor_count; j += 1)
            if(!editor_json_anchor_read(yyjson_arr_get(anchors, j),
                    &object->anchors[j], &loaded)) goto done;
        for(size_t j = 0; j < object->joint_count; j += 1)
            if(!editor_json_joint_read(yyjson_arr_get(joint_values, j),
                    &object->joint_items[j], &loaded)) goto done;
        for(size_t j = 0; j < object->soft_body_count; j += 1)
            if(!editor_json_soft_body_read(yyjson_arr_get(soft_body_values, j),
                    &object->soft_body_items[j], &loaded)) goto done;
        for(size_t j = 0; j < object->animated_sprite_count; j += 1)
            if(!editor_json_animated_sprite_read(
                    yyjson_arr_get(animated_sprite_values, j),
                    &object->animated_sprite_items[j], &loaded)) goto done;
        for(size_t j = 0; j < object->camera_count; j += 1) {
            yyjson_val *item = yyjson_arr_get(camera_values, j);
            EditorCamera *camera = &object->cameras[j];
            uint32_t kind;
            yyjson_val *zoom;
            if(!yyjson_is_obj(item) ||
                    !editor_json_uint(item, "id", &camera->id) || camera->id == 0 ||
                    !editor_json_name(item, camera->name) ||
                    !editor_json_position_read(yyjson_obj_get(item, "position"),
                        &camera->position) ||
                    !editor_json_real(item, "rotation", &camera->rotation) ||
                    !editor_json_real(item, "width", &camera->dimensions.x) ||
                    !editor_json_real(item, "height", &camera->dimensions.y) ||
                    !editor_json_uint(item, "attachment_kind", &kind) ||
                    kind > EDITOR_CAMERA_ATTACHMENT_ANCHOR ||
                    !editor_json_uint(item, "attachment", &camera->attachment) ||
                    !editor_json_uint(item, "attachment_soft_body",
                        &camera->attachment_soft_body) ||
                    !editor_json_bool(item, "inherit_orientation",
                        &camera->inherit_orientation) ||
                    !editor_json_bool(item, "visible", &camera->visible) ||
                    camera->dimensions.x <= 0.0f || camera->dimensions.y <= 0.0f)
                goto done;
            zoom = yyjson_obj_get(item, "zoom");
            if(zoom != NULL && (!yyjson_is_num(zoom) ||
                    (camera->zoom = (float)yyjson_get_real(zoom)) <= 0.0f)) goto done;
            if(zoom == NULL) camera->zoom = 1.0f;
            camera->attachment_kind = (EditorCameraAttachmentKind)kind;
            editor_project_property_name_format(camera->name, sizeof(camera->name),
                camera->name);
            if(loaded.next_camera_id <= camera->id)
                loaded.next_camera_id = camera->id + 1;
        }
        if(hierarchy != NULL) {
            object->hierarchy_count = yyjson_arr_size(hierarchy);
            if(!EDITOR_ARRAY_RESERVE(object->hierarchy,
                    object->hierarchy_capacity, object->hierarchy_count)) goto done;
            for(size_t j = 0; j < object->hierarchy_count; j += 1) {
                yyjson_val *item = yyjson_arr_get(hierarchy, j);
                uint32_t kind;
                if(!yyjson_is_obj(item) || !editor_json_uint(item, "kind", &kind) ||
                        kind > EDITOR_HIERARCHY_CAMERA ||
                        !editor_json_uint(item, "id", &object->hierarchy[j].id) ||
                        object->hierarchy[j].id == 0) goto done;
                object->hierarchy[j].kind = (EditorHierarchyItemKind)kind;
            }
        }
        {
            size_t serialized_count = object->hierarchy_count;
            size_t expected_count = object->rigid_body_count + object->joint_count +
                object->soft_body_count + object->sprite_count +
                object->animated_sprite_count + object->camera_count;
            editor_project_object_hierarchy_sync(object);
            if(hierarchy != NULL && (object->hierarchy_count != serialized_count ||
                    object->hierarchy_count != expected_count)) goto done;
        }
        if(loaded.next_id <= object->id) loaded.next_id = object->id + 1;
    }
    loaded.layout_viewport_count = layout_viewports == NULL ? 0 :
        yyjson_arr_size(layout_viewports);
    if(loaded.layout_viewport_count > EDITOR_LAYOUT_VIEWPORT_MAX ||
            !EDITOR_ARRAY_RESERVE(loaded.layout_viewports,
                loaded.layout_viewport_capacity, loaded.layout_viewport_count)) goto done;
    if(loaded.layout_viewport_count > 0) memset(loaded.layout_viewports, 0,
        loaded.layout_viewport_count * sizeof(*loaded.layout_viewports));
    for(size_t i = 0; i < loaded.layout_viewport_count; i += 1) {
        yyjson_val *value = yyjson_arr_get(layout_viewports, i);
        yyjson_val *camera_items = yyjson_obj_get(value, "screen_items");
        if(camera_items == NULL) camera_items = yyjson_obj_get(value, "camera_items");
        yyjson_val *ui_items = yyjson_obj_get(value, "ui_items");
        yyjson_val *overview_position = yyjson_obj_get(value, "overview_position");
        EditorLayoutViewport *viewport = &loaded.layout_viewports[i];
        uint32_t fit;
        viewport->config = rohr_viewport_config_default_get();
        viewport->background_color = 0x000000FFu;
        if(!yyjson_is_obj(value) ||
                !editor_json_uint(value, "id", &viewport->id) || viewport->id == 0 ||
                !editor_json_name(value, viewport->name) ||
                !editor_json_real(value, "x", &viewport->config.rectangle.x) ||
                !editor_json_real(value, "y", &viewport->config.rectangle.y) ||
                !editor_json_real(value, "width", &viewport->config.rectangle.width) ||
                !editor_json_real(value, "height", &viewport->config.rectangle.height) ||
                !editor_json_uint(value, "fit", &fit) || fit > SCREEN_FIT_COVER ||
                !editor_json_bool(value, "enabled", &viewport->enabled) ||
                !yyjson_is_arr(camera_items) ||
                viewport->config.rectangle.width <= 0.0f ||
                viewport->config.rectangle.height <= 0.0f) goto done;
        /* Runtime viewport rectangles use screen-space Y. The project overview
         * uses world-space Y so dragging matches objects. Preserve the legacy
         * on-screen placement while converting coordinate conventions. */
        viewport->overview_position = (Position){viewport->config.rectangle.x,
            -viewport->config.rectangle.y};
        if(overview_position != NULL && !editor_json_position_read(
                overview_position, &viewport->overview_position)) goto done;
        if(yyjson_obj_get(value, "background_color") != NULL &&
                !editor_json_uint(value, "background_color",
                    &viewport->background_color)) goto done;
        viewport->config.fit = (ScreenFit)fit;
        editor_project_object_name_format(viewport->name, sizeof(viewport->name),
            viewport->name);
        viewport->camera_item_count = yyjson_arr_size(camera_items);
        if(viewport->camera_item_count > EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX ||
                !EDITOR_ARRAY_RESERVE(viewport->camera_items,
                    viewport->camera_item_capacity,
                    viewport->camera_item_count)) goto done;
        for(size_t j = 0; j < viewport->camera_item_count; j += 1) {
            yyjson_val *item_value = yyjson_arr_get(camera_items, j);
            EditorViewportCameraItem *item = &viewport->camera_items[j];
            uint32_t item_fit;
            yyjson_val *content_x = yyjson_obj_get(item_value, "content_x");
            yyjson_val *content_y = yyjson_obj_get(item_value, "content_y");
            yyjson_val *content_width = yyjson_obj_get(item_value,
                "content_width_scale");
            yyjson_val *content_height = yyjson_obj_get(item_value,
                "content_height_scale");
            yyjson_val *content_rotation = yyjson_obj_get(item_value,
                "content_rotation");
            yyjson_val *drag_mode = yyjson_obj_get(item_value, "drag_mode");
            item->content_scale = (Scale){1.0f, 1.0f};
            yyjson_val *graphics_layer_value = yyjson_obj_get(item_value,
                "graphics_layer");
            if(!yyjson_is_obj(item_value) ||
                    !editor_json_uint(item_value, "id", &item->id) || item->id == 0 ||
                    !editor_json_name(item_value, item->name) ||
                    !editor_json_uint(item_value, "object", &item->object) ||
                    !editor_json_uint(item_value, "camera", &item->camera) ||
                    !editor_json_real(item_value, "x", &item->placement.rectangle.x) ||
                    !editor_json_real(item_value, "y", &item->placement.rectangle.y) ||
                    !editor_json_real(item_value, "width",
                        &item->placement.rectangle.width) ||
                    !editor_json_real(item_value, "height",
                        &item->placement.rectangle.height) ||
                    !editor_json_uint(item_value, "fit", &item_fit) ||
                    item_fit > SCREEN_FIT_COVER ||
                    !editor_json_real(item_value, "orientation",
                        &item->placement.orientation) ||
                    !editor_json_int(item_value, "layer", &item->placement.layer) ||
                    !editor_json_bool(item_value, "visible", &item->placement.visible) ||
                    item->placement.rectangle.width <= 0.0f ||
                    item->placement.rectangle.height <= 0.0f) goto done;
            if(drag_mode != NULL && (!yyjson_is_uint(drag_mode) ||
                    yyjson_get_uint(drag_mode) > VIEWPORT_ITEM_DRAG_XY)) goto done;
            item->placement.drag_mode = drag_mode == NULL ?
                VIEWPORT_ITEM_DRAG_NONE :
                (ViewportItemDragMode)yyjson_get_uint(drag_mode);
            if(graphics_layer_value != NULL &&
                    (!yyjson_is_uint(graphics_layer_value) ||
                    yyjson_get_uint(graphics_layer_value) > UINT32_MAX)) goto done;
            item->graphics_layer = graphics_layer_value == NULL ? 0 :
                (EditorGraphicsLayerId)yyjson_get_uint(graphics_layer_value);
            if((content_x != NULL && !editor_json_real(item_value, "content_x",
                        &item->content_offset.x)) ||
                    (content_y != NULL && !editor_json_real(item_value, "content_y",
                        &item->content_offset.y)) ||
                    (content_width != NULL && (!editor_json_real(item_value,
                        "content_width_scale", &item->content_scale.x) ||
                        item->content_scale.x <= 0.0f)) ||
                    (content_height != NULL && (!editor_json_real(item_value,
                        "content_height_scale", &item->content_scale.y) ||
                        item->content_scale.y <= 0.0f)) ||
                    (content_rotation != NULL && !editor_json_real(item_value,
                        "content_rotation", &item->content_rotation))) goto done;
            item->placement.fit = (ScreenFit)item_fit;
            editor_project_property_name_format(item->name, sizeof(item->name),
                item->name);
            if(strncmp(item->name, "camera_", 7) == 0)
                snprintf(item->name, sizeof(item->name), "screen_%u", item->id);
            if(loaded.next_viewport_camera_item_id <= item->id)
                loaded.next_viewport_camera_item_id = item->id + 1;
        }
        viewport->ui_item_count = ui_items == NULL ? 0 : yyjson_arr_size(ui_items);
        if((ui_items != NULL && !yyjson_is_arr(ui_items)) ||
                viewport->ui_item_count > EDITOR_LAYOUT_VIEWPORT_UI_MAX ||
                !EDITOR_ARRAY_RESERVE(viewport->ui_items,
                    viewport->ui_item_capacity, viewport->ui_item_count)) goto done;
        for(size_t j = 0; j < viewport->ui_item_count; j += 1) {
            yyjson_val *item_value = yyjson_arr_get(ui_items, j);
            EditorViewportUiItem *item = &viewport->ui_items[j];
            yyjson_val *definition_value;
            yyjson_val *graphics_layer_value;
            yyjson_val *drag_mode_value;
            *item = (EditorViewportUiItem){.scale = {1.0f, 1.0f}};
            if(!yyjson_is_obj(item_value) ||
                    !editor_json_uint(item_value, "id", &item->id) || item->id == 0 ||
                    !editor_json_name(item_value, item->name) ||
                    !editor_json_real(item_value, "x", &item->position.x) ||
                    !editor_json_real(item_value, "y", &item->position.y) ||
                    !editor_json_int(item_value, "layer", &item->layer) ||
                    !editor_json_bool(item_value, "visible", &item->visible))
                goto done;
            definition_value = yyjson_obj_get(item_value, "definition");
            if(definition_value != NULL) {
                if(!yyjson_is_uint(definition_value) ||
                        yyjson_get_uint(definition_value) == 0 ||
                        yyjson_get_uint(definition_value) > UINT32_MAX) goto done;
                item->definition = (EditorViewportUiDefinitionId)
                    yyjson_get_uint(definition_value);
            } else {
                EditorViewportUiDefinition legacy;
                if(loaded.ui_definition_count >= MAX_GRAPHICS_UI_ELEMENTS ||
                        !editor_json_ui_definition_read(item_value, &legacy) ||
                        !EDITOR_ARRAY_RESERVE(loaded.ui_definitions,
                            loaded.ui_definition_capacity,
                            loaded.ui_definition_count + 1)) goto done;
                legacy.id = loaded.next_ui_definition_id++;
                loaded.ui_definitions[loaded.ui_definition_count++] = legacy;
                item->definition = legacy.id;
            }
            if(!editor_json_optional_real(item_value, "scale_x", &item->scale.x,
                    1.0f) || !editor_json_optional_real(item_value, "scale_y",
                    &item->scale.y, 1.0f) || item->scale.x <= 0.0f ||
                    item->scale.y <= 0.0f || !editor_json_optional_real(item_value,
                    "rotation", &item->rotation, 0.0f) ||
                    !editor_json_optional_real(item_value, "clip_x",
                        &item->clip_rectangle.x, 0.0f) ||
                    !editor_json_optional_real(item_value, "clip_y",
                        &item->clip_rectangle.y, 0.0f) ||
                    !editor_json_optional_real(item_value, "clip_width",
                        &item->clip_rectangle.width, 0.0f) ||
                    !editor_json_optional_real(item_value, "clip_height",
                        &item->clip_rectangle.height, 0.0f)) goto done;
            {
                yyjson_val *clip_enabled = yyjson_obj_get(item_value, "clip_enabled");
                if(clip_enabled != NULL && !editor_json_bool(item_value,
                        "clip_enabled", &item->clip_enabled)) goto done;
            }
            graphics_layer_value = yyjson_obj_get(item_value, "graphics_layer");
            if(graphics_layer_value != NULL &&
                    (!yyjson_is_uint(graphics_layer_value) ||
                    yyjson_get_uint(graphics_layer_value) > UINT32_MAX)) goto done;
            item->graphics_layer = graphics_layer_value == NULL ? 0 :
                (EditorGraphicsLayerId)yyjson_get_uint(graphics_layer_value);
            drag_mode_value = yyjson_obj_get(item_value, "drag_mode");
            if(drag_mode_value != NULL && (!yyjson_is_uint(drag_mode_value) ||
                    yyjson_get_uint(drag_mode_value) > VIEWPORT_ITEM_DRAG_XY))
                goto done;
            item->drag_mode = drag_mode_value == NULL ? VIEWPORT_ITEM_DRAG_NONE :
                (ViewportItemDragMode)yyjson_get_uint(drag_mode_value);
            if(loaded.next_viewport_ui_item_id <= item->id)
                loaded.next_viewport_ui_item_id = item->id + 1;
        }
        if(loaded.next_layout_viewport_id <= viewport->id)
            loaded.next_layout_viewport_id = viewport->id + 1;
    }
    if(hierarchy != NULL) {
        loaded.hierarchy_count = yyjson_arr_size(hierarchy);
        if(loaded.hierarchy_count > loaded.object_count +
                    loaded.layout_viewport_count ||
                !EDITOR_ARRAY_RESERVE(loaded.hierarchy,
                    loaded.hierarchy_capacity, loaded.hierarchy_count)) goto done;
        for(size_t i = 0; i < loaded.hierarchy_count; i += 1) {
            yyjson_val *item = yyjson_arr_get(hierarchy, i);
            uint32_t kind;
            if(!yyjson_is_obj(item) || !editor_json_uint(item, "kind", &kind) ||
                    kind > EDITOR_PROJECT_HIERARCHY_VIEWPORT ||
                    !editor_json_uint(item, "id", &loaded.hierarchy[i].id) ||
                    loaded.hierarchy[i].id == 0) goto done;
            loaded.hierarchy[i].kind = (EditorProjectHierarchyItemKind)kind;
        }
    }
    {
        size_t serialized_count = loaded.hierarchy_count;
        editor_project_hierarchy_sync(&loaded);
        if(hierarchy != NULL && (loaded.hierarchy_count != serialized_count ||
                loaded.hierarchy_count != loaded.object_count +
                    loaded.layout_viewport_count)) goto done;
    }
    if(!editor_project_ui_definitions_refresh(&loaded)) goto done;
    if(!editor_json_references_valid(&loaded)) {
        result = editor_result_error(EDITOR_ERROR_REFERENCE_INVALID,
            "Project editor state '%s' contains an invalid entity, joint, beam, or collision-mask reference",
            path);
        goto done;
    }
    editor_project_destroy(project);
    *project = loaded;
    loaded = (EditorProject){0};
    result = editor_result_value(true);
done:
    yyjson_doc_free(document);
    return result;
}
