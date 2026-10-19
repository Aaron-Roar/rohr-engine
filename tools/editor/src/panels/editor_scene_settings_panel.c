/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_scene_settings_panel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void labels_destroy(EditorSceneSettingsLabel *labels, size_t count) {
    for(size_t i = 0; i < count; ++i) rohr_graphics_text_destroy(&labels[i].text);
    free(labels);
}
static bool labels_resize(EditorSceneSettingsLabel **labels, size_t *count, size_t desired,
        FontAsset *font) {
    if(*count == desired) return true;
    EditorSceneSettingsLabel *next = desired ? calloc(desired, sizeof(*next)) : NULL;
    if(desired && !next) return false;
    for(size_t i = 0; i < desired; ++i)
        if(!editor_mode_text_create(font, "", &next[i].text)) { labels_destroy(next, desired); return false; }
    labels_destroy(*labels, *count); *labels = next; *count = desired;
    return true;
}
static void label_sync(EditorSceneSettingsLabel *label, const char *name) {
    if(!strcmp(label->name, name)) return;
    if(rohr_graphics_text_value_set(&label->text, name))
        SDL_strlcpy(label->name, name, sizeof(label->name));
}
bool editor_scene_settings_panel_create(EditorSceneSettingsPanel *panel, FontAsset *font) {
    if(!panel || !font) return false;
    *panel = (EditorSceneSettingsPanel){.font = font};
#define CREATE(value, field) if(!editor_mode_text_create(font, value, &panel->field)) goto fail
    CREATE("Scene Settings", menu_label); CREATE("Scene Settings", title);
    CREATE("Close", close_label); CREATE("All", all_label); CREATE("None", none_label);
    CREATE("Viewport", viewport_label);
#undef CREATE
    return true;
fail:
    editor_scene_settings_panel_destroy(panel); return false;
}
void editor_scene_settings_panel_destroy(EditorSceneSettingsPanel *panel) {
    if(!panel) return;
    labels_destroy(panel->objects, panel->object_count);
    labels_destroy(panel->viewports, panel->viewport_count);
    free(panel->viewport_options);
    rohr_graphics_text_destroy(&panel->menu_label); rohr_graphics_text_destroy(&panel->title);
    rohr_graphics_text_destroy(&panel->close_label); rohr_graphics_text_destroy(&panel->all_label);
    rohr_graphics_text_destroy(&panel->none_label); rohr_graphics_text_destroy(&panel->viewport_label);
    *panel = (EditorSceneSettingsPanel){0};
}
bool editor_scene_settings_all_set(EditorProject *project, EditorHistory *history, bool included) {
    if(!project || !history || !editor_history_transaction_begin(history)) return false;
    editor_history_transaction_commands_suppress_set(history, true);
    for(size_t i = 0; i < project->object_count; ++i) {
        if(project->objects[i].simulation_excluded != included) continue;
        if(!editor_history_transaction_object_track(history, project->objects[i].id)) {
            editor_history_transaction_cancel(history); return false;
        }
        EditorCommand command = {.type = EDITOR_COMMAND_SIMULATION_OBJECT_SET,
            .data.simulation_object = {project->objects[i].id, included}};
        if(editor_command_execute(project, &command).kind == ERROR_RESULT_ERROR) {
            editor_history_transaction_cancel(history); return false;
        }
    }
    if(editor_history_transaction_end(history)) return true;
    editor_history_transaction_cancel(history);
    return false;
}
void editor_scene_settings_panel_draw(EditorSceneSettingsPanel *panel,
        EditorProject *project, EditorHistory *history, UIRect bounds) {
    if(!panel || !panel->open || !project) return;
    if(!labels_resize(&panel->objects, &panel->object_count, project->object_count, panel->font) ||
            !labels_resize(&panel->viewports, &panel->viewport_count, project->layout_viewport_count, panel->font)) return;
    /* The dropdown borrows label pointers only for this draw. */
    if(panel->viewport_option_count != panel->viewport_count + 1) {
        const TextAsset **options = realloc(panel->viewport_options,
            (panel->viewport_count + 1) * sizeof(*options));
        if(!options) return;
        panel->viewport_options = options;
        panel->viewport_option_count = panel->viewport_count + 1;
    }
    const TextAsset **options = panel->viewport_options;
    options[0] = &panel->none_label;
    size_t selected = 0;
    for(size_t i = 0; i < panel->viewport_count; ++i) {
        label_sync(&panel->viewports[i], project->layout_viewports[i].name);
        options[i + 1] = &panel->viewports[i].text;
        if(project->simulation_viewport == project->layout_viewports[i].id) selected = i + 1;
    }
    rohr_ui_modal_controls_begin();
    rohr_ui_surface(bounds, (Color){37, 42, 52, 255});
    rohr_ui_border(bounds, 2, (Color){5, 6, 8, 255});
    rohr_ui_label(&panel->title, (UIRect){bounds.x + 20, bounds.y + 14, bounds.width - 40, 32});
    rohr_ui_label(&panel->viewport_label, (UIRect){bounds.x + 20, bounds.y + 60, 120, 28});
    UIDropdownResult viewport = editor_mode_dropdown("editor.scene.viewport", options,
        panel->viewport_count + 1, selected,
        (UIRect){bounds.x + 150, bounds.y + 60, bounds.width - 170, 28}, NULL);
    if(viewport.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_SIMULATION_VIEWPORT_SET,
            .data.simulation_viewport = {viewport.selected_index ?
                project->layout_viewports[viewport.selected_index - 1].id : 0}};
        (void)editor_command_execute(project, &command);
    }
    if(rohr_ui_button("editor.scene.all", &panel->all_label,
            (UIRect){bounds.x + 20, bounds.y + 104, 90, 28}, NULL).clicked)
        (void)editor_scene_settings_all_set(project, history, true);
    if(rohr_ui_button("editor.scene.none", &panel->none_label,
            (UIRect){bounds.x + 120, bounds.y + 104, 90, 28}, NULL).clicked)
        (void)editor_scene_settings_all_set(project, history, false);
    UIRect list = {bounds.x + 20, bounds.y + 146, bounds.width - 40, bounds.height - 208};
    UIScrollRegionResult scroll = rohr_ui_scroll_region_begin("editor.scene.objects", list,
        (float)project->object_count * 34, panel->scroll, 34);
    panel->scroll = scroll.offset;
    for(size_t i = 0; i < project->object_count; ++i) {
        label_sync(&panel->objects[i], project->objects[i].name);
        char id[64]; snprintf(id, sizeof(id), "editor.scene.object.%u", project->objects[i].id);
        bool included = !project->objects[i].simulation_excluded;
        if(editor_mode_checkbox_left(id, &panel->objects[i].text,
                (UIRect){list.x + 6, list.y + (float)i * 34, list.width - 30, 28}, &included)) {
            EditorCommand command = {.type = EDITOR_COMMAND_SIMULATION_OBJECT_SET,
                .data.simulation_object = {project->objects[i].id, included}};
            (void)editor_command_execute(project, &command);
        }
    }
    rohr_ui_scroll_region_end();
    if(rohr_ui_button("editor.scene.close", &panel->close_label,
            (UIRect){bounds.x + bounds.width - 124, bounds.y + bounds.height - 48, 100, 32}, NULL).clicked)
        panel->open = false;
    rohr_ui_modal_controls_end();
}
