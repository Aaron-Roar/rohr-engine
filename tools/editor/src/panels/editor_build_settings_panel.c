/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_build_settings_panel.h"
#include "editor_build_notifications.h"
#include "editor_layout.h"
#include "editor_build.h"
#include "graphics/ui_internal.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

static bool editor_build_settings_text_create(const FontAsset *font,
        const char *value, TextAsset *text) {
    TextAssetResult result = rohr_graphics_text_create(
        (TextDescription){.font = font != NULL ? *font : (FontAsset){0},
            .value = value,
            .color = (Color){235, 238, 244, 255}});
    if(rohr_error_check(result)) return false;
    *text = result.result.value;
    return true;
}

static EditorResult editor_build_settings_layers_load(EditorConfig *base,
        EditorConfig *effective, const char *project_directory) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult result;
    editor_config_init(base);
    result = editor_config_sdk_path_get(path, sizeof(path), "editor.lua", true);
    if(editor_result_check(result)) return result;
    result = editor_config_file_merge(base, path, true);
    if(editor_result_check(result)) return result;
    if(snprintf(path, sizeof(path), "%s/editor.lua", project_directory) >=
            (int)sizeof(path)) return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Project editor.lua path is too long");
    result = editor_config_file_merge(base, path, false);
    if(editor_result_check(result)) return result;
    *effective = *base;
    if(snprintf(path, sizeof(path), "%s/.rohr/gui-overrides.lua",
            project_directory) >= (int)sizeof(path)) return editor_result_error(
        EDITOR_ERROR_CAPACITY, "Project GUI override path is too long");
    return editor_config_file_merge(effective, path, false);
}

static EditorConfigCommand editor_build_settings_builtin(bool configure) {
    EditorConfigCommand command = {.set = true};
    static const char *configure_arguments[] = {
        "cmake", "-S", "{project}", "-B", "{build}"};
    static const char *compile_arguments[] = {"cmake", "--build", "{build}"};
    const char *const *arguments = configure ? configure_arguments : compile_arguments;
    command.count = configure ? 5 : 3;
    for(size_t i = 0; i < command.count; i += 1)
        snprintf(command.arguments[i], sizeof(command.arguments[i]), "%s", arguments[i]);
    return command;
}

bool editor_build_settings_panel_create(EditorBuildSettingsPanel *panel,
        const FontAsset *font) {
    if(panel == NULL || font == NULL) return false;
    *panel = (EditorBuildSettingsPanel){.font = *font};
    for(int i = 0; i < EDITOR_PATH_COUNT; ++i)
        if(!editor_build_settings_text_create(font, editor_workspace_path_label_get(i), &panel->path_labels[i]) ||
                !editor_build_settings_text_create(font, "", &panel->path_fields[i]) ||
                !editor_build_settings_text_create(font, "", &panel->resolved_labels[i])) return false;
    if(!editor_build_settings_text_create(font, "Browse", &panel->browse_label) ||
            !editor_build_settings_text_create(font, "Reset", &panel->path_reset_label) ||
            !editor_build_settings_text_create(font, "", &panel->browser_field)) return false;
    return editor_build_settings_text_create(font, "Build Settings",
            &panel->title_label) &&
        editor_build_settings_text_create(font, "Build configure override",
            &panel->configure_label) &&
        editor_build_settings_text_create(font, "Build compile override",
            &panel->compile_label) &&
        editor_build_settings_text_create(font, "", &panel->configure_field) &&
        editor_build_settings_text_create(font, "", &panel->compile_field) &&
        editor_build_settings_text_create(font, "Apply", &panel->apply_label) &&
        editor_build_settings_text_create(font, "Cancel", &panel->cancel_label) &&
        editor_build_settings_text_create(font, "Reset to inherited",
            &panel->reset_label) &&
        editor_build_settings_text_create(font, "", &panel->error_label);
}

EditorResult editor_build_settings_panel_open(EditorBuildSettingsPanel *panel,
        const EditorWorkspace *workspace) {
    const char *project_directory = workspace ? workspace->directory : NULL;
    EditorConfig base;
    EditorConfig effective;
    EditorConfig overrides;
    const EditorConfigCommand *command;
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult result;
    if(panel == NULL || project_directory == NULL) return editor_result_error(
        EDITOR_ERROR_INVALID_ARGUMENT, "Build settings received an invalid project");
    result = editor_build_settings_layers_load(&base, &effective, project_directory);
    if(editor_result_check(result)) return result;
    command = editor_config_command_get(&base, EDITOR_CONFIG_FRONTEND_GUI,
        EDITOR_CONFIG_OPERATION_CONFIGURE);
    panel->inherited_configure = command == NULL ?
        editor_build_settings_builtin(true) : *command;
    command = editor_config_command_get(&base, EDITOR_CONFIG_FRONTEND_GUI,
        EDITOR_CONFIG_OPERATION_COMPILE);
    panel->inherited_compile = command == NULL ?
        editor_build_settings_builtin(false) : *command;
    command = editor_config_command_get(&effective, EDITOR_CONFIG_FRONTEND_GUI,
        EDITOR_CONFIG_OPERATION_CONFIGURE);
    if(command == NULL) command = &panel->inherited_configure;
    result = editor_config_command_expression_write(command, panel->configure,
        sizeof(panel->configure));
    if(editor_result_check(result)) return result;
    command = editor_config_command_get(&effective, EDITOR_CONFIG_FRONTEND_GUI,
        EDITOR_CONFIG_OPERATION_COMPILE);
    if(command == NULL) command = &panel->inherited_compile;
    result = editor_config_command_expression_write(command, panel->compile,
        sizeof(panel->compile));
    if(editor_result_check(result)) return result;
    editor_config_init(&overrides);
    if(snprintf(path, sizeof(path), "%s/.rohr/gui-overrides.lua",
            project_directory) >= (int)sizeof(path)) return editor_result_error(
        EDITOR_ERROR_CAPACITY, "Project GUI override path is too long");
    result = editor_config_file_merge(&overrides, path, false);
    if(editor_result_check(result)) return result;
    panel->configure_override = overrides.gui_configure.set;
    panel->compile_override = overrides.gui_compile.set;
    panel->error[0] = '\0';
    panel->draft = panel->original = *workspace;
    panel->scroll = 0; panel->apply_requested = false;
    editor_file_browser_destroy(&panel->browser);
    panel->open = true;
    return editor_result_value(true);
}

static void editor_build_settings_error_set(EditorBuildSettingsPanel *panel,
        EditorResult result) {
    if(!editor_result_check(result)) panel->error[0] = '\0';
    else snprintf(panel->error, sizeof(panel->error), "%s",
        result.result.error.message);
    (void)rohr_graphics_text_value_set(&panel->error_label, panel->error);
}

static EditorResult editor_build_settings_validate(
        const EditorBuildSettingsPanel *panel, EditorConfigCommand *configure,
        EditorConfigCommand *compile, const char *project_directory,
        char *lua_error, size_t lua_error_capacity) {
    EditorResult result = editor_config_command_expression_parse_detailed(
        panel->configure, configure, lua_error, lua_error_capacity);
    if(editor_result_check(result)) return editor_result_error(
        result.result.error.code, "Configure override: %s",
        result.result.error.message);
    result = editor_config_command_expression_parse_detailed(panel->compile,
        compile, lua_error, lua_error_capacity);
    if(editor_result_check(result)) return editor_result_error(
        result.result.error.code, "Compile override: %s",
        result.result.error.message);
    result = editor_config_command_executable_check(configure, project_directory);
    if(editor_result_check(result)) return editor_result_error(
        result.result.error.code, "Configure override: %s",
        result.result.error.message);
    result = editor_config_command_executable_check(compile, project_directory);
    if(editor_result_check(result)) return editor_result_error(
        result.result.error.code, "Compile override: %s",
        result.result.error.message);
    return editor_result_value(true);
}


EditorResult editor_build_settings_panel_apply(EditorBuildSettingsPanel *panel,
        EditorWorkspace *workspace, EditorProject *project) {
    EditorConfigCommand configure, compile;
    char lua_error[EDITOR_ERROR_MESSAGE_MAX] = {0};
    EditorResult result = editor_build_settings_validate(panel, &configure, &compile,
        workspace->directory, lua_error, sizeof(lua_error));
    if(editor_result_check(result)) return result;
    EditorWorkspace desired = panel->draft;
    result = editor_workspace_paths_validate(workspace, &desired);
    if(editor_result_check(result)) return result;
    char override_path[EDITOR_WORKSPACE_PATH_MAX * 2];
    snprintf(override_path, sizeof(override_path), "%s/.rohr/gui-overrides.lua", workspace->directory);
    SDL_PathInfo info; bool existed = SDL_GetPathInfo(override_path, &info);
    size_t length = 0; void *previous = existed ? SDL_LoadFile(override_path, &length) : NULL;
    if(existed && !previous) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not retain build command overrides");
    result = editor_config_gui_override_save(workspace->directory,
        panel->configure_override ? &configure : NULL, panel->compile_override ? &compile : NULL);
    if(!editor_result_check(result)) result = editor_workspace_paths_apply(workspace, project, &desired);
    if(editor_result_check(result)) {
        bool restored = true;
        if(existed) {
            FILE *file = fopen(override_path, "wb");
            if(file) { restored = fwrite(previous, 1, length, file) == length; restored &= fclose(file) == 0; }
            else restored = false;
        } else if(SDL_GetPathInfo(override_path, &info)) restored = SDL_RemovePath(override_path);
        if(!restored) result = editor_result_error(EDITOR_ERROR_FILE_IO,
            "Path change failed; restore build command overrides at %s. %s", override_path, result.result.error.message);
    }
    SDL_free(previous);
    editor_build_settings_error_set(panel, result);
    if(!editor_result_check(result)) { panel->open = false; panel->build_requested = true; }
    return result;
}

void editor_build_settings_panel_draw(EditorBuildSettingsPanel *panel,
        EditorTerminalPanel *terminal, EditorNotificationPanel *notifications, const char *project_directory,
        UIRect bounds) {
    (void)terminal; (void)notifications; (void)project_directory;
    if(!panel || !panel->open) return;
    rohr_ui_modal_controls_begin();
    if(panel->browser.active) {
        EditorFileBrowserResult selection = editor_file_browser_draw(&panel->browser, &panel->browser_field,
            &panel->apply_label, &panel->apply_label, &panel->apply_label, &panel->cancel_label,
            editor_window_width, EDITOR_WINDOW_HEIGHT);
        if(selection.submitted) SDL_strlcpy(editor_workspace_path_value_get(&panel->draft, panel->browse_path),
            selection.path, EDITOR_WORKSPACE_PATH_MAX);
        rohr_ui_modal_controls_end(); return;
    }
    rohr_ui_surface(bounds, (Color){37,42,52,255}); rohr_ui_border(bounds, 2, (Color){5,6,8,255});
    rohr_ui_label(&panel->title_label, (UIRect){bounds.x+20,bounds.y+14,bounds.width-40,32});
    UIRect body = {bounds.x+14,bounds.y+56,bounds.width-28,fmaxf(50,bounds.height-158)};
    panel->scroll = rohr_ui_scroll_region_begin("editor.build.settings.scroll", body,
        EDITOR_PATH_COUNT * 100 + 350, panel->scroll, 38).offset;
    float y = body.y;
    for(int i = 0; i < EDITOR_PATH_COUNT; ++i) {
        char id[80], resolved[EDITOR_WORKSPACE_PATH_MAX * 2];
        char *value = editor_workspace_path_value_get(&panel->draft, i);
        rohr_ui_label(&panel->path_labels[i], (UIRect){body.x+10,y,body.width-20,24}); y += 26;
        snprintf(id, sizeof(id), "editor.settings.path.%d", i);
        UIFieldResult edited = rohr_ui_field(id, (UIFieldBinding){.kind=UI_FIELD_STRING,.string=value,.string_capacity=EDITOR_WORKSPACE_PATH_MAX},
            &panel->path_fields[i], (UIRect){body.x+10,y,body.width-182,28}, NULL);
        if(edited.changed) panel->error[0] = 0;
        snprintf(id, sizeof(id), "editor.settings.path.%d.browse", i);
        if(rohr_ui_button(id, &panel->browse_label, (UIRect){body.x+body.width-164,y,78,28}, NULL).clicked) {
            char start[EDITOR_WORKSPACE_PATH_MAX * 2];
            if(editor_result_check(editor_workspace_path_get(&panel->draft, i, start, sizeof(start))))
                SDL_strlcpy(start, panel->original.directory, sizeof(start));
            SDL_PathInfo info;
            while(!SDL_GetPathInfo(start, &info) || info.type != SDL_PATHTYPE_DIRECTORY) {
                char *slash = strrchr(start, '/'); if(!slash || slash == start) break; *slash = 0;
            }
            panel->browse_path = i;
            EditorFileBrowserMode mode = i == EDITOR_PATH_ENGINE || i == EDITOR_PATH_BUILD ? EDITOR_FILE_BROWSER_DIRECTORY :
                i == EDITOR_PATH_STATE || i == EDITOR_PATH_APPLICATION || i == EDITOR_PATH_SCENE ? EDITOR_FILE_BROWSER_SAVE : EDITOR_FILE_BROWSER_CREATE_DIRECTORY;
            if(!editor_file_browser_open(&panel->browser, mode, start, &panel->font))
                SDL_strlcpy(panel->error, "Could not open the path browser", sizeof(panel->error));
            else {
                panel->browser.all_files = i >= EDITOR_PATH_APPLICATION;
                panel->browser.refresh_pending = true;
                (void)rohr_graphics_text_value_set(&panel->browser.name_label, "Directory name");
                if(mode == EDITOR_FILE_BROWSER_SAVE || mode == EDITOR_FILE_BROWSER_CREATE_DIRECTORY) {
                    char full[EDITOR_WORKSPACE_PATH_MAX * 2];
                    if(!editor_result_check(editor_workspace_path_get(&panel->draft, i, full, sizeof(full)))) {
                        const char *name = strrchr(full, '/');
                        SDL_strlcpy(panel->browser.filename, name ? name + 1 : full, sizeof(panel->browser.filename));
                    }
                }
            }
        }
        snprintf(id, sizeof(id), "editor.settings.path.%d.reset", i);
        if(rohr_ui_button(id, &panel->path_reset_label, (UIRect){body.x+body.width-80,y,68,28}, NULL).clicked) {
            EditorWorkspace defaults = {.config=editor_workspace_config_default_get()};
            SDL_strlcpy(value, i == EDITOR_PATH_PROJECT ? panel->original.directory :
                editor_workspace_path_value_get(&defaults, i), EDITOR_WORKSPACE_PATH_MAX);
            ui_field_focus_clear(); panel->error[0] = 0;
        }
        EditorResult r = editor_workspace_path_get(&panel->draft, i, resolved, sizeof(resolved));
        if(i >= EDITOR_PATH_APPLICATION && !*value && !memcmp(&panel->draft, &panel->original, sizeof(panel->draft))) {
            char built[sizeof(resolved)];
            if(!editor_result_check(editor_build_executable_get(&panel->draft,
                    i == EDITOR_PATH_APPLICATION ? EDITOR_BUILD_APPLICATION : EDITOR_BUILD_SCENE, built, sizeof(built))))
                SDL_strlcpy(resolved, built, sizeof(resolved));
        }
        (void)rohr_graphics_text_value_set(&panel->resolved_labels[i], editor_result_check(r) ? r.result.error.message : resolved);
        rohr_ui_label(&panel->resolved_labels[i], (UIRect){body.x+10,y+32,body.width-20,26}); y += 74;
    }
    rohr_ui_label(&panel->configure_label, (UIRect){body.x+10,y,body.width-20,24}); y += 28;
    if(rohr_ui_multiline_field("editor.settings.build.configure", (UIFieldBinding){UI_FIELD_STRING,panel->configure,sizeof(panel->configure),NULL},
            &panel->configure_field, (UIRect){body.x+10,y,body.width-20,84}, NULL).changed) panel->configure_override = true;
    y += 90;
    if(rohr_ui_button("editor.settings.build.configure.reset", &panel->reset_label,
            (UIRect){body.x+10,y,180,28},NULL).clicked) {
        (void)editor_config_command_expression_write(&panel->inherited_configure,panel->configure,sizeof(panel->configure));
        panel->configure_override=false;
    }
    y += 40; rohr_ui_label(&panel->compile_label,(UIRect){body.x+10,y,body.width-20,24}); y+=28;
    if(rohr_ui_multiline_field("editor.settings.build.compile",(UIFieldBinding){UI_FIELD_STRING,panel->compile,sizeof(panel->compile),NULL},
            &panel->compile_field,(UIRect){body.x+10,y,body.width-20,84},NULL).changed) panel->compile_override=true;
    y+=90;
    if(rohr_ui_button("editor.settings.build.compile.reset",&panel->reset_label,(UIRect){body.x+10,y,180,28},NULL).clicked) {
        (void)editor_config_command_expression_write(&panel->inherited_compile,panel->compile,sizeof(panel->compile));
        panel->compile_override=false;
    }
    rohr_ui_scroll_region_end();
    (void)rohr_graphics_text_value_set(&panel->error_label,panel->error[0] ? panel->error : "Apply saves the project and moves changed content locations.");
    rohr_ui_label(&panel->error_label,(UIRect){bounds.x+24,bounds.y+bounds.height-96,bounds.width-48,38});
    if(rohr_ui_button("editor.settings.build.apply",&panel->apply_label,
            (UIRect){bounds.x+bounds.width-250,bounds.y+bounds.height-48,100,32},NULL).clicked) panel->apply_requested=true;
    if(rohr_ui_button("editor.settings.build.cancel",&panel->cancel_label,
            (UIRect){bounds.x+bounds.width-140,bounds.y+bounds.height-48,100,32},NULL).clicked) panel->open=false;
    rohr_ui_modal_controls_end();
}

void editor_build_settings_panel_destroy(EditorBuildSettingsPanel *panel) {
    if(panel == NULL) return;
    editor_file_browser_destroy(&panel->browser);
    for(int i=0;i<EDITOR_PATH_COUNT;++i) {
        rohr_graphics_text_destroy(&panel->path_labels[i]); rohr_graphics_text_destroy(&panel->path_fields[i]);
        rohr_graphics_text_destroy(&panel->resolved_labels[i]);
    }
    rohr_graphics_text_destroy(&panel->browse_label); rohr_graphics_text_destroy(&panel->path_reset_label);
    rohr_graphics_text_destroy(&panel->browser_field);
    rohr_graphics_text_destroy(&panel->title_label);
    rohr_graphics_text_destroy(&panel->configure_label);
    rohr_graphics_text_destroy(&panel->compile_label);
    rohr_graphics_text_destroy(&panel->configure_field);
    rohr_graphics_text_destroy(&panel->compile_field);
    rohr_graphics_text_destroy(&panel->apply_label);
    rohr_graphics_text_destroy(&panel->cancel_label);
    rohr_graphics_text_destroy(&panel->reset_label);
    rohr_graphics_text_destroy(&panel->error_label);
    *panel = (EditorBuildSettingsPanel){0};
}
