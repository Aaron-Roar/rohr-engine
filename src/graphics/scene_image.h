/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_GRAPHICS_SCENE_IMAGE_H
#define ROHR_GRAPHICS_SCENE_IMAGE_H
#include "error.h"
#include <SDL3/SDL.h>
/* Private editor presentation resource. Create explicitly; destroy before
 * graphics_stop. Queued draws retain it until command completion/discard.
 * Upload once before drawing each frame, never while a draw remains queued. */
typedef struct GraphicsSceneImage GraphicsSceneImage;
GraphicsSceneImage *graphics_scene_image_create(void);
void graphics_scene_image_destroy(GraphicsSceneImage *image);
EngineResult graphics_scene_image_update(GraphicsSceneImage *image, const void *rgba,
    int width, int height, int pitch);
EngineResult graphics_scene_image_draw(GraphicsSceneImage *image, SDL_FRect destination);
/* Editor text-input placement uses SDL window coordinates. */
SDL_FPoint graphics_scene_window_point_get(SDL_FPoint screen);
#endif
