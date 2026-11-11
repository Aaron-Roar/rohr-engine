/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

#include <stdint.h>

typedef uint32_t AnimationId;
typedef uint32_t AnimationFrameId;
#define ANIMATION_ID_INVALID UINT32_C(0)
#define ANIMATION_FRAME_ID_INVALID UINT32_C(0)
#include <stdbool.h>
#include <time.h>
#include "engine.h"
#include "memory.h"


/** Stable entity id used by public APIs. */
typedef uint32_t Entity; //An id for an entity

/** Index into component tables used by engine internals and compute loops. */
typedef uint32_t EntityIndex; //An index into component tables

/** Result type for functions that return an EntityIndex. */
ERROR_DECLARE_RESULT_TYPE(EntityIndexResult, EntityIndex);

/** Invalid entity id sentinel. */
#define ENTITY_INVALID 0
/** Intentionally absent optional entity; identical to ENTITY_INVALID. */
#define ENTITY_NONE ENTITY_INVALID

/** Maximum number of live or reusable entity slots. */
#define MAX_ENTITIES 10000

/** Maximum number of reusable entity groups. */
#define MAX_GROUPS 10000

/** Maximum number of component bits reserved by the engine. */
#define MAX_COMPONENTS 100

/** Maximum entity name length, including the terminating null byte. */
#define ENTITY_NAME_MAX 64
/** Maximum group name length, including the terminating null byte. */
#define GROUP_NAME_MAX 64

/** Result type for functions that return an Entity. */
ERROR_DECLARE_RESULT_TYPE(EntityResult, Entity);

/** Stable group id used to refer to reusable entity groups. */
typedef uint32_t GroupId;

/** Invalid group id sentinel. */
#define GROUP_INVALID 0

/** Result type for functions that return a GroupId. */
ERROR_DECLARE_RESULT_TYPE(GroupIdResult, GroupId);

/** Fixed-size, engine-owned generic group name. */
typedef struct GroupName {
    char value[GROUP_NAME_MAX];
} GroupName;

/** Result type for functions that return a GroupName. */
ERROR_DECLARE_RESULT_TYPE(GroupNameResult, GroupName);

/** Component mask value for an entity. */
typedef uint32_t RohrComponentMask; //The bit mask for an entity's engine components

/** Object pool storing per-index entity component masks. */
MEMORY_DECLARE_OBJECT_POOL(EntityMaskPool, RohrComponentMask);

/** Pool backing the entity_mask table. */
extern EntityMaskPool entity_mask_pool;

/** Direct component mask table indexed by EntityIndex. */
#define entity_mask entity_mask_pool.objects

/** Component bits stored in RohrComponentMask values. */
typedef enum {
    /** No components. */
    ROHR_NONE                    = 0,
    /** Entity is static in the physics system. */
    ROHR_STATIC                  = 1 << 0,
    /** Entity is dynamic in the physics system. */
    ROHR_DYNAMIC                 = 1 << 1,
    /** Entity participates in collision checks. */
    ROHR_COLLISION               = 1 << 2,
    /** Entity has a hit box shape. */
    ROHR_HIT_BOX                 = 1 << 3,
    /** Entity has an axis lock constraint. */
    ROHR_AXIS_LOCK               = 1 << 4,
    /** Entity has an angle lock constraint. */
    ROHR_ANGLE_LOCK              = 1 << 5,

    /** Entity represents a force source. */
    ROHR_FORCE                   = 1 << 6,
    /** Entity represents a torque source. */
    ROHR_TORQUE                  = 1 << 7,
    /** Entity targets another entity. */
    ROHR_TARGETABLE              = 1 << 8,

    /** Entity has mass. */
    ROHR_MASS                    = 1 << 9,
    /** Reserved time-window component bit. */
    ROHR_TIMEWINDOW              = 1 << 10,
    /** Entity has a parent entity. */
    ROHR_PARENT                      = 1 << 11,
    /** Entity has one or more child entities. */
    ROHR_CHILD                       = 1 << 12,
    /** Entity has a transform lock constraint. */
    ROHR_TRANSFORM_LOCK              = 1 << 13,
    /** Entity represents a joint constraint. */
    ROHR_JOINT                       = 1 << 14,
    /** Entity has a lifetime expiry. */
    ROHR_LIFETIME                    = 1 << 15,
    /** Translation-only circular body used by collision/render helpers. */
    ROHR_PARTICLE                    = 1 << 16,
    /** Entity has an animated sprite. */
    ROHR_ANIMATED_SPRITE             = 1 << 17,
    /** Entity keeps physics values but skips physics updates. */
    ROHR_HOLD                        = 1 << 18,
    /** Entity belongs to one or more groups. */
    ROHR_GROUP                       = 1 << 19,
    /** Entity has a human-readable name used for state-file references. */
    ROHR_ENTITY_NAME                 = 1 << 20,
    /** Entity overrides the default collision category filtering. */
    ROHR_COLLISION_FILTER            = 1 << 21,
    /** Entity owns soft-body topology. */
    ROHR_SOFT_BODY                   = 1 << 22,
    /** Entity is a lightweight soft-body collision node. */
    ROHR_SOFT_BODY_NODE              = 1 << 23,
    /** Entity is a spring beam connecting soft-body nodes. */
    ROHR_SOFT_BODY_BEAM              = 1 << 24,
    /** Entity is a triangular soft-body surface. */
    ROHR_SOFT_BODY_AREA              = 1 << 25,
    /** Entity receives acceleration from the engine gravity stage. */
    ROHR_GRAVITY                     = 1 << 26,
    /** Entity follows authored motion but does not respond to physics forces. */
    ROHR_KINEMATIC_DRIVEN            = 1 << 27,
    /** Entity has a static sprite. */
    ROHR_SPRITE                      = 1 << 28,
    /** Entity has physics-owned animation-frame to hitbox bindings. */
    ROHR_HITBOX_ANIMATION_BINDING    = 1 << 29,
    /** Entity owns members of a spawned object instance. */
    ROHR_OBJECT                     = 1 << 30,
} Component;

/** Fixed-size, engine-owned entity name. */
typedef struct EntityName {
    char value[ENTITY_NAME_MAX];
} EntityName;

/** Caller-owned ID snapshot storage; owns no entities. */
typedef struct EntityList {
    Entity *data;
    size_t capacity;
    size_t count;
} EntityList;

/** Caller-owned copied-name snapshot storage; owns no entities. */
typedef struct EntityNameList {
    EntityName *data;
    size_t capacity;
    size_t count;
} EntityNameList;

/** Result type for functions that return an EntityName. */
ERROR_DECLARE_RESULT_TYPE(EntityNameResult, EntityName);

/** Copied metadata only; ownership is independent of parenting and connections. */
typedef struct EntityDescription {
    char name[ENTITY_NAME_MAX];
    Entity owner;
} EntityDescription;
ERROR_DECLARE_RESULT_TYPE(EntityDescriptionResult, EntityDescription);

/** Absolute deadlines. Enabled zero deadlines expire on the next lifecycle update. */
typedef struct EntityLifetimeDescription {
    Time expiry_time;
    Tick expiry_tick;
    bool time_enabled;
    bool tick_enabled;
} EntityLifetimeDescription;
ERROR_DECLARE_RESULT_TYPE(EntityLifetimeDescriptionResult, EntityLifetimeDescription);

/** Parent entity id alias. */
typedef Entity Parent;

/** Child entity id alias. */
typedef Entity Child;

/** Dynamic pool storing entity ids in a group. */
MEMORY_DECLARE_OBJECT_POOL(GroupEntityPool, Entity);

/** Group ownership kind used to protect specialized group invariants. */
typedef enum GroupKind {
    /** Generic application-owned group. */
    GROUP_KIND_GENERIC,
    /** Parent-owned group containing child entities. */
    GROUP_KIND_CHILDREN,
    /** Object-instance-owned members; mutate through object APIs only. */
    GROUP_KIND_OBJECT,
} GroupKind;

/** Dynamic group of entity ids. */
typedef struct EntityGroup {
    /** Group member entity ids. */
    GroupEntityPool entities;
    /** Ownership kind for mutation rules. */
    GroupKind kind;
} EntityGroup;

/** Result type for functions returning EntityGroup. */
ERROR_DECLARE_RESULT_TYPE(EntityGroupResult, EntityGroup);

/** Dynamic pool storing group ids on one entity. */
MEMORY_DECLARE_OBJECT_POOL(GroupIdPool, GroupId);

/** Dynamic group id list owned by one entity component. */
typedef struct EntityGroupMembership {
    /** Groups this entity belongs to. */
    GroupIdPool groups;
} EntityGroupMembership;

/** Result type for functions returning EntityGroupMembership. */
ERROR_DECLARE_RESULT_TYPE(EntityGroupMembershipResult, EntityGroupMembership);

/** Children component stores the group id containing a parent's children. */
typedef GroupId Children;

/** Result type for functions returning a children group id. */
ERROR_DECLARE_RESULT_TYPE(ChildrenResult, Children);

/** Result type for functions returning Parent. */
ERROR_DECLARE_RESULT_TYPE(ParentResult, Parent);

/** Expiry settings for an entity lifetime. */
typedef struct LifeTime {
    /** Engine time at which the entity expires, or zero when unused. */
    Time expirey_time;
    /** Engine tick at which the entity expires, or zero when unused. */
    Tick expirey_tick;
    /** Explicit enable flags preserve due deadlines at clock zero on restoration. */
    bool time_enabled;
    bool tick_enabled;
} LifeTime;

/** Pool storing parent entity ids by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(ParentPool, Parent);
/** Pool storing children group ids by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(ChildrenPool, Children);
/** Pool storing entity group memberships by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(EntityGroupMembershipPool, EntityGroupMembership);
/** Pool storing entity groups by group index. */
MEMORY_DECLARE_OBJECT_POOL(EntityGroupPool, EntityGroup);
/** Pool storing lifetime data by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(LifeTimePool, LifeTime);
/** Pool storing target entity ids by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(TargetPool, Entity);
/** Pool storing fixed-size entity names by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(EntityNamePool, EntityName);

/** Parent pool backing the parents table. */
extern ParentPool parents_pool;
/** Children pool backing the children table. */
extern ChildrenPool children_pool;
/** Entity group membership pool backing the entity_group_memberships table. */
extern EntityGroupMembershipPool entity_group_memberships_pool;
/** Global entity group pool. */
extern EntityGroupPool entity_groups_pool;
/** Lifetime pool backing the life_times table. */
extern LifeTimePool life_times_pool;
/** Target pool backing the targets table. */
extern TargetPool targets_pool;
/** Entity name pool backing the entity_names table. */
extern EntityNamePool entity_names_pool;

/** Parent table indexed by EntityIndex. */
#define parents parents_pool.objects
/** Group membership table indexed by EntityIndex. */
#define entity_group_memberships entity_group_memberships_pool.objects
/** Entity group table indexed by GroupId - 1. */
#define entity_groups entity_groups_pool.objects
/** Lifetime table indexed by EntityIndex. */
#define life_times life_times_pool.objects
/** Target table indexed by EntityIndex. */
#define targets targets_pool.objects
/** Entity name table indexed by EntityIndex. */
#define entity_names entity_names_pool.objects

/**
 * Assign a unique, non-empty name to an entity.
 *
 * @param entity Entity id to name.
 * @param name Null-terminated name shorter than ENTITY_NAME_MAX.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_name_set(Entity entity, const char *name);

/**
 * Find a live entity by its unique name.
 *
 * @param name Null-terminated entity name.
 * @return EntityResult containing the entity id, or an error.
 */
EntityResult entity_by_name_get(const char *name);

/**
 * Return a copy of an entity's fixed-size name component.
 *
 * @param entity Entity id to inspect.
 * @return EntityNameResult containing the name, or an error.
 */
EntityNameResult entity_name_get(Entity entity);

/**
 * Check whether an entity id currently refers to a live entity.
 *
 * @param entity Entity id to check.
 * @return true when the entity id is valid and live.
 */
bool entity_alive_check(Entity entity);

/**
 * Check whether an entity table index is currently live.
 *
 * @param index Entity table index to check.
 * @return true when the index is live.
 */
bool entity_index_alive_check(EntityIndex index);

/**
 * Return the number of currently alive entities.
 *
 * Systems can use this with entity_alive_at_get() to iterate only live entities
 * instead of scanning every possible entity slot.
 *
 * @return Number of alive entities.
 */
uint32_t entity_alive_count_get(void);

/**
 * Copy all live IDs in discovery order, without allocation or sorting.
 * Call between updates, serialized with entity creation, deletion and renaming.
 * Size caller storage with entity_alive_count_get(). No ordering stability is
 * promised across creation/deletion; deleted snapshot IDs become stale.
 * For every non-NULL list, count receives the required live count, including
 * on error. NULL list or NULL data with nonzero capacity returns
 * ERROR_ENGINE_ENTITY_LIST_INVALID. Insufficient capacity returns
 * ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED. Errors write no array elements;
 * success writes count elements and leaves any remaining capacity untouched.
 * NULL data with zero capacity is valid (success only when no entities live).
 * The caller owns storage; the snapshot neither owns nor retains entities.
 */
EngineResult entity_list_get(EntityList *list);

/**
 * Copy names with the same storage, error and ordering contract as entity_list_get.
 * Unnamed entities produce empty EntityName values. Names are independent copies
 * unaffected by later renaming/deletion. ID and name entries correspond when
 * no entities are created/deleted between the two calls.
 */
EngineResult entity_name_list_get(EntityNameList *list);

/**
 * Return the entity id stored at a dense alive-list position.
 *
 * The position is not a component table index and can change when entities are
 * deleted. Resolve the returned id with entity_index_get() before accessing
 * component tables.
 *
 * @param position Dense alive-list position.
 * @return EntityResult containing the entity id, or an error.
 */
EntityResult entity_alive_at_get(uint32_t position);

/**
 * Resolve an entity id to its current table index.
 *
 * @param entity Entity id to resolve.
 * @param index Out parameter receiving the current table index.
 * @return true when the entity resolves to a live index.
 */
bool entity_index_get(Entity entity, EntityIndex *index);

/**
 * Resolve a table index to its current entity id.
 *
 * @param index Entity table index to resolve.
 * @return EntityResult containing the entity id, or an error.
 */
EntityResult entity_from_index_get(EntityIndex index);

/**
 * Add a live entity and grow entity-indexed tables as needed.
 *
 * @return EntityResult containing a stable entity id, or an error.
 */
EntityResult entity_add(void);

/** Create metadata/ownership atomically. Empty uses "entity"; occupied names
 * append the lowest available _N suffix. Names are never truncated. */
EntityResult entity_description_add(EntityDescription description);
/** Replace metadata atomically. Requires a nonempty, unique, terminated name. */
EngineResult entity_description_set(Entity entity, EntityDescription description);
/** Return a copied name and current owner, without borrowing engine storage. */
EntityDescriptionResult entity_description_get(Entity entity);

/**
 * Delete an entity and make its id/index slots reusable.
 *
 * @param entity Entity id to delete.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_delete(Entity entity);

/**
 * Add one or more component bits to an entity.
 *
 * @param entity Entity id to update.
 * @param mask Component mask bits to add.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_components_add(Entity entity, RohrComponentMask mask);

/**
 * Check whether an entity has all requested components.
 *
 * @param entity Entity id to check.
 * @param components Component mask that must be present.
 * @return true when all requested bits are set.
 */
bool entity_components_check(Entity entity, RohrComponentMask components);

/**
 * Check whether an entity index has all requested components.
 *
 * @param index Entity table index to check.
 * @param components Component mask that must be present.
 * @return true when all requested bits are set.
 */
bool entity_index_components_check(EntityIndex index, RohrComponentMask components);

/**
 * Create a reusable group of entity ids.
 *
 * @return GroupIdResult containing a group id, or an error.
 */
GroupIdResult entity_group_create(void);

/** Assign a unique name to a generic entity group. */
EngineResult entity_group_name_set(GroupId group, const char *name);

/** Find a live generic group by name. */
GroupIdResult entity_group_by_name_get(const char *name);

/** Return a copy of a generic group's fixed-size name. */
GroupNameResult entity_group_name_get(GroupId group);

/**
 * Destroy a generic group and remove it from member entity group lists.
 *
 * Groups owned by parent-child relationships must be changed through the
 * parent/child APIs.
 *
 * @param group Group id to destroy.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_group_destroy(GroupId group);

/** Get the immediate lifetime owner, or ENTITY_INVALID for an independent entity. */
EntityResult entity_owner_get(Entity entity);
/**
 * Transfer lifetime ownership, or release with ENTITY_INVALID. Preserves world
 * state, parenting and all connections. Soft-body membership follows the
 * immediate owner without converting entity types. Ownership storage may
 * allocate; a failed transfer leaves the original ownership intact.
 */
EngineResult entity_owner_set(Entity entity, Entity owner);

/**
 * Add an entity to a generic group.
 *
 * Groups owned by parent-child relationships must be changed through the
 * parent/child APIs.
 *
 * @param group Group id to update.
 * @param entity Entity id to add.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_group_add(GroupId group, Entity entity);

/**
 * Remove an entity from a generic group.
 *
 * Groups owned by parent-child relationships must be changed through the
 * parent/child APIs.
 *
 * @param group Group id to update.
 * @param entity Entity id to remove.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_group_remove(GroupId group, Entity entity);

/**
 * Check whether an entity belongs to a group.
 *
 * @param group Group id to inspect.
 * @param entity Entity id to search for.
 * @return true when entity is a member of group.
 */
bool entity_group_entity_check(GroupId group, Entity entity);

/**
 * Get the group component for a group id.
 *
 * @param group Group id to inspect.
 * @return EntityGroupResult containing the group, or an error.
 */
EntityGroupResult entity_group_get(GroupId group);

/**
 * Get the groups assigned to an entity.
 *
 * @param entity Entity id to inspect.
 * @return EntityGroupMembershipResult containing group ids, or an error.
 */
EntityGroupMembershipResult entity_groups_get(Entity entity);

/**
 * Remove component bits from an entity.
 *
 * @param entity Entity id to update.
 * @param mask Component mask bits to remove.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_components_delete(Entity entity, RohrComponentMask mask);

/**
 * Set a parent-child relationship.
 *
 * @param parent Parent entity id.
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_child_set(Entity parent, Entity child);

/**
 * Set a parent-child relationship from the child side.
 *
 * @param child Child entity id.
 * @param parent Parent entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_parent_set(Entity child, Entity parent);

/**
 * Remove the parent from a child entity.
 *
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_parent_remove(Entity child);

/**
 * Remove a child from a parent entity.
 *
 * @param parent Parent entity id.
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_child_remove(Entity parent, Entity child);

/**
 * Get the children group id for an entity.
 *
 * @param entity Entity id to inspect.
 * @return ChildrenResult containing a group id, or an error.
 */
ChildrenResult entity_children_get(Entity entity);

/**
 * Get the parent entity for an entity.
 *
 * @param entity Entity id to inspect.
 * @return ParentResult containing parent id, or an error.
 */
ParentResult entity_parent_get(Entity entity);

/**
 * Replace absolute lifetime deadlines. Disabled deadlines are cleared; both
 * disabled removes lifetime data. Expiry occurs during lifecycle updates.
 *
 * @param entity Entity id to update.
 * @param description Complete replacement; time must be finite and nonnegative.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_lifetime_description_set(Entity entity, EntityLifetimeDescription description);
/** Return disabled zeros when a live entity has no lifetime configuration. */
EntityLifetimeDescriptionResult entity_lifetime_description_get(Entity entity);

/**
 * Remove lifetime data from an entity.
 *
 * @param entity Entity id to update.
 * @return EngineResult describing success or failure.
 */
EngineResult entity_life_time_remove(Entity entity);
#endif
