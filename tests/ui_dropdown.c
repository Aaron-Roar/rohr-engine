/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

int main(void) {
    UIRect bounds = {0.0f, 0.0f, 100.0f, 30.0f};
    const TextAsset *options[2] = {NULL, NULL};
    const TextAsset *long_options[10] = {0};
    const TextAsset *menu_options[5] = {0};

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("capturing-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open) return 1;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).pressed) return 2;
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open ||
            !rohr_ui_pointer_consumed_get()) return 3;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).clicked) return 4;
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "capturing-dropdown", options, 2, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(!result.changed || result.selected_index != 1 || result.open) return 5;
    }
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open) return 6;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).pressed) return 7;
    if(rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open ||
            !rohr_ui_pointer_consumed_get()) return 8;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).clicked) return 9;
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown(
        "bounded-dropdown", long_options, 10, 0,
        (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("bounded-dropdown", long_options,
            10, 0, (UIDropdownConfig){.visible_row_limit = 3},
            bounds, NULL).open) return 10;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 100.0f}});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "bounded-dropdown", long_options, 10, 0,
            (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
        if(!result.open || result.hovered_index != 2) return 11;
    }
    rohr_ui_frame_end();

    {
        SDL_Event wheel = {0};
        UIDropdownResult result;
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1.0f;
        rohr_ui_field_event_add(&wheel);
        rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 100.0f}});
        result = rohr_ui_dropdown(
            "bounded-dropdown", long_options, 10, 0,
            (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
        if(!result.open || result.hovered_index != 3) return 12;
        rohr_ui_frame_end();
    }

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-bounded-dropdown", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).pressed) return 13;
    if(rohr_ui_dropdown("bounded-dropdown", long_options,
            10, 0, (UIDropdownConfig){.visible_row_limit = 3},
            bounds, NULL).open || !rohr_ui_pointer_consumed_get()) return 14;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-bounded-dropdown", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).clicked) return 15;
    (void)rohr_ui_dropdown(
        "bounded-dropdown", long_options, 10, 0,
        (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL).open) return 16;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 310.0f}});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(!result.open || result.hovered_index != 9) return 17;
    }
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 310.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 310.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(result.open || !result.changed || result.selected_index != 9)
            return 18;
    }
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_menu("top-menu", NULL, menu_options, 5, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_menu("top-menu", NULL, menu_options, 5,
            bounds, NULL).open) return 19;
    rohr_ui_frame_end();

    {
        SDL_Event wheel = {0};
        UIDropdownResult result;
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1.0f;
        rohr_ui_field_event_add(&wheel);
        rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 160.0f}});
        result = rohr_ui_menu(
            "top-menu", NULL, menu_options, 5, bounds, NULL);
        if(!result.open || result.hovered_index != 4) return 20;
        rohr_ui_frame_end();
    }

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 160.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_menu("top-menu", NULL, menu_options, 5, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 160.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    {
        UIDropdownResult result = rohr_ui_menu(
            "top-menu", NULL, menu_options, 5, bounds, NULL);
        if(result.open || !result.changed || result.selected_index != 4)
            return 21;
    }
    rohr_ui_frame_end();
    return 0;
}
