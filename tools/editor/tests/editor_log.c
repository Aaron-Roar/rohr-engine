/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_log.h"
#include "editor_error.h"
#include "editors/editor_mode_controls.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width = 1024.0f;
float editor_window_width = 1280.0f;
float editor_window_height = 720.0f;
float editor_viewport_bottom = 720.0f;

static char contents[EDITOR_LOG_FILE_BYTES_MAX + 1];

static const char *read_log(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    size_t size = fread(contents, 1, sizeof(contents) - 1, file);
    assert(!ferror(file));
    contents[size] = '\0';
    assert(fclose(file) == 0);
    return contents;
}

int main(void) {
    char root[128], other[256], path[512], other_path[512], backup[528];
    snprintf(root, sizeof(root), "editor_log_test_%llu",
        (unsigned long long)SDL_GetPerformanceCounter());
    snprintf(other, sizeof(other), "%s_other", root);
    snprintf(path, sizeof(path), "%s/logs/editor.log", root);
    snprintf(other_path, sizeof(other_path), "%s/logs/editor.log", other);
    assert(SDL_CreateDirectory(root));
    assert(SDL_CreateDirectory(other));
    editor_log_open(root);
    editor_result_stderr_print(editor_result_error(EDITOR_ERROR_FILE_IO,
        "save failed"));
    assert(strstr(read_log(path), "[ERROR] editor_result: editor error 2: save failed"));
    for(int i = 0; i < 10; i += 1) editor_log_error("test", "repeat");
    assert(strstr(read_log(path), "Previous error repeated") == NULL);
    SDL_Delay(EDITOR_LOG_REPEAT_INTERVAL_MS + 5);
    editor_log_update();
    assert(strstr(read_log(path), "repeated 9 additional times"));
    editor_log_error("test", "repeat");
    editor_log_open(other);
    assert(strstr(read_log(path), "repeated 1 additional times"));
    editor_log_error("test", "other project only");
    assert(strstr(read_log(other_path), "other project only"));
    assert(strstr(read_log(path), "other project only") == NULL);
    editor_log_close();
    editor_log_error("test", "no project only");
    assert(strstr(read_log(other_path), "no project only") == NULL);
    editor_log_open(root);
    editor_log_error("test", "appended session");
    assert(strstr(read_log(path), "save failed"));
    assert(strstr(contents, "appended session"));
    editor_log_close();

    /* Trigger real text-pool exhaustion through the editor reporting path. */
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    TextAsset texts[MAX_TEXT_ASSETS] = {0}, failed = {0};
    size_t count = 0;
    while(count < MAX_TEXT_ASSETS) {
        TextAssetResult result = rohr_graphics_text_create(&font, "", (Color){255,255,255,255});
        if(rohr_error_check(result)) break;
        texts[count++] = result.result.value;
    }
    editor_log_open(root);
    for(int i = 0; i < 100; i += 1)
        assert(!editor_mode_text_create(&font, "capacity", &failed));
    editor_log_error("test", "different error flushes repeats");
    assert(strstr(read_log(path), "text asset capacity exceeded"));
    assert(strstr(contents, "repeated 99 additional times"));
    assert(strstr(contents, "error 35:"));
    const char *first = strstr(contents, "text asset capacity exceeded");
    assert(strstr(first + 1, "text asset capacity exceeded") == NULL);
    for(size_t i = 0; i < count; i += 1) rohr_graphics_text_destroy(&texts[i]);
    assert(editor_mode_text_create(&font, "recovered", &failed));
    rohr_graphics_text_destroy(&failed);
    editor_log_close();
    rohr_graphics_stop();
    rohr_engine_stop();

    /* Fill the current file to its exact limit, then rotate on the next record. */
    for(int generation = 0; generation < 5; generation += 1) {
        FILE *file = fopen(path, "wb");
        assert(file != NULL);
        assert(fprintf(file, "generation %d\n", generation) > 0);
        assert(fseek(file, EDITOR_LOG_FILE_BYTES_MAX - 1, SEEK_SET) == 0);
        assert(fputc('\n', file) != EOF);
        assert(fclose(file) == 0);
        editor_log_open(root);
        editor_log_close();
    }
    for(int i = 1; i <= EDITOR_LOG_BACKUP_COUNT; i += 1) {
        snprintf(backup, sizeof(backup), "%s.%d", path, i);
        char expected[32];
        snprintf(expected, sizeof(expected), "generation %d", 5 - i);
        assert(strncmp(read_log(backup), expected, strlen(expected)) == 0);
        assert(remove(backup) == 0);
    }
    snprintf(backup, sizeof(backup), "%s.4", path);
    assert(fopen(backup, "rb") == NULL);

    /* A regular file in place of a directory is unwritable on both platforms,
     * including when tests run with privileges that bypass permission bits. */
    editor_log_open(path);
    editor_log_error("test", "logging failure does not abort");
    editor_log_error("test", "logging failure does not abort");
    editor_log_close();
    editor_log_open(other);
    editor_log_error("test", "logging recovered");
    assert(strstr(read_log(other_path), "logging recovered"));
    editor_log_close();
    assert(remove(path) == 0);
    assert(remove(other_path) == 0);
    snprintf(path, sizeof(path), "%s/logs", root);
    snprintf(other_path, sizeof(other_path), "%s/logs", other);
    assert(SDL_RemovePath(path));
    assert(SDL_RemovePath(other_path));
    assert(SDL_RemovePath(root));
    assert(SDL_RemovePath(other));
    return 0;
}
