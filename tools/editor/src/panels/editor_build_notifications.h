/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_BUILD_NOTIFICATIONS_H
#define ROHR_EDITOR_BUILD_NOTIFICATIONS_H

#include "editor_project.h"
#include "editor_error.h"
#include "panels/editor_terminal_panel.h"
#include "panels/editor_notification_panel.h"

void editor_build_notification_codegen_success(EditorTerminalPanel *terminal, EditorNotificationPanel *system,
    const EditorProject *project, bool tree_shown);
void editor_build_notification_process_failure(EditorTerminalPanel *terminal, EditorNotificationPanel *system,
    const char *stage, int exit_code);
void editor_build_notification_start_failure(EditorTerminalPanel *terminal, EditorNotificationPanel *system,
    const char *operation, const char *reason);
void editor_build_notification_configuration_failure(
    EditorTerminalPanel *terminal, EditorNotificationPanel *system, const char *parser_error,
    const char *lua_error);

/* These output operations never change terminal visibility or focus. */
void editor_build_output_write(void *context, const char *bytes, size_t count);
void editor_build_status_write(EditorTerminalPanel *terminal, const char *stage,
    const char *status, const char *detail);
void editor_build_notification_result_failure(EditorTerminalPanel *terminal,
    EditorNotificationPanel *system, const char *operation, EditorResult result);

#endif
