/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_mass_properties.h"
#include "physics/physics_internal.h"
#include "physics/collision/shape_decomposition.h"

#include "physics/soft_body/soft_body_mass.h"
#include <float.h>
#include <math.h>

EditorMassProperties editor_mass_properties_get(const EditorRigidBody *body) {
    EditorMassProperties result = {0};
    Shape shape = {0}, prepared;
    Position centroid;
    double unit_inertia;
    if(body == NULL) return result;
    result.angular_response = !body->static_body && !body->rotation_locked &&
        !body->particle;
    if(body->standalone_particle) {
        result.center = body->particle_origin;
        result.center_available = isfinite(result.center.x) && isfinite(result.center.y);
    } else if(body->center_of_mass_explicit) {
        result.center = body->center_of_mass_offset;
        result.center_available = isfinite(result.center.x) && isfinite(result.center.y);
    }
    if(body->hitbox_count == 0 || body->hitboxes == NULL) return result;
    const EditorHitbox *box = &body->hitboxes[
        body->active_hitbox_index < body->hitbox_count ? body->active_hitbox_index : 0];
    if(box->vertices == NULL || box->vertex_count > MAX_VERTICIES) return result;
    shape.amount_of_vertices = box->vertex_count;
    for(size_t i = 0; i < box->vertex_count; i += 1)
        shape.vertices[i] = box->vertices[i].position;
    if(!physics_shape_collision_prepare(shape, &prepared) ||
            !physics_shape_mass_properties_get(&prepared, &centroid, &unit_inertia))
        return result;
    if(!body->standalone_particle && !body->center_of_mass_explicit) {
        result.center = centroid;
        result.center_available = true;
    }
    double dx = (double)centroid.x - result.center.x;
    double dy = (double)centroid.y - result.center.y;
    double inertia = body->mass_value * (unit_inertia + dx * dx + dy * dy);
    if(result.center_available && isfinite(inertia) && inertia >= 0 &&
            inertia <= FLT_MAX && (inertia == 0 || (float)inertia > 0)) {
        result.inertia = (float)inertia;
        result.inertia_available = true;
    }
    return result;
}

EditorCommandResult editor_center_of_mass_set(EditorProject *project,
        EditorObjectId object, EditorRigidBodyId body, bool explicit_mode,
        Position offset) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY,
            .object = object, .item = body,
            .property = EDITOR_PROPERTY_CENTER_OF_MASS,
            .value_kind = EDITOR_PROPERTY_VALUE_CENTER_OF_MASS,
            .value.center_of_mass = {explicit_mode,
                explicit_mode ? offset : (Position){0}}}};
    return editor_command_execute(project, &command);
}

EditorCommandResult editor_center_of_mass_mode_set(EditorProject *project,
        EditorObjectId object, EditorRigidBodyId body, bool explicit_mode) {
    EditorObject *owner = NULL;
    if(project != NULL)
        for(size_t i = 0; i < project->object_count; i += 1)
            if(project->objects[i].id == object) owner = &project->objects[i];
    EditorMassProperties current = editor_mass_properties_get(
        editor_project_rigid_body_get(owner, body));
    return editor_center_of_mass_set(project, object, body, explicit_mode,
        current.center_available ? current.center : (Position){0});
}

EditorSoftMassProperties editor_soft_mass_properties_get(const EditorSoftBody *body) {
    EditorSoftMassProperties result = {0};
    double total = 0, x = 0, y = 0;
    if(body == NULL || body->node_count > SOFT_BODY_MAX_NODES ||
            (body->node_count > 0 && body->nodes == NULL)) return result;
    for(size_t i = 0; i < body->node_count; i++) {
        const EditorSoftNode *node = &body->nodes[i];
        if(!isfinite(node->node_mass) || node->node_mass < 0 ||
                !isfinite(node->position.x) || !isfinite(node->position.y)) return result;
        total += node->node_mass;
        x += (double)node->position.x * node->node_mass;
        y += (double)node->position.y * node->node_mass;
    }
    if(total > FLT_MAX) return result;
    result.mass_available = true;
    result.total = (float)total;
    if(total > 0) {
        result.center = (Position){(float)(x / total), (float)(y / total)};
        result.center_available = true;
    }
    return result;
}

bool editor_soft_mass_values_set(EditorSoftBody *body, bool redistribute,
        float total, Position center) {
    Mass prior[SOFT_BODY_MAX_NODES], next[SOFT_BODY_MAX_NODES];
    Position points[SOFT_BODY_MAX_NODES];
    if(body == NULL || body->node_count == 0 || body->node_count > SOFT_BODY_MAX_NODES ||
            body->nodes == NULL) return false;
    for(size_t i = 0; i < body->node_count; i++) {
        prior[i] = body->nodes[i].node_mass;
        points[i] = body->nodes[i].position;
    }
    if(!(redistribute ? physics_soft_mass_distribution_get(points, prior,
            body->node_count, center, next) : physics_soft_mass_scale_get(prior,
            body->node_count, total, next))) return false;
    for(size_t i = 0; i < body->node_count; i++) body->nodes[i].node_mass = next[i];
    return true;
}

EditorCommandResult editor_soft_center_of_mass_set(EditorProject *project,
        EditorObjectId object, EditorSoftBodyId body, Position offset) {
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {.kind = EDITOR_ITEM_SOFT_BODY,
            .object = object, .item = body,
            .property = EDITOR_PROPERTY_CENTER_OF_MASS,
            .value_kind = EDITOR_PROPERTY_VALUE_CENTER_OF_MASS,
            .value.center_of_mass = {true, offset}}};
    return editor_command_execute(project, &command);
}
