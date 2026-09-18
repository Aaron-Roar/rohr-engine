/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

int main(void) {
    UIRect bounds = {0.0f, 0.0f, 100.0f, 30.0f};
    const TextAsset *options[2] = {NULL, NULL};

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("capturing-dropdown", options, 2, 0, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, bounds, NULL).open) return 1;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).pressed) return 2;
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, bounds, NULL).open || !rohr_ui_pointer_consumed_get()) return 3;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).clicked) return 4;
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "capturing-dropdown", options, 2, 0, bounds, NULL);
        if(!result.changed || result.selected_index != 1 || result.open) return 5;
    }
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0, bounds, NULL);
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, bounds, NULL).open) return 6;
    rohr_ui_frame_end();

    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).pressed) return 7;
    if(rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, bounds, NULL).open || !rohr_ui_pointer_consumed_get()) return 8;
    rohr_ui_frame_end();
    rohr_ui_frame_begin((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).clicked) return 9;
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0, bounds, NULL);
    rohr_ui_frame_end();
    return 0;
}
