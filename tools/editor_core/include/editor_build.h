/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_BUILD_H
#define ROHR_EDITOR_BUILD_H
#include "editor_config.h"
#include "editor_build_process.h"
#include "editor_scene_session.h"
typedef enum EditorBuildTarget { EDITOR_BUILD_APPLICATION, EDITOR_BUILD_SCENE, EDITOR_BUILD_PROJECT } EditorBuildTarget;
typedef enum EditorBuildStage { EDITOR_BUILD_IDLE, EDITOR_BUILD_STOPPING_SCENE, EDITOR_BUILD_CONFIGURING, EDITOR_BUILD_COMPILING } EditorBuildStage;
typedef struct EditorBuild EditorBuild;
/* Shared editor/CLI paths and relocation recovery. Only stale CMake metadata
 * is archived; saved configuration, project files, and valid caches are retained. */
EditorResult editor_build_workspace_get(const EditorWorkspace *workspace, EditorWorkspace *absolute);
EditorResult editor_build_cache_prepare(const EditorWorkspace *workspace, bool configure_enabled,
    EditorBuildOutput output, void *context);
EditorResult editor_build_cache_configured(const EditorWorkspace *workspace);
/* Shared configure-stage preparation, including missing CMake scaffolding.
 * A null compile command means the default CMake build command. */
EditorResult editor_build_prepare(const EditorWorkspace *workspace, bool configure_enabled,
    const EditorConfigCommand *compile, EditorBuildOutput output, void *context);
/* Explicit allocation. The session and output context are borrowed during a
 * build; cancel/destroy the build before destroying either borrowed owner. */
EditorBuild *editor_build_create(void);
EditorResult editor_build_start(EditorBuild *build, const EditorWorkspace *workspace,
    const EditorConfig *config, const char *sdk, const char *configure_option,
    EditorBuildTarget target, EditorSceneSession *scene,
    EditorBuildOutput output, void *context);
/* Poll the borrowed scene separately while waiting for it to stop. */
EditorResult editor_build_update(EditorBuild *build, bool *finished, int *exit_code);
void editor_build_cancel(EditorBuild *build);
void editor_build_destroy(EditorBuild *build);
bool editor_build_active_check(const EditorBuild *build);
EditorBuildStage editor_build_stage_get(const EditorBuild *build);
bool editor_build_compiled_check(const EditorBuild *build);
/* Resolves only a successfully compiled scene from this build. Failed, skipped,
 * cancelled, active, and application-only builds cannot prepare older output. */
EditorResult editor_build_scene_prepared_get(const EditorBuild *build,
    char *output, size_t capacity);
EditorBuildTarget editor_build_target_get(const EditorBuild *build);
const char *editor_build_target_label_get(EditorBuildTarget target);
EditorResult editor_build_target_name_get(const EditorWorkspace *workspace,
    EditorBuildTarget target, char *output, size_t capacity);
/* Resolves existing output only. No generation, configuration, or compilation.
 * Prefer CMake's actual output metadata; older default layouts have a fallback.
 * With several configurations, choose the newest existing executable. */
EditorResult editor_build_executable_get(const EditorWorkspace *workspace,
    EditorBuildTarget target, char *output, size_t capacity);
EditorResult editor_build_compile_arguments_get(const EditorConfigCommand *command,
    const char *project, const char *directory, const char *sdk, const char *target,
    char arguments[EDITOR_CONFIG_ARGUMENT_MAX][EDITOR_CONFIG_ARGUMENT_LENGTH_MAX],
    const char *output[EDITOR_CONFIG_ARGUMENT_MAX + 1]);
#endif
