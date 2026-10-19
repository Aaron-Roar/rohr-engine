/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SCENE_SETTINGS_PANEL_H
#define ROHR_EDITOR_SCENE_SETTINGS_PANEL_H
#include "editor_history.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorSceneSettingsLabel {
    TextAsset text;
    char name[EDITOR_OBJECT_NAME_MAX];
} EditorSceneSettingsLabel;
typedef struct EditorSceneSettingsPanel {
    bool open;
    float scroll;
    FontAsset *font; /* Borrowed until panel destruction. */
    TextAsset menu_label, title, close_label, all_label, none_label, viewport_label;
    EditorSceneSettingsLabel *objects, *viewports;
    const TextAsset **viewport_options;
    size_t object_count, viewport_count;
    size_t viewport_option_count;
} EditorSceneSettingsPanel;
bool editor_scene_settings_panel_create(EditorSceneSettingsPanel *panel, FontAsset *font);
void editor_scene_settings_panel_destroy(EditorSceneSettingsPanel *panel);
void editor_scene_settings_panel_draw(EditorSceneSettingsPanel *panel,
    EditorProject *project, EditorHistory *history, UIRect bounds);
bool editor_scene_settings_all_set(EditorProject *project, EditorHistory *history, bool included);
#endif
