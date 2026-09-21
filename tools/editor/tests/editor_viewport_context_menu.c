/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "viewport/menus/editor_viewport_context_menu.h"

#include <stdio.h>
#include <string.h>

static bool selection_equal(EditorSelectionRef first, EditorSelectionRef second) {
    return first.kind == second.kind && first.object == second.object &&
        first.parent == second.parent && first.container == second.container &&
        first.item == second.item;
}

static bool element_to_element_check(void) {
    EditorViewportContextMenu menu = {0};
    EditorSelectionRef first = {EDITOR_SELECTION_OBJECT, 1, 0, 0, 1};
    EditorSelectionRef second = {EDITOR_SELECTION_SPRITE, 1, 0, 0, 8};
    Position first_pointer = {120.0f, 180.0f};
    Position second_pointer = {420.0f, 260.0f};
    editor_viewport_context_menu_open(&menu, first_pointer, &first, false);
    editor_viewport_context_menu_replace(&menu, second_pointer, &second, true);
    return menu.open && menu.target_valid && menu.from_column &&
        selection_equal(menu.target, second) &&
        menu.position.x == second_pointer.x && menu.position.y == second_pointer.y;
}

static bool element_to_background_check(void) {
    EditorViewportContextMenu menu = {0};
    EditorSelectionRef target = {EDITOR_SELECTION_UI_SHAPE, 3, 0, 0, 7};
    Position background_pointer = {300.0f, 500.0f};
    editor_viewport_context_menu_open(&menu, (Position){100.0f, 100.0f},
        &target, false);
    editor_viewport_context_menu_replace(&menu, background_pointer, NULL, false);
    return menu.open && !menu.target_valid && !menu.from_column &&
        menu.position.x == background_pointer.x &&
        menu.position.y == background_pointer.y;
}

static bool rename_to_element_check(void) {
    EditorViewportContextMenu menu = {0};
    EditorSelectionRef first = {EDITOR_SELECTION_UI_TEXT, 4, 0, 0, 2};
    EditorSelectionRef second = {EDITOR_SELECTION_UI_SLIDER, 4, 0, 0, 5};
    editor_viewport_context_menu_open(&menu, (Position){90.0f, 90.0f},
        &first, true);
    snprintf(menu.rename_original, sizeof(menu.rename_original), "%s", "Original");
    snprintf(menu.rename_value, sizeof(menu.rename_value), "%s", "Unsaved");
    menu.renaming = true;
    menu.rename_focus_pending = true;
    editor_viewport_context_menu_replace(&menu, (Position){220.0f, 240.0f},
        &second, false);
    return menu.open && !menu.renaming && !menu.rename_focus_pending &&
        strcmp(menu.rename_value, "Original") == 0 && menu.target_valid &&
        selection_equal(menu.target, second);
}

int main(void) {
    if(!element_to_element_check()) {
        fprintf(stderr, "element-to-element context replacement failed\n");
        return 1;
    }
    if(!element_to_background_check()) {
        fprintf(stderr, "element-to-background context replacement failed\n");
        return 1;
    }
    if(!rename_to_element_check()) {
        fprintf(stderr, "rename-to-element context replacement failed\n");
        return 1;
    }
    return 0;
}
