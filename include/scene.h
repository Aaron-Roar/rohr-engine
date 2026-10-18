/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_SCENE_H
#define ROHR_SCENE_H
#include "engine.h"
#include "report.h"

/** Caller-owned context must outlive scene_run. The runner owns engine/graphics
 * startup and teardown. Shutdown is called after any attempted initialization,
 * including failed initialization, while engine resources are still available.
 * Update runs after engine clock/lifecycle and before physics, only when running.
 * It returns an independently owned report; NULL means no gameplay callback. */
typedef struct SceneCallbacks {
    void *context;
    EngineResult (*initialize)(void *context);
    Report (*update)(void *context, Tick ticks, Time time);
    void (*shutdown)(void *context);
} SceneCallbacks;

/** Blocking scene entry point, intended for its own process. With no hosting
 * arguments it opens a normal window. The editor supplies the private hosting
 * arguments. Hosted runs allocate bounded transport/frame buffers (up to 4096²
 * RGBA pixels per frame) and render at most 60 frames/s. Returned reports own
 * all diagnostic data after shutdown. Requires an engine not already running. */
Report scene_run(int argc, char **argv, SceneCallbacks callbacks);
Report rohr_scene_run(int argc, char **argv, SceneCallbacks callbacks);
#endif
