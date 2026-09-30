/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_GRAPHICS_TRIANGLE_DRAW_H
#define ROHR_GRAPHICS_TRIANGLE_DRAW_H
#include "graphics.h"
#include <stddef.h>
/* Internal visual-only triangle batches; no collision-shape validation. */
bool graphics_screen_triangles_draw(const Position *vertices, size_t count, Color color);
#endif
