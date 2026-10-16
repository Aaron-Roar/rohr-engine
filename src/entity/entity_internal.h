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

/* Protected ownership construction used by spawning and state restoration. */
EngineResult entity_object_initialize(Entity object);
EngineResult entity_object_member_restore(Entity object, Entity member,
    const char *description_name);
typedef struct ObjectMemberAlias {
    Entity entity;
    EntityName name;
} ObjectMemberAlias;
EngineResult entity_object_alias_restore(Entity object, Entity member, const char *name);
const ObjectMemberAlias *entity_object_aliases_get(Entity object, size_t *count);

#endif
