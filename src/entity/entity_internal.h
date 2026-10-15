/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_ENTITY_INTERNAL_H
#define ROHR_ENTITY_INTERNAL_H

#include "entity_components.h"

/* Creation only: setters must preserve the entity's current name. */
EntityResult entity_named_create(const char *name);
EntityResult entity_default_create(const char *kind, Entity owner);

/* Loading a new child must not leave a new empty group on failure. */
EngineResult entity_parent_restore(Entity child, Entity parent);

#endif
