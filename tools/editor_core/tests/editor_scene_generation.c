/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_workspace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OK(value) do { EditorResult r = (value); if(editor_result_check(r)) { fprintf(stderr, "%s\n", r.result.error.message); assert(false); } } while(0)
static char *read_file(const EditorWorkspace *workspace, const char *relative) {
    char path[2048]; SDL_snprintf(path, sizeof(path), "%s/%s", workspace->directory, relative);
    char *text = SDL_LoadFile(path, NULL); assert(text); return text;
}
static size_t occurrences(const char *text, const char *needle) {
    size_t count = 0; while((text = strstr(text, needle))) { ++count; text += strlen(needle); } return count;
}
static void check_generated(const EditorWorkspace *workspace, size_t objects, size_t screens, size_t viewports) {
    char *text = read_file(workspace, "src/generated/scene/project_objects.c");
    assert(occurrences(text, "entity_object_add(objects.") == objects); SDL_free(text);
    text = read_file(workspace, "src/generated/scene/project_viewports.c");
    assert(occurrences(text, "rohr_screen_create(") == screens);
    assert(occurrences(text, "rohr_viewport_create(") == viewports); SDL_free(text);
}
int main(int argc, char **argv) {
    assert(argc >= 2);
    static EditorProject project; EditorWorkspace workspace = {0};
    OK(editor_workspace_create(&workspace, &project, argv[1]));
    assert(project.object_count == 2 && project.layout_viewport_count == 1);
    EditorObject *starter = &project.objects[0], *tug = &project.objects[1];
    EditorObjectId starter_id = starter->id, tug_id = tug->id;
    EditorCameraId starter_camera = starter->cameras[0].id;
    EditorCamera *camera = editor_project_camera_add(&project, tug); assert(camera);
    SDL_strlcpy(camera->name, "tug_camera", sizeof(camera->name));
    camera->dimensions = (Scale){320, 240};
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&project); assert(viewport);
    SDL_strlcpy(viewport->name, "simulation_viewport", sizeof(viewport->name));
    EditorLayoutViewportId viewport_id = viewport->id;
    viewport->config.rectangle = (ViewportRectangle){20, 30, 600, 400};
    EditorViewportCameraItem *screen = editor_viewport_camera_add(&project, viewport, starter_id, starter_camera); assert(screen);
    screen->placement.rectangle = (ViewportRectangle){-20, 10, 320, 240};
    screen->placement.orientation = 12; screen->placement.layer = 0;
    screen = editor_viewport_camera_add(&project, viewport, tug_id, camera->id); assert(screen);
    screen->placement.rectangle = (ViewportRectangle){250, 100, 300, 220};
    screen->placement.orientation = -18; screen->placement.layer = 2;
    for(int i = 0; i < 2; ++i) {
        EditorViewportUiItem *ui = editor_viewport_ui_add(&project, viewport, EDITOR_VIEWPORT_UI_SHAPE); assert(ui);
        ui->position = (Position){40.0f + 80.0f * i, 40}; ui->layer = 1 + i * 2;
    }
    assert(editor_workspace_save(&workspace, &project));
    OK(editor_workspace_scene_generate(&workspace, &project, NULL));
    check_generated(&workspace, 2, 1, 1); /* Default uses the first, not both, viewports. */
    char *app_objects = read_file(&workspace, "src/generated/project_objects.c");
    char *app_viewports = read_file(&workspace, "src/generated/project_viewports.c");
    EditorSceneSelection selection = {.objects_selected = true, .objects = &tug_id, .object_count = 1, .viewport = viewport_id};
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 1, 1, 1);
    char *text = read_file(&workspace, "src/generated/project_objects.c"); assert(!strcmp(text, app_objects)); SDL_free(text);
    text = read_file(&workspace, "src/generated/project_viewports.c"); assert(!strcmp(text, app_viewports)); SDL_free(text);
    selection.viewport = project.layout_viewports[0].id;
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 1, 0, 1);
    selection.object_count = 0; selection.objects = NULL; selection.viewport = viewport_id;
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 0, 0, 1);
    selection.objects_selected = false;
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 2, 2, 1);
    char *before = read_file(&workspace, "src/generated/scene/project_objects.c");
    EditorObjectId invalid = UINT32_MAX;
    selection.objects_selected = true; selection.objects = &invalid; selection.object_count = 1;
    assert(editor_result_check(editor_workspace_scene_generate(&workspace, &project, &selection)));
    selection.objects = &starter_id; selection.viewport = UINT32_MAX;
    assert(editor_result_check(editor_workspace_scene_generate(&workspace, &project, &selection)));
    EditorObjectId duplicates[] = {starter_id, starter_id}; selection.objects = duplicates; selection.object_count = 2; selection.viewport = 0;
    assert(editor_result_check(editor_workspace_scene_generate(&workspace, &project, &selection)));
    text = read_file(&workspace, "src/generated/scene/project_objects.c"); assert(!strcmp(text, before)); SDL_free(text); SDL_free(before);
    size_t viewport_count = project.layout_viewport_count;
    project.layout_viewport_count = 0;
    selection = (EditorSceneSelection){0};
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 2, 0, 0);
    selection.objects_selected = true;
    OK(editor_workspace_scene_generate(&workspace, &project, &selection)); check_generated(&workspace, 0, 0, 0);
    project.layout_viewport_count = viewport_count;
    /* Leave an authored two-viewport fixture for installed-SDK/CLI probes. */
    selection = (EditorSceneSelection){.viewport = viewport_id};
    if(argc > 2 && !strcmp(argv[2], "tug")) selection = (EditorSceneSelection){.objects_selected = true, .objects = &tug_id, .object_count = 1};
    if(argc > 2 && !strcmp(argv[2], "empty")) selection = (EditorSceneSelection){.objects_selected = true, .viewport = viewport_id};
    if(argc > 2 && !strcmp(argv[2], "no_viewport")) {
        project.layout_viewport_count = 0;
        selection = (EditorSceneSelection){.objects_selected = true};
    }
    OK(editor_workspace_scene_generate(&workspace, &project, &selection));
    project.layout_viewport_count = viewport_count;
    SDL_free(app_objects); SDL_free(app_viewports); editor_project_destroy(&project); return 0;
}
