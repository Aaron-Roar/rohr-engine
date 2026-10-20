/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/physics/editor_force.h"
#include "editor_history.h"
#include "editor_layout.h"
#include "panels/editor_scene_settings_panel.h"
#include <assert.h>

float editor_viewport_width = 900, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;

static Position screen(Position world) {
    return (Position){450 + world.x, 377 - world.y};
}
static void begin(const EditorProject *p, const EditorCommand *c, void *h) {
    editor_history_command_begin(h, p, c);
}
static void finish(const EditorCommand *c, const EditorCommandResult *r, void *h) {
    editor_history_command_finish(h, c, r);
}

static UIDropdownResult frame(EditorModeContext *context, const EditorSelectionRef *refs,
        size_t count, Position pointer, MouseButtonState button, bool blocked) {
    const TextAsset *options[12] = {0};
    editor_mode_reference_frame_begin(context->viewport, blocked);
    rohr_ui_input_set((UIInput){pointer, button});
    UIDropdownResult result = editor_mode_reference_dropdown("reference", options,
        count, 0, (UIRect){950, 50, 250, 28}, NULL, context, refs);
    if(context->viewport->reference_picker_open) assert(result.open);
    rohr_ui_update();
    return result;
}
static void open_picker(EditorModeContext *context, const EditorSelectionRef *refs, size_t count) {
    (void)frame(context, refs, count, (Position){960, 60}, MOUSE_BUTTON_STATE_PRESSED, false);
    assert(frame(context, refs, count, (Position){960, 60}, MOUSE_BUTTON_STATE_RELEASED, false).open);
}

static void geometry_and_interaction_check(EditorProject *p, EditorViewportState *state) {
    EditorObject *o = editor_project_selected_get(p);
    EditorRigidBody *a = editor_project_rigid_body_add(p, o);
    assert(a);
    uint32_t aid = a->id;
    EditorRigidBody *b = editor_project_rigid_body_add(p, o);
    assert(b);
    uint32_t bid = b->id;
    a = editor_project_rigid_body_get(o, aid);
    a->position = b->position = (Position){0};
    EditorSelectionRef refs[12] = {{0},
        {EDITOR_SELECTION_RIGID_BODY, o->id, 0, 0, aid},
        {EDITOR_SELECTION_RIGID_BODY, o->id, 0, 0, bid}};
    Position at = screen((Position){0});
    /* Draw order wins, even if the current editing selection points behind it. */
    state->selected_rigid_body = aid;
    assert(editor_viewport_reference_at_get(p, state, at, refs, 3) == 2);
    b->visible = false;
    assert(editor_viewport_reference_at_get(p, state, at, refs, 3) == 1);
    b->visible = true;
    /* An incompatible foreground body does not cover the allowed body. */
    assert(editor_viewport_reference_at_get(p, state, at, refs, 2) == 1);
    b->graphics_layer.value = -1;
    assert(editor_viewport_reference_at_get(p, state, at, refs, 3) == 1);
    b->graphics_layer.value = 0;
    o->visible = false;
    assert(editor_viewport_reference_at_get(p, state, at, refs, 3) == -1);
    o->visible = true;

    EditorModeContext context = {.project = p, .viewport = state};
    open_picker(&context, refs, 3);
    assert(frame(&context, refs, 3, (Position){20, 80}, MOUSE_BUTTON_STATE_PRESSED, false).open);
    assert(frame(&context, refs, 3, (Position){20, 80}, MOUSE_BUTTON_STATE_RELEASED, false).open);
    assert(!state->reference_picker_hovered);
    state->reference_pick_overlay = (UIRect){at.x - 10, at.y - 10, 20, 20};
    (void)frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_PRESSED, false);
    UIDropdownResult covered = frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_RELEASED, false);
    assert(covered.open && !covered.changed && !state->reference_picker_hovered);
    state->reference_pick_overlay = (UIRect){0};
    UIDropdownResult hover = frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_UP, false);
    assert(hover.hovered_index == 2 && state->reference_picker_hovered);
    assert(state->reference_preview.item == bid && state->selected_rigid_body == aid);
    (void)frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_PRESSED, false);
    UIDropdownResult result = frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_RELEASED, false);
    assert(result.changed && result.selected_index == 2 && !result.open);

    /* Releasing over a different target or empty space does not commit. */
    open_picker(&context, refs, 3);
    (void)frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_PRESSED, false);
    result = frame(&context, refs, 3, (Position){20, 80}, MOUSE_BUTTON_STATE_RELEASED, false);
    assert(result.open && !result.changed);
    SDL_Event escape = {.type = SDL_EVENT_KEY_DOWN}; escape.key.key = SDLK_ESCAPE;
    rohr_ui_event_add(&escape);
    result = frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_UP, false);
    assert(!result.open && !result.changed && !state->reference_picker_hovered);
    open_picker(&context, refs, 3);
    assert(!frame(&context, refs, 3, at, MOUSE_BUTTON_STATE_UP, true).open);

    /* Heading is not an option and ten options fit beneath it when space allows. */
    open_picker(&context, refs, 12);
    float hint_height = state->reference_hint.size.y + 12;
    assert(frame(&context, refs, 12, (Position){970, 85}, MOUSE_BUTTON_STATE_UP, false).hovered_index == -1);
    Position tenth = {970, 78 + hint_height + 9 * 28 + 10};
    assert(frame(&context, refs, 12, tenth, MOUSE_BUTTON_STATE_UP, false).hovered_index == 9);
    (void)frame(&context, refs, 12, (Position){1250, 700}, MOUSE_BUTTON_STATE_PRESSED, false);
    (void)frame(&context, refs, 12, (Position){1250, 700}, MOUSE_BUTTON_STATE_RELEASED, false);

    /* Nodes/beams remain selectable without entering the owning soft body. */
    EditorSoftBody *soft = editor_project_soft_body_add(p, o); assert(soft);
    EditorSoftNode *node = editor_project_soft_node_add(p, soft, (Position){150, 0}); assert(node);
    uint32_t n1 = node->id;
    node = editor_project_soft_node_add(p, soft, (Position){210, 0}); assert(node);
    EditorSoftBeam *beam = editor_project_soft_beam_add(p, soft, n1, node->id); assert(beam);
    refs[1] = (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE, o->id, soft->id, 0, n1};
    refs[2] = (EditorSelectionRef){EDITOR_SELECTION_SOFT_BEAM, o->id, soft->id, 0, beam->id};
    assert(editor_viewport_reference_at_get(p, state, screen((Position){150, 0}), refs, 3) == 1);
    assert(editor_viewport_reference_at_get(p, state, screen((Position){180, 0}), refs, 3) == 2);
    refs[1] = (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY, o->id, 0, 0, soft->id};
    assert(editor_viewport_reference_at_get(p, state, screen((Position){180, 0}), refs, 2) == 1);
    soft->visible = false;
    assert(editor_viewport_reference_at_get(p, state, screen((Position){180, 0}), refs, 3) == -1);
    EditorAnchor *anchor = editor_project_anchor_add(p, o, (Position){150, 100}, 0);
    assert(anchor);
    refs[1] = (EditorSelectionRef){EDITOR_SELECTION_ANCHOR, o->id, 0, 0, anchor->id};
    assert(editor_viewport_reference_at_get(p, state, screen((Position){150, 100}), refs, 2) == 1);
    refs[1].object += 1000;
    assert(editor_viewport_reference_at_get(p, state, screen((Position){150, 100}), refs, 2) == -1);
    refs[1] = (EditorSelectionRef){EDITOR_SELECTION_HITBOX, o->id, aid, 0, a->hitboxes[0].id};
    assert(editor_viewport_reference_at_get(p, state, at, refs, 2) == 1);
    b->standalone_particle = true; b->particle = true; b->particle_radius = 20;
    refs[1] = (EditorSelectionRef){EDITOR_SELECTION_PARTICLE, o->id, 0, 0, bid};
    assert(editor_viewport_reference_at_get(p, state, at, refs, 2) == 1);
    b->standalone_particle = false; b->particle = false;
}

static void force_frame(EditorForceEditor *editor, EditorModeContext *context,
        Position point, MouseButtonState button) {
    editor_mode_reference_frame_begin(context->viewport, false);
    rohr_ui_input_set((UIInput){point, button});
    (void)editor_force_editor_draw(editor, context);
    assert(!editor_viewport_update(context->viewport, context->project, point, button,
        MOUSE_BUTTON_STATE_UP, false, 0, true));
    rohr_ui_update();
}

static void force_history_check(EditorProject *p, EditorViewportState *state) {
    EditorObject *o = editor_project_selected_get(p);
    EditorForce *force = editor_project_force_add(p, o); assert(force);
    uint32_t force_id = force->id, body_id = o->rigid_bodies[1].id;
    state->mode = EDITOR_VIEWPORT_FORCE; state->selection = EDITOR_SELECTION_FORCE;
    state->selected_force = force_id;
    EditorHistory history = {0}; assert(editor_history_init(&history, p));
    editor_command_executing_callback_set(begin, &history);
    editor_command_finished_callback_set(finish, &history);
    FontAsset font = rohr_graphics_font_default_get();
    EditorForceEditor editor; assert(editor_force_editor_create(&editor, &font));
    editor_mode_accordion_section_expanded_set(&editor.visuals.outer, false);
    editor_mode_accordion_section_expanded_set(&editor.location_section, false);
    editor_mode_accordion_section_expanded_set(&editor.orientation_section, false);
    EditorModeContext context = {.project = p, .viewport = state, .history = &history,
        .x = 900, .width = 380};
    force_frame(&editor, &context, (Position){1100, 232}, MOUSE_BUTTON_STATE_PRESSED);
    force_frame(&editor, &context, (Position){1100, 232}, MOUSE_BUTTON_STATE_RELEASED);
    assert(state->reference_picker_open);
    force_frame(&editor, &context, screen((Position){0}), MOUSE_BUTTON_STATE_PRESSED);
    assert(history.undo_count == 0);
    force_frame(&editor, &context, screen((Position){0}), MOUSE_BUTTON_STATE_RELEASED);
    assert(!state->reference_picker_open && history.undo_count == 1);
    assert(force->description.target.kind == OBJECT_SOURCE_BODY && force->description.target.id == body_id);
    assert(state->selected_force == force_id && state->mode == EDITOR_VIEWPORT_FORCE);
    assert(!state->dragged_force && !state->group_dragging && !state->marquee_active);
    assert(editor_history_undo(&history));
    o = editor_project_selected_get(p);
    assert(o->force_items[0].description.target.kind == OBJECT_SOURCE_NONE);
    assert(editor_history_redo(&history));
    o = editor_project_selected_get(p);
    assert(o->force_items[0].description.target.id == body_id);
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    editor_force_editor_destroy(&editor); editor_history_destroy(&history);
}

static void composition_check(EditorProject *p, EditorViewportState *state) {
    EditorObject *object = editor_project_selected_get(p);
    EditorCamera *camera = editor_project_camera_add(p, object); assert(camera);
    uint32_t first_camera = camera->id;
    camera = editor_project_camera_add(p, object); assert(camera);
    uint32_t second_camera = camera->id;
    EditorLayoutViewport *layout = editor_project_layout_viewport_add(p); assert(layout);
    uint32_t layout_id = layout->id;
    layout->config.rectangle = (ViewportRectangle){0, 0, 200, 150};
    EditorViewportCameraItem *first = editor_viewport_camera_add(p, layout, object->id, first_camera);
    assert(first);
    uint32_t first_id = first->id;
    EditorViewportCameraItem *second = editor_viewport_camera_add(p, layout, object->id, second_camera);
    assert(second);
    first = &layout->camera_items[0]; assert(first->id == first_id);
    first->placement.rectangle = (ViewportRectangle){0, 0, 100, 100};
    first->placement.layer = 1;
    second->placement.rectangle = (ViewportRectangle){25, 25, 50, 50};
    second->placement.orientation = 35;
    second->placement.layer = 2;
    EditorViewportUiItem *ui = editor_viewport_ui_add(p, layout, EDITOR_VIEWPORT_UI_SHAPE);
    assert(ui); ui->position = (Position){30, 30}; ui->layer = 3;
    state->mode = EDITOR_VIEWPORT_LAYOUT_CAMERA_EDITOR;
    state->selected_layout_viewport = layout_id;
    EditorSelectionRef refs[] = {
        {EDITOR_SELECTION_CAMERA, object->id, 0, 0, first_camera},
        {EDITOR_SELECTION_CAMERA, object->id, 0, 0, second_camera}};
    Position overlap = {500, 427};
    EditorSelectionRef ordinary;
    assert(editor_viewport_selection_at_get(p, state, overlap, &ordinary));
    assert(ordinary.kind == EDITOR_SELECTION_UI_SHAPE);
    assert(editor_viewport_reference_at_get(p, state, overlap, refs, 2) == 1);
    /* The incompatible UI and camera source do not obscure an allowed source. */
    assert(editor_viewport_reference_at_get(p, state, overlap, refs, 1) == 0);
    second->placement.visible = false;
    assert(editor_viewport_reference_at_get(p, state, overlap, refs, 2) == 0);
    first->placement.visible = false;
    assert(editor_viewport_reference_at_get(p, state, overlap, refs, 2) == -1);
    first->placement.visible = second->placement.visible = true;

    /* The modal scene selector permits picking the exposed overview only. */
    state->mode = EDITOR_VIEWPORT_HIERARCHY;
    layout->overview_position = (Position){-300, 100};
    p->simulation_viewport = 0;
    FontAsset font = rohr_graphics_font_default_get();
    EditorSceneSettingsPanel panel = {0};
    assert(editor_scene_settings_panel_create(&panel, &font)); panel.open = true;
    UIRect bounds = {500, 50, 500, 300};
    Position target = {170, 290};
    for(unsigned step = 0; step < 4; ++step) {
        Position pointer = step < 2 ? (Position){660, 120} : target;
        MouseButtonState button = step % 2 ? MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_PRESSED;
        editor_mode_reference_frame_begin(state, false);
        rohr_ui_input_set((UIInput){pointer, button}); rohr_ui_modal_set(bounds);
        editor_scene_settings_panel_draw(&panel, p, NULL, state, bounds);
        if(step == 1) assert(state->reference_picker_open);
        if(step == 2) assert(state->reference_preview.kind == EDITOR_SELECTION_LAYOUT_VIEWPORT);
        rohr_ui_update();
    }
    assert(p->simulation_viewport == layout_id && !state->reference_picker_open);
    editor_scene_settings_panel_destroy(&panel);
}

int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    EditorProject project; editor_project_init(&project);
    assert(editor_project_object_add(&project, (Position){0}));
    project.viewport_local_view = false; project.viewport_camera_zoom = 1;
    project.viewport_camera_offset = (Position){0};
    EditorViewportState state = {0}; editor_viewport_state_init(&state);
    state.mode = EDITOR_VIEWPORT_OBJECT;
    geometry_and_interaction_check(&project, &state);
    force_history_check(&project, &state);
    composition_check(&project, &state);
    editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
