/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/stat.h>
#endif
#include "editor_workspace.h"
#include "editor_build.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OK(call) do { EditorResult r = (call); if(editor_result_check(r)) { fprintf(stderr, "%s: %s\n", #call, r.result.error.message); assert(false); } } while(0)
static EditorProject project, loaded;
static char path[EDITOR_WORKSPACE_PATH_MAX * 2];
static const char *at(const EditorWorkspace *w, EditorWorkspacePath kind, const char *name) {
    char root[sizeof(path)]; OK(editor_workspace_path_get(w, kind, root, sizeof(root)));
    OK(editor_workspace_path_resolve(root, name, path, sizeof(path))); return path;
}
static void write_file(const char *name, const char *text) {
    FILE *file = fopen(name, "wb"); assert(file);
    assert(fputs(text, file) >= 0); assert(!fclose(file));
}
static void content(const char *name, const char *expected) {
    char *text = SDL_LoadFile(name, NULL); assert(text && !strcmp(text, expected)); SDL_free(text);
}
static bool exists(const char *name) { SDL_PathInfo info; return SDL_GetPathInfo(name, &info); }
static void rejected(EditorWorkspace *w, EditorWorkspace *desired) {
    EditorWorkspace before = *w;
    assert(editor_result_check(editor_workspace_paths_apply(w, &project, desired)));
    assert(!memcmp(w, &before, sizeof(before)));
    content(at(w, EDITOR_PATH_SOURCE, "main.c"), "/* developer-owned entry point */\n");
}
int main(int argc, char **argv) {
    assert(argc >= 2);
    if(argc == 4 && !strcmp(argv[1], "configure")) {
        EditorWorkspace w = {0}; OK(editor_workspace_load(&w, &project, argv[2]));
        EditorWorkspace desired = w;
        SDL_strlcpy(desired.config.engine_directory, argv[3], sizeof(desired.config.engine_directory));
        SDL_strlcpy(desired.config.source_directory, "code files", sizeof(desired.config.source_directory));
        SDL_strlcpy(desired.config.asset_directory, "../shared assets", sizeof(desired.config.asset_directory));
        SDL_strlcpy(desired.config.generated_directory, "../shared generated", sizeof(desired.config.generated_directory));
        SDL_strlcpy(desired.config.object_directory, "documents", sizeof(desired.config.object_directory));
        SDL_strlcpy(desired.config.editor_state_file, "documents/state.json", sizeof(desired.config.editor_state_file));
        SDL_strlcpy(desired.config.build_directory, "../shared build", sizeof(desired.config.build_directory));
        SDL_strlcpy(desired.config.application_executable, "../output app/demo", sizeof(desired.config.application_executable));
        SDL_strlcpy(desired.config.scene_executable, "../output scene/demo", sizeof(desired.config.scene_executable));
        OK(editor_workspace_paths_apply(&w, &project, &desired));
        desired = w; SDL_strlcat(desired.directory, " moved", sizeof(desired.directory));
        OK(editor_workspace_paths_apply(&w, &project, &desired));
        editor_workspace_close(&w, &project); return 0;
    }
    char root[EDITOR_WORKSPACE_PATH_MAX], outside[sizeof(root)];
    SDL_snprintf(root, sizeof(root), "%s/%llu/original project", argv[1], (unsigned long long)SDL_GetPerformanceCounter());
    EditorWorkspace w = {0}, desired, reopened;
    OK(editor_workspace_create(&w, &project, root));
    assert(!w.config.build_directory[0] && !w.config.application_executable[0]);
    assert(!strcmp(w.config.generated_directory, "src/generated"));
    write_file(at(&w, EDITOR_PATH_SOURCE, "main.c"), "/* developer-owned entry point */\n");
    write_file(at(&w, EDITOR_PATH_ASSETS, "retained.txt"), "asset bytes");
    desired = w;
    SDL_strlcpy(desired.config.source_directory, "source code", sizeof(desired.config.source_directory));
    SDL_strlcpy(desired.config.asset_directory, "resources", sizeof(desired.config.asset_directory));
    SDL_strlcpy(desired.config.object_directory, "documents", sizeof(desired.config.object_directory));
    SDL_strlcpy(desired.config.build_directory, "build cache", sizeof(desired.config.build_directory));
    SDL_strlcpy(desired.config.application_executable, "bin/app custom", sizeof(desired.config.application_executable));
    SDL_strlcpy(desired.config.scene_executable, "preview/scene custom", sizeof(desired.config.scene_executable));
    OK(editor_workspace_paths_apply(&w, &project, &desired));
    assert(!strcmp(w.config.generated_directory, "source code/generated"));
    assert(!strcmp(w.config.editor_state_file, "documents/project.rohr.json"));
    content(at(&w, EDITOR_PATH_SOURCE, "main.c"), "/* developer-owned entry point */\n");
    content(at(&w, EDITOR_PATH_ASSETS, "retained.txt"), "asset bytes");
    assert(editor_workspace_c_generate(&w, &project));
    OK(editor_workspace_load(&reopened, &loaded, w.directory));
    assert(!memcmp(&w.config, &reopened.config, sizeof(w.config))); editor_workspace_close(&reopened, &loaded);
    OK(editor_workspace_asset_path_get(&w, "assets/retained.txt", path, sizeof(path))); content(path, "asset bytes");
    static char arguments[EDITOR_CONFIG_ARGUMENT_MAX][EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
    const char *output[EDITOR_CONFIG_ARGUMENT_MAX + 1] = {arguments[0], NULL};
    strcpy(arguments[0], "{source}|{assets}|{generated}|{objects}|{state}|{application}|{scene}");
    OK(editor_config_workspace_arguments_expand(&w, arguments, output)); assert(!strchr(arguments[0], '{'));
    /* External contents stay outside a later project move. */
    SDL_snprintf(outside, sizeof(outside), "%s external assets", root);
    desired = w; SDL_strlcpy(desired.config.asset_directory, outside, sizeof(desired.config.asset_directory));
    OK(editor_workspace_paths_apply(&w, &project, &desired));
    desired = w; SDL_snprintf(desired.directory, sizeof(desired.directory), "%s moved", root);
    OK(editor_workspace_paths_apply(&w, &project, &desired));
    assert(!exists(root)); assert(!strcmp(w.config.asset_directory, outside));
    content(at(&w, EDITOR_PATH_ASSETS, "retained.txt"), "asset bytes");
    content(at(&w, EDITOR_PATH_SOURCE, "main.c"), "/* developer-owned entry point */\n");
    /* Occupied, destructive, and aliased destinations leave the project intact. */
    desired = w; SDL_strlcpy(desired.config.source_directory, "documents", sizeof(desired.config.source_directory)); rejected(&w, &desired);
    desired = w; SDL_strlcpy(desired.config.generated_directory, "resources", sizeof(desired.config.generated_directory));
    SDL_strlcpy(desired.config.asset_directory, "resources", sizeof(desired.config.asset_directory)); rejected(&w, &desired);
    desired = w; SDL_strlcpy(desired.config.application_executable, "source code/main.c", sizeof(desired.config.application_executable)); rejected(&w, &desired);
#ifndef _WIN32
    char link[sizeof(root)]; SDL_snprintf(link, sizeof(link), "%s/link", w.directory);
    assert(!symlink(outside, link));
    desired = w; SDL_strlcpy(desired.config.generated_directory, "link/danger", sizeof(desired.config.generated_directory)); rejected(&w, &desired);
    assert(SDL_RemovePath(link));
    /* Copy failure after retaining the original must roll back the whole move. */
    SDL_snprintf(link, sizeof(link), "%s/unsupported_fifo", w.directory); assert(!mkfifo(link, 0600));
    desired = w; SDL_snprintf(desired.directory, sizeof(desired.directory), "%s failed", root); rejected(&w, &desired);
    assert(!exists(desired.directory)); assert(SDL_RemovePath(link));
#endif
    /* A metadata failure after relocating content restores both locations. */
    char settings[sizeof(path)]; SDL_strlcpy(settings, at(&w, EDITOR_PATH_PROJECT, ".rohr/project_paths.cmake"), sizeof(settings));
    char *settings_text = SDL_LoadFile(settings, NULL); assert(settings_text);
    assert(SDL_RemovePath(settings)); assert(SDL_CreateDirectory(settings));
    desired = w; SDL_strlcpy(desired.config.source_directory, "metadata rollback", sizeof(desired.config.source_directory));
    rejected(&w, &desired); assert(!exists(at(&w, EDITOR_PATH_PROJECT, "metadata rollback")));
    assert(SDL_RemovePath(settings)); write_file(settings, settings_text); SDL_free(settings_text);
    /* Custom scaffolding is preserved and rejected before any content move. */
    const char *cmake_path = at(&w, EDITOR_PATH_PROJECT, "CMakeLists.txt");
    char *cmake = SDL_LoadFile(cmake_path, NULL); assert(cmake);
    write_file(cmake_path, "# developer custom scaffold\n");
    desired = w; SDL_strlcpy(desired.config.source_directory, "new source", sizeof(desired.config.source_directory)); rejected(&w, &desired);
    content(at(&w, EDITOR_PATH_PROJECT, "CMakeLists.txt"), "# developer custom scaffold\n");
    write_file(at(&w, EDITOR_PATH_PROJECT, "CMakeLists.txt"), cmake); SDL_free(cmake);
    /* Reset overrides moves managed content back and restores inherited outputs. */
    desired = w; desired.config = editor_workspace_config_default_get();
    SDL_strlcpy(desired.config.name, w.config.name, sizeof(desired.config.name));
    OK(editor_workspace_paths_apply(&w, &project, &desired));
    assert(!exists(outside)); assert(!w.config.application_executable[0]);
    content(at(&w, EDITOR_PATH_SOURCE, "main.c"), "/* developer-owned entry point */\n");
    if(argc > 2) {
        desired = w; SDL_snprintf(desired.directory, sizeof(desired.directory), "%s/c7_%llu", argv[2], (unsigned long long)SDL_GetPerformanceCounter());
        OK(editor_workspace_paths_apply(&w, &project, &desired));
        content(at(&w, EDITOR_PATH_ASSETS, "retained.txt"), "asset bytes");
        desired = w; SDL_snprintf(desired.directory, sizeof(desired.directory), "%s returned", root);
        OK(editor_workspace_paths_apply(&w, &project, &desired));
    }
    editor_workspace_close(&w, &project);
    puts("workspace path relocation checks passed"); return 0;
}
