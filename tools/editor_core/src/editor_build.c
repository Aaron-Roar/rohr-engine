/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _XOPEN_SOURCE 700
#endif
#include "editor_build.h"
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

struct EditorBuild {
    EditorWorkspace workspace;
    EditorConfigCommand configure, compile;
    char sdk[EDITOR_WORKSPACE_PATH_MAX * 2], option[EDITOR_WORKSPACE_PATH_MAX * 2];
    char arguments[EDITOR_CONFIG_ARGUMENT_MAX][EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
    const char *argv[EDITOR_CONFIG_ARGUMENT_MAX + 1];
    EditorBuildTarget target;
    EditorBuildStage stage;
    EditorBuildProcess *process;
    EditorSceneSession *scene;
    EditorBuildOutput output;
    void *context;
    bool compiled;
    bool succeeded;
};
static bool target_valid_check(EditorBuildTarget target) {
    return target >= EDITOR_BUILD_APPLICATION && target <= EDITOR_BUILD_PROJECT;
}
const char *editor_build_target_label_get(EditorBuildTarget target) {
    switch(target) {
        case EDITOR_BUILD_APPLICATION: return "Application";
        case EDITOR_BUILD_SCENE: return "Scene";
        case EDITOR_BUILD_PROJECT: return "Project";
        default: return "Unknown";
    }
}
static const char *target_key(EditorBuildTarget target) {
    return target == EDITOR_BUILD_APPLICATION ? "application" : target == EDITOR_BUILD_SCENE ? "scene" : "project";
}
static EditorResult path_copy(char *output, size_t capacity, const char *text) {
    if(!output || !text || !capacity || strlen(text) >= capacity)
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Build path or target is too long");
    memcpy(output, text, strlen(text) + 1); return editor_result_value(true);
}
EditorResult editor_build_workspace_get(const EditorWorkspace *workspace, EditorWorkspace *absolute) {
    if(!workspace || !workspace->open || !absolute) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Build needs an open project");
    *absolute = *workspace;
#ifdef _WIN32
    char *resolved = _fullpath(NULL, workspace->directory, 0);
#else
    char *resolved = realpath(workspace->directory, NULL);
#endif
    if(!resolved) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not resolve project directory: %s", workspace->directory);
    EditorResult result = path_copy(absolute->directory, sizeof(absolute->directory), resolved);
    free(resolved);
    if(!editor_result_check(result)) result = editor_workspace_path_get(workspace, EDITOR_PATH_BUILD,
        absolute->config.build_directory, sizeof(absolute->config.build_directory));
    return result;
}
static char *metadata_read(const char *path) {
    SDL_PathInfo info;
    if(!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE || !info.size ||
            info.size >= EDITOR_CONFIG_ARGUMENT_LENGTH_MAX) return NULL;
    size_t size; char *text = SDL_LoadFile(path, &size);
    if(text && memchr(text, 0, size)) { SDL_free(text); return NULL; }
    return text;
}
/* CMake caches are local build state, not portable project configuration. */
static bool build_separator_check(char value) {
#ifdef _WIN32
    return value == '/' || value == '\\';
#else
    return value == '/';
#endif
}
static bool build_path_same_check(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    while(na > 1 && build_separator_check(a[na - 1])) --na;
    while(nb > 1 && build_separator_check(b[nb - 1])) --nb;
    if(na != nb) return false;
    for(size_t i = 0; i < na; ++i) {
        char ca = a[i], cb = b[i];
#ifdef _WIN32
        if(ca == '\\') ca = '/';
        if(cb == '\\') cb = '/';
        ca = (char)SDL_tolower(ca); cb = (char)SDL_tolower(cb);
#endif
        if(ca != cb) return false;
    }
    return true;
}
static bool build_file_check(const char *path) {
    SDL_PathInfo info;
    return SDL_GetPathInfo(path, &info);
}
static bool build_project_path_check(const char *path, const char *root) {
    size_t length = strlen(root);
    while(length > 1 && build_separator_check(root[length - 1])) --length;
    if(strlen(path) <= length) return false;
    char prefix[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(length >= sizeof(prefix)) return false;
    memcpy(prefix, path, length); prefix[length] = 0;
    return build_path_same_check(prefix, root) &&
        ((length == 1 && root[0] == '/') || build_separator_check(path[length]));
}
static EditorResult build_location_get(const EditorWorkspace *workspace,
        bool *stale, bool *cached, bool *pending) {
    char directory[EDITOR_WORKSPACE_PATH_MAX * 2], path[EDITOR_WORKSPACE_PATH_MAX * 2];
    *stale = *cached = *pending = false;
    SDL_snprintf(directory, sizeof(directory), "%s", workspace->config.build_directory);
    /* Never recover a copied project by modifying a build-directory link into
     * its original project (or another external directory). */
    bool linked = false;
#ifdef _WIN32
    wchar_t *wide = (wchar_t *)SDL_iconv_string("UTF-16LE", "UTF-8", directory, strlen(directory) + 1);
    if(!wide) return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not inspect build directory");
    DWORD attributes = GetFileAttributesW(wide); SDL_free(wide);
    linked = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat info;
    linked = lstat(directory, &info) == 0 && S_ISLNK(info.st_mode);
#endif
    if(linked) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Build directory is a link; use a project-owned build directory: %s", directory);
    SDL_snprintf(path, sizeof(path), "%s/rohr_reconfigure_required.txt", directory);
    *pending = build_file_check(path);
    SDL_snprintf(path, sizeof(path), "%s/CMakeCache.txt", directory);
    if(!build_file_check(path)) return editor_result_value(true);
    size_t size = 0; char *text = SDL_LoadFile(path, &size);
    if(!text || memchr(text, 0, size)) {
        SDL_free(text);
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not read CMake cache: %s", path);
    }
    const char *keys[] = {"CMAKE_HOME_DIRECTORY:INTERNAL=", "CMAKE_CACHEFILE_DIR:INTERNAL="};
    const char *expected[] = {workspace->directory, directory};
    bool found[2] = {false, false};
    for(char *line = text; line && *line;) {
        char *next = strchr(line, '\n'); if(next) *next++ = 0;
        size_t length = strlen(line); if(length && line[length - 1] == '\r') line[length - 1] = 0;
        for(size_t i = 0; i < SDL_arraysize(keys); ++i) {
            size_t prefix = strlen(keys[i]);
            if(!strncmp(line, keys[i], prefix)) {
                found[i] = true;
                *stale |= !build_path_same_check(line + prefix, expected[i]);
            }
        }
        line = next;
    }
    *cached = found[0] && found[1];
    char configured_path[sizeof(path)];
    SDL_snprintf(configured_path, sizeof(configured_path), "%s/rohr_paths_configured.cmake", directory);
    if(build_file_check(configured_path)) {
        size_t old_size = 0, new_size = 0;
        char *old = SDL_LoadFile(configured_path, &old_size);
        SDL_snprintf(path, sizeof(path), "%s/.rohr/project_paths.cmake", workspace->directory);
        char *current = SDL_LoadFile(path, &new_size);
        *stale |= !old || !current || old_size != new_size || memcmp(old, current, old_size);
        SDL_free(old); SDL_free(current);
    }
    SDL_free(text); return editor_result_value(true);
}
static EditorResult build_metadata_archive(const char *directory, const char *archive, const char *name) {
    char source[EDITOR_WORKSPACE_PATH_MAX * 2], target[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(SDL_snprintf(source, sizeof(source), "%s/%s", directory, name) >= (int)sizeof(source) ||
            SDL_snprintf(target, sizeof(target), "%s/%s", archive, name) >= (int)sizeof(target))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Relocated build metadata path is too long");
    if(build_file_check(source) && !SDL_RenamePath(source, target))
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not set aside relocated metadata %s: %s", source, SDL_GetError());
    return editor_result_value(true);
}
EditorResult editor_build_cache_prepare(const EditorWorkspace *workspace, bool configure_enabled,
        EditorBuildOutput output, void *context) {
    EditorWorkspace absolute;
    EditorResult result = editor_build_workspace_get(workspace, &absolute);
    if(editor_result_check(result)) return result;
    bool stale, cached, pending;
    result = build_location_get(&absolute, &stale, &cached, &pending);
    if(editor_result_check(result) || (!stale && !pending)) return result;
    if(!configure_enabled) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Project location or paths changed; enable configuration before rebuilding %s", absolute.directory);
    if(output) {
        const char *message = "\r\n[Build relocation] Reconfiguring at the current project location using saved settings. Cache-only options are not migrated.\r\n";
        output(context, message, strlen(message));
    }
    if(!stale) return editor_result_value(true); /* Retry an interrupted reconfiguration. */
    char directory[EDITOR_WORKSPACE_PATH_MAX * 2], archive[EDITOR_WORKSPACE_PATH_MAX * 2], marker[EDITOR_WORKSPACE_PATH_MAX * 2];
    SDL_snprintf(directory, sizeof(directory), "%s", absolute.config.build_directory);
    if(SDL_snprintf(archive, sizeof(archive), "%s/rohr_relocated_cache_%llu", directory,
            (unsigned long long)SDL_GetPerformanceCounter()) >= (int)sizeof(archive))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Relocated cache archive path is too long");
    if(!SDL_CreateDirectory(archive)) return editor_result_error(EDITOR_ERROR_FILE_IO,
        "Could not create cache archive %s: %s", archive, SDL_GetError());
    SDL_snprintf(marker, sizeof(marker), "%s/rohr_reconfigure_required.txt", directory);
    FILE *file = fopen(marker, "wb");
    if(!file) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not record relocation: %s", marker);
    bool written = fputs("Reconfigure before running this relocated project.\n", file) >= 0;
    bool closed = fclose(file) == 0;
    if(!written || !closed) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not write relocation marker: %s", marker);
    result = build_metadata_archive(directory, archive, "CMakeFiles");
    if(editor_result_check(result)) return result;
    const char *patterns[] = {"rohr_application_*.path", "rohr_scene_*.path", "rohr_target_*.txt"};
    for(size_t i = 0; i < SDL_arraysize(patterns); ++i) {
        int count = 0; char **files = SDL_GlobDirectory(directory, patterns[i], 0, &count);
        if(!files) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not list build metadata: %s", directory);
        for(int j = 0; j < count; ++j) {
            result = build_metadata_archive(directory, archive, files[j]);
            if(editor_result_check(result)) break;
        }
        SDL_free(files); if(editor_result_check(result)) return result;
    }
    /* Keep the stale cache until all other moves succeed so recovery is retryable. */
    result = build_metadata_archive(directory, archive, "CMakeCache.txt");
    if(!editor_result_check(result) && output) {
        output(context, archive, strlen(archive)); output(context, "\r\n", 2);
    }
    return result;
}
static bool build_default_cmake_compile_check(const EditorConfigCommand *compile) {
    if(!compile) return true; /* Default compile command. */
    if(compile->count < 3 || strcmp(compile->arguments[2], "{build}")) return false;
    const char *name = compile->arguments[0];
    for(const char *at = name; *at; ++at)
        if(*at == '/' || *at == '\\') name = at + 1;
    return (!SDL_strcasecmp(name, "cmake") || !SDL_strcasecmp(name, "cmake.exe")) &&
        !strcmp(compile->arguments[1], "--build");
}
EditorResult editor_build_prepare(const EditorWorkspace *workspace, bool configure_enabled,
        const EditorConfigCommand *compile, EditorBuildOutput output, void *context) {
    EditorWorkspace absolute;
    EditorResult result = editor_build_workspace_get(workspace, &absolute);
    if(editor_result_check(result)) return result;
    result = editor_workspace_build_paths_write(&absolute);
    if(editor_result_check(result)) return result;
    /* Check relocation/link safety before creating project scaffolding. */
    result = editor_build_cache_prepare(&absolute, configure_enabled, output, context);
    if(editor_result_check(result)) return result;
    bool root_restored = false, scene_restored = false;
    result = editor_workspace_cmake_restore(&absolute, &root_restored, &scene_restored);
    const char *names[] = {"CMakeLists.txt", "src/generated/project_scene.cmake"};
    bool restored[] = {root_restored, scene_restored};
    for(size_t i = 0; output && i < SDL_arraysize(names); ++i) {
        if(!restored[i]) continue;
        char message[EDITOR_WORKSPACE_PATH_MAX * 2];
        SDL_snprintf(message, sizeof(message), "\r\n[Build preparation] Restored %s/%s\r\n", absolute.directory, names[i]);
        output(context, message, strlen(message));
    }
    if(editor_result_check(result) || configure_enabled || !build_default_cmake_compile_check(compile)) return result;
    /* Custom non-CMake workflows do not require CMake's disposable metadata.
     * Default-directory CMake builds with configuration disabled need a prepared
     * tree. An explicit alternate build path is validated by its custom command. */
    const char *required[] = {"CMakeCache.txt", "CMakeFiles"};
    for(size_t i = 0; i < SDL_arraysize(required); ++i) {
        char path[EDITOR_WORKSPACE_PATH_MAX * 2]; SDL_PathInfo info;
        SDL_snprintf(path, sizeof(path), "%s/%s", absolute.config.build_directory, required[i]);
        SDL_PathType expected = i == 0 ? SDL_PATHTYPE_FILE : SDL_PATHTYPE_DIRECTORY;
        if(!SDL_GetPathInfo(path, &info) || info.type != expected)
            return editor_result_error(EDITOR_ERROR_FILE_IO,
                "Missing CMake build metadata %s; enable configuration before rebuilding", path);
    }
    return editor_result_value(true);
}
EditorResult editor_build_cache_configured(const EditorWorkspace *workspace) {
    EditorWorkspace absolute;
    EditorResult result = editor_build_workspace_get(workspace, &absolute);
    if(editor_result_check(result)) return result;
    bool stale, cached, pending;
    result = build_location_get(&absolute, &stale, &cached, &pending);
    if(editor_result_check(result) || !pending) return result;
    if(stale || !cached) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Configure did not recreate a valid CMake cache for relocated project %s", absolute.directory);
    char marker[EDITOR_WORKSPACE_PATH_MAX * 2];
    SDL_snprintf(marker, sizeof(marker), "%s/rohr_reconfigure_required.txt", absolute.config.build_directory);
    if(!SDL_RemovePath(marker)) return editor_result_error(EDITOR_ERROR_FILE_IO,
        "Could not finish relocation recovery: %s", marker);
    return editor_result_value(true);
}

EditorResult editor_build_target_name_get(const EditorWorkspace *workspace,
        EditorBuildTarget target, char *output, size_t capacity) {
    if(!workspace || !workspace->open || !target_valid_check(target))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Build target needs an open project");
    char path[EDITOR_WORKSPACE_PATH_MAX * 2], directory[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult resolved = editor_workspace_path_get(workspace, EDITOR_PATH_BUILD, directory, sizeof(directory));
    if(editor_result_check(resolved)) return resolved;
    if(SDL_snprintf(path, sizeof(path), "%s/rohr_target_%s.txt", directory,
            target_key(target)) >= (int)sizeof(path)) return editor_result_error(EDITOR_ERROR_CAPACITY, "Build metadata path is too long");
    char *name = metadata_read(path);
    if(name) { EditorResult result = path_copy(output, capacity, name); SDL_free(name); return result; }
    if(SDL_snprintf(path, sizeof(path), "%s%s", workspace->config.name,
            target == EDITOR_BUILD_APPLICATION ? "" : target == EDITOR_BUILD_SCENE ? "_scene" : "_project") >= (int)sizeof(path))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Build target is too long");
    return path_copy(output, capacity, path);
}
static void newest_executable(const char *candidate, char *chosen, size_t capacity, SDL_Time *newest) {
    SDL_PathInfo info;
    if(candidate && *candidate && strlen(candidate) < capacity && SDL_GetPathInfo(candidate, &info) &&
            info.type == SDL_PATHTYPE_FILE && info.size > 0 && (!chosen[0] || info.modify_time > *newest)) {
        SDL_strlcpy(chosen, candidate, capacity); *newest = info.modify_time;
    }
}
EditorResult editor_build_executable_get(const EditorWorkspace *workspace,
        EditorBuildTarget target, char *output, size_t capacity) {
    if(!workspace || !workspace->open || !output || !capacity ||
            (target != EDITOR_BUILD_APPLICATION && target != EDITOR_BUILD_SCENE))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Run needs an open project and application or scene target");
    EditorWorkspace absolute;
    EditorResult resolved = editor_build_workspace_get(workspace, &absolute);
    if(editor_result_check(resolved)) return resolved;
    workspace = &absolute;
    bool stale, cached, pending;
    resolved = build_location_get(workspace, &stale, &cached, &pending);
    if(editor_result_check(resolved)) return resolved;
    if(stale || pending) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Project moved; rebuild %s before running from %s", editor_build_target_label_get(target), workspace->directory);
    char directory[EDITOR_WORKSPACE_PATH_MAX * 2], pattern[80], path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char chosen[EDITOR_WORKSPACE_PATH_MAX * 2] = {0}; SDL_Time newest = 0;
    if(SDL_snprintf(directory, sizeof(directory), "%s", workspace->config.build_directory) >= (int)sizeof(directory))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Build directory is too long");
    SDL_snprintf(pattern, sizeof(pattern), "rohr_%s_*.path", target_key(target));
    int count = 0; char **files = SDL_GlobDirectory(directory, pattern, 0, &count);
    for(int i = 0; files && i < count; ++i) {
        if(SDL_snprintf(path, sizeof(path), "%s/%s", directory, files[i]) >= (int)sizeof(path)) continue;
        char *candidate = metadata_read(path);
        /* Without cache provenance, never follow an absolute record outside the
         * current project: it may belong to the still-existing original copy. */
        if(candidate && (cached || build_project_path_check(candidate, workspace->directory)))
            newest_executable(candidate, chosen, sizeof(chosen), &newest);
        SDL_free(candidate);
    }
    SDL_free(files);
    /* A project last configured before output metadata was introduced still runs. */
    if(count == 0) {
        static const char *configurations[] = {"", "Debug/", "Release/", "RelWithDebInfo/", "MinSizeRel/"};
        for(size_t i = 0; i < SDL_arraysize(configurations); ++i) {
            if(SDL_snprintf(path, sizeof(path), "%s/%s%s%s%s", directory, configurations[i], workspace->config.name,
                    target == EDITOR_BUILD_SCENE ? "_scene" : "",
#ifdef _WIN32
                    ".exe"
#else
                    ""
#endif
                    ) >= (int)sizeof(path)) continue;
            newest_executable(path, chosen, sizeof(chosen), &newest);
        }
    }
    const char *override = target == EDITOR_BUILD_SCENE ? workspace->config.scene_executable : workspace->config.application_executable;
    if(*override) {
        char expected[EDITOR_WORKSPACE_PATH_MAX * 2];
        resolved = editor_workspace_path_get(workspace, target == EDITOR_BUILD_SCENE ? EDITOR_PATH_SCENE : EDITOR_PATH_APPLICATION, expected, sizeof(expected));
        if(editor_result_check(resolved)) return resolved;
        if(!chosen[0] || !build_path_same_check(chosen, expected))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Rebuild %s at its configured output: %s", target_key(target), expected);
    }
    if(!chosen[0]) return editor_result_error(EDITOR_ERROR_FILE_IO,
        "No built %s executable was found. Use Build %s first.", target_key(target), editor_build_target_label_get(target));
    return path_copy(output, capacity, chosen);
}
EditorResult editor_build_compile_arguments_get(const EditorConfigCommand *command,
        const char *project, const char *directory, const char *sdk, const char *target,
        char arguments[EDITOR_CONFIG_ARGUMENT_MAX][EDITOR_CONFIG_ARGUMENT_LENGTH_MAX],
        const char *output[EDITOR_CONFIG_ARGUMENT_MAX + 1]) {
    EditorConfigCommand defaults = {.set = true, .count = 3};
    SDL_strlcpy(defaults.arguments[0], "cmake", sizeof(defaults.arguments[0]));
    SDL_strlcpy(defaults.arguments[1], "--build", sizeof(defaults.arguments[1]));
    SDL_strlcpy(defaults.arguments[2], "{build}", sizeof(defaults.arguments[2]));
    if(!command) command = &defaults;
    EditorResult result = editor_config_command_target_expand(command, project, directory, sdk, target, arguments, output);
    if(editor_result_check(result) || command->count == 0) return result;
    bool target_placeholder = false;
    for(size_t i = 0; i < command->count; ++i) target_placeholder |= strstr(command->arguments[i], "{target}") != NULL;
    if(target_placeholder) return editor_result_value(true);
    const char *program = strrchr(arguments[0], '/'); program = program ? program + 1 : arguments[0];
    const char *backslash = strrchr(program, '\\'); if(backslash) program = backslash + 1;
    if((SDL_strcasecmp(program, "cmake") && SDL_strcasecmp(program, "cmake.exe")) ||
            command->count < 3 || strcmp(arguments[1], "--build"))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "A targeted compile command must contain {target} to select Application, Scene, or Project");
    for(size_t i = 2; i < command->count; ++i)
        if(!strcmp(arguments[i], "-t") || !strcmp(arguments[i], "--target") || !strncmp(arguments[i], "--target=", 9) || !strcmp(arguments[i], "--"))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Use {target} in the configured compile target");
    if(command->count + 2 > EDITOR_CONFIG_ARGUMENT_MAX) return editor_result_error(EDITOR_ERROR_CAPACITY, "No room for build target arguments");
    SDL_strlcpy(arguments[command->count], "--target", sizeof(arguments[0]));
    result = path_copy(arguments[command->count + 1], sizeof(arguments[0]), target);
    if(editor_result_check(result)) return result;
    output[command->count] = arguments[command->count]; output[command->count + 1] = arguments[command->count + 1];
    output[command->count + 2] = NULL;
    return editor_result_value(true);
}
EditorBuild *editor_build_create(void) { return SDL_calloc(1, sizeof(EditorBuild)); }
static void stage_output(EditorBuild *build, const char *status) {
    if(!build->output) return;
    const char *stage = build->stage == EDITOR_BUILD_CONFIGURING ? "Configure" : "Compile";
    char message[160];
    SDL_snprintf(message, sizeof(message), "\r\n[%s] %s\r\n", stage, status);
    build->output(build->context, message, strlen(message));
}
static void command_display(EditorBuild *build) {
    if(!build->output) return;
    build->output(build->context, "\r\n$", 3);
    for(size_t i = 0; build->argv[i]; ++i) {
        build->output(build->context, " ", 1);
        /* Display argv only; this text is never submitted to a shell. */
        build->output(build->context, build->argv[i], strlen(build->argv[i]));
    }
    build->output(build->context, "\r\n", 2);
}
static EditorResult stage_start(EditorBuild *build, bool configure) {
    build->stage = configure ? EDITOR_BUILD_CONFIGURING : EDITOR_BUILD_COMPILING;
    stage_output(build, "START");
    if(configure) {
        EditorResult prepared = editor_build_prepare(&build->workspace,
            build->configure.count != 0, &build->compile, build->output, build->context);
        if(editor_result_check(prepared)) return prepared;
    }
    char directory[EDITOR_WORKSPACE_PATH_MAX * 2], target[EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
    SDL_snprintf(directory, sizeof(directory), "%s", build->workspace.config.build_directory);
    EditorResult result = editor_build_target_name_get(&build->workspace, build->target, target, sizeof(target));
    if(editor_result_check(result)) return result;
    if(configure) result = editor_config_command_target_expand(&build->configure,
        build->workspace.directory, directory, build->sdk, target, build->arguments, build->argv);
    else result = editor_build_compile_arguments_get(&build->compile,
        build->workspace.directory, directory, build->sdk, target, build->arguments, build->argv);
    if(editor_result_check(result)) return result;
    result = editor_config_workspace_arguments_expand(&build->workspace, build->arguments, build->argv);
    if(editor_result_check(result)) return result;
    if(!build->argv[0]) {
        stage_output(build, "SKIPPED (command disabled)");
        return editor_result_value(true);
    }
    if(!build->process && !(build->process = editor_build_process_create()))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not allocate build process");
    command_display(build);
    result = editor_build_process_start(build->process, build->workspace.directory, build->argv);
    if(!editor_result_check(result) && !configure) build->compiled = true;
    return result;
}
EditorResult editor_build_start(EditorBuild *build, const EditorWorkspace *workspace,
        const EditorConfig *config, const char *sdk, const char *configure_option,
        EditorBuildTarget target, EditorSceneSession *scene, EditorBuildOutput output, void *context) {
    if(!build || !workspace || !workspace->open || !config || !target_valid_check(target) || editor_build_active_check(build))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "A build is already active or no project is open");
    build->succeeded = false;
    EditorResult resolved = editor_build_workspace_get(workspace, &build->workspace);
    if(editor_result_check(resolved)) return resolved;
    build->target = target; build->scene = scene; build->compiled = false;
    build->output = output; build->context = context;
    EditorResult result = path_copy(build->sdk, sizeof(build->sdk), sdk ? sdk : "");
    if(editor_result_check(result)) return result;
    if(workspace->config.engine_directory[0]) {
        result = editor_workspace_path_get(workspace, EDITOR_PATH_ENGINE, build->sdk, sizeof(build->sdk));
        if(editor_result_check(result)) return result;
        if(SDL_snprintf(build->option, sizeof(build->option), "-DROHR_ENGINE_SOURCE_ROOT=%s", build->sdk) >= (int)sizeof(build->option))
            return editor_result_error(EDITOR_ERROR_CAPACITY, "Engine path is too long");
    } else result = path_copy(build->option, sizeof(build->option), configure_option ? configure_option : "");
    if(editor_result_check(result)) return result;
    const EditorConfigCommand *command = editor_config_command_get(config, EDITOR_CONFIG_FRONTEND_GUI, EDITOR_CONFIG_OPERATION_CONFIGURE);
    if(command) build->configure = *command;
    else {
        build->configure = (EditorConfigCommand){.set = true, .count = 6};
        const char *defaults[] = {"cmake", "-S", "{project}", "-B", "{build}", build->option};
        for(size_t i = 0; i < 6; ++i) SDL_strlcpy(build->configure.arguments[i], defaults[i], sizeof(build->configure.arguments[i]));
    }
    command = editor_config_command_get(config, EDITOR_CONFIG_FRONTEND_GUI, EDITOR_CONFIG_OPERATION_COMPILE);
    if(command) build->compile = *command;
    else {
        build->compile = (EditorConfigCommand){.set = true, .count = 3};
        SDL_strlcpy(build->compile.arguments[0], "cmake", sizeof(build->compile.arguments[0]));
        SDL_strlcpy(build->compile.arguments[1], "--build", sizeof(build->compile.arguments[1]));
        SDL_strlcpy(build->compile.arguments[2], "{build}", sizeof(build->compile.arguments[2]));
    }
    if(target != EDITOR_BUILD_APPLICATION && editor_scene_session_active_check(scene)) {
        editor_scene_session_stop(scene); build->stage = EDITOR_BUILD_STOPPING_SCENE;
        return editor_result_value(true);
    }
    result = stage_start(build, true);
    if(editor_result_check(result)) {
        stage_output(build, "FAIL (could not start command)");
        build->stage = EDITOR_BUILD_IDLE;
    }
    return result;
}
EditorResult editor_build_update(EditorBuild *build, bool *finished, int *exit_code) {
    if(!build || !finished || !exit_code) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid build update arguments");
    *finished = false; *exit_code = 0;
    if(build->stage == EDITOR_BUILD_IDLE) return editor_result_value(true);
    EditorResult result = editor_result_value(true);
    if(build->stage == EDITOR_BUILD_STOPPING_SCENE) {
        if(editor_scene_session_active_check(build->scene)) return result;
        result = stage_start(build, true);
    } else {
        bool done = true;
        if(editor_build_process_active_check(build->process)) result = editor_build_process_poll(build->process, build->output, build->context, &done, exit_code);
        if(!editor_result_check(result) && !done) return result;
        if(!editor_result_check(result) && build->argv[0]) {
            char status[80];
            SDL_snprintf(status, sizeof(status), "%s (exit %d)", *exit_code ? "FAIL" : "SUCCESS", *exit_code);
            stage_output(build, status);
        }
        if(!editor_result_check(result) && *exit_code == 0 && build->stage == EDITOR_BUILD_CONFIGURING) {
            result = editor_build_cache_configured(&build->workspace);
            if(!editor_result_check(result)) result = stage_start(build, false);
        } else {
            build->succeeded = !editor_result_check(result) && *exit_code == 0 && build->compiled;
            build->stage = EDITOR_BUILD_IDLE; *finished = true;
        }
    }
    if(editor_result_check(result)) {
        stage_output(build, "FAIL (command setup or polling error)");
        build->stage = EDITOR_BUILD_IDLE;
        editor_build_cancel(build); *finished = true; *exit_code = 1;
    }
    return result;
}
void editor_build_cancel(EditorBuild *build) {
    if(!build) return;
    if(editor_build_active_check(build) && build->output) {
        const char *message = "\r\n[Build] CANCELLED\r\n";
        build->output(build->context, message, strlen(message));
    }
    build->succeeded = false;
    editor_build_process_destroy(build->process); build->process = NULL; build->stage = EDITOR_BUILD_IDLE;
}
void editor_build_destroy(EditorBuild *build) { if(build) { editor_build_cancel(build); SDL_free(build); } }
bool editor_build_active_check(const EditorBuild *build) { return build && build->stage != EDITOR_BUILD_IDLE; }
EditorBuildStage editor_build_stage_get(const EditorBuild *build) { return build ? build->stage : EDITOR_BUILD_IDLE; }
bool editor_build_compiled_check(const EditorBuild *build) { return build && build->compiled; }
EditorResult editor_build_scene_prepared_get(const EditorBuild *build, char *output, size_t capacity) {
    if(output && capacity) output[0] = 0;
    if(!build || !build->succeeded || editor_build_active_check(build) ||
            build->target == EDITOR_BUILD_APPLICATION)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Scene preparation requires successful compilation");
    return editor_build_executable_get(&build->workspace, EDITOR_BUILD_SCENE, output, capacity);
}
EditorBuildTarget editor_build_target_get(const EditorBuild *build) { return build ? build->target : EDITOR_BUILD_PROJECT; }
