/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_SCENE_RUNTIME_H
#define ROHR_SCENE_RUNTIME_H
#include "error.h"
#include <SDL3/SDL.h>
/* Private scene-runner hooks, configured only while graphics is stopped.
 * Surfaces are borrowed for the callback; SDL resources never cross processes. */
typedef EngineResult (*SceneCapture)(void *context, const SDL_Surface *surface);
typedef bool (*SceneCaptureReady)(void *context);
void graphics_scene_configure(bool hosted, SceneCapture capture, SceneCaptureReady ready, void *context);
EngineResult audio_scene_pause_set(bool paused);
#endif
