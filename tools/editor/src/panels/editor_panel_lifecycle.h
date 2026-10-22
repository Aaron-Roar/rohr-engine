/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_PANEL_LIFECYCLE_H
#define ROHR_EDITOR_PANEL_LIFECYCLE_H
#include "editor_navigation.h"

/* Only navigation ancestors need retention; this exceeds supported nesting. */
#define EDITOR_PANEL_HISTORY_MAX 128
typedef struct EditorPanelKey {
    EditorViewportMode mode;
    EditorSelectionRef item;
    EditorOriginKind origin;
    bool text_child;
    bool bulk;
} EditorPanelKey;
typedef struct EditorPanelVisit {
    EditorPanelKey key;
    float offset, measured_height;
} EditorPanelVisit;
typedef struct EditorPanelLifecycle {
    EditorPanelVisit current, previous[EDITOR_PANEL_HISTORY_MAX];
    size_t count;
    bool initialized, back_requested;
} EditorPanelLifecycle;

EditorPanelKey editor_panel_key_get(EditorProject *project, const EditorViewportState *state);
bool editor_panel_key_equal(EditorPanelKey a, EditorPanelKey b);
bool editor_panel_lifecycle_update(EditorPanelLifecycle *panel, EditorProject *project,
    const EditorViewportState *state);
void editor_panel_back(EditorPanelLifecycle *panel, EditorViewportState *state);
float editor_panel_body_top_get(EditorViewportMode mode, bool has_name);
#endif
