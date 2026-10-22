/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#endif
#include "editor_workspace.h"
#include "editor_config.h"
#undef animated_sprites
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

static const size_t fields[EDITOR_PATH_COUNT] = {
    0, offsetof(EditorWorkspaceConfig, engine_directory),
    offsetof(EditorWorkspaceConfig, source_directory), offsetof(EditorWorkspaceConfig, asset_directory),
    offsetof(EditorWorkspaceConfig, generated_directory), offsetof(EditorWorkspaceConfig, object_directory),
    offsetof(EditorWorkspaceConfig, editor_state_file), offsetof(EditorWorkspaceConfig, build_directory),
    offsetof(EditorWorkspaceConfig, application_executable), offsetof(EditorWorkspaceConfig, scene_executable)
};
const char *editor_workspace_path_label_get(EditorWorkspacePath kind) {
    static const char *labels[] = {"Project directory", "Engine / SDK", "Source directory", "Asset directory",
        "Generated source", "Object directory", "Editor state file", "Build directory", "Application executable", "Scene executable"};
    return kind >= 0 && kind < EDITOR_PATH_COUNT ? labels[kind] : "";
}
char *editor_workspace_path_value_get(EditorWorkspace *workspace, EditorWorkspacePath kind) {
    if(!workspace || kind < 0 || kind >= EDITOR_PATH_COUNT) return NULL;
    return kind == EDITOR_PATH_PROJECT ? workspace->directory : (char *)&workspace->config + fields[kind];
}
static bool absolute_check(const char *path) {
    return path && (path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':'));
}
EditorResult editor_workspace_path_resolve(const char *root, const char *value, char *output, size_t capacity) {
    if(!root || !value || !output || !capacity) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid path argument");
    char joined[EDITOR_WORKSPACE_PATH_MAX * 4], normalized[sizeof(joined)];
    char *cwd = NULL;
    if(!absolute_check(root)) cwd = SDL_GetCurrentDirectory();
    int count = absolute_check(value) ? snprintf(joined, sizeof(joined), "%s", value) :
        snprintf(joined, sizeof(joined), "%s%s%s%s", cwd ? cwd : "", root, root[0] ? "/" : "", value);
    SDL_free(cwd);
    if(count < 0 || count >= (int)sizeof(joined)) return editor_result_error(EDITOR_ERROR_CAPACITY, "Path is too long");
    for(char *p = joined; *p; ++p) {
#ifdef _WIN32
        if(*p == '\\') *p = '/';
#endif
        if((unsigned char)*p < 32 || *p == ';' || *p == '"' || *p == '$' || *p == '\\')
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Unsupported character in path: %s", joined);
    }
    size_t n = 0, starts[EDITOR_WORKSPACE_PATH_MAX], depth = 0, at = 0;
    if(joined[0] && joined[1] == ':') { normalized[n++] = joined[0]; normalized[n++] = ':'; at = 2; }
    if(joined[at] == '/') { normalized[n++] = '/'; ++at;
        if(joined[at] == '/') { normalized[n++] = '/'; ++at; }
    }
    while(joined[at]) {
        while(joined[at] == '/') ++at;
        size_t begin = at; while(joined[at] && joined[at] != '/') ++at;
        size_t size = at - begin;
        if(!size || (size == 1 && joined[begin] == '.')) continue;
        if(size == 2 && !strncmp(joined + begin, "..", 2)) {
            if(depth) n = starts[--depth];
            continue;
        }
        if(depth >= SDL_arraysize(starts)) return editor_result_error(EDITOR_ERROR_CAPACITY, "Too many path components");
        starts[depth++] = n;
        if(n && normalized[n - 1] != '/') normalized[n++] = '/';
        memcpy(normalized + n, joined + begin, size); n += size;
    }
    normalized[n] = 0;
    if(n >= capacity) return editor_result_error(EDITOR_ERROR_CAPACITY, "Resolved path is too long: %s", normalized);
    memcpy(output, normalized, n + 1); return editor_result_value(true);
}
bool editor_workspace_path_contains_check(const char *root, const char *path) {
    size_t n = strlen(root);
    while(n > 1 && root[n - 1] == '/') --n;
    if(strlen(path) < n) return false;
#ifdef _WIN32
    return !SDL_strncasecmp(root, path, n) && (path[n] == 0 || path[n] == '/' || (n == 1 && root[0] == '/'));
#else
    return !strncmp(root, path, n) && (path[n] == 0 || path[n] == '/' || (n == 1 && root[0] == '/'));
#endif
}

static bool path_same(const char *a, const char *b) {
#ifdef _WIN32
    return !SDL_strcasecmp(a, b);
#else
    return !strcmp(a, b);
#endif
}
static bool path_exists(const char *path) {
#ifndef _WIN32
    struct stat s; return lstat(path, &s) == 0;
#else
    wchar_t *wide = (wchar_t *)SDL_iconv_string("UTF-16LE", "UTF-8", path, strlen(path) + 1);
    if(!wide) return false;
    DWORD flags = GetFileAttributesW(wide); SDL_free(wide);
    return flags != INVALID_FILE_ATTRIBUTES;
#endif
}
/* Relocation must not traverse links into another location. Links within a
 * copied POSIX tree remain links; their targets are never copied or removed. */
static bool linked_check(const char *path) {
#ifndef _WIN32
    struct stat info;
    return lstat(path, &info) == 0 && S_ISLNK(info.st_mode);
#else
    wchar_t *wide = (wchar_t *)SDL_iconv_string("UTF-16LE", "UTF-8", path, strlen(path) + 1);
    if(!wide) return true;
    DWORD info = GetFileAttributesW(wide); SDL_free(wide);
    return info != INVALID_FILE_ATTRIBUTES && (info & FILE_ATTRIBUTE_REPARSE_POINT);
#endif
}
static bool ancestors_unlinked_check(const char *path) {
    char part[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(SDL_strlcpy(part, path, sizeof(part)) >= sizeof(part)) return false;
    for(char *at = part + 1;; ++at) {
        if(*at && *at != '/') continue;
        char saved = *at; *at = 0;
        bool linked = linked_check(part); *at = saved;
        if(linked) return false;
        if(!saved) return true;
    }
}
static bool parent_create(const char *path) {
    char parent[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(SDL_strlcpy(parent, path, sizeof(parent)) >= sizeof(parent)) return false;
    char *slash = strrchr(parent, '/'); if(!slash) return false;
    if(slash == parent) return true;
    *slash = 0; return SDL_CreateDirectory(parent);
}
static bool tree_remove(const char *path);
static SDL_EnumerationResult SDLCALL remove_child(void *context, const char *directory, const char *name) {
    bool *ok = context; char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    *ok = snprintf(path, sizeof(path), "%s/%s", directory, name) < (int)sizeof(path) && tree_remove(path);
    return *ok ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}
static bool tree_remove(const char *path) {
    SDL_PathInfo info;
#ifdef _WIN32
    if(linked_check(path)) return SDL_RemovePath(path);
#else
    struct stat s;
    if(lstat(path, &s) != 0) return errno == ENOENT;
    if(S_ISLNK(s.st_mode)) return SDL_RemovePath(path);
#endif
    if(!SDL_GetPathInfo(path, &info)) return !path_exists(path);
    if(info.type == SDL_PATHTYPE_DIRECTORY) {
        bool ok = true;
        if(!SDL_EnumerateDirectory(path, remove_child, &ok) || !ok) return false;
    }
    return SDL_RemovePath(path);
}
typedef struct CopyTree { const char *target; bool ok; } CopyTree;
static bool tree_copy(const char *source, const char *target);
static SDL_EnumerationResult SDLCALL copy_child(void *context, const char *directory, const char *name) {
    CopyTree *copy = context;
    char source[EDITOR_WORKSPACE_PATH_MAX * 2], target[sizeof(source)];
    copy->ok = snprintf(source, sizeof(source), "%s/%s", directory, name) < (int)sizeof(source) &&
        snprintf(target, sizeof(target), "%s/%s", copy->target, name) < (int)sizeof(target) && tree_copy(source, target);
    return copy->ok ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}
static bool tree_copy(const char *source, const char *target) {
    if(path_exists(target) || !parent_create(target)) return false;
#ifndef _WIN32
    struct stat mode;
    if(lstat(source, &mode) != 0) return false;
    if(S_ISLNK(mode.st_mode)) {
        char value[EDITOR_WORKSPACE_PATH_MAX * 2];
        ssize_t n = readlink(source, value, sizeof(value) - 1);
        if(n < 0 || (size_t)n >= sizeof(value) - 1) return false;
        value[n] = 0; return symlink(value, target) == 0;
    }
#else
    wchar_t *wide = (wchar_t *)SDL_iconv_string("UTF-16LE", "UTF-8", source, strlen(source) + 1);
    if(!wide) return false;
    DWORD attributes = GetFileAttributesW(wide); SDL_free(wide);
    if(attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
    SDL_PathInfo info;
    if(!SDL_GetPathInfo(source, &info)) return false;
    if(info.type == SDL_PATHTYPE_DIRECTORY) {
        CopyTree copy = {target, true};
        return SDL_CreateDirectory(target) && SDL_EnumerateDirectory(source, copy_child, &copy) && copy.ok;
    }
    if(info.type != SDL_PATHTYPE_FILE) return false;
    FILE *in = fopen(source, "rb"), *out = in ? fopen(target, "wbx") : NULL;
    if(!in || !out) { if(in) fclose(in); return false; }
    unsigned char buffer[16384], verify[16384]; bool ok = true; size_t n;
    while((n = fread(buffer, 1, sizeof(buffer), in)) > 0)
        if(fwrite(buffer, 1, n, out) != n) { ok = false; break; }
    ok &= !ferror(in); ok &= fclose(out) == 0; rewind(in);
    FILE *check = ok ? fopen(target, "rb") : NULL;
    if(!check) ok = false;
    while(ok && (n = fread(buffer, 1, sizeof(buffer), in)) > 0)
        ok = fread(verify, 1, n, check) == n && !memcmp(buffer, verify, n);
    if(check) { ok &= !ferror(in) && fgetc(check) == EOF; ok &= fclose(check) == 0; }
    ok &= fclose(in) == 0;
#ifndef _WIN32
    if(ok) ok = chmod(target, mode.st_mode & 0777) == 0;
#endif
    return ok;
}

static bool settings_same(const EditorWorkspace *a, const EditorWorkspace *b) {
    if(!path_same(a->directory, b->directory)) return false;
    for(int k = EDITOR_PATH_ENGINE; k < EDITOR_PATH_COUNT; ++k)
        if(strcmp(editor_workspace_path_value_get((EditorWorkspace *)a, k),
                editor_workspace_path_value_get((EditorWorkspace *)b, k))) return false;
    return true;
}
EditorResult editor_workspace_paths_validate(const EditorWorkspace *workspace, EditorWorkspace *desired) {
    if(!workspace || !workspace->open || !desired) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "No project is open");
    EditorWorkspace draft = *desired; desired->open = true;
    char old_root[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult r = editor_workspace_path_get(workspace, EDITOR_PATH_PROJECT, old_root, sizeof(old_root));
    if(editor_result_check(r)) return r;
    r = editor_workspace_path_resolve(old_root, !strcmp(draft.directory, workspace->directory) ? old_root : draft.directory, desired->directory, sizeof(desired->directory));
    if(editor_result_check(r)) return r;
    bool moved = !path_same(old_root, desired->directory);
    if(moved && (editor_workspace_path_contains_check(old_root, desired->directory) ||
            editor_workspace_path_contains_check(desired->directory, old_root) || path_exists(desired->directory)))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Project destination must be new and outside the current project: %s", desired->directory);
    if(moved && (!ancestors_unlinked_check(old_root) || !ancestors_unlinked_check(desired->directory)))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Project relocation cannot traverse symbolic links; use physical paths");
    EditorWorkspaceConfig defaults = editor_workspace_config_default_get();
    for(int k = EDITOR_PATH_ENGINE; k < EDITOR_PATH_COUNT; ++k) {
        char *value = editor_workspace_path_value_get(desired, k);
        const char *old_value = editor_workspace_path_value_get((EditorWorkspace *)workspace, k);
        if(!*value) {
            if(k >= EDITOR_PATH_SOURCE && k <= EDITOR_PATH_STATE)
                SDL_strlcpy(value, (const char *)&defaults + fields[k], EDITOR_WORKSPACE_PATH_MAX);
            else continue;
        }
        char resolved[EDITOR_WORKSPACE_PATH_MAX * 2];
        /* Unchanged absolute internal paths follow the project; external paths never do. */
        if(moved && !strcmp(old_value, value)) {
            r = editor_workspace_path_get(workspace, k, resolved, sizeof(resolved));
            if(editor_result_check(r)) return r;
            if(editor_workspace_path_contains_check(old_root, resolved)) {
                char suffix[sizeof(resolved)]; SDL_strlcpy(suffix, resolved + strlen(old_root), sizeof(suffix));
                r = editor_workspace_path_resolve(desired->directory, *suffix == '/' ? suffix + 1 : suffix, resolved, sizeof(resolved));
            }
        } else r = editor_workspace_path_resolve(desired->directory, value, resolved, sizeof(resolved));
        if(editor_result_check(r)) return r;
        const char *stored = resolved;
        if(editor_workspace_path_contains_check(desired->directory, resolved)) stored += strlen(desired->directory) + (resolved[strlen(desired->directory)] == '/');
        if(!*stored || strlen(stored) >= EDITOR_WORKSPACE_PATH_MAX)
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid %s: %s", editor_workspace_path_label_get(k), resolved);
        SDL_strlcpy(value, stored, EDITOR_WORKSPACE_PATH_MAX);
    }
    /* Unedited children follow their content directory, regardless of whether
     * the original manifest used relative or absolute internal paths. */
    for(int parent = EDITOR_PATH_SOURCE; parent <= EDITOR_PATH_OBJECTS; ++parent) {
        char old_parent[EDITOR_WORKSPACE_PATH_MAX * 2], new_parent[sizeof(old_parent)];
        r = editor_workspace_path_get(workspace, parent, old_parent, sizeof(old_parent)); if(editor_result_check(r)) return r;
        r = editor_workspace_path_get(desired, parent, new_parent, sizeof(new_parent)); if(editor_result_check(r)) return r;
        if(path_same(old_parent, new_parent)) continue;
        for(int child = parent + 1; child <= EDITOR_PATH_STATE; ++child) {
            if(strcmp(editor_workspace_path_value_get((EditorWorkspace *)workspace, child),
                    editor_workspace_path_value_get(&draft, child))) continue;
            char old_child[sizeof(old_parent)], combined[sizeof(old_parent)];
            r = editor_workspace_path_get(workspace, child, old_child, sizeof(old_child)); if(editor_result_check(r)) return r;
            if(!editor_workspace_path_contains_check(old_parent, old_child) || path_same(old_parent, old_child)) continue;
            if(SDL_snprintf(combined, sizeof(combined), "%s%s", new_parent, old_child + strlen(old_parent)) >= (int)sizeof(combined))
                return editor_result_error(EDITOR_ERROR_CAPACITY, "Nested path is too long");
            const char *stored = combined;
            if(editor_workspace_path_contains_check(desired->directory, combined)) stored += strlen(desired->directory) + 1;
            if(SDL_strlcpy(editor_workspace_path_value_get(desired, child), stored, EDITOR_WORKSPACE_PATH_MAX) >= EDITOR_WORKSPACE_PATH_MAX)
                return editor_result_error(EDITOR_ERROR_CAPACITY, "Nested path is too long");
        }
    }
    for(int k = EDITOR_PATH_SOURCE; k <= EDITOR_PATH_STATE; ++k) {
        char from[EDITOR_WORKSPACE_PATH_MAX * 2], to[sizeof(from)], equivalent[sizeof(from)];
        r = editor_workspace_path_get(workspace, k, from, sizeof(from)); if(editor_result_check(r)) return r;
        r = editor_workspace_path_get(desired, k, to, sizeof(to)); if(editor_result_check(r)) return r;
        SDL_strlcpy(equivalent, from, sizeof(equivalent));
        if(moved && editor_workspace_path_contains_check(old_root, from))
            snprintf(equivalent, sizeof(equivalent), "%s%s", desired->directory, from + strlen(old_root));
        if(path_same(equivalent, to)) continue;
        if(!ancestors_unlinked_check(from) || !ancestors_unlinked_check(to))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Relocation cannot traverse symbolic links: %s -> %s", from, to);
        if(path_exists(to) || editor_workspace_path_contains_check(from, to) || editor_workspace_path_contains_check(to, from))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Destination is occupied or overlaps its source: %s", to);
    }
    char locations[EDITOR_PATH_COUNT][EDITOR_WORKSPACE_PATH_MAX * 2];
    for(int k = EDITOR_PATH_SOURCE; k < EDITOR_PATH_COUNT; ++k) {
        r = editor_workspace_path_get(desired, k, locations[k], sizeof(locations[k]));
        if(editor_result_check(r)) return r;
        SDL_PathInfo info;
        if(SDL_GetPathInfo(locations[k], &info) &&
                info.type != (k == EDITOR_PATH_STATE || k >= EDITOR_PATH_APPLICATION ? SDL_PATHTYPE_FILE : SDL_PATHTYPE_DIRECTORY))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Wrong file type for %s: %s", editor_workspace_path_label_get(k), locations[k]);
        for(int other = EDITOR_PATH_SOURCE; other < k; ++other) {
            bool nested = editor_workspace_path_contains_check(locations[other], locations[k]);
            bool reverse = editor_workspace_path_contains_check(locations[k], locations[other]);
            /* Only source/generated and objects/state have nested content ownership.
             * Build outputs may live in the build directory. */
            bool allowed = nested && !path_same(locations[other], locations[k]) &&
                ((other == EDITOR_PATH_SOURCE && k == EDITOR_PATH_GENERATED) ||
                 (other == EDITOR_PATH_OBJECTS && k == EDITOR_PATH_STATE) ||
                 (other == EDITOR_PATH_BUILD && k >= EDITOR_PATH_APPLICATION));
            if((nested || reverse) && !allowed)
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "%s overlaps %s", editor_workspace_path_label_get(k), editor_workspace_path_label_get(other));
        }
        if(path_same(desired->directory, locations[k]))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "%s cannot replace the project root", editor_workspace_path_label_get(k));
    }
    if(desired->config.engine_directory[0]) {
        char engine[EDITOR_WORKSPACE_PATH_MAX * 2]; SDL_PathInfo info;
        r = editor_workspace_path_get(desired, EDITOR_PATH_ENGINE, engine, sizeof(engine)); if(editor_result_check(r)) return r;
        if(!SDL_GetPathInfo(engine, &info) || info.type != SDL_PATHTYPE_DIRECTORY)
            return editor_result_error(EDITOR_ERROR_FILE_IO, "Engine/SDK directory does not exist: %s", engine);
    }
    return settings_same(workspace, desired) ? editor_result_value(true) : editor_workspace_scaffold_paths_check(workspace);
}
EditorResult editor_workspace_path_get(const EditorWorkspace *workspace, EditorWorkspacePath kind, char *output, size_t capacity) {
    if(!workspace || kind < 0 || kind >= EDITOR_PATH_COUNT) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid workspace path");
    if(kind == EDITOR_PATH_PROJECT) return editor_workspace_path_resolve("", workspace->directory, output, capacity);
    const char *value = (const char *)&workspace->config + fields[kind];
    EditorWorkspaceConfig defaults;
    if(!*value && kind >= EDITOR_PATH_SOURCE && kind <= EDITOR_PATH_STATE) {
        defaults = editor_workspace_config_default_get(); value = (const char *)&defaults + fields[kind];
    }
    if(kind == EDITOR_PATH_ENGINE && !*value) return editor_config_sdk_root_get(output, capacity);
    if(kind == EDITOR_PATH_BUILD && !*value) value = "build";
    if((kind == EDITOR_PATH_APPLICATION || kind == EDITOR_PATH_SCENE) && !*value) {
        char build[EDITOR_WORKSPACE_PATH_MAX * 2], name[EDITOR_WORKSPACE_NAME_MAX + 16];
        EditorResult r = editor_workspace_path_get(workspace, EDITOR_PATH_BUILD, build, sizeof(build));
        if(editor_result_check(r)) return r;
        snprintf(name, sizeof(name), "%s%s%s", workspace->config.name, kind == EDITOR_PATH_SCENE ? "_scene" : "",
#ifdef _WIN32
            ".exe"
#else
            ""
#endif
        );
        return editor_workspace_path_resolve(build, name, output, capacity);
    }
    return editor_workspace_path_resolve(workspace->directory, value, output, capacity);
}
EditorResult editor_workspace_asset_path_get(const EditorWorkspace *workspace, const char *asset, char *output, size_t capacity) {
    if(!workspace || !asset) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Missing asset path");
    if(!strncmp(asset, "assets/", 7) || !strncmp(asset, "assets\\", 7)) {
        char root[EDITOR_WORKSPACE_PATH_MAX * 2];
        EditorResult r = editor_workspace_path_get(workspace, EDITOR_PATH_ASSETS, root, sizeof(root));
        if(editor_result_check(r)) return r;
        return editor_workspace_path_resolve(root, asset + 7, output, capacity);
    }
    return editor_workspace_path_resolve(workspace->directory, asset, output, capacity);
}

typedef struct PathMove {
    char source[EDITOR_WORKSPACE_PATH_MAX * 2], target[EDITOR_WORKSPACE_PATH_MAX * 2];
    char recovery[EDITOR_WORKSPACE_PATH_MAX * 2];
    bool retained, created;
} PathMove;
typedef struct PathSnapshot {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char recovery[EDITOR_WORKSPACE_PATH_MAX * 2 + 48];
    void *bytes;
    size_t size;
    bool existed;
} PathSnapshot;
static bool asset_name_update(char *value, size_t capacity, const EditorWorkspace *workspace,
        const EditorWorkspace *desired) {
    if(!*value || !strncmp(value, "assets/", 7)) return true;
    char path[EDITOR_WORKSPACE_PATH_MAX * 2], assets[sizeof(path)], root[sizeof(path)], result[sizeof(path)];
    if(editor_result_check(editor_workspace_path_resolve(workspace->directory, value, path, sizeof(path))) ||
            editor_result_check(editor_workspace_path_get(workspace, EDITOR_PATH_ASSETS, assets, sizeof(assets))) ||
            editor_result_check(editor_workspace_path_get(workspace, EDITOR_PATH_PROJECT, root, sizeof(root)))) return false;
    if(editor_workspace_path_contains_check(assets, path))
        snprintf(result, sizeof(result), "assets%s", path + strlen(assets));
    else if(absolute_check(value) && editor_workspace_path_contains_check(root, path))
        snprintf(result, sizeof(result), "%s%s", desired->directory, path + strlen(root));
    else SDL_strlcpy(result, value, sizeof(result));
    return SDL_strlcpy(value, result, capacity) < capacity;
}
static bool asset_names_update(EditorProject *project, const EditorWorkspace *workspace, const EditorWorkspace *desired) {
    for(size_t i = 0; i < project->ui_font_count; ++i)
        if(!asset_name_update(project->ui_fonts[i].path, sizeof(project->ui_fonts[i].path), workspace, desired)) return false;
    for(size_t i = 0; i < project->audio_definition_count; ++i)
        if(!asset_name_update(project->audio_definitions[i].path, sizeof(project->audio_definitions[i].path), workspace, desired)) return false;
    for(size_t o = 0; o < project->object_count; ++o) {
        EditorObject *object = &project->objects[o];
        for(size_t i = 0; i < object->sprite_count; ++i)
            if(!asset_name_update(object->sprites[i].path, sizeof(object->sprites[i].path), workspace, desired)) return false;
        for(size_t i = 0; i < object->animated_sprite_count; ++i)
            for(size_t f = 0; f < object->animated_sprite_items[i].frame_count; ++f)
                if(!asset_name_update(object->animated_sprite_items[i].frames[f].path,
                        sizeof(object->animated_sprite_items[i].frames[f].path), workspace, desired)) return false;
    }
    return true;
}
static bool snapshot_capture(PathSnapshot *snapshot, const char *path) {
    if(SDL_strlcpy(snapshot->path, path, sizeof(snapshot->path)) >= sizeof(snapshot->path)) return false;
    snapshot->existed = path_exists(path);
    if(!snapshot->existed) return true;
    if(!ancestors_unlinked_check(path)) return false;
    snapshot->bytes = SDL_LoadFile(path, &snapshot->size);
    if(!snapshot->bytes) return false;
    SDL_snprintf(snapshot->recovery, sizeof(snapshot->recovery), "%s.rohr_save_%llu", path,
        (unsigned long long)SDL_GetPerformanceCounter());
    FILE *file = fopen(snapshot->recovery, "wbx");
    bool ok = file != NULL;
    if(file) {
        ok = fwrite(snapshot->bytes, 1, snapshot->size, file) == snapshot->size;
        ok &= fclose(file) == 0;
        if(!ok) (void)SDL_RemovePath(snapshot->recovery);
    }
    SDL_free(snapshot->bytes); snapshot->bytes = NULL;
    return ok;
}
static bool snapshot_restore(PathSnapshot *snapshot) {
    if(!snapshot->existed) return !path_exists(snapshot->path) || SDL_RemovePath(snapshot->path);
    /* Rename restores even when a failed metadata write exhausted disk space. */
    return SDL_RenamePath(snapshot->recovery, snapshot->path);
}
static EditorResult move_prepare(PathMove *move, bool directory) {
    if(path_exists(move->target)) return editor_result_error(EDITOR_ERROR_FILE_IO, "Destination became occupied: %s", move->target);
    if(!parent_create(move->target)) return editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot create destination parent: %s", move->target);
    if(!path_exists(move->source)) {
        if(directory) move->created = SDL_CreateDirectory(move->target);
        else { FILE *f = fopen(move->target, "wbx"); move->created = f && fclose(f) == 0; }
        return move->created ? editor_result_value(true) : editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot create %s", move->target);
    }
    if(snprintf(move->recovery, sizeof(move->recovery), "%s.rohr_move_%llu", move->source,
            (unsigned long long)SDL_GetPerformanceCounter()) >= (int)sizeof(move->recovery) || path_exists(move->recovery))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Recovery path unavailable: %s", move->source);
    if(!SDL_RenamePath(move->source, move->recovery))
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot retain %s: %s", move->source, SDL_GetError());
    move->retained = true;
    move->created = true; /* A failed copy may have partially created its destination. */
    if(!tree_copy(move->recovery, move->target))
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Copy/verification failed: %s -> %s; recovery: %s", move->source, move->target, move->recovery);
    return editor_result_value(true);
}
EditorResult editor_workspace_paths_apply(EditorWorkspace *workspace, EditorProject *project,
        const EditorWorkspace *requested) {
    if(!project || !requested) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Missing project or paths");
    EditorWorkspace desired = *requested;
    EditorResult result = editor_workspace_paths_validate(workspace, &desired);
    if(editor_result_check(result)) return result;
    if(settings_same(workspace, &desired)) return editor_workspace_save(workspace, project) ? editor_result_value(true) :
        editor_result_error(EDITOR_ERROR_FILE_IO, "Could not save project settings");
    EditorProject *updated = calloc(1, sizeof(*updated));
    if(!updated || !editor_project_clone(updated, project) || !asset_names_update(updated, workspace, &desired)) {
        if(updated) editor_project_destroy(updated);
        free(updated); return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not prepare relocated project state");
    }
    PathMove moves[EDITOR_PATH_COUNT] = {0}; size_t count = 0;
    PathSnapshot snapshots[5] = {0}; size_t saved = 0;
    char old_root[EDITOR_WORKSPACE_PATH_MAX * 2];
    result = editor_workspace_path_get(workspace, EDITOR_PATH_PROJECT, old_root, sizeof(old_root));
    if(editor_result_check(result)) { editor_project_destroy(updated); free(updated); return result; }
    const EditorWorkspacePath order[] = {EDITOR_PATH_PROJECT, EDITOR_PATH_SOURCE, EDITOR_PATH_ASSETS,
        EDITOR_PATH_GENERATED, EDITOR_PATH_OBJECTS, EDITOR_PATH_STATE};
    for(size_t step = 0; step < SDL_arraysize(order); ++step) {
        char source[EDITOR_WORKSPACE_PATH_MAX * 2], target[sizeof(source)];
        result = editor_workspace_path_get(workspace, order[step], source, sizeof(source)); if(editor_result_check(result)) goto rollback;
        result = editor_workspace_path_get(&desired, order[step], target, sizeof(target)); if(editor_result_check(result)) goto rollback;
        for(size_t previous = 0; previous < count; ++previous) {
            if(editor_workspace_path_contains_check(moves[previous].source, source)) {
                char mapped[sizeof(source)];
                if(snprintf(mapped, sizeof(mapped), "%s%s", moves[previous].target,
                        source + strlen(moves[previous].source)) >= (int)sizeof(mapped)) {
                    result = editor_result_error(EDITOR_ERROR_CAPACITY, "Mapped path is too long"); goto rollback;
                }
                SDL_strlcpy(source, mapped, sizeof(source));
            }
        }
        if(path_same(source, target)) continue;
        PathMove *move = &moves[count++];
        SDL_strlcpy(move->source, source, sizeof(move->source)); SDL_strlcpy(move->target, target, sizeof(move->target));
        result = move_prepare(move, order[step] != EDITOR_PATH_STATE);
        if(editor_result_check(result)) goto rollback;
    }
    /* Snapshots protect files updated in place when their parent did not move. */
    const char *names[] = {"project.rohr.json", "CMakeLists.txt", ".rohr/project_paths.cmake"};
    for(size_t i = 0; i < SDL_arraysize(names); ++i) {
        char path[EDITOR_WORKSPACE_PATH_MAX * 2];
        result = editor_workspace_path_resolve(desired.directory, names[i], path, sizeof(path));
        if(editor_result_check(result) || !snapshot_capture(&snapshots[saved], path)) goto capture_failed;
        ++saved;
    }
    char state[EDITOR_WORKSPACE_PATH_MAX * 2], generated[sizeof(state)], scene[sizeof(state)];
    result = editor_workspace_path_get(&desired, EDITOR_PATH_STATE, state, sizeof(state)); if(editor_result_check(result)) goto rollback;
    if(!snapshot_capture(&snapshots[saved], state)) goto capture_failed;
    ++saved;
    result = editor_workspace_path_get(&desired, EDITOR_PATH_GENERATED, generated, sizeof(generated)); if(editor_result_check(result)) goto rollback;
    result = editor_workspace_path_resolve(generated, "project_scene.cmake", scene, sizeof(scene)); if(editor_result_check(result)) goto rollback;
    if(!snapshot_capture(&snapshots[saved], scene)) goto capture_failed;
    ++saved;
    if(!parent_create(state) || !SDL_CreateDirectory(generated) || !editor_workspace_save(&desired, updated)) {
        result = editor_result_error(EDITOR_ERROR_FILE_IO, "Could not save relocated project; restoring previous paths"); goto rollback;
    }
    result = editor_workspace_scaffold_paths_write(&desired); if(editor_result_check(result)) goto rollback;
    *workspace = desired;
    editor_project_destroy(project); *project = *updated; free(updated);
    bool clean = true;
    for(size_t i = 0; i < saved; ++i) if(snapshots[i].existed && !SDL_RemovePath(snapshots[i].recovery)) {
        clean = false; SDL_Log("Paths applied; metadata recovery copy retained at %s", snapshots[i].recovery);
    }
    /* Remove nested recovery copies before removing the retained original root. */
    for(size_t i = count; i > 0; --i) if(moves[i - 1].retained && !tree_remove(moves[i - 1].recovery)) {
        clean = false; SDL_Log("Paths applied; recovery copy retained at %s", moves[i - 1].recovery);
    }
    return editor_result_value(clean);
capture_failed:
    result = editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot snapshot project metadata before applying paths");
rollback: {
    editor_project_destroy(updated); free(updated);
    bool restored = true;
    for(size_t i = saved; i > 0; --i) { restored &= snapshot_restore(&snapshots[i - 1]); SDL_free(snapshots[i - 1].bytes); }
    for(size_t i = count; i > 0; --i) {
        PathMove *move = &moves[i - 1];
        bool removed = !move->created || tree_remove(move->target);
        restored &= removed;
        if(move->retained && (!removed || !SDL_RenamePath(move->recovery, move->source))) {
            return editor_result_error(EDITOR_ERROR_FILE_IO,
                "Move failed and automatic rollback needs recovery. Original files retained at %s; destination %s. %s",
                move->recovery, move->target, result.result.error.message);
        }
    }
    if(!restored) return editor_result_error(EDITOR_ERROR_FILE_IO, "Move failed; metadata restoration needs review; .rohr_save_ recovery files remain under the configured paths for %s. %s", old_root, result.result.error.message);
    return result;
}
}
