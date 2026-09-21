/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_VIEWPORT_CONTEXT_MENU_H
#define EDITOR_VIEWPORT_CONTEXT_MENU_H

#include "editor_viewport.h"

typedef enum EditorContextMenuAction {
    EDITOR_CONTEXT_MENU_NONE,
    EDITOR_CONTEXT_MENU_OPEN,
    EDITOR_CONTEXT_MENU_VISIBILITY,
    EDITOR_CONTEXT_MENU_RENAME,
    EDITOR_CONTEXT_MENU_DELETE
} EditorContextMenuAction;

typedef struct EditorViewportContextMenu {
    TextAsset open_label;
    TextAsset hide_label;
    TextAsset show_label;
    TextAsset rename_label;
    TextAsset copy_label;
    TextAsset duplicate_label;
    TextAsset delete_label;
    TextAsset accept_label;
    TextAsset cancel_label;
    TextAsset rename_field;
    TextAsset background_labels[5];
    bool open;
    bool target_valid;
    bool from_column;
    Position position;
    UIRect bounds;
    EditorSelectionRef target;
    float scroll_offset;
    char rename_value[EDITOR_OBJECT_NAME_MAX];
    char rename_original[EDITOR_OBJECT_NAME_MAX];
    bool renaming;
    bool rename_focus_pending;
} EditorViewportContextMenu;

bool editor_viewport_context_menu_create(EditorViewportContextMenu *menu,
    FontAsset *font);
void editor_viewport_context_menu_destroy(EditorViewportContextMenu *menu);
void editor_viewport_context_menu_open(EditorViewportContextMenu *menu,
    Position position, const EditorSelectionRef *target, bool from_column);
void editor_viewport_context_menu_replace(EditorViewportContextMenu *menu,
    Position position, const EditorSelectionRef *target, bool from_column);
EditorContextMenuAction editor_viewport_context_menu_draw(
    EditorViewportContextMenu *menu, const MouseState *mouse,
    bool target_visible, const char *target_name, float window_width, float menu_height,
    float viewport_bottom, float window_height);
void editor_viewport_context_menu_close(EditorViewportContextMenu *menu);
void editor_viewport_context_menu_cancel(EditorViewportContextMenu *menu);
bool editor_viewport_context_menu_open_check(
    const EditorViewportContextMenu *menu);
bool editor_viewport_context_menu_point_contains(
    const EditorViewportContextMenu *menu, Position point);

#endif
