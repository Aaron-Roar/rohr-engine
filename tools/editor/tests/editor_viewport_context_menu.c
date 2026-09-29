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

static bool inline_rename_frame(EditorViewportContextMenu *menu, Position pointer,
        MouseButtonState button, bool *submitted) {
    UIRect row = {100, 100, 200, 30};
    rohr_ui_frame_begin((UIInput){.pointer = pointer, .primary_button = button});
    if(editor_viewport_context_menu_modal_check(menu))
        rohr_ui_modal_set((UIRect){0, 0, 500, 500});
    /* Match the real column order: the element button precedes the rename field. */
    UIButtonResult element = rohr_ui_button("element", NULL, row, NULL);
    UIFieldResult field = editor_viewport_context_menu_inline_rename_draw(menu, row);
    UIButtonResult neighbor = rohr_ui_button("neighbor", NULL,
        (UIRect){310, 100, 150, 30}, NULL);
    bool passed = !element.pressed && !element.clicked && !element.double_clicked &&
        !element.focus_changed && !neighbor.pressed && !neighbor.clicked;
    if(submitted != NULL) *submitted = field.submitted;
    rohr_ui_frame_end();
    return passed;
}

static bool inline_rename_check(void) {
    EditorViewportContextMenu menu = {0};
    EngineResult started = rohr_engine_start();
    if(rohr_error_check(started)) return false;
    started = rohr_graphics_start();
    if(rohr_error_check(started)) { rohr_engine_stop(); return false; }
    FontAsset font = rohr_graphics_font_default_get();
    bool passed = editor_viewport_context_menu_create(&menu, &font);
    if(!passed) goto done;
    menu.renaming = true;
    menu.from_column = true;
    menu.rename_focus_pending = true;
    snprintf(menu.rename_value, sizeof(menu.rename_value), "abcdef");
    snprintf(menu.rename_original, sizeof(menu.rename_original), "abcdef");
    if(!editor_viewport_context_menu_modal_check(&menu)) { passed = false; goto done; }
    passed = inline_rename_frame(&menu, (Position){0}, MOUSE_BUTTON_STATE_UP, NULL);
    /* Built-in font glyphs are eight pixels wide; clicking index two must insert there. */
    Position caret = {192, 115};
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_PRESSED, NULL);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_RELEASED, NULL);
    SDL_Event event = {.type = SDL_EVENT_TEXT_INPUT};
    event.text.text = "X";
    rohr_ui_field_event_add(&event);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_UP, NULL) &&
        strcmp(menu.rename_value, "abXcdef") == 0 && menu.renaming;
    /* Select a range by dragging across the row and release over another element. */
    passed = passed && inline_rename_frame(&menu, (Position){180, 115}, MOUSE_BUTTON_STATE_PRESSED, NULL);
    passed = passed && inline_rename_frame(&menu, (Position){350, 115}, MOUSE_BUTTON_STATE_DOWN, NULL);
    passed = passed && inline_rename_frame(&menu, (Position){350, 115}, MOUSE_BUTTON_STATE_RELEASED, NULL);
    event.text.text = "Y";
    rohr_ui_field_event_add(&event);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_UP, NULL) &&
        strcmp(menu.rename_value, "aY") == 0 && menu.renaming;
    caret = (Position){200, 115};
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_PRESSED, NULL);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_RELEASED, NULL);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_PRESSED, NULL);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_RELEASED, NULL);
    event.text.text = "renamed";
    rohr_ui_field_event_add(&event);
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_UP, NULL) &&
        strcmp(menu.rename_value, "renamed") == 0 && menu.renaming;
    event = (SDL_Event){.type = SDL_EVENT_KEY_DOWN};
    event.key.key = SDLK_RETURN;
    rohr_ui_field_event_add(&event);
    bool submitted = false;
    passed = passed && inline_rename_frame(&menu, caret, MOUSE_BUTTON_STATE_UP, &submitted) && submitted;
    editor_viewport_context_menu_cancel(&menu);
    passed = passed && !editor_viewport_context_menu_modal_check(&menu);
    rohr_ui_field_focus_clear();
    rohr_ui_frame_begin((UIInput){.pointer = caret, .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    passed = passed && rohr_ui_button("element", NULL, (UIRect){100,100,200,30}, NULL).pressed;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = caret, .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    passed = passed && rohr_ui_button("element", NULL, (UIRect){100,100,200,30}, NULL).clicked;
    rohr_ui_frame_end();
done:
    rohr_ui_field_focus_clear();
    editor_viewport_context_menu_destroy(&menu);
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed;
}

int main(void) {
    if(!inline_rename_check()) {
        fprintf(stderr, "inline rename allowed element input or lost text editing\n");
        return 1;
    }
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
