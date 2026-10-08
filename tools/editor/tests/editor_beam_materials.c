/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/soft_body/editor_soft_beam.h"
#include "editors/multi/editor_bulk_panel.h"
#include "editor_navigation.h"
#include "editor_workspace.h"
#include <editor_soft_area.h>
#include "yyjson/yyjson.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static void begin(const EditorProject *project, const EditorCommand *command, void *context) {
    editor_history_command_begin(context, project, command);
}
static void finish(const EditorCommand *command, const EditorCommandResult *result, void *context) {
    editor_history_command_finish(context, command, result);
}
static void panel(EditorSoftBeamEditor *ui, EditorModeContext *context,
        Position pointer, MouseButtonState button, float shift) {
    rohr_ui_frame_begin((UIInput){pointer, button});
    rohr_ui_clip_begin((UIRect){0, 0, 400, 720});
    rohr_ui_translation_y_push(shift);
    editor_mode_accordion_layout_measure_reset();
    (void)editor_soft_beam_editor_draw(ui, context);
    rohr_ui_translation_y_pop(); rohr_ui_clip_end(); rohr_ui_frame_end();
}
static void click(EditorSoftBeamEditor *ui, EditorModeContext *context, Position pointer) {
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_PRESSED, 0);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_RELEASED, 0);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_UP, 0);
}
static void number_scrolled(EditorSoftBeamEditor *ui, EditorModeContext *context,
        float y, const char *value, float shift) {
    Position pointer = {200, y + shift + 10};
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_PRESSED, shift);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_RELEASED, shift);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_UP, shift);
    SDL_Event key = {.type = SDL_EVENT_KEY_DOWN};
    key.key.key = SDLK_A; key.key.mod = SDL_KMOD_CTRL;
    rohr_ui_field_event_add(&key);
    key.key.mod = 0;
    for(const char *at = value; *at != '\0'; at += 1) {
        key.key.key = (SDL_Keycode)*at; rohr_ui_field_event_add(&key);
    }
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_UP, shift);
    rohr_ui_field_focus_clear();
}
static void number(EditorSoftBeamEditor *ui, EditorModeContext *context, float y, const char *value) {
    number_scrolled(ui, context, y, value, 0);
}
static void fixture_remove(const char *root) {
    const char *files[] = {"project.rohr.json", "objects/project.rohr.json", "src/main.c",
        "src/generated/project_objects.c", "src/generated/project_objects.h",
        "src/generated/project_viewports.c", "src/generated/project_viewports.h",
        "CMakeLists.txt", ".gitignore", "editor.lua", "assets/tutorial_frame_1.png",
        "assets/tutorial_frame_2.png", "src/generated", "src", "assets", "objects", ""};
    for(size_t i = 0; i < sizeof(files) / sizeof(files[0]); i += 1) {
        char path[2048]; snprintf(path, sizeof(path), "%s/%s", root, files[i]);
        (void)SDL_RemovePath(path);
    }
}

static EditorSoftBody *soft_get(EditorProject *project) {
    return &project->objects[0].soft_body_items[0];
}
static void scene_create(EditorProject *project) {
    EditorObject *object = editor_project_object_add(project, (Position){0});
    assert(object != NULL);
    snprintf(object->name, sizeof(object->name), "beam_window");
    EditorCamera *camera = editor_project_camera_add(project, object);
    assert(camera != NULL); camera->dimensions = (Scale){640, 480};
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(project);
    assert(viewport != NULL);
    viewport->config.rectangle = (ViewportRectangle){0, 0, 1280, 720};
    assert(editor_viewport_camera_add(project, viewport, object->id, camera->id) != NULL);
    EditorSoftBody *soft = editor_project_soft_body_add(project, object);
    assert(soft != NULL);
    Position points[] = {{-140,-140},{140,-140},{140,140},{-140,140},
        {-80,-80},{80,-80},{80,80},{-80,80}};
    EditorSoftNodeId ids[8];
    for(size_t i = 0; i < 8; i += 1) {
        EditorSoftNode *node = editor_project_soft_node_add(project, soft, points[i]);
        assert(node != NULL); ids[i] = node->id;
        node->node_mass = 10000; node->gravity_enabled = false;
        node->radius = 4; node->restitution = 1; node->friction = 0;
    }
    for(size_t i = 0; i < 8; i += 1) {
        EditorSoftBeam *beam = editor_project_soft_beam_add(project, soft, ids[i], ids[(i / 4) * 4 + (i + 1) % 4]);
        assert(beam != NULL && beam->friction == 0 && beam->restitution == .25f);
        beam->stiffness = 100; beam->damping = 1;
    }
    EditorSoftArea *area = editor_soft_area_add(soft); assert(area != NULL);
    EditorSoftHole *hole = editor_soft_hole_add(area); assert(hole != NULL);
    EditorSoftAreaLoop outer = {.node_count=4}, inner = {.node_count=4};
    memcpy(outer.nodes, ids, 4 * sizeof(ids[0]));
    memcpy(inner.nodes, ids + 4, 4 * sizeof(ids[0]));
    assert(editor_soft_area_loop_set(soft, area->id, 0, outer));
    assert(editor_soft_area_loop_set(soft, area->id, hole->id, inner));
    area->color = UINT32_C(0x306faaff);
    EditorRigidBody *particle = editor_project_rigid_body_add(project, object);
    assert(particle != NULL);
    snprintf(particle->name, sizeof(particle->name), "bouncing_particle");
    particle->particle = particle->standalone_particle = true;
    particle->particle_auto_fit = false; particle->particle_radius = 8;
    particle->gravity_enabled = false; particle->friction = 0; particle->restitution = 1;
    particle->initial_velocity = (Velocity){90, 120};
    assert(editor_project_particle_hitbox_sync(project, particle));
}
static void saved_materials_check(const char *state_path) {
    yyjson_doc *document = yyjson_read_file(state_path, 0, NULL, NULL); assert(document != NULL);
    yyjson_mut_doc *changed = yyjson_doc_mut_copy(document, NULL); assert(changed != NULL);
    yyjson_doc_free(document);
    yyjson_mut_val *object = yyjson_mut_arr_get(yyjson_mut_obj_get(yyjson_mut_doc_get_root(changed), "objects"), 0);
    yyjson_mut_val *soft = yyjson_mut_arr_get(yyjson_mut_obj_get(object, "soft_bodies"), 0);
    yyjson_mut_val *beam = yyjson_mut_arr_get(yyjson_mut_obj_get(soft, "beams"), 0);
    assert(beam != NULL);
    yyjson_mut_obj_remove_str(beam, "friction"); yyjson_mut_obj_remove_str(beam, "restitution");
    assert(yyjson_mut_write_file(state_path, changed, 0, NULL, NULL));
    EditorProject loaded; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded, state_path)));
    assert(soft_get(&loaded)->beams[0].friction == 0 && soft_get(&loaded)->beams[0].restitution == .25f);
    /* Invalid saved material rejects the load and preserves the existing project. */
    yyjson_mut_obj_add_real(changed, beam, "friction", -.1);
    assert(yyjson_mut_write_file(state_path, changed, 0, NULL, NULL));
    assert(editor_result_check(editor_project_load(&loaded, state_path)));
    assert(soft_get(&loaded)->beams[0].friction == 0);
    yyjson_mut_obj_remove_str(beam, "friction");
    yyjson_mut_obj_add_real(changed, beam, "restitution", 1.1);
    assert(yyjson_mut_write_file(state_path, changed, 0, NULL, NULL));
    assert(editor_result_check(editor_project_load(&loaded, state_path)));
    editor_project_destroy(&loaded); yyjson_mut_doc_free(changed);
}
int main(int argc, char **argv) {
    assert(!rohr_error_check(rohr_engine_start())); assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorSoftBeamEditor ui; assert(editor_soft_beam_editor_create(&ui, &font));
    assert(!ui.material_section.expanded);
    EditorProject project = {0}; EditorWorkspace workspace = {0};
    EditorViewportState state = {0}; editor_viewport_state_init(&state);
    char fixture[1024];
    snprintf(fixture, sizeof(fixture), "beam_materials_%llu", (unsigned long long)SDL_GetTicksNS());
    if(argc == 2) snprintf(fixture, sizeof(fixture), "%s", argv[1]);
    assert(!editor_result_check(editor_workspace_create(&workspace, &project, fixture)));
    editor_project_destroy(&project); editor_project_init(&project); scene_create(&project);
    EditorSoftBody *soft = soft_get(&project);
    EditorSelectionRef selection = {EDITOR_SELECTION_SOFT_BEAM, project.objects[0].id, soft->id, 0, soft->beams[0].id};
    assert(editor_viewport_selection_set(&project, &state, selection, false));
    state.mode = EDITOR_VIEWPORT_SOFT_BEAM;
    project.navigation = editor_navigation_state_get(&project, &state);
    EditorHistory history; assert(editor_history_init(&history, &project));
    editor_command_executing_callback_set(begin, &history); editor_command_finished_callback_set(finish, &history);
    EditorModeContext context = {.project=&project, .history=&history, .viewport=&state, .width=400};
    click(&ui, &context, (Position){100, 314}); assert(ui.material_section.expanded);
    number(&ui, &context, 340, ".75"); number_scrolled(&ui, &context, 372, "1", -160);
    assert(soft->beams[0].friction == .75f && soft->beams[0].restitution == 1);
    assert(editor_history_undo(&history)); soft = soft_get(&project);
    assert(soft->beams[0].restitution == .25f && state.mode == EDITOR_VIEWPORT_SOFT_BEAM);
    assert(editor_history_redo(&history)); soft = soft_get(&project);
    assert(soft->beams[0].restitution == 1);
    size_t before = history.undo_count;
    number(&ui, &context, 372, "2"); soft = soft_get(&project);
    assert(soft->beams[0].restitution == 1 && history.undo_count == before);
    number(&ui, &context, 340, "-1"); soft = soft_get(&project);
    assert(soft->beams[0].friction == .75f && history.undo_count == before);
    click(&ui, &context, (Position){100, ui.collision.header_bounds[0].y + 10});
    click(&ui, &context, (Position){100, ui.collision.header_bounds[1].y + 10});
    assert(ui.collision.open[0] && ui.collision.open[1]);
    assert(ui.collision.list_bounds[0].y == ui.collision.header_bounds[0].y + 32);
    assert(ui.collision.header_bounds[1].y == ui.collision.list_bounds[0].y + ui.collision.list_bounds[0].height);
    for(size_t i = 1; i < soft->beam_count; i += 1) {
        selection.item = soft->beams[i].id;
        assert(editor_viewport_selection_set(&project, &state, selection, true));
    }
    EditorBulkPanel bulk; assert(editor_bulk_panel_create(&bulk, &font));
    rohr_ui_frame_begin((UIInput){0});
    (void)editor_bulk_panel_draw(&bulk, &project, &state, &history, NULL, 0, 400, 700, NULL, NULL);
    rohr_ui_frame_end();
    assert(bulk.property_count == 6);
    EditorPropertySetCommand property = {.property=EDITOR_PROPERTY_RESTITUTION,
        .value_kind=EDITOR_PROPERTY_VALUE_FLOAT, .value.number=1};
    before = history.undo_count;
    assert(editor_bulk_property_set(&project, &state, &history, &property));
    assert(history.undo_count == before + 1);
    soft = soft_get(&project);
    for(size_t i = 0; i < soft->beam_count; i += 1) assert(soft->beams[i].restitution == 1);
    assert(editor_history_undo(&history)); assert(soft_get(&project)->beams[1].restitution == .25f);
    assert(editor_history_redo(&history));
    property.property = EDITOR_PROPERTY_FRICTION; property.value.number = 0;
    assert(editor_bulk_property_set(&project, &state, &history, &property));
    property.value.number = NAN;
    assert(!editor_bulk_property_set(&project, &state, &history, &property));
    soft = soft_get(&project);
    for(size_t i = 0; i < soft->beam_count; i += 1) assert(soft->beams[i].friction == 0);
    /* CLI parsing/execution addresses the same per-beam properties. */
    char *args[] = {"rohr-cli", "soft-beam", "set", "project.rohr.json", "1", "1", "1", "friction", ".5"};
    EditorCommand command; const char *path;
    assert(!editor_result_check(editor_command_cli_parse(9, args, &path, &command)));
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    soft = soft_get(&project); assert(soft->beams[0].friction == .5f);
    EditorSoftBody clone = {0}; assert(editor_project_soft_body_clone(&clone, soft));
    assert(clone.beams[0].friction == .5f && clone.beams[0].restitution == 1);
    editor_project_soft_body_destroy(&clone);
    assert(editor_workspace_save(&workspace, &project));
    char state_path[2048]; snprintf(state_path, sizeof(state_path), "%s/objects/project.rohr.json", fixture);
    EditorProject loaded; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded, state_path)));
    assert(soft_get(&loaded)->beams[0].friction == .5f && soft_get(&loaded)->beams[0].restitution == 1);
    assert(editor_workspace_c_generate(&workspace, &loaded));
    editor_project_destroy(&loaded);
    char source_path[2048]; snprintf(source_path, sizeof(source_path), "%s/src/generated/project_objects.c", fixture);
    char *source = SDL_LoadFile(source_path, NULL); assert(source != NULL);
    assert(strstr(source, "rohr_physics_friction_set(object->beam_1, 0.500000000f)") != NULL);
    assert(strstr(source, "rohr_physics_restitution_set(object->beam_1, 1.00000000f)") != NULL);
    SDL_free(source);
    saved_materials_check(state_path);
    assert(editor_workspace_save(&workspace, &project));
    editor_bulk_panel_destroy(&bulk); editor_soft_beam_editor_destroy(&ui);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    editor_viewport_assets_destroy();
    if(argc != 2) fixture_remove(fixture);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
