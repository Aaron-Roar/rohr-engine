/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_OBJECT_CATALOG_H
#define EDITOR_OBJECT_CATALOG_H
#include "editor_project.h"
#include <stdio.h>
void editor_catalog_input_binding_write(FILE *source, const InputBinding *binding);
bool editor_object_catalog_write(FILE *header, FILE *source, const EditorProject *project);
bool editor_project_catalog_write(FILE *header, FILE *source, const EditorProject *project);
EditorResult editor_project_catalog_validate(const EditorProject *project);
#endif
