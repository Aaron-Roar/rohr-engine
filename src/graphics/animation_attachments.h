/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_ANIMATION_ATTACHMENTS_H
#define ROHR_ANIMATION_ATTACHMENTS_H
#include "graphics.h"

/* Runtime state restores lifetime ownership separately from drawing targets. */
EngineResult graphics_animated_sprite_attachment_restore(Entity animation, Entity target);

#endif
