/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_LOG_H
#define ROHR_EDITOR_LOG_H

#define EDITOR_LOG_FILE_BYTES_MAX (1024 * 1024)
#define EDITOR_LOG_BACKUP_COUNT 3
#define EDITOR_LOG_REPEAT_INTERVAL_MS 1000

/* GUI-thread owned. Opening replaces the previous project sink. With no sink,
 * errors still go to stderr. No graphics resources or heap storage are owned. */
void editor_log_open(const char *project_directory);
void editor_log_close(void);
void editor_log_update(void);
void editor_log_error(const char *source, const char *format, ...);

#endif
