/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/physics/editor_particle.h"
#include "editor_navigation.h"
#include "editor_workspace.h"
#include "editor_mass_properties.h"
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
static void panel(EditorParticleEditor *ui, EditorModeContext *context,
        Position pointer, MouseButtonState button, float shift) {
    rohr_ui_frame_begin((UIInput){pointer, button});
    rohr_ui_clip_begin((UIRect){0, 0, 400, 720});
    rohr_ui_translation_y_push(shift);
    editor_mode_accordion_layout_measure_reset();
    (void)editor_particle_editor_draw(ui, context);
    rohr_ui_translation_y_pop(); rohr_ui_clip_end(); rohr_ui_frame_end();
}
static void click(EditorParticleEditor *ui, EditorModeContext *context, Position pointer) {
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_PRESSED, 0);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_RELEASED, 0);
    panel(ui, context, pointer, MOUSE_BUTTON_STATE_UP, 0);
}
static void number_scrolled(EditorParticleEditor *ui, EditorModeContext *context,
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
static void number(EditorParticleEditor *ui, EditorModeContext *context, float y, const char *value) {
    number_scrolled(ui, context, y, value, 0);
}
static void only_section(EditorParticleEditor *ui, EditorModeAccordionSection *section) {
    EditorModeAccordionSection *sections[] = {&ui->transform_section, &ui->initial_motion_section,
        &ui->physics_section, &ui->material_section, &ui->collision_section,
        &ui->parenting_section, &ui->appearance_section, &ui->geometry_section};
    for(size_t i = 0; i < 8; i += 1)
        editor_mode_accordion_section_expanded_set(sections[i], sections[i] == section);
}
static void values_check(const EditorRigidBody *body) {
    assert(body->standalone_particle && body->particle);
    assert(body->mass_value == 17 && body->friction == .25f && body->restitution == .75f);
    assert(body->position.x == 12 && body->position.y == -24);
    assert(body->initial_velocity.x == 3 && body->initial_velocity.y == -4);
    assert(body->initial_acceleration.x == 5 && body->initial_acceleration.y == -6);
    assert(body->rotation == 0 && body->initial_angular_velocity == 0);
    assert(!body->center_of_mass_explicit && body->static_body && !body->gravity_enabled);
    assert(body->particle_radius == 23 && body->particle_rigid_vertices == 12);
    assert(body->particle_origin.x == 7 && body->particle_origin.y == -8);
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
int main(int argc, char **argv) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorParticleEditor ui;
    assert(editor_particle_editor_create(&ui, &font));
    assert(ui.transform_section.expanded && !ui.physics_section.expanded);
    EditorProject project = {0}; EditorWorkspace workspace = {0};
    EditorViewportState state = {0}; EditorHistory history;
    editor_viewport_state_init(&state);
    char fixture[1024];
    snprintf(fixture, sizeof(fixture), "particle_authoring_%llu", (unsigned long long)SDL_GetTicksNS());
    if(argc == 2) snprintf(fixture, sizeof(fixture), "%s", argv[1]);
    assert(!editor_result_check(editor_workspace_create(&workspace, &project, fixture)));
    EditorObject *object = &project.objects[0];
    project.selected = object->id;
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    assert(body != NULL);
    body->particle = body->standalone_particle = true;
    body->particle_auto_fit = false;
    body->rotation = body->initial_angular_velocity = 0;
    body->gravity_enabled = true;
    snprintf(body->name, sizeof(body->name), "authored_particle");
    assert(editor_project_particle_hitbox_sync(&project, body));
    EditorRigidBodyId body_id = body->id;
    EditorObjectId object_id = object->id;
    state.selected_rigid_body = body_id;
    state.selection = EDITOR_SELECTION_PARTICLE; state.mode = EDITOR_VIEWPORT_PARTICLE;
    project.navigation = editor_navigation_state_get(&project, &state);
    assert(editor_history_init(&history, &project));
    editor_command_executing_callback_set(begin, &history);
    editor_command_finished_callback_set(finish, &history);
    EditorModeContext context = {.project=&project, .history=&history, .viewport=&state, .width=400};
    /* Newly opened particles support the same name focus routing as bodies. */
    assert(strcmp(editor_mode_name_field_id_get(state.mode), "editor.particle.name") == 0);
    assert(editor_mode_name_focus_request(&state));
    editor_mode_name_focus_apply(&state);
    panel(&ui, &context, (Position){0}, MOUSE_BUTTON_STATE_UP, 0);
    SDL_Event typed = {.type = SDL_EVENT_TEXT_INPUT};
    typed.text.text = "particle_final"; rohr_ui_field_event_add(&typed);
    panel(&ui, &context, (Position){0}, MOUSE_BUTTON_STATE_UP, 0);
    assert(strcmp(body->name, "particle_final") == 0);
    rohr_ui_field_focus_clear();
    number(&ui, &context, 154, "12");
    number(&ui, &context, 186, "-24");
    only_section(&ui, &ui.initial_motion_section);
    number(&ui, &context, 190, "3"); number(&ui, &context, 222, "-4");
    number(&ui, &context, 254, "5"); number(&ui, &context, 286, "-6");
    /* Expand Physics with a real click, then author mass and verify history. */
    only_section(&ui, NULL);
    panel(&ui, &context, (Position){0}, MOUSE_BUTTON_STATE_UP, 0);
    float collapsed_height = editor_mode_accordion_layout_measure_get();
    click(&ui, &context, (Position){100, 200});
    assert(ui.physics_section.expanded);
    assert(editor_mode_accordion_layout_measure_get() > collapsed_height);
    float old_mass = body->mass_value;
    size_t previous_undo = history.undo_count;
    number(&ui, &context, 260, "17");
    assert(body->mass_value == 17 && history.undo_count == previous_undo + 1);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->mass_value == old_mass);
    assert(editor_history_redo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->mass_value == 17 && state.mode == EDITOR_VIEWPORT_PARTICLE);
    previous_undo = history.undo_count;
    number(&ui, &context, 260, "-1");
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->mass_value == 17 && history.undo_count == previous_undo);
    click(&ui, &context, (Position){200, 238});
    click(&ui, &context, (Position){200, 290});
    assert(body->static_body);
    click(&ui, &context, (Position){200, 302});
    assert(!body->gravity_enabled);
    /* Standalone COM mode is a centroid readout, never an explicit dropdown. */
    click(&ui, &context, (Position){200, 338});
    click(&ui, &context, (Position){200, 390});
    assert(!body->center_of_mass_explicit && !editor_mass_properties_get(body).angular_response);
    only_section(&ui, &ui.material_section);
    number(&ui, &context, 262, ".25"); number(&ui, &context, 294, ".75");
    previous_undo = history.undo_count;
    number(&ui, &context, 294, "2");
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->restitution == .75f && history.undo_count == previous_undo);
    only_section(&ui, &ui.geometry_section);
    number(&ui, &context, 406, "23"); number(&ui, &context, 438, "12");
    previous_undo = history.undo_count;
    click(&ui, &context, (Position){350, 416});
    assert(body->particle_auto_fit && history.undo_count == previous_undo + 1);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(!body->particle_auto_fit && body->particle_radius == 23);
    assert(editor_history_redo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->particle_auto_fit);
    click(&ui, &context, (Position){350, 416});
    number(&ui, &context, 406, "23");
    number_scrolled(&ui, &context, 470, "7", -200);
    number(&ui, &context, 502, "-8");
    values_check(body);
    /* The dedicated radius editor remains available. */
    rohr_ui_frame_begin((UIInput){0});
    assert(!editor_particle_radius_editor_draw(&ui, &context));
    rohr_ui_frame_end();
    only_section(&ui, &ui.collision_section);
    panel(&ui, &context, (Position){0}, MOUSE_BUTTON_STATE_UP, 0);
    click(&ui, &context, (Position){100, ui.collision.header_bounds[0].y + 10});
    click(&ui, &context, (Position){100, ui.collision.header_bounds[1].y + 10});
    assert(ui.collision.open[0] && ui.collision.open[1]);
    assert(ui.collision.list_bounds[0].y == ui.collision.header_bounds[0].y + 32);
    assert(ui.collision.header_bounds[1].y == ui.collision.list_bounds[0].y + ui.collision.list_bounds[0].height);
    float expanded_height = editor_mode_accordion_layout_measure_get();
    panel(&ui, &context, (Position){0}, MOUSE_BUTTON_STATE_UP, -180);
    assert(editor_mode_accordion_layout_measure_get() == expanded_height);
    /* Parenting uses the normal entity dropdown and command history. */
    only_section(&ui, &ui.parenting_section);
    click(&ui, &context, (Position){200, 344});
    click(&ui, &context, (Position){200, 400});
    assert(body->parent == project.objects[0].rigid_bodies[0].id);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body_id);
    assert(body->parent == 0);
    /* Save/reload and generate from the UI-authored values, not a second fixture. */
    assert(editor_workspace_save(&workspace, &project));
    EditorProject loaded; EditorWorkspace loaded_workspace = {0}; editor_project_init(&loaded);
    assert(!editor_result_check(editor_workspace_load(&loaded_workspace, &loaded, fixture)));
    values_check(editor_project_rigid_body_get(&loaded.objects[0], body_id));
    assert(editor_workspace_c_generate(&loaded_workspace, &loaded));
    char source_path[2048]; snprintf(source_path, sizeof(source_path), "%s/src/generated/project_objects.c", fixture);
    char *source = SDL_LoadFile(source_path, NULL); assert(source != NULL);
    const char *config = strstr(source, "const ParticleConfig starter_particle_final_config");
    assert(config != NULL);
    const char *end = strstr(config, "};"); assert(end != NULL);
    const char *expected[] = {".mass_value = 17.0000000f", ".friction = 0.250000000f",
        ".restitution = 0.750000000f", ".position = {12.0000000f, -24.0000000f}",
        ".velocity = {3.00000000f, -4.00000000f}", ".acceleration = {5.00000000f, -6.00000000f}",
        ".local_origin = {7.00000000f, -8.00000000f}", ".static_body = true", ".gravity_enabled = false"};
    for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i += 1) {
        const char *match = strstr(config, expected[i]); assert(match != NULL && match < end);
    }
    SDL_free(source); editor_project_destroy(&loaded);
    /* Ordinary rigid bodies with particle properties share mass and retain rigid navigation. */
    body->standalone_particle = false;
    only_section(&ui, &ui.physics_section);
    number(&ui, &context, 260, "19");
    assert(body->mass_value == 19);
    assert(editor_project_rigid_body_get(&project.objects[0], body_id)->mass_value == 19);
    EditorSelectionRef selection = {EDITOR_SELECTION_PARTICLE, object_id, 0, 0, body_id};
    assert(editor_viewport_selection_set(&project, &state, selection, false));
    assert(editor_navigation_selected_open(&project, &state));
    assert(state.mode == EDITOR_VIEWPORT_RIGID_BODY);
    editor_command_executing_callback_set(NULL, NULL); editor_command_finished_callback_set(NULL, NULL);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    editor_particle_editor_destroy(&ui); editor_viewport_assets_destroy();
    if(argc != 2) fixture_remove(fixture);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
