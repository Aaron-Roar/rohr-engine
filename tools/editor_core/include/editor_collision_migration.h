/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_COLLISION_MIGRATION_H
#define ROHR_EDITOR_COLLISION_MIGRATION_H

#include "editor_error.h"
#include <stddef.h>

typedef struct EditorCollisionMigration EditorCollisionMigration;

/* Explicit allocation; destroy the plan even after apply fails. Supply all
 * interacting project/state files together. Preparation never writes files. */
EditorResult editor_collision_migration_prepare(const char *const *paths,
    size_t count, EditorCollisionMigration **out);
/* Borrowed JSON, valid until destroy; includes per-profile and named-bit maps. */
const char *editor_collision_migration_report_get(const EditorCollisionMigration *plan);
/* Refuses changed inputs or existing sidecars. Keeps .collision_original copies
 * and a .collision_migration.json report beside the first input. */
EditorResult editor_collision_migration_apply(EditorCollisionMigration *plan);
void editor_collision_migration_destroy(EditorCollisionMigration *plan);

#endif
