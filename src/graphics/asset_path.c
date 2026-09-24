/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/asset_path.h"

#include <SDL3/SDL.h>

#include <string.h>

static bool graphics_asset_path_absolute_check(const char *path) {
    if(path == NULL || path[0] == '\0') return false;
    return path[0] == '/' || path[0] == '\\' ||
        (((path[0] >= 'A' && path[0] <= 'Z') ||
          (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':');
}

static bool graphics_asset_path_normalize(char *path) {
    size_t *segments;
    size_t length;
    size_t read = 0;
    size_t written = 0;
    size_t segment_count = 0;

    if(path == NULL) return false;
    length = strlen(path);
    segments = SDL_malloc((length + 1) * sizeof(*segments));
    if(segments == NULL) return false;
    for(size_t i = 0; i < length; i += 1)
        if(path[i] == '\\') path[i] = '/';
    if(length >= 2 && path[1] == ':') {
        path[written++] = path[read++];
        path[written++] = path[read++];
        if(path[read] == '/') path[written++] = path[read++];
    } else if(path[0] == '/') {
        path[written++] = '/';
        read += 1;
        if(path[read] == '/') {
            path[written++] = '/';
            read += 1;
        }
    }
    while(read < length) {
        size_t start;
        size_t amount;
        size_t rollback;
        while(read < length && path[read] == '/') read += 1;
        start = read;
        while(read < length && path[read] != '/') read += 1;
        amount = read - start;
        if(amount == 0 || (amount == 1 && path[start] == '.')) continue;
        if(amount == 2 && path[start] == '.' && path[start + 1] == '.') {
            if(segment_count > 0) written = segments[--segment_count];
            continue;
        }
        rollback = written;
        if(written > 0 && path[written - 1] != '/') path[written++] = '/';
        segments[segment_count++] = rollback;
        memmove(path + written, path + start, amount);
        written += amount;
    }
    if(written > 1 && path[written - 1] == '/') written -= 1;
    path[written] = '\0';
    SDL_free(segments);
    return true;
}

char *graphics_asset_path_resolve(const char *path) {
    char *directory = NULL;
    char *resolved = NULL;
    size_t length;

    if(path == NULL || path[0] == '\0') return NULL;
    if(graphics_asset_path_absolute_check(path)) {
        resolved = SDL_strdup(path);
    } else {
        directory = SDL_GetCurrentDirectory();
        if(directory == NULL) return NULL;
        if(SDL_asprintf(&resolved, "%s%s%s", directory,
                directory[0] != '\0' &&
                    directory[strlen(directory) - 1] != '/' &&
                    directory[strlen(directory) - 1] != '\\' ? "/" : "",
                path) < 0) resolved = NULL;
        SDL_free(directory);
    }
    if(resolved == NULL) return NULL;
    length = strlen(resolved);
    if(length == 0 || !graphics_asset_path_normalize(resolved)) {
        SDL_free(resolved);
        return NULL;
    }
    return resolved;
}
