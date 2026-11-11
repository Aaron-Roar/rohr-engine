/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_PROJECT_INTERNAL_H
#define ROHR_PROJECT_INTERNAL_H
#include "project_instance.h"
typedef struct ViewportList {
    ViewportId values[MAX_VIEWPORTS];
    size_t count;
} ViewportList;
ERROR_DECLARE_RESULT_TYPE(ViewportListResult, ViewportList);

/* Private ownership checks used by engine regression tests. */
EntityResult project_root_get(ProjectId project);
ViewportIdResult project_viewport_by_name_get(ProjectId project, const char *name);
ViewportListResult project_viewports_get(ProjectId project);
InputControllerIdResult project_controller_by_name_get(ProjectId project, const char *name);
SoundResult project_sound_by_name_get(ProjectId project, const char *name);
MusicResult project_music_by_name_get(ProjectId project, const char *name);
void project_instances_destroy(void);
#endif
