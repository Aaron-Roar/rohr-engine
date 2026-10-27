/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_DIAGNOSTICS_H
#define ROHR_EDITOR_DIAGNOSTICS_H
#include "editor_command.h"
#include "editor_viewport.h"
#include "editor_workspace.h"

#define EDITOR_DIAGNOSTIC_COLOR ((Color){235, 75, 80, 255})
/* Derived editor state only. Entries own their text and contain stable item IDs.
 * Refresh replaces the snapshot; destroy releases its sole owned allocation. */
typedef struct EditorDiagnostic {
    EditorSelectionRef target;
    char fields[192]; /* comma-separated exact control IDs or final components */
    char reason[256];
} EditorDiagnostic;
typedef struct EditorDiagnostics {
    EditorDiagnostic *items;
    size_t count, capacity;
    bool allocation_failed;
} EditorDiagnostics;
void editor_diagnostics_refresh(EditorDiagnostics *diagnostics, EditorProject *project,
    const EditorWorkspace *workspace);
void editor_diagnostics_destroy(EditorDiagnostics *diagnostics);
bool editor_diagnostic_target_check(EditorSelectionRef target, EditorSelectionRef item,
    bool descendants);
const EditorDiagnostic *editor_diagnostics_item_get(const EditorDiagnostics *diagnostics,
    EditorSelectionRef item, bool descendants);
bool editor_diagnostics_field_check(const EditorDiagnostics *diagnostics,
    EditorSelectionRef item, const char *control);
bool editor_diagnostics_section_check(const EditorDiagnostics *diagnostics,
    EditorSelectionRef item, const char *section);
bool editor_diagnostics_contains_check(const EditorDiagnostics *diagnostics,
    const EditorDiagnostic *error);
EditorSelectionRef editor_diagnostic_entity_get(EditorObject *object, ObjectEntityReference reference);
EditorSelectionRef editor_diagnostic_command_target_get(const EditorCommand *command);
#endif
