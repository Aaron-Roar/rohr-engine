/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_ENTITY_SETTINGS_PANEL_H
#define ROHR_EDITOR_ENTITY_SETTINGS_PANEL_H
#include "editor_mode_context.h"
#include "editor_mode_controls.h"

#define EDITOR_ENTITY_SETTING_COUNT 23
typedef struct EditorEntityReferenceOption {
    TextAsset text;
    char name[EDITOR_OBJECT_NAME_MAX * 2];
    ObjectEntityReference reference;
    EditorSelectionRef selection;
} EditorEntityReferenceOption;
typedef struct EditorEntitySettingsPanel {
    FontAsset *font;
    EditorModeAccordionSection sections[5];
    TextAsset labels[EDITOR_ENTITY_SETTING_COUNT], fields[EDITOR_ENTITY_SETTING_COUNT];
    EditorEntityReferenceOption *references;
    const TextAsset **options;
    EditorSelectionRef *selections;
} EditorEntitySettingsPanel;

bool editor_entity_settings_panel_create(EditorEntitySettingsPanel *panel, FontAsset *font);
void editor_entity_settings_panel_destroy(EditorEntitySettingsPanel *panel);
bool editor_entity_settings_panel_draw(EditorEntitySettingsPanel *panel,
    const EditorModeContext *context, float y);
#endif
