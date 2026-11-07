/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EXAMPLE_VIEWPORT_H
#define EXAMPLE_VIEWPORT_H

#include "rohr.h"

bool example_viewport_create(CameraId camera, Color background, ViewportId *viewport);
bool example_body_appearance_set(Entity entity, Color fill, Color border, int layer);
void example_viewport_destroy(ViewportId *viewport);
bool example_ui_viewport_create(Color background, ViewportId *viewport);
ViewportItemId example_label_add(ViewportId viewport, TextAsset text, UIRect bounds);
ViewportItemId example_button_add(ViewportId viewport, TextAsset text, UIRect bounds,
    const UIButtonStyle *style);
ViewportItemId example_rectangle_add(ViewportId viewport, UIRect bounds, Color color, int layer);
bool example_button_clicked_check(ViewportItemId item, bool *armed);

#endif
