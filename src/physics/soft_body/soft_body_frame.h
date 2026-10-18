/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_SOFT_BODY_FRAME_H
#define ROHR_SOFT_BODY_FRAME_H
#include "physics.h"

/* Private, derived tracking samples. Capacity grows with the entity tables;
 * reset/update never allocate or move nodes. No serialized cache is needed. */
EngineResult physics_soft_body_frame_reserve(size_t capacity);
void physics_soft_body_frame_destroy(void);
void physics_soft_body_frame_clear(EntityIndex index);
void physics_soft_body_frame_reset(EntityIndex body);
void physics_soft_body_frames_update(Report *report);
#endif
