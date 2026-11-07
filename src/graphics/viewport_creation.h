/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_VIEWPORT_CREATION_INTERNAL_H
#define ROHR_VIEWPORT_CREATION_INTERNAL_H
#include "graphics.h"
bool graphics_started_check(void);
void graphics_viewport_resources_release(ViewportId viewport);
void graphics_ui_owned_destroy(GraphicsUiId ui);
bool graphics_viewport_recipe_check(ViewportId viewport);
ViewportItemIdResult graphics_viewport_owned_camera_add(ViewportId viewport,
    CameraId camera, ViewportItemDescription placement);
#endif
