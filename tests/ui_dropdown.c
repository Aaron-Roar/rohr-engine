/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include <assert.h>

static UIDropdownResult fit_frame(size_t count, UIRect bounds, Position pointer,
        MouseButtonState button, float wheel, bool nested, bool actions) {
    const TextAsset *options[12] = {0};
    TextAsset action = {0};
    UIDropdownResult result;
    rohr_ui_input_set((UIInput){.pointer = pointer, .primary_button = button});
    if(wheel != 0) {
        SDL_Event event = {.type = SDL_EVENT_MOUSE_WHEEL};
        event.wheel.y = wheel;
        rohr_ui_field_event_add(&event);
    }
    if(nested) {
        if(pointer.y == 235) {
            UIButtonResult behind = rohr_ui_button("behind-upward-popup", NULL,
                (UIRect){20, 220, 160, 30}, NULL);
            assert(!behind.hovered && !behind.pressed && !behind.clicked);
        }
        UIScrollRegionResult scroll = rohr_ui_scroll_region_begin("parent",
            (UIRect){0, 480, 800, 100}, 200, 40, 20);
        assert(scroll.offset == 40);
    }
    result = rohr_ui_dropdown_actions("fitted", options, count, 0,
        actions ? &action : NULL, 0,
        (UIDropdownConfig){.visible_row_limit = 10, .fit_window = true},
        bounds, NULL);
    if(nested) rohr_ui_scroll_region_end();
    rohr_ui_update();
    return result;
}

static void fit_open(size_t count, UIRect bounds, bool nested, bool actions) {
    Position button = {bounds.x + 5, bounds.y + 5 - (nested ? 40 : 0)};
    (void)fit_frame(count, bounds, button, MOUSE_BUTTON_STATE_PRESSED, 0, nested, actions);
    assert(fit_frame(count, bounds, button, MOUSE_BUTTON_STATE_RELEASED,
        0, nested, actions).open);
}

static void fit_close(size_t count, UIRect bounds, bool nested, bool actions) {
    assert(!fit_frame(count, bounds, (Position){0, 0},
        MOUSE_BUTTON_STATE_PRESSED, 0, nested, actions).open);
    (void)fit_frame(count, bounds, (Position){0, 0},
        MOUSE_BUTTON_STATE_RELEASED, 0, nested, actions);
}

static void fitted_dropdown_check(void) {
    UIRect bounds = {20, 20, 160, 30};
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    assert(rohr_graphics_logical_size_set(800, 600));

    /* Short lists have no blank rows; ten options are all accessible. */
    fit_open(2, bounds, false, false);
    assert(fit_frame(2, bounds, (Position){30, 95}, 0, 0, false, false).hovered_index == 1);
    assert(fit_frame(2, bounds, (Position){30, 115}, 0, 0, false, false).hovered_index == -1);
    fit_close(2, bounds, false, false);
    fit_open(10, bounds, false, false);
    assert(fit_frame(10, bounds, (Position){30, 335}, 0, -1, false, false).hovered_index == 9);
    fit_close(10, bounds, false, false);

    /* Longer lists scroll, then select by the absolute option index. */
    fit_open(12, bounds, false, false);
    assert(fit_frame(12, bounds, (Position){30, 355}, 0, 0, false, false).hovered_index == -1);
    assert(fit_frame(12, bounds, (Position){30, 335}, 0, -1, false, false).hovered_index == 10);
    assert(fit_frame(12, bounds, (Position){30, 335}, 0, -1, false, false).hovered_index == 11);
    (void)fit_frame(12, bounds, (Position){30, 335}, MOUSE_BUTTON_STATE_PRESSED, 0, false, false);
    UIDropdownResult selected = fit_frame(12, bounds, (Position){30, 335},
        MOUSE_BUTTON_STATE_RELEASED, 0, false, false);
    assert(selected.changed && selected.selected_index == 11 && !selected.open);

    /* Open above a low field, outside its translated parent scroll clip.
       Popup scrolling must not move the parent. Row actions use the same bounds. */
    bounds.y = 560;
    fit_open(12, bounds, true, true);
    assert(fit_frame(12, bounds, (Position){90, 235}, 0, 0, true, true).hovered_index == 0);
    assert(fit_frame(12, bounds, (Position){90, 235}, 0, -1, true, true).hovered_index == 1);
    (void)fit_frame(12, bounds, (Position){30, 235}, MOUSE_BUTTON_STATE_PRESSED, 0, true, true);
    selected = fit_frame(12, bounds, (Position){30, 235}, MOUSE_BUTTON_STATE_RELEASED, 0, true, true);
    assert(selected.action_index == 1 && !selected.open);

    /* Limited space reduces rows, and growing the window clamps the scroll offset. */
    bounds = (UIRect){740, 290, 160, 30};
    fit_open(12, bounds, false, false);
    assert(fit_frame(12, bounds, (Position){700, 35}, 0, 0, false, false).hovered_index == 0);
    assert(fit_frame(12, bounds, (Position){700, 5}, 0, 0, false, false).hovered_index == -1);
    /* Track click and thumb drag reach the last option without selecting it. */
    (void)fit_frame(12, bounds, (Position){798, 285}, MOUSE_BUTTON_STATE_PRESSED, 0, false, false);
    (void)fit_frame(12, bounds, (Position){798, 285}, MOUSE_BUTTON_STATE_RELEASED, 0, false, false);
    assert(fit_frame(12, bounds, (Position){700, 275}, 0, 0, false, false).hovered_index == 11);
    (void)fit_frame(12, bounds, (Position){798, 100}, MOUSE_BUTTON_STATE_PRESSED, 0, false, false);
    (void)fit_frame(12, bounds, (Position){798, 20}, MOUSE_BUTTON_STATE_DOWN, 0, false, false);
    (void)fit_frame(12, bounds, (Position){798, 285}, MOUSE_BUTTON_STATE_DOWN, 0, false, false);
    (void)fit_frame(12, bounds, (Position){798, 285}, MOUSE_BUTTON_STATE_RELEASED, 0, false, false);
    assert(fit_frame(12, bounds, (Position){700, 275}, 0, 0, false, false).hovered_index == 11);
    assert(rohr_graphics_logical_size_set(800, 900));
    assert(fit_frame(12, bounds, (Position){700, 335}, 0, 0, false, false).hovered_index == 2);
    fit_close(12, bounds, false, false);
    rohr_graphics_stop();
    rohr_engine_stop();
}

int main(void) {
    UIRect bounds = {0.0f, 0.0f, 100.0f, 30.0f};
    const TextAsset *options[2] = {NULL, NULL};
    const TextAsset *long_options[10] = {0};
    const TextAsset *menu_options[12] = {0};

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("capturing-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open) return 1;
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).pressed) return 2;
    if(!rohr_ui_dropdown("capturing-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open ||
            !rohr_ui_pointer_consumed_get()) return 3;
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 70.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-option", NULL,
            (UIRect){0.0f, 60.0f, 100.0f, 30.0f}, NULL).clicked) return 4;
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "capturing-dropdown", options, 2, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(!result.changed || result.selected_index != 1 || result.open) return 5;
    }
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open) return 6;
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).pressed) return 7;
    if(rohr_ui_dropdown("dismiss-dropdown", options,
            2, 0, (UIDropdownConfig){0}, bounds, NULL).open ||
            !rohr_ui_pointer_consumed_get()) return 8;
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-dropdown-dismiss", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).clicked) return 9;
    (void)rohr_ui_dropdown("dismiss-dropdown", options, 2, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown(
        "bounded-dropdown", long_options, 10, 0,
        (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("bounded-dropdown", long_options,
            10, 0, (UIDropdownConfig){.visible_row_limit = 3},
            bounds, NULL).open) return 10;
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 100.0f}});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "bounded-dropdown", long_options, 10, 0,
            (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
        if(!result.open || result.hovered_index != 2) return 11;
    }
    rohr_ui_update();

    {
        SDL_Event wheel = {0};
        UIDropdownResult result;
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1.0f;
        rohr_ui_field_event_add(&wheel);
        rohr_ui_input_set((UIInput){.pointer = {10.0f, 100.0f}});
        result = rohr_ui_dropdown(
            "bounded-dropdown", long_options, 10, 0,
            (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
        if(!result.open || result.hovered_index != 3) return 12;
        rohr_ui_update();
    }

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    if(rohr_ui_button("behind-bounded-dropdown", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).pressed) return 13;
    if(rohr_ui_dropdown("bounded-dropdown", long_options,
            10, 0, (UIDropdownConfig){.visible_row_limit = 3},
            bounds, NULL).open || !rohr_ui_pointer_consumed_get()) return 14;
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 130.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(rohr_ui_button("behind-bounded-dropdown", NULL,
            (UIRect){0.0f, 120.0f, 100.0f, 30.0f}, NULL).clicked) return 15;
    (void)rohr_ui_dropdown(
        "bounded-dropdown", long_options, 10, 0,
        (UIDropdownConfig){.visible_row_limit = 3}, bounds, NULL);
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL).open) return 16;
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 310.0f}});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(!result.open || result.hovered_index != 9) return 17;
    }
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 310.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_dropdown("unlimited-dropdown", long_options, 10, 0,
        (UIDropdownConfig){0}, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 310.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    {
        UIDropdownResult result = rohr_ui_dropdown(
            "unlimited-dropdown", long_options, 10, 0,
            (UIDropdownConfig){0}, bounds, NULL);
        if(result.open || !result.changed || result.selected_index != 9)
            return 18;
    }
    rohr_ui_update();

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_menu("top-menu", NULL, menu_options, 12, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 10.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    if(!rohr_ui_menu("top-menu", NULL, menu_options, 12,
            bounds, NULL).open) return 19;
    rohr_ui_update();

    {
        SDL_Event wheel = {0};
        UIDropdownResult result;
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1.0f;
        rohr_ui_field_event_add(&wheel);
        rohr_ui_input_set((UIInput){.pointer = {10.0f, 370.0f}});
        result = rohr_ui_menu(
            "top-menu", NULL, menu_options, 12, bounds, NULL);
        if(!result.open || result.hovered_index != 11) return 20;
        rohr_ui_update();
    }

    rohr_ui_input_set((UIInput){.pointer = {10.0f, 370.0f},
        .primary_button = MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_menu("top-menu", NULL, menu_options, 12, bounds, NULL);
    rohr_ui_update();
    rohr_ui_input_set((UIInput){.pointer = {10.0f, 370.0f},
        .primary_button = MOUSE_BUTTON_STATE_RELEASED});
    {
        UIDropdownResult result = rohr_ui_menu(
            "top-menu", NULL, menu_options, 12, bounds, NULL);
        if(result.open || !result.changed || result.selected_index != 11)
            return 21;
    }
    rohr_ui_update();
    fitted_dropdown_check();
    return 0;
}
