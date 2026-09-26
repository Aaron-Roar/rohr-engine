/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_RIGID_BODY_H
#define EDITOR_RIGID_BODY_H

#include "editors/editor_mode_context.h"
#include "editor_center_of_mass.h"
#include "editors/editor_mode_controls.h"

typedef bool (*EditorRigidBodyCollisionMenuFunction)(void *context,
    const char *id_prefix, EditorProject *project, uint64_t *active_masks,
    EditorObjectId object, EditorRigidBodyId body,
    EditorCollisionFilterKind filter, float x, float y, float width,
    bool *field_active, size_t *row_count);

typedef struct EditorRigidBodyEditor {
    FontAsset *font;
    EditorCenterOfMassEditor center_of_mass;
    TextAsset name_label, x_label, y_label, rotation_label;
    TextAsset velocity_x_label, velocity_y_label, acceleration_x_label;
    TextAsset acceleration_y_label, angular_velocity_label;
    TextAsset mass_label, friction_label, restitution_label;
    TextAsset border_color_label, surface_color_label;
    TextAsset parent_label, none_label;
    TextAsset gravity_label, dynamic_label, static_label;
    TextAsset rotation_unlocked_label, rotation_locked_label;
    TextAsset collision_label, particle_label;
    TextAsset collision_category_label, collide_with_label;
    TextAsset origin_label, active_hitbox_label, add_hitbox_label, delete_label;
    TextAsset bind_frames_label;
    TextAsset visibility_label, visible_label, hidden_label;
    TextAsset x_field, y_field, rotation_field;
    TextAsset velocity_x_field, velocity_y_field, acceleration_x_field;
    TextAsset acceleration_y_field, angular_velocity_field;
    TextAsset mass_field, friction_field, restitution_field;
    TextAsset body_names[EDITOR_RIGID_BODY_MAX];
    TextAsset parent_names[EDITOR_RIGID_BODY_MAX];
    TextAsset hitbox_names[EDITOR_BODY_HITBOX_MAX];
    TextAsset frame_names[MAX_ANIMATIONS_FRAMES];
    char body_cache[EDITOR_RIGID_BODY_MAX][EDITOR_OBJECT_NAME_MAX];
    char parent_cache[EDITOR_RIGID_BODY_MAX][EDITOR_OBJECT_NAME_MAX];
    char hitbox_cache[EDITOR_BODY_HITBOX_MAX][EDITOR_OBJECT_NAME_MAX];
    char frame_cache[MAX_ANIMATIONS_FRAMES][EDITOR_OBJECT_NAME_MAX];
    bool collision_category_open;
    bool collide_with_open;
    EditorHitboxId binding_hitbox_open;
    EditorModeAccordionSection transform_section;
    EditorModeAccordionSection initial_motion_section;
    EditorModeAccordionSection physics_section;
    EditorModeAccordionSection material_section;
    EditorModeAccordionSection collision_section;
    EditorModeAccordionSection parenting_section;
    EditorModeAccordionSection appearance_section;
    EditorModeAccordionSection geometry_section;
} EditorRigidBodyEditor;

bool editor_rigid_body_editor_create(EditorRigidBodyEditor *editor,
    FontAsset *font);
void editor_rigid_body_editor_destroy(EditorRigidBodyEditor *editor);
bool editor_rigid_body_editor_draw(EditorRigidBodyEditor *editor,
    const EditorModeContext *context,
    EditorRigidBodyCollisionMenuFunction collision_menu,
    void *collision_menu_context);

#endif
