/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_panel_lifecycle.h"
#include "editor_layout.h"
#include <string.h>

EditorPanelKey editor_panel_key_get(EditorProject *project, const EditorViewportState *state) {
    EditorPanelKey key = {.mode = state->mode, .bulk = state->selected_item_count > 1};
    EditorViewportState opened = *state;
    if(state->mode == EDITOR_VIEWPORT_HIERARCHY) return key;
    if(!editor_navigation_panel_visibility_ref_get(project, state, &key.item)) {
        if(editor_navigation_open_item_selection_set(&opened))
            (void)editor_viewport_selection_ref_get(project, &opened, &key.item);
        else (void)editor_viewport_selection_ref_get(project, state, &key.item);
    }
    if(state->mode == EDITOR_VIEWPORT_ORIGIN) key.origin = state->selected_origin_kind;
    if(state->mode == EDITOR_VIEWPORT_UI_TEXT_EDITOR) key.text_child = state->selected_viewport_ui_text_child;
    return key;
}
bool editor_panel_key_equal(EditorPanelKey a, EditorPanelKey b) {
    return a.mode == b.mode && a.bulk == b.bulk && a.origin == b.origin && a.text_child == b.text_child &&
        a.item.kind == b.item.kind && a.item.object == b.item.object &&
        a.item.parent == b.item.parent && a.item.container == b.item.container && a.item.item == b.item.item;
}
static bool visit_valid(EditorProject *project, EditorPanelVisit visit) {
    char name[EDITOR_OBJECT_NAME_MAX];
    return visit.key.mode == EDITOR_VIEWPORT_HIERARCHY ||
        editor_navigation_selection_name_get(project, visit.key.item, name, sizeof(name));
}
bool editor_panel_lifecycle_update(EditorPanelLifecycle *panel, EditorProject *project,
        const EditorViewportState *state) {
    EditorPanelKey next = editor_panel_key_get(project, state);
    for(size_t i = 0; i < panel->count;) {
        if(visit_valid(project, panel->previous[i])) { ++i; continue; }
        memmove(&panel->previous[i], &panel->previous[i + 1],
            (--panel->count - i) * sizeof(panel->previous[0]));
    }
    bool back = panel->back_requested;
    panel->back_requested = false;
    if(panel->initialized && editor_panel_key_equal(panel->current.key, next)) return false;
    EditorPanelVisit incoming = {.key = next};
    if(back) {
        for(size_t i = panel->count; i > 0; --i) if(editor_panel_key_equal(panel->previous[i - 1].key, next)) {
            incoming = panel->previous[i - 1]; panel->count = i - 1; break;
        }
    } else if(panel->initialized && visit_valid(project, panel->current)) {
        if(panel->count == EDITOR_PANEL_HISTORY_MAX) {
            memmove(panel->previous, panel->previous + 1,
                (--panel->count) * sizeof(panel->previous[0]));
        }
        panel->previous[panel->count++] = panel->current;
    }
    panel->current = incoming; panel->initialized = true;
    return true;
}
void editor_panel_back(EditorPanelLifecycle *panel, EditorViewportState *state) {
    panel->back_requested = true;
    editor_viewport_back(state);
}
float editor_panel_body_top_get(EditorViewportMode mode, bool has_name) {
    if(!has_name) return 0;
    /* Existing editor layouts retain their local coordinates. Their name row
     * is supplied by the fixed header and clipped out of the property area. */
    if(mode == EDITOR_VIEWPORT_OBJECT) return 96;
    if(mode == EDITOR_VIEWPORT_INPUT_CONTROLLER || mode == EDITOR_VIEWPORT_INPUT_ACTION)
        return EDITOR_MENU_HEIGHT + 94;
    if(mode == EDITOR_VIEWPORT_INPUT_BINDING) return EDITOR_MENU_HEIGHT + 118;
    return 76;
}
