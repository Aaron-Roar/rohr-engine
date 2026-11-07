/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_LAYER_STATE_H
#define ROHR_LAYER_STATE_H
#include "graphics.h"

/* Internal enumeration for persistence and creation rollback. Names are borrowed until registry mutation. */
bool graphics_layer_registry_entry_get(size_t slot, LayerDescription *description);
#endif
