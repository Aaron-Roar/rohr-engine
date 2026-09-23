/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <string.h>

static bool directory_equal(const char *a, const char *b) {
    size_t a_length;
    size_t b_length;
    if(a == NULL || b == NULL) return false;
    a_length = strlen(a);
    b_length = strlen(b);
    while(a_length > 0 && (a[a_length - 1] == '/' || a[a_length - 1] == '\\'))
        a_length -= 1;
    while(b_length > 0 && (b[b_length - 1] == '/' || b[b_length - 1] == '\\'))
        b_length -= 1;
    return a_length == b_length && SDL_strncasecmp(a, b, a_length) == 0;
}

int main(void) {
    const char *base = rohr_directory_base_get();
    char *original = SDL_GetCurrentDirectory();
    char *current;
    EngineResult result;

    if(base == NULL || original == NULL) {
        SDL_free(original);
        return 1;
    }
    result = rohr_directory_working_set(NULL);
    if(!rohr_error_check(result) ||
            result.result.error != ERROR_ENGINE_DIRECTORY_WORKING_SET_FAILED) {
        SDL_free(original);
        return 1;
    }
    result = rohr_directory_working_set(base);
    if(rohr_error_check(result)) {
        SDL_free(original);
        return 1;
    }
    current = SDL_GetCurrentDirectory();
    if(!directory_equal(base, current)) {
        (void)rohr_directory_working_set(original);
        SDL_free(current);
        SDL_free(original);
        return 1;
    }
    SDL_free(current);
    result = rohr_directory_working_set(original);
    SDL_free(original);
    return rohr_error_check(result) ? 1 : 0;
}
