/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
/* Exercise the actual menu controller, including its pre-subprocess branches. */
#define main editor_gui_entry
#include "../src/main.c"
#undef main
#include <assert.h>

#define OK(call) do { EditorResult r = (call); if(editor_result_check(r)) { fprintf(stderr, "%s: %s\n", #call, r.result.error.message); assert(false); } } while(0)
static EditorProject project;
static EditorWorkspace workspace;
static EditorBuildRuntime runtime;
static EditorTerminalPanel terminal;
static EditorNotificationPanel notifications;
static char output[128 * 1024];
static size_t output_begin;
static const char *transcript(void) {
    size_t used = 0, lines = rohr_terminal_line_count_get(terminal.terminal);
    for(size_t i = output_begin; i < lines; ++i) {
        RohrTerminalLineView line = rohr_terminal_line_get(terminal.terminal, i);
        for(size_t j = 0; j < line.cell_count; ++j) {
            assert(used + 2 < sizeof(output));
            output[used++] = line.cells[j].codepoint < 128 && line.cells[j].codepoint ?
                (char)line.cells[j].codepoint : ' ';
        }
        output[used++] = '\n';
    }
    output[used] = 0; return output;
}
static void begin_output(void) {
    editor_build_output_write(&terminal, "\r\n", 2);
    output_begin = rohr_terminal_line_count_get(terminal.terminal) - 1;
}
static void contains(const char *text) {
    if(!strstr(transcript(), text)) { fprintf(stderr, "Missing %s in:\n%s\n", text, output); assert(false); }
}
static void save_file(const char *name, const char *text) {
    char path[4096]; snprintf(path, sizeof(path), "%s/%s", workspace.directory, name);
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fwrite(text, 1, strlen(text), file) == strlen(text)); assert(!fclose(file));
}
static void action(int index) {
    editor_build_request(&runtime, index, &terminal, &notifications);
    Uint64 deadline = SDL_GetTicks() + 30000;
    do {
        /* Both optional tree output and editor-operation logging are disabled. */
        editor_build_runtime_update(&runtime, &workspace, &project, &terminal, &notifications, false);
        assert(!terminal.visible && !terminal.focused);
        SDL_Delay(1);
    } while((runtime.pending || editor_build_active_check(runtime.build)) && SDL_GetTicks() < deadline);
    assert(!runtime.pending && !editor_build_active_check(runtime.build));
}
static void cmake_file(const char *extra) {
    char text[4096];
    snprintf(text, sizeof(text), "cmake_minimum_required(VERSION 3.20)\nproject(diagnostics C)\n"
        "add_executable(%s diagnostic_source.c)\n%s\n", workspace.config.name, extra);
    save_file("CMakeLists.txt", text);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    assert(editor_notification_panel_create(&notifications, &font, &font));
    char directory[EDITOR_WORKSPACE_PATH_MAX];
    snprintf(directory, sizeof(directory), "%s/run_%llu", argv[1],
        (unsigned long long)SDL_GetPerformanceCounter());
    OK(editor_workspace_create(&workspace, &project, directory));
    assert(editor_terminal_panel_project_open(&terminal, workspace.directory));
    /* Wider lines keep filenames and original messages searchable without wrapping. */
    rohr_terminal_destroy(terminal.terminal); terminal.terminal = NULL;
    RohrTerminalConfig terminal_config = rohr_terminal_config_default_get();
    terminal_config.working_directory = workspace.directory;
    terminal_config.columns = 2048;
    assert(rohr_terminal_create(&terminal.terminal, &terminal_config).success);
    assert(editor_scene_panel_create(&runtime.panel, &font, &terminal, &notifications));

    /* Deliberate generation failure before any build process, then recovery. */
    EditorJoint *joint = editor_project_joint_add(&project, &project.objects[0], EDITOR_JOINT_REVOLUTE);
    assert(joint); joint->anchor_a = UINT32_MAX; joint->anchor_b = 0;
    EditorResult expected = editor_workspace_editor_generate(&workspace, &project);
    assert(editor_result_check(expected));
    begin_output(); action(EDITOR_BUILD_ACTION_GENERATE);
    contains("[Generate App C] FAIL"); contains(expected.result.error.message);
    contains(workspace.directory); assert(!runtime.build);
    const EditorNotificationRecord *entry = editor_notification_store_newest_get(&notifications.store, 0);
    assert(entry && strstr(entry->detail, "terminal"));
    assert(editor_project_joint_remove(&project.objects[0], joint->id));
    begin_output(); action(EDITOR_BUILD_ACTION_GENERATE);
    contains("[Generate App C] SUCCESS"); contains("Terminal tree output is disabled.");

    static EditorConfigCommand configure, compile;
    OK(editor_config_command_expression_parse("{\"cmake\",\"-S\",\"{project}\",\"-B\",\"{build}\"}", &configure));
    OK(editor_config_command_expression_parse("{\"cmake\",\"--build\",\"{build}\",\"--target\",\"{target}\"}", &compile));
    OK(editor_config_gui_override_save(workspace.directory, &configure, &compile));
    cmake_file("message(FATAL_ERROR \"configuration_failure_marker\")");
    save_file("diagnostic_source.c", "#error compiler_failure_marker\nint main(void) { return 0; }\n");
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("[Configure] START"); contains("configuration_failure_marker"); contains("Configure command exited with status");
    assert(!strstr(transcript(), "[Compile] START"));
    cmake_file("message(STATUS \"configuration_success_marker\")");
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("[Configure] SUCCESS"); contains("[Compile] START");
    contains("diagnostic_source.c"); contains("compiler_failure_marker"); contains("Compile command exited with status");
    save_file("diagnostic_source.c", "int main(void) { return 0; }\n");
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("[Compile] SUCCESS"); contains("completed successfully");

    /* Missing scaffolding is restored even with custom commands, with output
     * retained in a hidden terminal. A path failure stops before the command. */
    char root_cmake[4096], scene_cmake[4096];
    snprintf(root_cmake, sizeof(root_cmake), "%s/CMakeLists.txt", workspace.directory);
    snprintf(scene_cmake, sizeof(scene_cmake), "%s/src/generated/project_scene.cmake", workspace.directory);
    assert(SDL_RemovePath(root_cmake) && SDL_RemovePath(scene_cmake));
    OK(editor_config_command_expression_parse("{\"cmake\",\"-E\",\"echo\",\"recovery_custom_configure\"}", &configure));
    OK(editor_config_command_expression_parse("{\"cmake\",\"-E\",\"echo\",\"{target}\"}", &compile));
    OK(editor_config_gui_override_save(workspace.directory, &configure, &compile));
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("[Build preparation] Restored"); contains("CMakeLists.txt"); contains("project_scene.cmake");
    contains("recovery_custom_configure"); contains("[Compile] SUCCESS");
    assert(SDL_RemovePath(root_cmake) && SDL_CreateDirectory(root_cmake));
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("Expected a CMake file"); contains(root_cmake);
    assert(!strstr(transcript(), "recovery_custom_configure"));
    assert(SDL_RemovePath(root_cmake)); cmake_file("");
    OK(editor_config_command_expression_parse("{\"cmake\",\"--build\",\"{build}\",\"--target\",\"{target}\"}", &compile));

    /* Settings parsing, command startup, and missing executable diagnostics. */
    save_file(".rohr/gui-overrides.lua", "this is not valid Lua !!\n");
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE); contains("[Build configuration] FAIL");
    OK(editor_config_command_expression_parse("{\"/missing/c1-configure-program\"}", &configure));
    OK(editor_config_gui_override_save(workspace.directory, &configure, &compile));
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("/missing/c1-configure-program"); contains("FAIL");
    /* This tiny fixture deliberately has no application at the usual output path. */
    char executable[4096]; snprintf(executable, sizeof(executable), "%s/build/%s%s",
        workspace.directory, workspace.config.name,
#ifdef _WIN32
        ".exe"
#else
        ""
#endif
    );
    (void)SDL_RemovePath(executable);
    begin_output(); editor_run_request(&runtime, &workspace, false, &terminal, &notifications);
    contains("[Run Application] FAIL"); contains("No built application executable");

    /* Cancel an active child, retain prior output, then successfully build again. */
    OK(editor_config_command_expression_parse("{\"cmake\",\"-E\",\"sleep\",\"10\"}", &configure));
    OK(editor_config_gui_override_save(workspace.directory, &configure, &compile));
    begin_output(); editor_build_request(&runtime, EDITOR_BUILD_ACTION_COMPILE, &terminal, &notifications);
    editor_build_runtime_update(&runtime, &workspace, &project, &terminal, &notifications, false);
    assert(editor_build_active_check(runtime.build));
    editor_build_runtime_cancel(&runtime); contains("[Configure] START"); contains("[Build] CANCELLED");
    OK(editor_config_command_expression_parse("{\"cmake\",\"-S\",\"{project}\",\"-B\",\"{build}\"}", &configure));
    OK(editor_config_gui_override_save(workspace.directory, &configure, &compile));
    begin_output(); action(EDITOR_BUILD_ACTION_COMPILE);
    contains("[Compile] SUCCESS"); assert(!strstr(transcript(), "CANCELLED"));
    assert(!terminal.visible && !terminal.focused);
    editor_terminal_panel_visible_toggle(&terminal); assert(terminal.visible);
    contains("[Compile] SUCCESS");
    editor_build_runtime_cancel(&runtime); editor_build_destroy(runtime.build);
    editor_scene_panel_destroy(&runtime.panel);
    editor_notification_panel_destroy(&notifications);
    editor_terminal_panel_project_close(&terminal);
    editor_project_destroy(&project);
    rohr_engine_stop();
    return 0;
}
