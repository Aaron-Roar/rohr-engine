/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "browser/editor_file_browser.h"
#include "panels/editor_terminal_panel.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "editor text input line %d: %s\n", __LINE__, #c); return false; } } while(0)

/* Give the bundled SDL dummy backend real keyboard focus without a desktop. */
extern bool SDL_SetKeyboardFocus(SDL_Window *window);
float editor_window_width = 1000.0f;
float editor_window_height = 720.0f;
float editor_viewport_width = 1000.0f;
float editor_viewport_bottom = 600.0f;
static SDL_Window *window;
static FontAsset font;
static TextAsset display;
static EditorTerminalPanel terminal;

static bool event_add(SDL_Event *event, bool modal) {
    bool consumed = editor_terminal_panel_event_add(&terminal, event,
        editor_viewport_width, editor_viewport_bottom, modal);
    if(!consumed) rohr_ui_event_add(event);
    return consumed;
}

static void press(Position point, bool modal) {
    SDL_WarpMouseInWindow(window, point.x, point.y);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    (void)event_add(&event, modal);
}

static bool text_add(const char *value, bool modal) {
    /* Inject only after verifying the OS text-input session is enabled. */
    CHECK(SDL_TextInputActive(window));
    SDL_Event event = {.type = SDL_EVENT_TEXT_INPUT};
    event.text.text = value;
    CHECK(!event_add(&event, modal));
    return true;
}

static void key_add(SDL_Keycode key, SDL_Keymod modifiers, bool modal) {
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.key = key;
    event.key.mod = modifiers;
    (void)event_add(&event, modal);
}

static UIFieldResult field_frame(char *value, Position point, MouseButtonState button) {
    rohr_ui_frame_begin((UIInput){.pointer = point, .primary_button = button});
    UIFieldResult result = rohr_ui_field("name", (UIFieldBinding){
        .kind = UI_FIELD_STRING, .string = value, .string_capacity = 64},
        &display, (UIRect){100, 100, 300, 40}, NULL);
    rohr_ui_frame_end();
    return result;
}

static bool terminal_transfer_check(void) {
    char value[64] = "name";
    Position field = {250, 120}, console = {250, 650};
    terminal = (EditorTerminalPanel){.visible = true};
    rohr_ui_field_focus_clear();
    CHECK(SDL_StopTextInput(window));
    press(field, false);
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_PRESSED).active);
    (void)field_frame(value, field, MOUSE_BUTTON_STATE_RELEASED);
    CHECK(SDL_TextInputActive(window));
    press(field, false);
    /* Outside clicks must not stop a session the terminal doesn't own. */
    CHECK(SDL_TextInputActive(window));
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_PRESSED).active);
    (void)field_frame(value, field, MOUSE_BUTTON_STATE_RELEASED);
    CHECK(text_add("renamed", false));
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_UP).changed);
    CHECK(strcmp(value, "renamed") == 0);

    press(console, false);
    CHECK(terminal.focused && SDL_TextInputActive(window));
    CHECK(!field_frame(value, console, MOUSE_BUTTON_STATE_PRESSED).active);
    (void)field_frame(value, console, MOUSE_BUTTON_STATE_RELEASED);
    press(field, false);
    CHECK(!terminal.focused && !SDL_TextInputActive(window));
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_PRESSED).active);
    CHECK(SDL_TextInputActive(window));
    (void)field_frame(value, field, MOUSE_BUTTON_STATE_RELEASED);

    editor_terminal_panel_visible_toggle(&terminal);
    press(field, false);
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_PRESSED).active);
    (void)field_frame(value, field, MOUSE_BUTTON_STATE_RELEASED);
    CHECK(text_add("hidden", false));
    CHECK(field_frame(value, field, MOUSE_BUTTON_STATE_UP).changed);
    CHECK(strcmp(value, "hidden") == 0);
    rohr_ui_field_focus_clear();
    return true;
}

static void browser_frame(EditorFileBrowser *browser, Position point,
        MouseButtonState button) {
    rohr_ui_frame_begin((UIInput){.pointer = point, .primary_button = button});
    rohr_ui_modal_set((UIRect){0, 34, 1000, 686});
    rohr_ui_modal_controls_begin();
    (void)editor_file_browser_draw(browser, &display, NULL, NULL, NULL, NULL,
        editor_window_width, editor_viewport_bottom);
    rohr_ui_modal_controls_end();
    rohr_ui_frame_end();
}

static bool project_name_check(void) {
    EditorFileBrowser browser;
    editor_file_browser_init(&browser);
    CHECK(editor_file_browser_open(&browser, EDITOR_FILE_BROWSER_CREATE_DIRECTORY,
        ROHR_TEST_BROWSER_DIRECTORY, &font));
    terminal = (EditorTerminalPanel){.visible = true};
    press((Position){250, 650}, false);
    CHECK(terminal.focused);
    /* Opening a modal with terminal focus must release that focus immediately
       when its events arrive, even when the pointer is over the terminal. */
    press((Position){250, 650}, true);
    CHECK(!terminal.focused);
    browser_frame(&browser, (Position){0}, MOUSE_BUTTON_STATE_UP);
    Position field = {716, 514};
    press(field, true);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_PRESSED);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_RELEASED);
    CHECK(SDL_TextInputActive(window));
    key_add(SDLK_END, SDL_KMOD_NONE, true);
    CHECK(text_add("x", true));
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_UP);
    CHECK(strcmp(browser.filename, "project-dirx") == 0);

    press(field, true);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_PRESSED);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_RELEASED);
    CHECK(text_add("new-project", true));
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_UP);
    CHECK(strcmp(browser.filename, "new-project") == 0);
    key_add(SDLK_BACKSPACE, SDL_KMOD_NONE, true);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_UP);
    CHECK(strcmp(browser.filename, "new-projec") == 0);
    key_add(SDLK_HOME, SDL_KMOD_NONE, true);
    key_add(SDLK_DELETE, SDL_KMOD_NONE, true);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_UP);
    CHECK(strcmp(browser.filename, "ew-projec") == 0);
    CHECK(!terminal.focused);

    press((Position){20, 40}, true);
    browser_frame(&browser, (Position){20, 40}, MOUSE_BUTTON_STATE_PRESSED);
    browser_frame(&browser, (Position){20, 40}, MOUSE_BUTTON_STATE_RELEASED);
    press(field, true);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_PRESSED);
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_RELEASED);
    key_add(SDLK_A, SDL_KMOD_CTRL, true);
    CHECK(text_add("refocused", true));
    browser_frame(&browser, field, MOUSE_BUTTON_STATE_UP);
    CHECK(strcmp(browser.filename, "refocused") == 0);
    rohr_ui_field_focus_clear();
    editor_file_browser_destroy(&browser);
    return true;
}

int main(void) {
    if(rohr_error_check(rohr_engine_start()) || rohr_error_check(rohr_graphics_start())) return 1;
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    window = count > 0 ? windows[0] : NULL;
    SDL_free(windows);
    if(window == NULL || !SDL_SetKeyboardFocus(window)) return 1;
    FontAssetResult loaded = rohr_graphics_font_load((FontDescriptor){ROHR_TEST_FONT_PATH, 18});
    if(rohr_error_check(loaded)) return 1;
    font = loaded.result.value;
    TextAssetResult text = rohr_graphics_text_create(&font, "", (Color){255, 255, 255, 255});
    if(rohr_error_check(text)) return 1;
    display = text.result.value;
    bool passed = terminal_transfer_check() && project_name_check();
    rohr_ui_field_focus_clear();
    rohr_graphics_text_destroy(&display);
    rohr_graphics_font_release(&font);
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
