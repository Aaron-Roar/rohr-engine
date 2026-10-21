/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "panels/editor_build_notifications.h"

#include "editor_workspace.h"
#include "editor_log.h"

#include <stdio.h>
#include <string.h>

void editor_build_notification_codegen_success(EditorTerminalPanel *terminal, EditorNotificationPanel *system,
        const EditorProject *project, bool tree_shown) {
    char detail[768];
    if(project == NULL) return;
    snprintf(detail, sizeof(detail),
        "Updated generated project files:\n"
        "- src/generated/project_objects.h\n"
        "- src/generated/project_objects.c\n"
        "- src/generated/project_viewports.h\n"
        "- src/generated/project_viewports.c\n"
        "- src/generated/project_scene.cmake\n"
        "- src/generated/project_scene.c\n"
        "- src/generated/scene/project_objects.{h,c}\n"
        "- src/generated/scene/project_viewports.{h,c}\n\n"
        "Generated %zu object%s.%s",
        project->object_count, project->object_count == 1 ? "" : "s",
        tree_shown ? " The generated object tree is shown in the terminal." :
            " Terminal tree output is disabled.");
    editor_build_status_write(terminal, "Generate App C", "SUCCESS", detail);
    editor_notification_panel_push(system, "Generate App C - SUCCESS", detail);
}

void editor_build_output_write(void *context, const char *bytes, size_t count) {
    EditorTerminalPanel *terminal = context;
    if(!terminal || !terminal->terminal || !bytes) return;
    RohrTerminalResult result = rohr_terminal_output_write(terminal->terminal, bytes, count);
    if(!result.success) editor_log_error("build", "Terminal output failed: %s",
        rohr_terminal_error_message_get(&result));
    terminal->scroll_offset = 0;
}

static void output_text(EditorTerminalPanel *terminal, const char *text) {
    if(text) editor_build_output_write(terminal, text, strlen(text));
}

void editor_build_status_write(EditorTerminalPanel *terminal, const char *stage,
        const char *status, const char *detail) {
    output_text(terminal, "\r\n["); output_text(terminal, stage);
    output_text(terminal, "] "); output_text(terminal, status);
    output_text(terminal, "\r\n");
    if(detail && *detail) {
        output_text(terminal, detail); output_text(terminal, "\r\n");
    }
}

void editor_build_notification_start_failure(EditorTerminalPanel *terminal,
        EditorNotificationPanel *system, const char *operation, const char *reason) {
    char summary[EDITOR_NOTIFICATION_SUMMARY_MAX];
    char detail[EDITOR_NOTIFICATION_DETAIL_MAX];
    if(!operation || !reason) return;
    editor_build_status_write(terminal, operation, "FAIL", reason);
    editor_log_error("build", "%s: %s", operation, reason);
    snprintf(summary, sizeof(summary), "%s - FAIL", operation);
    snprintf(detail, sizeof(detail), "See the terminal output for details.\n\n%s", reason);
    editor_notification_panel_push(system, summary, detail);
}

void editor_build_notification_result_failure(EditorTerminalPanel *terminal,
        EditorNotificationPanel *system, const char *operation, EditorResult result) {
    if(!editor_result_check(result)) return;
    char detail[EDITOR_ERROR_MESSAGE_MAX + 64];
    snprintf(detail, sizeof(detail), "Editor error %d: %s", (int)result.result.error.code,
        result.result.error.message[0] ? result.result.error.message : "No detailed editor error was provided.");
    editor_build_notification_start_failure(terminal, system, operation, detail);
}

void editor_build_notification_process_failure(EditorTerminalPanel *terminal,
        EditorNotificationPanel *system, const char *stage, int exit_code) {
    char detail[128];
    snprintf(detail, sizeof(detail), "%s command exited with status %d.", stage, exit_code);
    editor_build_notification_start_failure(terminal, system, stage, detail);
}

void editor_build_notification_configuration_failure(EditorTerminalPanel *terminal,
        EditorNotificationPanel *system, const char *parser_error, const char *lua_error) {
    if(!parser_error) return;
    editor_log_error("build_configuration", "Parser error: %s\nLua error: %s",
        parser_error, lua_error ? lua_error : "No Lua runtime error.");
    /* Stream each original message separately to avoid truncating combined diagnostics. */
    editor_build_status_write(terminal, "Build configuration", "Parser error", parser_error);
    editor_build_status_write(terminal, "Build configuration", "Lua error",
        lua_error && *lua_error ? lua_error : "No Lua runtime error.");
    char detail[EDITOR_NOTIFICATION_DETAIL_MAX];
    snprintf(detail, sizeof(detail),
        "See the terminal output for complete details.\n\nParser error:\n%s\n\nLua error:\n%s",
        parser_error, lua_error && *lua_error ? lua_error : "No Lua runtime error.");
    editor_notification_panel_push(system, "Build configuration (GUI) - FAIL", detail);
}
