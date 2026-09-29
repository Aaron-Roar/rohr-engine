/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_GRAPHICS_TEXTURE_DRAW_H
#define ROHR_GRAPHICS_TEXTURE_DRAW_H

#include "graphics.h"

/* Internal screen-space drawing for editor previews. Direction mirrors texture
 * content horizontally before clockwise rotation, as runtime sprites do.
 * The queued command retains the texture; callers keep their own reference. */
void graphics_screen_texture_direction_draw(TextureAsset texture, Position center,
    Scale size, Orientation orientation, Direction direction);

#endif
