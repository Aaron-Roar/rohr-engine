/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_WORKSPACE_H
#define ROHR_EDITOR_WORKSPACE_H

#include "editor_project.h"

#define EDITOR_WORKSPACE_FORMAT_VERSION 1
#define EDITOR_WORKSPACE_PATH_MAX 1024
#define EDITOR_WORKSPACE_NAME_MAX 64

typedef struct EditorWorkspaceConfig {
    uint32_t format_version;
    char name[EDITOR_WORKSPACE_NAME_MAX];
    char source_directory[EDITOR_WORKSPACE_PATH_MAX];
    char generated_directory[EDITOR_WORKSPACE_PATH_MAX];
    char asset_directory[EDITOR_WORKSPACE_PATH_MAX];
    char object_directory[EDITOR_WORKSPACE_PATH_MAX];
    char editor_state_file[EDITOR_WORKSPACE_PATH_MAX];
    char engine_directory[EDITOR_WORKSPACE_PATH_MAX];
    char build_directory[EDITOR_WORKSPACE_PATH_MAX];
    char application_executable[EDITOR_WORKSPACE_PATH_MAX];
    char scene_executable[EDITOR_WORKSPACE_PATH_MAX];
} EditorWorkspaceConfig;

typedef struct EditorWorkspace {
    EditorWorkspaceConfig config;
    char directory[EDITOR_WORKSPACE_PATH_MAX];
    bool open;
} EditorWorkspace;

typedef enum EditorWorkspacePath {
    EDITOR_PATH_PROJECT, EDITOR_PATH_ENGINE, EDITOR_PATH_SOURCE,
    EDITOR_PATH_ASSETS, EDITOR_PATH_GENERATED, EDITOR_PATH_OBJECTS,
    EDITOR_PATH_STATE, EDITOR_PATH_BUILD, EDITOR_PATH_APPLICATION,
    EDITOR_PATH_SCENE, EDITOR_PATH_COUNT
} EditorWorkspacePath;
const char *editor_workspace_path_label_get(EditorWorkspacePath kind);
char *editor_workspace_path_value_get(EditorWorkspace *workspace, EditorWorkspacePath kind);
EditorResult editor_workspace_path_resolve(const char *root, const char *value, char *output, size_t capacity);
EditorResult editor_workspace_path_get(const EditorWorkspace *workspace, EditorWorkspacePath kind,
    char *output, size_t capacity);
bool editor_workspace_path_contains_check(const char *root, const char *path);
/* Resolve a stable project asset name (assets/...) to its authored storage. */
EditorResult editor_workspace_asset_path_get(const EditorWorkspace *workspace,
    const char *asset, char *output, size_t capacity);
/* Apply is synchronous; callers must stop builds/scenes and release project handles first. */
/* Normalize a draft and reject conflicts without mutating the filesystem. */
EditorResult editor_workspace_paths_validate(const EditorWorkspace *workspace, EditorWorkspace *desired);
/* Save and relocate transactionally. A successful false value means a recovery
 * copy could not be removed; its location is logged. Authored data is replaced
 * only after files and metadata are committed. */
EditorResult editor_workspace_paths_apply(EditorWorkspace *workspace, EditorProject *project,
    const EditorWorkspace *desired);
EditorResult editor_workspace_scaffold_paths_check(const EditorWorkspace *workspace);
EditorResult editor_workspace_scaffold_paths_write(const EditorWorkspace *workspace);
EditorResult editor_workspace_build_paths_write(const EditorWorkspace *workspace);

typedef enum EditorWorkspaceCommandType {
    EDITOR_WORKSPACE_COMMAND_CREATE,
    EDITOR_WORKSPACE_COMMAND_LOAD,
    EDITOR_WORKSPACE_COMMAND_SAVE,
    EDITOR_WORKSPACE_COMMAND_GENERATE_C
} EditorWorkspaceCommandType;

typedef struct EditorWorkspaceCommand {
    EditorWorkspaceCommandType type;
    char directory[EDITOR_WORKSPACE_PATH_MAX];
} EditorWorkspaceCommand;

/* Borrowed selection for one generation. Zero initialization includes all
 * objects and the first viewport. objects_selected with count zero includes none. */
typedef struct EditorSceneSelection {
    const EditorObjectId *objects;
    size_t object_count;
    bool objects_selected;
    EditorLayoutViewportId viewport;
    /* Distinguishes explicit no viewport (ID zero) from the first-viewport default. */
    bool viewport_selected;
} EditorSceneSelection;
EditorResult editor_workspace_scene_generate(const EditorWorkspace *workspace,
    const EditorProject *project, const EditorSceneSelection *selection);
EditorResult editor_workspace_editor_generate(const EditorWorkspace *workspace,
    const EditorProject *project);

/* Restore only missing build scaffolding; never generate C sources. Output flags
 * also describe successfully restored files when a later restoration fails. */
EditorResult editor_workspace_cmake_restore(const EditorWorkspace *workspace,
    bool *root_restored, bool *scene_restored);
EditorWorkspaceConfig editor_workspace_config_default_get(void);
EditorResult editor_workspace_create(EditorWorkspace *workspace, EditorProject *project,
    const char *directory);
EditorResult editor_workspace_load(EditorWorkspace *workspace, EditorProject *project,
    const char *directory);
bool editor_workspace_save(const EditorWorkspace *workspace,
    const EditorProject *project);
bool editor_workspace_c_generate(const EditorWorkspace *workspace,
    const EditorProject *project);
void editor_workspace_close(EditorWorkspace *workspace, EditorProject *project);
EditorResult editor_workspace_command_execute(EditorWorkspace *workspace,
    EditorProject *project, const EditorWorkspaceCommand *command);
EditorResult editor_workspace_command_cli_write(const EditorWorkspaceCommand *command,
    char *output, size_t output_capacity);

#endif
