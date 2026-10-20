/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_APPLICATION_SETTINGS_PANEL_H
#define ROHR_EDITOR_APPLICATION_SETTINGS_PANEL_H
#include "editors/editor_mode_controls.h"

typedef struct EditorApplicationSettingsPanel {
    bool open;
    float scroll;
    uint32_t selected;
    EditorAudioDefinition draft;
    FontAsset *font; /* Borrowed. */
    TextAsset menu_label, title, close_label, add_label, remove_label, error_label;
    TextAsset labels[15], fields[15], modes[3], new_label;
    TextAsset *audio_labels;
    const TextAsset **audio_options;
    size_t audio_count;
} EditorApplicationSettingsPanel;
bool editor_application_settings_panel_create(EditorApplicationSettingsPanel *panel, FontAsset *font);
void editor_application_settings_panel_destroy(EditorApplicationSettingsPanel *panel);
void editor_application_settings_panel_draw(EditorApplicationSettingsPanel *panel,
    EditorProject *project, UIRect bounds);
#endif
