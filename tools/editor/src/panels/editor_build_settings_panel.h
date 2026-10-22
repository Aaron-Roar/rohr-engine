/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_BUILD_SETTINGS_PANEL_H
#define ROHR_EDITOR_BUILD_SETTINGS_PANEL_H

#include "editor_config.h"
#include "editor_notification_panel.h"
#include "editor_terminal_panel.h"
#include "rohr.h"
#include "browser/editor_file_browser.h"

typedef struct EditorBuildSettingsPanel {
    bool open;
    bool build_requested;
    bool apply_requested;
    EditorWorkspace draft;
    EditorWorkspace original;
    FontAsset font; /* Borrowed for the panel lifetime. */
    float scroll;
    int browse_path;
    EditorFileBrowser browser;
    TextAsset path_labels[EDITOR_PATH_COUNT];
    TextAsset path_fields[EDITOR_PATH_COUNT];
    TextAsset resolved_labels[EDITOR_PATH_COUNT];
    TextAsset browse_label, path_reset_label, browser_field;
    bool configure_override;
    bool compile_override;
    char configure[UI_FIELD_EDIT_MAX];
    char compile[UI_FIELD_EDIT_MAX];
    char error[EDITOR_ERROR_MESSAGE_MAX];
    EditorConfigCommand inherited_configure;
    EditorConfigCommand inherited_compile;
    TextAsset title_label;
    TextAsset configure_label;
    TextAsset compile_label;
    TextAsset configure_field;
    TextAsset compile_field;
    TextAsset apply_label;
    TextAsset cancel_label;
    TextAsset reset_label;
    TextAsset error_label;
} EditorBuildSettingsPanel;

bool editor_build_settings_panel_create(EditorBuildSettingsPanel *panel,
    const FontAsset *font);
EditorResult editor_build_settings_panel_open(EditorBuildSettingsPanel *panel,
    const EditorWorkspace *workspace);
EditorResult editor_build_settings_panel_apply(EditorBuildSettingsPanel *panel,
    EditorWorkspace *workspace, EditorProject *project);
void editor_build_settings_panel_draw(EditorBuildSettingsPanel *panel,
    EditorTerminalPanel *terminal, EditorNotificationPanel *notifications, const char *project_directory,
    UIRect bounds);
void editor_build_settings_panel_destroy(EditorBuildSettingsPanel *panel);

#endif
