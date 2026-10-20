/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_application_settings_panel.h"
#include "editor_layout.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *labels[] = {"Apply Window Settings", "Window Mode", "Window Width",
    "Window Height", "Logical Width", "Logical Height", "Automatic Aspect Ratio",
    "Audio Definition", "Name", "Path", "Music", "Volume", "Pan", "Loop", "Playback Rate"};

bool editor_application_settings_panel_create(EditorApplicationSettingsPanel *p, FontAsset *font) {
    if(!p || !font) return false;
    *p = (EditorApplicationSettingsPanel){.font = font, .draft.playback_rate = 1};
#define CREATE(text, member) if(!editor_mode_text_create(font, text, &p->member)) goto fail
    CREATE("Application", menu_label); CREATE("Application Settings", title);
    CREATE("Close", close_label); CREATE("Add Definition", add_label);
    CREATE("Remove", remove_label); CREATE("", error_label); CREATE("New Definition", new_label);
    CREATE("Windowed", modes[0]); CREATE("Borderless Fullscreen", modes[1]); CREATE("Fullscreen", modes[2]);
#undef CREATE
    for(size_t i = 0; i < 15; ++i)
        if(!editor_mode_text_create(font, labels[i], &p->labels[i]) ||
                !editor_mode_text_create(font, "", &p->fields[i])) goto fail;
    return true;
fail:
    editor_application_settings_panel_destroy(p);
    return false;
}
void editor_application_settings_panel_destroy(EditorApplicationSettingsPanel *p) {
    if(!p) return;
    for(size_t i = 0; i < p->audio_count; ++i) rohr_graphics_text_destroy(&p->audio_labels[i]);
    free(p->audio_labels); free(p->audio_options);
    for(size_t i = 0; i < 15; ++i) {
        rohr_graphics_text_destroy(&p->labels[i]); rohr_graphics_text_destroy(&p->fields[i]);
    }
    for(size_t i = 0; i < 3; ++i) rohr_graphics_text_destroy(&p->modes[i]);
    rohr_graphics_text_destroy(&p->menu_label); rohr_graphics_text_destroy(&p->title);
    rohr_graphics_text_destroy(&p->close_label); rohr_graphics_text_destroy(&p->add_label);
    rohr_graphics_text_destroy(&p->remove_label); rohr_graphics_text_destroy(&p->error_label);
    rohr_graphics_text_destroy(&p->new_label);
    *p = (EditorApplicationSettingsPanel){0};
}

static bool options_sync(EditorApplicationSettingsPanel *p, const EditorProject *project) {
    if(!p->audio_options || p->audio_count != project->audio_definition_count) {
        size_t count = project->audio_definition_count;
        TextAsset *texts = count ? calloc(count, sizeof(*texts)) : NULL;
        const TextAsset **options = calloc(count + 1, sizeof(*options));
        if((count && !texts) || !options) { free(texts); free(options); return false; }
        for(size_t i = 0; i < count; ++i)
            if(!editor_mode_text_create(p->font, "", &texts[i])) {
                for(size_t j = 0; j < count; ++j) rohr_graphics_text_destroy(&texts[j]);
                free(texts); free(options); return false;
            }
        for(size_t i = 0; i < p->audio_count; ++i) rohr_graphics_text_destroy(&p->audio_labels[i]);
        free(p->audio_labels); free(p->audio_options);
        p->audio_labels = texts; p->audio_options = options; p->audio_count = count;
    }
    p->audio_options[0] = &p->new_label;
    for(size_t i = 0; i < p->audio_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX + 16];
        snprintf(name, sizeof(name), "%s: %s", project->audio_definitions[i].music ? "Music" : "Sound",
            project->audio_definitions[i].name);
        if(!rohr_graphics_text_value_set(&p->audio_labels[i], name)) return false;
        p->audio_options[i + 1] = &p->audio_labels[i];
    }
    return true;
}

static UIFieldResult field(EditorApplicationSettingsPanel *p, size_t index,
        UIFieldBinding binding, UIRect row) {
    char id[64]; snprintf(id, sizeof(id), "editor.application.field.%zu", index);
    rohr_ui_label(&p->labels[index], (UIRect){row.x, row.y, 170, 28});
    return editor_mode_field(id, binding, &p->fields[index],
        (UIRect){row.x + 180, row.y, row.width - 180, 28}, NULL);
}
static bool integer_field(EditorApplicationSettingsPanel *p, size_t index, int *value, UIRect row) {
    char text[32]; snprintf(text, sizeof(text), "%d", *value);
    UIFieldResult result = field(p, index, (UIFieldBinding){.kind = UI_FIELD_STRING,
        .string = text, .string_capacity = sizeof(text)}, row);
    if(!result.changed) return false;
    char *end; errno = 0; long parsed = strtol(text, &end, 10);
    if(errno || end == text || *end || parsed <= 0 || parsed > INT_MAX) return false;
    *value = (int)parsed;
    return true;
}
static void execute(EditorApplicationSettingsPanel *p, EditorProject *project, EditorCommand command) {
    EditorCommandResult result = editor_command_execute(project, &command);
    (void)rohr_graphics_text_value_set(&p->error_label,
        result.kind == ERROR_RESULT_ERROR ? "Invalid value, duplicate name, or missing path." : "");
    if(result.kind == ERROR_RESULT_VALUE && command.type == EDITOR_COMMAND_AUDIO_ADD)
        p->selected = result.result.object;
}

void editor_application_settings_panel_draw(EditorApplicationSettingsPanel *p,
        EditorProject *project, UIRect bounds) {
    if(!p || !p->open || !project || !options_sync(p, project)) return;
    rohr_ui_modal_controls_begin();
    rohr_ui_surface(bounds, (Color){37, 42, 52, 255});
    rohr_ui_border(bounds, 2, (Color){5, 6, 8, 255});
    rohr_ui_label(&p->title, (UIRect){bounds.x + 20, bounds.y + 14, bounds.width - 40, 32});
    UIRect list = {bounds.x + 20, bounds.y + 60, bounds.width - 40, bounds.height - 160};
    UIScrollRegionResult scroll = rohr_ui_scroll_region_begin("editor.application.scroll", list, 620, p->scroll, 36);
    p->scroll = scroll.offset;
    UIRect row = {list.x + 8, list.y, list.width - 32, 28};
    EditorCommand window = {.type = EDITOR_COMMAND_APPLICATION_WINDOW_SET,
        .data.application_window = {project->application_window_enabled, project->application_window}};
    bool changed = editor_mode_checkbox_left("editor.application.enabled", &p->labels[0],
        row, &window.data.application_window.enabled);
    row.y += 36;
    const TextAsset *modes[] = {&p->modes[0], &p->modes[1], &p->modes[2]};
    rohr_ui_label(&p->labels[1], (UIRect){row.x, row.y, 170, 28});
    UIDropdownResult mode = rohr_ui_dropdown("editor.application.mode", modes, 3,
        (size_t)window.data.application_window.description.mode,
        (UIDropdownConfig){.visible_row_limit = EDITOR_DROPDOWN_VISIBLE_ROW_LIMIT, .fit_window = true},
        (UIRect){row.x + 180, row.y, row.width - 180, 28}, NULL);
    if(mode.changed) { window.data.application_window.description.mode = (GraphicsWindowMode)mode.selected_index; changed = true; }
    int *dimensions[] = {&window.data.application_window.description.window_width,
        &window.data.application_window.description.window_height,
        &window.data.application_window.description.logical_width,
        &window.data.application_window.description.logical_height};
    for(size_t i = 0; i < 4; ++i) { row.y += 36; changed |= integer_field(p, i + 2, dimensions[i], row); }
    row.y += 36;
    changed |= editor_mode_checkbox_left("editor.application.aspect", &p->labels[6], row,
        &window.data.application_window.description.aspect_ratio_auto);
    if(changed) execute(p, project, window);
    row.y += 48;
    size_t selected = 0;
    for(size_t i = 0; i < project->audio_definition_count; ++i)
        if(project->audio_definitions[i].id == p->selected) selected = i + 1;
    if(!selected) p->selected = 0;
    rohr_ui_label(&p->labels[7], (UIRect){row.x, row.y, 170, 28});
    UIDropdownResult audio = rohr_ui_dropdown("editor.application.audio", p->audio_options,
        p->audio_count + 1, selected,
        (UIDropdownConfig){.visible_row_limit = EDITOR_DROPDOWN_VISIBLE_ROW_LIMIT, .fit_window = true},
        (UIRect){row.x + 180, row.y, row.width - 180, 28}, NULL);
    if(audio.changed) {
        selected = audio.selected_index;
        p->selected = selected ? project->audio_definitions[selected - 1].id : 0;
    }
    EditorAudioDefinition value = selected ? project->audio_definitions[selected - 1] : p->draft;
    row.y += 36;
    changed = field(p, 8, (UIFieldBinding){.kind = UI_FIELD_STRING, .string = value.name,
        .string_capacity = sizeof(value.name)}, row).changed;
    row.y += 36;
    changed |= field(p, 9, (UIFieldBinding){.kind = UI_FIELD_STRING, .string = value.path,
        .string_capacity = sizeof(value.path)}, row).changed;
    row.y += 36;
    if(editor_mode_checkbox_left("editor.application.music", &p->labels[10], row, &value.music)) {
        if(value.music) value.pan = 0;
        changed = true;
    }
    float *numbers[] = {&value.volume, &value.pan, &value.playback_rate};
    for(size_t i = 0; i < 3; ++i) {
        if(i == 1 && value.music) continue;
        row.y += 36;
        changed |= field(p, i == 2 ? 14 : 11 + i,
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = numbers[i]}, row).changed;
    }
    row.y += 36;
    changed |= editor_mode_checkbox_left("editor.application.loop", &p->labels[13], row, &value.loop);
    if(changed && selected) execute(p, project, (EditorCommand){.type = EDITOR_COMMAND_AUDIO_SET, .data.audio = value});
    if(!selected) p->draft = value;
    row.y += 36;
    if(!selected && rohr_ui_button("editor.application.add", &p->add_label, row, NULL).clicked) {
        execute(p, project, (EditorCommand){.type = EDITOR_COMMAND_AUDIO_ADD, .data.audio = value});
    } else if(selected && rohr_ui_button("editor.application.remove", &p->remove_label, row, NULL).clicked) {
        execute(p, project, (EditorCommand){.type = EDITOR_COMMAND_AUDIO_REMOVE, .data.audio = value});
        p->selected = 0;
    }
    rohr_ui_scroll_region_end();
    rohr_ui_label(&p->error_label, (UIRect){bounds.x + 20, bounds.y + bounds.height - 90, bounds.width - 40, 28});
    if(rohr_ui_button("editor.application.close", &p->close_label,
            (UIRect){bounds.x + bounds.width - 124, bounds.y + bounds.height - 48, 100, 32}, NULL).clicked)
        p->open = false;
    rohr_ui_modal_controls_end();
}
