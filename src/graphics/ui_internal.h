/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_UI_INTERNAL_H
#define ROHR_UI_INTERNAL_H
/* Cancel transient interactions when a tool replaces its control tree.
 * Does not change submitted drawing, layout scopes, or application data. */
void ui_interaction_cancel(void);
/* Fixed tool navigation may dismiss a property dropdown on its selecting press. */
void ui_dropdown_dismiss(void);
float ui_scroll_content_height_get(void);
float ui_navigation_scroll_delta_get(const char *scroll_id);
#endif
