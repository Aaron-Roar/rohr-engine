/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_mass_properties.h"
#include <assert.h>
#include <math.h>

static bool near(float a, float b) { return fabsf(a - b) < 0.002f; }

int main(void) {
    EditorProject project;
    editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){10, 20});
    assert(object != NULL);
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    assert(body != NULL);
    EditorHitbox *box = &body->hitboxes[0];
    Shape rectangle = {.amount_of_vertices = 4,
        .vertices = {{2, 3}, {6, 3}, {6, 5}, {2, 5}}};
    box->vertex_count = rectangle.amount_of_vertices;
    for(size_t i = 0; i < box->vertex_count; i += 1)
        box->vertices[i].position = rectangle.vertices[i];
    body->mass_value = 12;
    body->position = (Position){50, -40};
    body->rotation = 0.9f;
    EditorMassProperties properties = editor_mass_properties_get(body);
    assert(properties.center_available && properties.inertia_available);
    assert(near(properties.center.x, 4) && near(properties.center.y, 4));
    assert(near(properties.inertia, 20));
    assert(editor_center_of_mass_mode_set(&project, object->id, body->id, true).kind ==
        ERROR_RESULT_VALUE);
    assert(body->center_of_mass_explicit && near(body->center_of_mass_offset.x, 4));
    assert(near(editor_mass_properties_get(body).inertia, 20));
    assert(editor_center_of_mass_set(&project, object->id, body->id, true,
        (Position){3, -2}).kind == ERROR_RESULT_VALUE);
    properties = editor_mass_properties_get(body);
    assert(near(properties.inertia, 464));
    assert(editor_center_of_mass_set(&project, object->id, body->id, true,
        (Position){NAN, 0}).kind == ERROR_RESULT_ERROR);
    assert(near(body->center_of_mass_offset.x, 3) &&
        near(body->center_of_mass_offset.y, -2));
    EditorRigidBody duplicate = {0};
    assert(editor_project_rigid_body_clone(&duplicate, body));
    duplicate.center_of_mass_offset.x = 100;
    duplicate.hitboxes[0].vertices[0].position.x = 100;
    assert(near(body->center_of_mass_offset.x, 3) &&
        near(box->vertices[0].position.x, 2));
    editor_project_rigid_body_destroy(&duplicate);

    /* Compare the editor readout to the actual runtime API. */
    assert(!rohr_error_check(rohr_engine_start()));
    EntityResult created = rohr_entity_add();
    assert(!rohr_error_check(created));
    Entity entity = created.result.value;
    assert(!rohr_error_check(rohr_physics_mass_set(entity, 12)));
    assert(!rohr_error_check(rohr_physics_hitbox_set(entity, rectangle)));
    assert(!rohr_error_check(rohr_physics_center_of_mass_local_position_set(entity,
        body->center_of_mass_offset)));
    MomentOfInertiaResult inertia = rohr_physics_moment_of_inertia_get(entity);
    assert(!rohr_error_check(inertia) && near(inertia.result.value, properties.inertia));
    rohr_engine_stop();

    body->static_body = true;
    properties = editor_mass_properties_get(body);
    assert(properties.inertia_available && near(properties.inertia, 464) &&
        !properties.angular_response);
    body->static_body = false;
    body->rotation_locked = true;
    assert(!editor_mass_properties_get(body).angular_response);
    body->rotation_locked = false;
    body->particle = true;
    assert(editor_mass_properties_get(body).center_available &&
        !editor_mass_properties_get(body).angular_response);
    body->standalone_particle = true;
    body->center_of_mass_explicit = false;
    body->center_of_mass_offset = (Position){0};
    body->particle_origin = (Position){7, 8};
    properties = editor_mass_properties_get(body);
    assert(near(properties.center.x, 7) && near(properties.center.y, 8));
    assert(editor_center_of_mass_mode_set(&project, object->id, body->id, true).kind ==
        ERROR_RESULT_ERROR);
    body->standalone_particle = false;
    body->particle = false;
    assert(editor_center_of_mass_mode_set(&project, object->id, body->id, false).kind ==
        ERROR_RESULT_VALUE);
    assert(!body->center_of_mass_explicit && body->center_of_mass_offset.x == 0 &&
        body->center_of_mass_offset.y == 0);
    box->vertex_count = 2;
    properties = editor_mass_properties_get(body);
    assert(!properties.center_available && !properties.inertia_available);
    assert(editor_center_of_mass_mode_set(&project, object->id, body->id, true).kind ==
        ERROR_RESULT_VALUE);
    assert(body->center_of_mass_offset.x == 0 && body->center_of_mass_offset.y == 0);
    properties = editor_mass_properties_get(body);
    assert(properties.center_available && !properties.inertia_available);
    /* Active variant changes update automatic COM immediately; visibility is
     * presentation-only and must not remove authored mass geometry. */
    EditorHitbox *variant = editor_project_hitbox_add(&project, body);
    assert(variant != NULL);
    variant->vertex_count = 4;
    for(size_t i = 0; i < 4; i += 1) {
        variant->vertices[i].position = rectangle.vertices[3 - i];
        variant->vertices[i].position.x += 10;
    }
    variant->visible = false;
    body->active_hitbox_index = 1;
    assert(editor_center_of_mass_mode_set(&project, object->id, body->id, false).kind ==
        ERROR_RESULT_VALUE);
    properties = editor_mass_properties_get(body);
    assert(properties.center_available && near(properties.center.x, 14) &&
        near(properties.inertia, 20));
    editor_project_destroy(&project);
    return 0;
}
