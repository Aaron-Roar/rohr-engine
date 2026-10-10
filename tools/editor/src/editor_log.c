/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_log.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define EDITOR_LOG_PATH_MAX 4096
#define EDITOR_LOG_MESSAGE_MAX 4096
#define EDITOR_LOG_SOURCE_MAX 256

static FILE *editor_log_file;
static char editor_log_path[EDITOR_LOG_PATH_MAX];
static char editor_log_last_message[EDITOR_LOG_MESSAGE_MAX];
static char editor_log_last_source[EDITOR_LOG_SOURCE_MAX];
static unsigned long long editor_log_repeats;
static Uint64 editor_log_repeat_time;
static bool editor_log_failed;

static void editor_log_failure(void) {
    if(editor_log_file != NULL) {
        fclose(editor_log_file);
        editor_log_file = NULL;
    }
    if(!editor_log_failed)
        fprintf(stderr, "Editor log unavailable: %s\n", editor_log_path);
    editor_log_failed = true;
}

static bool editor_log_rotate(void) {
    char from[EDITOR_LOG_PATH_MAX + 16];
    char to[EDITOR_LOG_PATH_MAX + 16];
    FILE *file = editor_log_file;
    editor_log_file = NULL;
    if(fclose(file) != 0) return false;
    snprintf(to, sizeof(to), "%s.%d", editor_log_path, EDITOR_LOG_BACKUP_COUNT);
    if(remove(to) != 0 && errno != ENOENT) return false;
    for(int i = EDITOR_LOG_BACKUP_COUNT; i > 0; i -= 1) {
        if(i == 1) snprintf(from, sizeof(from), "%s", editor_log_path);
        else snprintf(from, sizeof(from), "%s.%d", editor_log_path, i - 1);
        snprintf(to, sizeof(to), "%s.%d", editor_log_path, i);
        if(rename(from, to) != 0 && errno != ENOENT) return false;
    }
    editor_log_file = fopen(editor_log_path, "ab");
    return editor_log_file != NULL;
}

static void editor_log_write(const char *severity, const char *source,
        const char *message) {
    char timestamp[32] = "time unavailable";
    char record[EDITOR_LOG_MESSAGE_MAX + EDITOR_LOG_SOURCE_MAX + 80];
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    if(editor_log_file == NULL) return;
    if(utc != NULL) strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc);
    int length = snprintf(record, sizeof(record), "%s [%s] %s: %s\n",
        timestamp, severity, source, message);
    long size = ftell(editor_log_file);
    if(length < 0 || (size_t)length >= sizeof(record) || size < 0) {
        editor_log_failure();
        return;
    }
    if(size + length > EDITOR_LOG_FILE_BYTES_MAX && !editor_log_rotate()) {
        editor_log_failure();
        return;
    }
    if(fwrite(record, 1, (size_t)length, editor_log_file) != (size_t)length ||
            fflush(editor_log_file) != 0) editor_log_failure();
}

static void editor_log_repeats_flush(void) {
    char message[EDITOR_LOG_MESSAGE_MAX];
    if(editor_log_repeats == 0) return;
    snprintf(message, sizeof(message), "Previous error repeated %llu additional times",
        editor_log_repeats);
    fprintf(stderr, "%s: %s\n", editor_log_last_source, message);
    editor_log_write("ERROR", editor_log_last_source, message);
    editor_log_repeats = 0;
    editor_log_repeat_time = SDL_GetTicks();
}

void editor_log_update(void) {
    if(SDL_GetTicks() - editor_log_repeat_time >= EDITOR_LOG_REPEAT_INTERVAL_MS)
        editor_log_repeats_flush();
}

void editor_log_error(const char *source, const char *format, ...) {
    char message[EDITOR_LOG_MESSAGE_MAX];
    char origin[EDITOR_LOG_SOURCE_MAX];
    va_list arguments;
    if(format == NULL) return;
    snprintf(origin, sizeof(origin), "%s", source == NULL ? "editor" : source);
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    size_t length = strlen(message);
    while(length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r'))
        message[--length] = '\0';
    if(strcmp(origin, editor_log_last_source) == 0 &&
            strcmp(message, editor_log_last_message) == 0) {
        editor_log_repeats += 1;
        editor_log_update();
        return;
    }
    editor_log_repeats_flush();
    snprintf(editor_log_last_source, sizeof(editor_log_last_source), "%s", origin);
    snprintf(editor_log_last_message, sizeof(editor_log_last_message), "%s", message);
    editor_log_repeat_time = SDL_GetTicks();
    fprintf(stderr, "%s\n", message);
    editor_log_write("ERROR", origin, message);
}

void editor_log_close(void) {
    editor_log_repeats_flush();
    editor_log_write("INFO", "session", "Project editor log closed");
    if(editor_log_file != NULL) {
        FILE *file = editor_log_file;
        editor_log_file = NULL;
        if(fclose(file) != 0) editor_log_failure();
    }
    editor_log_last_message[0] = '\0';
    editor_log_last_source[0] = '\0';
}

void editor_log_open(const char *project_directory) {
    char directory[EDITOR_LOG_PATH_MAX];
    editor_log_close();
    editor_log_failed = false;
    editor_log_path[0] = '\0';
    if(project_directory == NULL || project_directory[0] == '\0') return;
    int length = snprintf(editor_log_path, sizeof(editor_log_path),
        "%s/logs/editor.log", project_directory);
    if(length < 0 || (size_t)length >= sizeof(editor_log_path)) {
        editor_log_failure();
        return;
    }
    snprintf(directory, sizeof(directory), "%s/logs", project_directory);
    if(!SDL_CreateDirectory(directory)) {
        editor_log_failure();
        return;
    }
    editor_log_file = fopen(editor_log_path, "ab");
    if(editor_log_file == NULL || fseek(editor_log_file, 0, SEEK_END) != 0) {
        editor_log_failure();
        return;
    }
    editor_log_write("INFO", "session", "Project editor log opened");
}
