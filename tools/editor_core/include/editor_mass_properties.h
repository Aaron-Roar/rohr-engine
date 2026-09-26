/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_MASS_PROPERTIES_H
#define EDITOR_MASS_PROPERTIES_H

#include "editor_command.h"

typedef struct EditorMassProperties {
    Position center;
    float inertia;
    bool center_available;
    bool inertia_available;
    bool angular_response;
} EditorMassProperties;

/* Pure query of the authored active geometry; owns no runtime entities. */
EditorMassProperties editor_mass_properties_get(const EditorRigidBody *body);
EditorCommandResult editor_center_of_mass_set(EditorProject *project,
    EditorObjectId object, EditorRigidBodyId body, bool explicit_mode,
    Position offset);
EditorCommandResult editor_center_of_mass_mode_set(EditorProject *project,
    EditorObjectId object, EditorRigidBodyId body, bool explicit_mode);

#endif
