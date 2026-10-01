/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_viewport_context_menu.h"

#include <math.h>
#include <stdio.h>

static bool text_create(FontAsset *font, const char *value, TextAsset *output) {
    TextAssetResult result = rohr_graphics_text_create(font, value,
        (Color){235, 238, 245, 255});
    if(rohr_error_check(result)) return false;
    *output = result.result.value;
    return true;
}

bool editor_viewport_context_menu_create(EditorViewportContextMenu *menu,
        FontAsset *font) {
    static const char *background[] = {
        "Add", "Paste", "Select All", "Hide All", "Show All"
    };
    if(menu == NULL || font == NULL) return false;
    *menu = (EditorViewportContextMenu){0};
    if(!text_create(font, "Open Editor", &menu->open_label) ||
            !text_create(font, "Hide", &menu->hide_label) ||
            !text_create(font, "Show", &menu->show_label) ||
            !text_create(font, "Rename", &menu->rename_label) ||
            !text_create(font, "Copy", &menu->copy_label) ||
            !text_create(font, "Duplicate", &menu->duplicate_label) ||
            !text_create(font, "Delete", &menu->delete_label) ||
            !text_create(font, "Accept", &menu->accept_label) ||
            !text_create(font, "Cancel", &menu->cancel_label) ||
            !text_create(font, "", &menu->rename_field)) goto fail;
    for(size_t i = 0; i < 5; i += 1)
        if(!text_create(font, background[i], &menu->background_labels[i])) goto fail;
    return true;
fail:
    editor_viewport_context_menu_destroy(menu);
    return false;
}

void editor_viewport_context_menu_destroy(EditorViewportContextMenu *menu) {
    if(menu == NULL) return;
    rohr_graphics_text_destroy(&menu->open_label);
    rohr_graphics_text_destroy(&menu->hide_label);
    rohr_graphics_text_destroy(&menu->show_label);
    rohr_graphics_text_destroy(&menu->rename_label);
    rohr_graphics_text_destroy(&menu->copy_label);
    rohr_graphics_text_destroy(&menu->duplicate_label);
    rohr_graphics_text_destroy(&menu->delete_label);
    rohr_graphics_text_destroy(&menu->accept_label);
    rohr_graphics_text_destroy(&menu->cancel_label);
    rohr_graphics_text_destroy(&menu->rename_field);
    for(size_t i = 0; i < 5; i += 1)
        rohr_graphics_text_destroy(&menu->background_labels[i]);
    *menu = (EditorViewportContextMenu){0};
}

void editor_viewport_context_menu_open(EditorViewportContextMenu *menu,
        Position position, const EditorSelectionRef *target, bool from_column) {
    if(menu == NULL) return;
    menu->open = true;
    menu->target_valid = target != NULL;
    menu->target = target == NULL ? (EditorSelectionRef){0} : *target;
    menu->from_column = from_column;
    menu->position = position;
    menu->scroll_offset = 0.0f;
    menu->renaming = false;
}

void editor_viewport_context_menu_replace(EditorViewportContextMenu *menu,
        Position position, const EditorSelectionRef *target, bool from_column) {
    if(menu == NULL) return;
    editor_viewport_context_menu_cancel(menu);
    editor_viewport_context_menu_open(menu, position, target, from_column);
}

static bool action_button(const char *id, const TextAsset *label, UIRect bounds,
        bool enabled) {
    if(!enabled) {
        rohr_ui_button_disabled(bounds, NULL);
        rohr_ui_label(label, bounds);
        return false;
    }
    return rohr_ui_button(id, label, bounds, NULL).clicked;
}

EditorContextMenuAction editor_viewport_context_menu_draw(
        EditorViewportContextMenu *menu, const UIPointerState *pointer_state,
        bool target_visible, const char *target_name, float window_width,
        float menu_height,
        float viewport_bottom, float window_height) {
    Position pointer;
    UIRect bounds;
    EditorContextMenuAction action = EDITOR_CONTEXT_MENU_NONE;
    float height, content_height;
    bool scroll_active;
    if(menu == NULL || pointer_state == NULL || !menu->open) return action;
    pointer = rohr_graphics_mouse_screen_position_get();
    content_height = menu->renaming ? 74.0f : menu->target_valid ? 198.0f : 162.0f;
    height = fminf(content_height + 4.0f,
        fmaxf(42.0f, fminf(viewport_bottom, window_height) - menu_height));
    menu->position.x = fmaxf(0.0f, fminf(menu->position.x, window_width - 180.0f));
    menu->position.y = fmaxf(menu_height, fminf(menu->position.y,
        fminf(viewport_bottom, window_height) - height));
    bounds = (UIRect){menu->position.x, menu->position.y, 176.0f, height};
    menu->bounds = bounds;
    rohr_ui_surface(bounds, (Color){24, 27, 34, 255});
    rohr_ui_border(bounds, 2.0f, (Color){0, 0, 0, 255});
    scroll_active = !menu->renaming;
    if(scroll_active)
        menu->scroll_offset = rohr_ui_scroll_region_begin(
            "editor.context.scroll", (UIRect){bounds.x + 2.0f,
                bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f},
            content_height, menu->scroll_offset, 32.0f).offset;
    if(menu->renaming) {
        UIRect field_bounds = {bounds.x + 4.0f, bounds.y + 4.0f,
            bounds.width - 8.0f, 30.0f};
        UIFieldBinding binding = {.kind = UI_FIELD_STRING,
            .string = menu->rename_value,
            .string_capacity = sizeof(menu->rename_value)};
        UIFieldResult field;
        if(menu->rename_focus_pending) {
            ui_field_focus_set("editor.context.rename.field", binding,
                &menu->rename_field, true);
            menu->rename_focus_pending = false;
        }
        field = rohr_ui_field("editor.context.rename.field", binding,
            &menu->rename_field, field_bounds, NULL);
        if(field.changed)
            (void)rohr_graphics_text_value_set(&menu->rename_field,
                menu->rename_value);
        if(field.submitted || rohr_ui_button("editor.context.rename.accept",
                &menu->accept_label, (UIRect){bounds.x + 4.0f, bounds.y + 40.0f,
                    80.0f, 30.0f}, NULL).clicked) {
            action = EDITOR_CONTEXT_MENU_RENAME;
            menu->renaming = false;
        } else if(rohr_ui_button("editor.context.rename.cancel",
                &menu->cancel_label, (UIRect){bounds.x + 92.0f,
                    bounds.y + 40.0f, 80.0f, 30.0f}, NULL).clicked ||
                rohr_ui_key_pressed_check(SDLK_ESCAPE)) {
            snprintf(menu->rename_value, sizeof(menu->rename_value), "%s",
                menu->rename_original);
            menu->renaming = false;
            menu->open = false;
        }
    } else if(menu->target_valid) {
        const TextAsset *visibility = target_visible ? &menu->hide_label :
            &menu->show_label;
        const TextAsset *labels[] = {&menu->open_label, visibility,
            &menu->rename_label, &menu->copy_label, &menu->duplicate_label,
            &menu->delete_label};
        const EditorContextMenuAction actions[] = {EDITOR_CONTEXT_MENU_OPEN,
            EDITOR_CONTEXT_MENU_VISIBILITY, EDITOR_CONTEXT_MENU_RENAME,
            EDITOR_CONTEXT_MENU_NONE, EDITOR_CONTEXT_MENU_NONE,
            EDITOR_CONTEXT_MENU_DELETE};
        for(size_t i = 0; i < 6; i += 1) {
            char id[64];
            bool enabled = i != 3 && i != 4 &&
                !(i == 1 && menu->target.kind == EDITOR_SELECTION_SOFT_HOLE);
            snprintf(id, sizeof(id), "editor.context.action.%zu", i);
            if(action_button(id, labels[i], (UIRect){bounds.x + 4.0f,
                    bounds.y + 4.0f + (float)i * 32.0f,
                    bounds.width - 8.0f, 28.0f}, enabled)) {
                if(actions[i] == EDITOR_CONTEXT_MENU_RENAME) {
                    snprintf(menu->rename_original,
                        sizeof(menu->rename_original), "%s",
                        target_name == NULL ? "" : target_name);
                    snprintf(menu->rename_value, sizeof(menu->rename_value), "%s",
                        menu->rename_original);
                    (void)rohr_graphics_text_value_set(&menu->rename_field,
                        menu->rename_value);
                    menu->renaming = true;
                    menu->rename_focus_pending = true;
                    if(menu->from_column) menu->open = false;
                } else action = actions[i];
            }
        }
    } else {
        for(size_t i = 0; i < 5; i += 1) {
            char id[64];
            snprintf(id, sizeof(id), "editor.context.background.%zu", i);
            (void)action_button(id, &menu->background_labels[i],
                (UIRect){bounds.x + 4.0f, bounds.y + 4.0f + (float)i * 32.0f,
                    bounds.width - 8.0f, 28.0f}, false);
        }
    }
    if(scroll_active) rohr_ui_scroll_region_end();
    if(action != EDITOR_CONTEXT_MENU_NONE) menu->open = false;
    if(pointer_state->button_states[MOUSE_BUTTON_LEFT] ==
            MOUSE_BUTTON_STATE_PRESSED &&
            (pointer.x < bounds.x || pointer.x > bounds.x + bounds.width ||
             pointer.y < bounds.y || pointer.y > bounds.y + bounds.height)) {
        if(menu->renaming) {
            action = EDITOR_CONTEXT_MENU_RENAME;
            menu->renaming = false;
        }
        menu->open = false;
    }
    return action;
}

void editor_viewport_context_menu_close(EditorViewportContextMenu *menu) {
    if(menu != NULL) menu->open = false;
}

void editor_viewport_context_menu_cancel(EditorViewportContextMenu *menu) {
    if(menu == NULL) return;
    snprintf(menu->rename_value, sizeof(menu->rename_value), "%s",
        menu->rename_original);
    menu->renaming = false;
    menu->rename_focus_pending = false;
    menu->open = false;
}

bool editor_viewport_context_menu_open_check(
        const EditorViewportContextMenu *menu) {
    return menu != NULL && menu->open;
}

bool editor_viewport_context_menu_modal_check(
        const EditorViewportContextMenu *menu) {
    return menu != NULL && (menu->open || (menu->renaming && menu->from_column));
}

UIFieldResult editor_viewport_context_menu_inline_rename_draw(
        EditorViewportContextMenu *menu, UIRect bounds) {
    if(menu == NULL || !menu->renaming || !menu->from_column)
        return (UIFieldResult){0};
    UIFieldBinding binding = {.kind = UI_FIELD_STRING,
        .string = menu->rename_value, .string_capacity = sizeof(menu->rename_value)};
    /* The row underneath remains outside modal controls and cannot claim input. */
    rohr_ui_modal_controls_begin();
    if(menu->rename_focus_pending) {
        ui_field_focus_set("editor.context.column.rename", binding,
            &menu->rename_field, true);
        menu->rename_focus_pending = false;
    }
    UIFieldResult result = rohr_ui_field("editor.context.column.rename", binding,
        &menu->rename_field, bounds, NULL);
    rohr_ui_modal_controls_end();
    return result;
}

bool editor_viewport_context_menu_point_contains(
        const EditorViewportContextMenu *menu, Position point) {
    UIRect bounds;
    if(menu == NULL || !menu->open) return false;
    bounds = menu->bounds;
    return point.x >= bounds.x && point.x <= bounds.x + bounds.width &&
        point.y >= bounds.y && point.y <= bounds.y + bounds.height;
}
