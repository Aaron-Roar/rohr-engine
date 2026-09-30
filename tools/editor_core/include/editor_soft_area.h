/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SOFT_AREA_H
#define ROHR_EDITOR_SOFT_AREA_H
#include "editor_project.h"

/* Returned vertices are local to the soft body and owned by the area's cache.
 * First access and changed poses may allocate/grow derived storage.
 * They remain valid until its geometry is refreshed or the area is destroyed. */
typedef struct EditorSoftAreaFill {
    const Vec2D *vertices;
    size_t vertex_count;
    bool complete;
    uint32_t incomplete_loop; /* zero is outer; holes are one-based */
} EditorSoftAreaFill;

EditorSoftArea *editor_soft_area_add(EditorSoftBody *body);
EditorSoftHole *editor_soft_hole_add(EditorSoftArea *area);
void editor_soft_area_cache_destroy(EditorSoftArea *area);
bool editor_soft_area_remove(EditorSoftBody *body, EditorSoftAreaId id);
bool editor_soft_area_references_check(const EditorSoftBody *body,
    const EditorSoftArea *area);
bool editor_soft_area_node_check(const EditorSoftArea *area, EditorSoftNodeId id);
EditorSoftAreaFill editor_soft_area_fill_get(const EditorSoftBody *body,
    EditorSoftArea *area);
bool editor_soft_area_point_check(const EditorSoftBody *body,
    EditorSoftArea *area, Position local);
EditorResult editor_soft_areas_generation_validate(const EditorProject *project);
#endif
