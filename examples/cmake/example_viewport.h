/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EXAMPLE_VIEWPORT_H
#define EXAMPLE_VIEWPORT_H

#include "rohr.h"

bool example_viewport_create(CameraRenderCallback callback, void *context,
    ViewportId *viewport);
void example_viewport_destroy(ViewportId *viewport);

#endif
