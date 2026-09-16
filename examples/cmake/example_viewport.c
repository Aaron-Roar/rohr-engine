/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "example_viewport.h"

bool example_viewport_create(CameraRenderCallback callback, void *context,
        ViewportId *viewport) {
    CameraId camera;
    ViewportConfig config;
    ViewportIdResult created;

    if(callback == NULL || viewport == NULL) return false;
    *viewport = VIEWPORT_INVALID;
    camera = rohr_camera_active_get();
    if(camera == CAMERA_INVALID || rohr_error_check(
            rohr_camera_render_callback_set(camera, callback, context))) return false;
    config = rohr_viewport_config_default_get();
    config.rectangle = (ViewportRectangle){0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
    created = rohr_viewport_create(config);
    if(rohr_error_check(created)) return false;
    *viewport = created.result.value;
    if(rohr_error_check(rohr_viewport_camera_set(*viewport, camera)) ||
            rohr_error_check(rohr_viewport_enable_set(*viewport))) {
        example_viewport_destroy(viewport);
        return false;
    }
    return true;
}

void example_viewport_destroy(ViewportId *viewport) {
    if(viewport == NULL || *viewport == VIEWPORT_INVALID) return;
    (void)rohr_viewport_destroy(*viewport);
    *viewport = VIEWPORT_INVALID;
}
