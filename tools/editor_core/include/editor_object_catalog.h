/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_OBJECT_CATALOG_H
#define EDITOR_OBJECT_CATALOG_H
#include "editor_project.h"
#include <stdio.h>
bool editor_object_catalog_write(FILE *header, FILE *source, const EditorProject *project);
#endif
