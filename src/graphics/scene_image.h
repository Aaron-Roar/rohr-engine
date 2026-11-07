/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_GRAPHICS_SCENE_IMAGE_H
#define ROHR_GRAPHICS_SCENE_IMAGE_H
#include "error.h"
#include "report.h"
/* Private editor canvas presentation: commands already compose the editor window.
 * Shares camera, commands, recording and presentation; bypasses runtime composition. */
Report graphics_editor_show(void);
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
/* Queue pixel-space drawing into an explicitly owned, reusable render target.
 * The callback's layers must precede the layer used to composite the image.
 * Nested captures and reuse while commands still reference the image fail.
 * Clip/layer state is restored; failures join the graphics submission report. */
EngineResult graphics_scene_image_render(GraphicsSceneImage *image, int width,
    int height, void (*draw)(void *), void *context);
EngineResult graphics_scene_image_draw_rotated(GraphicsSceneImage *image,
    SDL_FRect destination, double degrees);
/* Editor text-input placement uses SDL window coordinates. */
SDL_FPoint graphics_scene_window_point_get(SDL_FPoint screen);
#endif
