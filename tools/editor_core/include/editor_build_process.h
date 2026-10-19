/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_BUILD_PROCESS_H
#define ROHR_EDITOR_BUILD_PROCESS_H
#include "editor_error.h"
#include <stdbool.h>
#include <stddef.h>
/* Explicitly allocated process owner. Builds use a private process group/job so
 * cancelling CMake also cancels compiler descendants. Arguments never use a shell. */
typedef struct EditorBuildProcess EditorBuildProcess;
typedef void (*EditorBuildOutput)(void *context, const char *bytes, size_t count);
EditorBuildProcess *editor_build_process_create(void);
EditorResult editor_build_process_start(EditorBuildProcess *process, const char *directory,
    const char *const *arguments);
/* At most 64 KiB of output per poll. Completion includes draining output and
 * reaping the process. The callback borrows bytes for this invocation only. */
EditorResult editor_build_process_poll(EditorBuildProcess *process,
    EditorBuildOutput output, void *context, bool *finished, int *exit_code);
/* POSIX: forward an interrupt, then kill the group after two seconds.
 * Windows: attempt CTRL_BREAK and terminate the job (GUI children have no console). */
void editor_build_process_interrupt(EditorBuildProcess *process);
bool editor_build_process_active_check(const EditorBuildProcess *process);
void editor_build_process_destroy(EditorBuildProcess *process);
#endif
