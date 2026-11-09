/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_PROJECT_INSTANCE_H
#define ROHR_PROJECT_INSTANCE_H

#include "object.h"
#include "input.h"
#include "audio.h"

#define ROHR_PROJECT_LIMIT 16
#define ROHR_PROJECT_INVALID UINT64_C(0)
typedef uint64_t ProjectId;
ERROR_DECLARE_RESULT_TYPE(ProjectIdResult, ProjectId);

/* Project composition uses the same input-owned recipes as standalone creation. */
typedef ControllerActionDescription ProjectActionDescription;
typedef ControllerDescription ProjectControllerDescription;
typedef struct ProjectViewportDescription {
    const char *name;
    /** An immutable recipe without runtime bindings or a template chain. */
    const ViewportCreateDescription *description;
} ProjectViewportDescription;
typedef struct ProjectSoundDescription {
    const char *name;
    SoundDescription description;
} ProjectSoundDescription;
typedef struct ProjectMusicDescription {
    const char *name;
    MusicDescription description;
} ProjectMusicDescription;

/** Immutable input borrowed only during creation. The initial root member keeps
 * object_create(description) available for entity-hierarchy-only creation.
 * The root must be a parentless container. Names are unique within each list.
 * All lists are bounded by their corresponding engine resource limits. */
typedef struct ProjectDescription {
    ObjectDescription root;
    const ProjectViewportDescription *viewports;
    size_t viewport_count;
    const ProjectControllerDescription *controllers;
    size_t controller_count;
    const ProjectSoundDescription *sounds;
    size_t sound_count;
    const ProjectMusicDescription *music;
    size_t music_count;
} ProjectDescription;
typedef struct ViewportList {
    ViewportId values[MAX_VIEWPORTS];
    size_t count;
} ViewportList;
ERROR_DECLARE_RESULT_TYPE(ViewportListResult, ViewportList);

/** Allocate one owner, create its hierarchy and viewports, create independent
 * input state, and load all audio stopped. Services must already be started.
 * Never applies global settings, advances systems, presents, or starts playback.
 * Failure rolls back this attempt. Project instances share the engine world. */
ProjectIdResult rohr_internal_project_create(const ProjectDescription *description);
/** Destroy owned viewports before entities and release input/audio. Zero is a
 * no-op; stale nonzero IDs fail. Engine shutdown also destroys remaining owners.
 * Lookup results are borrowed: do not independently destroy these owners.
 * Individual entity members may be changed, extended, or deleted normally. */
EngineResult project_destroy(ProjectId project);
EntityResult project_root_get(ProjectId project);
ViewportIdResult project_viewport_by_name_get(ProjectId project, const char *name);
ViewportListResult project_viewports_get(ProjectId project);
InputControllerIdResult project_controller_by_name_get(ProjectId project, const char *name);
SoundResult project_sound_by_name_get(ProjectId project, const char *name);
MusicResult project_music_by_name_get(ProjectId project, const char *name);

/* Same typed, address-preserving convention as entity/object creation. */
static inline const ObjectDescription *rohr_internal_project_root_ref(
        const ProjectDescription *description) { return &description->root; }
static inline const ProjectDescription *rohr_internal_project_description_ref(
        const ProjectDescription *description) { return description; }
#undef ROHR_INTERNAL_PROJECT_ROOT_TYPES
#define ROHR_INTERNAL_PROJECT_ROOT_TYPES(apply) \
    apply(ProjectDescription, rohr_internal_project_root_ref)
#define ROHR_INTERNAL_PROJECT_CREATE_TYPES(apply)
#define project_create(...) \
    rohr_internal_project_create(_Generic(&(__VA_ARGS__), \
        ProjectDescription *: rohr_internal_project_description_ref, \
        const ProjectDescription *: rohr_internal_project_description_ref \
        ROHR_INTERNAL_PROJECT_CREATE_TYPES(ROHR_INTERNAL_CREATION_TYPE))(&(__VA_ARGS__)))

#endif
