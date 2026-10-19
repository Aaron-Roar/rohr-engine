/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_CAMERA_STATE_H
#define ROHR_CAMERA_STATE_H
#include "graphics.h"
typedef struct CameraStateSnapshot {
    Camera value;
    CameraAttachmentDescription attachment;
    bool attached;
    bool enabled;
    bool pause_with_engine;
    bool active;
} CameraStateSnapshot;
/* Read stored state without resolving attachments or changing active camera. */
EngineResult graphics_camera_state_get(CameraId camera, CameraStateSnapshot *snapshot);
#endif
