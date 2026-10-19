/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#else
#include <windows.h>
#endif
#include "editor_build.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OK(call) do { EditorResult r = (call); if(editor_result_check(r)) { fprintf(stderr, "%s: %s\n", #call, r.result.error.message); assert(false); } } while(0)
static char captured[32768];
static size_t used;
static void capture(void *context, const char *bytes, size_t count) {
    (void)context; assert(used + count < sizeof(captured));
    memcpy(captured + used, bytes, count); used += count; captured[used] = 0;
}
static void reset_output(void) { used = 0; captured[0] = 0; }
static void save(const char *path, const char *text) {
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fwrite(text, 1, strlen(text), file) == strlen(text)); assert(fclose(file) == 0);
}
static int wait_process(EditorBuildProcess *process) {
    Uint64 deadline = SDL_GetTicks() + 6000; bool done = false; int code = 0;
    while(!done && SDL_GetTicks() < deadline) {
        OK(editor_build_process_poll(process, capture, NULL, &done, &code)); SDL_Delay(1);
    }
    assert(done && !editor_build_process_active_check(process)); return code;
}
static int wait_build(EditorBuild *build, EditorSceneSession *scene) {
    Uint64 deadline = SDL_GetTicks() + 6000; bool done = false; int code = 0;
    while(!done && SDL_GetTicks() < deadline) {
        if(scene) (void)editor_scene_session_update(scene);
        OK(editor_build_update(build, &done, &code)); SDL_Delay(1);
    }
    assert(done && !editor_build_active_check(build)); return code;
}
static void process_checks(const char *self, const char *directory) {
    EditorBuildProcess *process = editor_build_process_create(); assert(process);
    const char *arguments[] = {self, "echo", "literal space", "$(not-a-shell);&", "quote\"and\\", "", "trailing\\", NULL};
    reset_output(); OK(editor_build_process_start(process, directory, arguments));
    assert(editor_result_check(editor_build_process_start(process, directory, arguments)));
    assert(wait_process(process) == 0);
    assert(!strcmp(captured, "[literal space][$(not-a-shell);&][quote\"and\\][][trailing\\]"));
    const char *failed[] = {self, "exit", "23", NULL};
    OK(editor_build_process_start(process, directory, failed)); assert(wait_process(process) == 23);
    const char *exit259[] = {self, "exit", "259", NULL};
    OK(editor_build_process_start(process, directory, exit259));
#ifdef _WIN32
    assert(wait_process(process) == 259);
#else
    assert(wait_process(process) == 3);
#endif
    /* A descendant that ignores interruption must still be killed with its parent. */
    const char *tree[] = {self, "tree", self, NULL};
    reset_output(); OK(editor_build_process_start(process, directory, tree));
    Uint64 deadline = SDL_GetTicks() + 5000;
    while(!strstr(captured, "READY") && SDL_GetTicks() < deadline) {
        bool done; int code; OK(editor_build_process_poll(process, capture, NULL, &done, &code)); assert(!done); SDL_Delay(1);
    }
    assert(strstr(captured, "READY"));
    editor_build_process_interrupt(process); assert(wait_process(process) != 0);
    editor_build_process_destroy(process);
    /* Completion requires EOF from the pipe held by both parent and descendant.
     * The descendant never closes it or exits voluntarily. */
}
static void arguments_checks(void) {
    static char storage[EDITOR_CONFIG_ARGUMENT_MAX][EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
    const char *args[EDITOR_CONFIG_ARGUMENT_MAX + 1]; EditorConfigCommand command;
    OK(editor_build_compile_arguments_get(NULL, "/project space", "/build space", "/sdk", "Demo_scene", storage, args));
    assert(!strcmp(args[0], "cmake") && !strcmp(args[2], "/build space") && !strcmp(args[3], "--target") && !strcmp(args[4], "Demo_scene") && !args[5]);
    OK(editor_config_command_expression_parse("{\"custom\",\"{target}\",\"{project}\"}", &command));
    OK(editor_build_compile_arguments_get(&command, "/project", "/build", "/sdk", "Demo_project", storage, args));
    assert(!strcmp(args[1], "Demo_project") && !strcmp(args[2], "/project") && !args[3]);
    OK(editor_config_command_expression_parse("{\"custom\",\"{build}\"}", &command));
    assert(editor_result_check(editor_build_compile_arguments_get(&command, "p", "b", "s", "t", storage, args)));
    OK(editor_config_command_expression_parse("{\"cmake\",\"--build\",\"{build}\",\"--target\",\"wrong\"}", &command));
    assert(editor_result_check(editor_build_compile_arguments_get(&command, "p", "b", "s", "t", storage, args)));
    OK(editor_config_command_expression_parse("{}", &command));
    OK(editor_build_compile_arguments_get(&command, "p", "b", "s", "t", storage, args)); assert(!args[0]);
}
static void executable_checks(EditorWorkspace *workspace) {
    char path[2048], executable[2048], selected[2048];
    assert(editor_result_check(editor_build_executable_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))));
    SDL_snprintf(executable, sizeof(executable), "%s/build/Demo_scene%s", workspace->directory,
#ifdef _WIN32
        ".exe"
#else
        ""
#endif
    ); save(executable, "old build");
    OK(editor_build_executable_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))); assert(!strcmp(selected, executable));
    SDL_snprintf(path, sizeof(path), "%s/build/rohr_scene_Debug.path", workspace->directory);
    save(path, "/missing/scene");
    assert(editor_result_check(editor_build_executable_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))));
    save(path, executable);
    OK(editor_build_executable_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))); assert(!strcmp(selected, executable));
    char newer[2048]; SDL_snprintf(newer, sizeof(newer), "%s/build/custom scene output", workspace->directory);
    SDL_Delay(20); save(newer, "new build");
    SDL_snprintf(path, sizeof(path), "%s/build/rohr_scene_Release.path", workspace->directory); save(path, newer);
    OK(editor_build_executable_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))); assert(!strcmp(selected, newer));
    /* Resolution leaves generated source and output bytes untouched. */
    size_t size; char *bytes = SDL_LoadFile(newer, &size); assert(bytes && size == 9 && !strcmp(bytes, "new build")); SDL_free(bytes);
    SDL_snprintf(path, sizeof(path), "%s/build/rohr_target_scene.txt", workspace->directory); save(path, "Custom_scene");
    OK(editor_build_target_name_get(workspace, EDITOR_BUILD_SCENE, selected, sizeof(selected))); assert(!strcmp(selected, "Custom_scene"));
}
static void controller_checks(const char *self, const char *child, EditorWorkspace *workspace) {
    static EditorConfig config; editor_config_init(&config);
    config.gui_configure = (EditorConfigCommand){.set = true, .count = 3};
    SDL_strlcpy(config.gui_configure.arguments[0], self, sizeof(config.gui_configure.arguments[0]));
    SDL_strlcpy(config.gui_configure.arguments[1], "echo", sizeof(config.gui_configure.arguments[1]));
    SDL_strlcpy(config.gui_configure.arguments[2], "CONFIGURED", sizeof(config.gui_configure.arguments[2]));
    config.gui_compile = config.gui_configure;
    SDL_strlcpy(config.gui_compile.arguments[2], "{target}", sizeof(config.gui_compile.arguments[2]));
    EditorBuild *build = editor_build_create(); assert(build);
    for(int target = EDITOR_BUILD_APPLICATION; target <= EDITOR_BUILD_PROJECT; ++target) {
        reset_output(); OK(editor_build_start(build, workspace, &config, "", "", target, NULL, capture, NULL));
        assert(editor_result_check(editor_build_start(build, workspace, &config, "", "", target, NULL, capture, NULL)));
        assert(wait_build(build, NULL) == 0 && editor_build_compiled_check(build));
        assert(strstr(captured, "[CONFIGURED]") && strstr(captured, target == EDITOR_BUILD_APPLICATION ? "[Demo]" : target == EDITOR_BUILD_SCENE ? "[Custom_scene]" : "[Demo_project]"));
    }
    SDL_strlcpy(config.gui_configure.arguments[1], "exit", sizeof(config.gui_configure.arguments[1]));
    SDL_strlcpy(config.gui_configure.arguments[2], "7", sizeof(config.gui_configure.arguments[2]));
    reset_output(); OK(editor_build_start(build, workspace, &config, "", "", EDITOR_BUILD_SCENE, NULL, capture, NULL));
    assert(wait_build(build, NULL) == 7 && !editor_build_compiled_check(build)); assert(!strstr(captured, "Custom_scene"));
    config.gui_configure.count = 0; config.gui_compile.count = 0;
    OK(editor_build_start(build, workspace, &config, "", "", EDITOR_BUILD_APPLICATION, NULL, NULL, NULL));
    assert(wait_build(build, NULL) == 0 && !editor_build_compiled_check(build));
    EditorSceneSession *scene = editor_scene_session_create(); assert(scene);
    for(int target = EDITOR_BUILD_APPLICATION; target <= EDITOR_BUILD_PROJECT; ++target) {
        assert(!error_check(editor_scene_session_launch(scene, child)));
        Uint64 deadline = SDL_GetTicks() + 10000;
        while(editor_scene_session_state_get(scene) != EDITOR_SCENE_RUNNING && SDL_GetTicks() < deadline) {
            (void)editor_scene_session_update(scene); SDL_Delay(1);
        }
        assert(editor_scene_session_state_get(scene) == EDITOR_SCENE_RUNNING);
        OK(editor_build_start(build, workspace, &config, "", "", target, scene, NULL, NULL));
        if(target != EDITOR_BUILD_APPLICATION) {
            assert(editor_build_stage_get(build) == EDITOR_BUILD_STOPPING_SCENE);
            bool done; int code; OK(editor_build_update(build, &done, &code)); assert(!done);
        }
        assert(wait_build(build, scene) == 0);
        assert(editor_scene_session_active_check(scene) == (target == EDITOR_BUILD_APPLICATION));
        editor_scene_session_stop(scene);
        while(editor_scene_session_active_check(scene)) { (void)editor_scene_session_update(scene); SDL_Delay(1); }
    }
    editor_scene_session_destroy(scene); editor_build_destroy(build);
}
/* Optional installed-SDK probe: uses the same controller and resolver as the GUI. */
static int project_probe(char **argv) {
    static EditorProject project;
    static EditorConfig config;
    EditorWorkspace workspace = {0};
    EditorWorkspaceCommand command = {.type = EDITOR_WORKSPACE_COMMAND_LOAD};
    SDL_strlcpy(command.directory, argv[2], sizeof(command.directory));
    editor_project_init(&project); OK(editor_workspace_command_execute(&workspace, &project, &command));
    EditorBuildTarget target = !strcmp(argv[3], "app") ? EDITOR_BUILD_APPLICATION :
        !strcmp(argv[3], "scene") ? EDITOR_BUILD_SCENE : EDITOR_BUILD_PROJECT;
    int code = 0;
    if(strcmp(argv[1], "resolve")) {
        char option[2048]; SDL_snprintf(option, sizeof(option), "-DCMAKE_PREFIX_PATH=%s", argv[4]);
        editor_config_init(&config); EditorBuild *build = editor_build_create(); assert(build);
        OK(editor_build_start(build, &workspace, &config, argv[4], option, target, NULL, NULL, NULL));
        /* Compiling a fresh SDK project can exceed the short unit-test deadline. */
        bool done = false; Uint64 deadline = SDL_GetTicks() + 120000;
        while(!done && SDL_GetTicks() < deadline) { OK(editor_build_update(build, &done, &code)); SDL_Delay(1); }
        assert(done); editor_build_destroy(build);
    }
    if(!code && target != EDITOR_BUILD_PROJECT) {
        char executable[2048]; OK(editor_build_executable_get(&workspace, target, executable, sizeof(executable)));
        puts(executable);
    }
    editor_project_destroy(&project); return code;
}
int main(int argc, char **argv) {
    if(argc == 5 && (!strcmp(argv[1], "project") || !strcmp(argv[1], "resolve"))) return project_probe(argv);
    if(argc > 1 && !strcmp(argv[1], "echo")) { for(int i = 2; i < argc; ++i) printf("[%s]", argv[i]); return 0; }
    if(argc > 1 && !strcmp(argv[1], "exit")) return atoi(argv[2]);
    if(argc > 1 && !strcmp(argv[1], "stubborn")) {
#ifndef _WIN32
        signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);
#endif
        puts("READY"); fflush(stdout);
        for(;;) SDL_Delay(10);
    }
    if(argc > 1 && !strcmp(argv[1], "tree")) {
#ifndef _WIN32
        signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);
        pid_t child = fork(); assert(child >= 0);
        if(!child) { execl(argv[2], argv[2], "stubborn", (char *)NULL); _exit(127); }
        while(waitpid(child, NULL, 0) < 0) {}
#else
        const char *args[] = {argv[2], "stubborn", NULL};
        SDL_Process *child = SDL_CreateProcess(args, false); assert(child); int code;
        assert(SDL_WaitProcess(child, true, &code)); SDL_DestroyProcess(child);
#endif
        return 0;
    }
    assert(argc == 4);
    EditorWorkspace workspace = {.open = true};
    SDL_strlcpy(workspace.directory, argv[3], sizeof(workspace.directory)); SDL_strlcpy(workspace.config.name, "Demo", sizeof(workspace.config.name));
    assert(SDL_CreateDirectory(workspace.directory)); char build[2048]; SDL_snprintf(build, sizeof(build), "%s/build", workspace.directory); assert(SDL_CreateDirectory(build));
    const char *fixtures[] = {"build/Demo_scene", "build/Demo_scene.exe",
        "build/rohr_scene_Debug.path", "build/rohr_scene_Release.path",
        "build/custom scene output", "build/rohr_target_scene.txt"};
    for(size_t i = 0; i < SDL_arraysize(fixtures); ++i) {
        char path[2048]; SDL_snprintf(path, sizeof(path), "%s/%s", workspace.directory, fixtures[i]); (void)SDL_RemovePath(path);
    }
    arguments_checks(); process_checks(argv[1], workspace.directory); executable_checks(&workspace); controller_checks(argv[1], argv[2], &workspace);
    return 0;
}
