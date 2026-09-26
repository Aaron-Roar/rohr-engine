/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_mass_properties.h"
#include "physics/physics_internal.h"
#include "physics/collision/shape_decomposition.h"

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
