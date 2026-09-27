/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "browser/editor_file_browser.h"
#include "editor_navigation.h"
#include "editor_layout.h"
#include "editors/editor_mode_controls.h"
#include "editors/multi/editor_bulk_panel.h"
#include "panels/editor_input_settings_panel.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

float editor_viewport_width = WINDOW_WIDTH * 0.8f;
float editor_window_width = WINDOW_WIDTH;
float editor_window_height = WINDOW_HEIGHT;
float editor_viewport_bottom = WINDOW_HEIGHT;

static bool accordion_layout_metrics_check(void) {
    const float fixed_rows[] = {28.0f, 28.0f, 48.0f};
    const float dynamic_rows[] = {28.0f, 86.0f};
    EditorModeAccordionLayoutMetrics collapsed =
        editor_mode_accordion_layout_metrics_get(
            100.0f, 6.0f, 6.0f, fixed_rows, 3, 10.0f, false);
    EditorModeAccordionLayoutMetrics expanded =
        editor_mode_accordion_layout_metrics_get(
            100.0f, 6.0f, 6.0f, fixed_rows, 3, 10.0f, true);
    EditorModeAccordionLayoutMetrics dynamic =
        editor_mode_accordion_layout_metrics_get(
            expanded.next_y, 6.0f, 6.0f, dynamic_rows, 2, 10.0f, true);
    EditorModeAccordionLayoutResult section = {
        .content_y = expanded.content_y,
        .content_height = expanded.content_height,
        .expanded = true};
    const EditorModeAccordionLayoutGroup groups[] = {
        {.row_count = 2, .row_height = 28.0f, .row_gap = 10.0f},
        {.row_count = 4, .row_height = 26.0f, .row_gap = 4.0f,
            .gap_before = 8.0f}};
    editor_mode_accordion_layout_measure_reset();
    editor_mode_accordion_layout_measure_include(321.0f);
    return fabsf(collapsed.content_y - 136.0f) < 0.001f &&
        fabsf(collapsed.content_height - 136.0f) < 0.001f &&
        fabsf(collapsed.next_y - 136.0f) < 0.001f &&
        fabsf(expanded.next_y - 272.0f) < 0.001f &&
        fabsf(dynamic.content_height - 136.0f) < 0.001f &&
        fabsf(dynamic.next_y - 444.0f) < 0.001f &&
        fabsf(editor_mode_accordion_layout_groups_height_get(groups, 2) -
            190.0f) < 0.001f &&
        fabsf(editor_mode_accordion_layout_group_row_y(
            &section, groups, 2, 0, 1) - 174.0f) < 0.001f &&
        fabsf(editor_mode_accordion_layout_group_row_y(
            &section, groups, 2, 1, 3) - 300.0f) < 0.001f &&
        fabsf(editor_mode_accordion_layout_row_y(
            &section, fixed_rows, 2, 10.0f) - 212.0f) < 0.001f &&
        fabsf(editor_mode_accordion_layout_measure_get() - 321.0f) < 0.001f;
}

static bool created_name_focus_mapping_check(void) {
    const EditorViewportMode modes[] = {
        EDITOR_VIEWPORT_OBJECT, EDITOR_VIEWPORT_RIGID_BODY, EDITOR_VIEWPORT_HITBOX,
        EDITOR_VIEWPORT_JOINT, EDITOR_VIEWPORT_ANCHOR,
        EDITOR_VIEWPORT_SOFT_BODY, EDITOR_VIEWPORT_SOFT_NODE,
        EDITOR_VIEWPORT_SOFT_BEAM, EDITOR_VIEWPORT_SOFT_AREA,
        EDITOR_VIEWPORT_LINE, EDITOR_VIEWPORT_VERTEX,
        EDITOR_VIEWPORT_SPRITE, EDITOR_VIEWPORT_ANIMATED_SPRITE,
        EDITOR_VIEWPORT_ANIMATION_FRAME, EDITOR_VIEWPORT_CAMERA_ENTITY,
        EDITOR_VIEWPORT_LAYOUT, EDITOR_VIEWPORT_LAYOUT_CAMERA_EDITOR,
        EDITOR_VIEWPORT_UI_SHAPE_EDITOR, EDITOR_VIEWPORT_UI_TEXT_EDITOR,
        EDITOR_VIEWPORT_UI_SLIDER_EDITOR,
        EDITOR_VIEWPORT_INPUT_CONTROLLER, EDITOR_VIEWPORT_INPUT_ACTION,
        EDITOR_VIEWPORT_INPUT_BINDING};
    EditorViewportState state = {0};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i += 1)
        if(editor_mode_name_field_id_get(modes[i]) == NULL) return false;
    if(editor_mode_name_field_id_get(EDITOR_VIEWPORT_HIERARCHY) != NULL ||
            editor_mode_name_field_id_get(EDITOR_VIEWPORT_PARTICLE) != NULL)
        return false;
    state.mode = EDITOR_VIEWPORT_INPUT_ACTION;
    if(!editor_mode_name_focus_request(&state) ||
            !editor_mode_name_focus_pending_check(&state) ||
            state.name_focus_mode != EDITOR_VIEWPORT_INPUT_ACTION) return false;
    state.mode = EDITOR_VIEWPORT_INPUT_CONTROLLER;
    return !editor_mode_name_focus_pending_check(&state);
}

static bool created_name_focus_replacement_check(void) {
    EditorViewportState state = {.mode = EDITOR_VIEWPORT_INPUT_ACTION};
    char name[32] = "action_1";
    SDL_Event typed = {0};
    UIFieldBinding binding = {.kind = UI_FIELD_STRING, .string = name,
        .string_capacity = sizeof(name)};
    UIRect bounds = {0.0f, 0.0f, 100.0f, 30.0f};
    if(!editor_mode_name_focus_request(&state)) return false;
    editor_mode_name_focus_apply(&state);
    rohr_ui_frame_begin((UIInput){0});
    if(!editor_mode_field("editor.input.action.name", binding, NULL,
            bounds, NULL).active) return false;
    rohr_ui_frame_end();
    typed.type = SDL_EVENT_TEXT_INPUT;
    typed.text.text = "j";
    rohr_ui_field_event_add(&typed);
    rohr_ui_frame_begin((UIInput){0});
    if(!editor_mode_field("editor.input.action.name", binding, NULL,
            bounds, NULL).changed || strcmp(name, "j") != 0) return false;
    rohr_ui_frame_end();
    rohr_ui_field_focus_clear();
    return true;
}

static bool input_key_capture_check(void) {
    InputBinding binding = {0};
    SDL_Scancode pending = SDL_SCANCODE_UNKNOWN;
    if(editor_input_key_capture_apply(SDL_SCANCODE_LCTRL,
            SDL_SCANCODE_UNKNOWN, SDL_KMOD_LCTRL, &pending, &binding) ||
            pending != SDL_SCANCODE_LCTRL) return false;
    if(!editor_input_key_capture_apply(SDL_SCANCODE_A,
            SDL_SCANCODE_UNKNOWN, SDL_KMOD_LCTRL, &pending, &binding) ||
            binding.source != INPUT_BINDING_KEY ||
            binding.input.key != SDL_SCANCODE_A ||
            binding.modifiers != SDL_KMOD_LCTRL ||
            pending != SDL_SCANCODE_UNKNOWN) return false;
    if(editor_input_key_capture_apply(SDL_SCANCODE_LSHIFT,
            SDL_SCANCODE_UNKNOWN, SDL_KMOD_LSHIFT, &pending, &binding) ||
            !editor_input_key_capture_apply(SDL_SCANCODE_UNKNOWN,
                SDL_SCANCODE_LSHIFT, SDL_KMOD_NONE, &pending, &binding) ||
            binding.input.key != SDL_SCANCODE_LSHIFT ||
            binding.modifiers != SDL_KMOD_NONE) return false;
    return editor_input_key_capture_apply(SDL_SCANCODE_7,
        SDL_SCANCODE_UNKNOWN, SDL_KMOD_NONE, &pending, &binding) &&
        binding.input.key == SDL_SCANCODE_7;
}

static bool screen_rotation_pointer_check(float width, float height, float zoom) {
    EditorProject project;
    EditorViewportState state = {0};
    EditorObject *object;
    EditorCamera *camera;
    EditorLayoutViewport *viewport;
    EditorViewportCameraItem *screen;
    Position view_center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
        EDITOR_MENU_HEIGHT +
            (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
    Position screen_center;
    float arm;
    Position handle;
    Position target;
    bool result = false;
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    object = editor_project_object_add(&project, (Position){0});
    camera = editor_project_camera_add(&project, object);
    viewport = editor_project_layout_viewport_add(&project);
    screen = object == NULL || camera == NULL || viewport == NULL ? NULL :
        editor_viewport_camera_add(&project, viewport, object->id, camera->id);
    if(screen == NULL) goto done;
    screen->placement.rectangle = (ViewportRectangle){0.0f, 50.0f,
        width, height};
    project.viewport_camera_zoom = zoom;
    project.viewport_camera_offset = (Vec2D){0};
    state.mode = EDITOR_VIEWPORT_LAYOUT_CAMERA_EDITOR;
    state.selected_layout_viewport = viewport->id;
    state.selected_viewport_camera_item = screen->id;
    screen_center = (Position){width * 0.5f, 50.0f + height * 0.5f};
    arm = height * 0.5f + 30.0f / zoom;
    handle = (Position){view_center.x +
            (viewport->config.rectangle.x + screen_center.x) * zoom,
        view_center.y + (viewport->config.rectangle.y + screen_center.y - arm) * zoom};
    if(!editor_viewport_update(&state, &project, handle,
            MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0.0f,
            false) || !state.rotated_viewport_item ||
            editor_viewport_transform_active_check(&state)) goto done;
    target = (Position){view_center.x +
            (viewport->config.rectangle.x + screen_center.x + arm) * zoom,
        view_center.y + (viewport->config.rectangle.y + screen_center.y) * zoom};
    if(!editor_viewport_update(&state, &project, target,
            MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP, false, 0.0f,
            false) || fabsf(screen->placement.orientation - 90.0f) >
                0.001f) goto done;
    for(int step = 2; step <= 9; step += 1) {
        float radians = math_degrees_to_radians(step * 90.0f);
        target = (Position){view_center.x + (viewport->config.rectangle.x +
                screen_center.x + sinf(radians) * arm) * zoom,
            view_center.y + (viewport->config.rectangle.y + screen_center.y -
                cosf(radians) * arm) * zoom};
        if(!editor_viewport_update(&state, &project, target,
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP, false, 0, false) ||
                fabsf(screen->placement.orientation - step * 90.0f) > 0.001f) goto done;
    }
    result = true;
done:
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    return result;
}

static bool hitbox_vertex_zoom_pick_check(void) {
    EditorProject project;
    EditorViewportState state = {0};
    EditorObject *object;
    EditorRigidBody *body;
    EditorHitbox *hitbox;
    Position view_center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
        EDITOR_MENU_HEIGHT +
            (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
    Position vertex;
    Position pointer;
    bool result = false;

    editor_project_init(&project);
    editor_viewport_state_init(&state);
    object = editor_project_object_add(&project, (Position){0});
    body = object == NULL ? NULL : editor_project_rigid_body_add(&project, object);
    hitbox = body == NULL || body->hitbox_count == 0 ? NULL : &body->hitboxes[0];
    if(hitbox == NULL || hitbox->vertex_count == 0) goto done;
    project.viewport_camera_zoom = 0.1f;
    project.viewport_camera_offset = (Vec2D){0};
    project.viewport_local_view = false;
    state.mode = EDITOR_VIEWPORT_HITBOX;
    state.selection = EDITOR_SELECTION_HITBOX;
    state.selected_rigid_body = body->id;
    state.selected_hitbox = hitbox->id;
    vertex = hitbox->vertices[0].position;
    pointer = (Position){view_center.x + vertex.x * project.viewport_camera_zoom +
            9.0f,
        view_center.y - vertex.y * project.viewport_camera_zoom};
    if(!editor_viewport_update(&state, &project, pointer,
            MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false) || state.mode != EDITOR_VIEWPORT_VERTEX ||
            state.selection != EDITOR_SELECTION_VERTEX ||
            state.selected_vertex != 0) goto done;
    result = true;
done:
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    return result;
}

static bool hitbox_line_zoom_pick_check(void) {
    const float zooms[] = {0.1f, 0.5f, 1.0f, 2.0f, 4.0f};
    const float offsets[] = {-6.5f, -5.5f, 5.5f, 6.5f};
    Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
    for(size_t z = 0; z < sizeof(zooms) / sizeof(zooms[0]); z += 1) {
        for(int rotated = 0; rotated < 2; rotated += 1) {
            for(size_t p = 0; p < sizeof(offsets) / sizeof(offsets[0]); p += 1) {
                EditorProject project;
                EditorViewportState state;
                bool selected = false;
                editor_project_init(&project);
                editor_viewport_state_init(&state);
                EditorObject *object = editor_project_object_add(&project, (Position){0});
                EditorRigidBody *body = object == NULL ? NULL :
                    editor_project_rigid_body_add(&project, object);
                if(body == NULL) {
                    editor_viewport_state_destroy(&state);
                    editor_project_destroy(&project);
                    return false;
                }
                EditorHitbox *box = &body->hitboxes[0];
                /* Keep endpoints well clear of the pointer in screen space. */
                box->vertices[0].position = (Position){-100 / zooms[z], -60 / zooms[z]};
                box->vertices[1].position = (Position){100 / zooms[z], -60 / zooms[z]};
                box->vertices[2].position = (Position){0, 100 / zooms[z]};
                body->rotation = rotated ? 45.0f : 0.0f;
                project.viewport_camera_zoom = zooms[z];
                project.viewport_camera_offset = (Vec2D){0};
                project.viewport_local_view = false;
                state.mode = EDITOR_VIEWPORT_HITBOX;
                state.selection = EDITOR_SELECTION_HITBOX;
                state.selected_rigid_body = body->id;
                state.selected_hitbox = box->id;
                Vec2D screen_offset = math_vector_rotate(
                    (Vec2D){0, -60 + offsets[p]}, body->rotation);
                Position pointer = {center.x + screen_offset.x, center.y - screen_offset.y};
                (void)editor_viewport_update(&state, &project, pointer,
                    MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false);
                selected = state.selection == EDITOR_SELECTION_LINE &&
                    state.mode == EDITOR_VIEWPORT_LINE && state.selected_line == 0;
                (void)editor_viewport_update(&state, &project, pointer,
                    MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP, false, 0, false);
                editor_viewport_state_destroy(&state);
                editor_project_destroy(&project);
                if(selected != (fabsf(offsets[p]) < 6.0f)) {
                    fprintf(stderr, "line pick mismatch: zoom %g rotation %d offset %g\n",
                        zooms[z], rotated * 45, offsets[p]);
                    return false;
                }
            }
        }
    }
    return true;
}

static bool navigation_mode_open_check(EditorProject *project,
        EditorViewportState *state, EditorHierarchySelection selection,
        EditorViewportMode expected) {
    state->selection = selection;
    state->mode = EDITOR_VIEWPORT_HIERARCHY;
    return editor_navigation_selected_open(project, state) && state->mode == expected;
}

static bool modifier_click_toggle_check(EditorProject *project,
        EditorViewportState *state, EditorSelectionRef fallback,
        EditorSelectionRef clicked) {
    editor_viewport_selection_clear(state);
    if(!editor_viewport_selection_set(project, state, fallback, false) ||
            !editor_viewport_selection_set(project, state, clicked, true) ||
            state->selected_item_count != 2 ||
            !editor_viewport_selection_contains(state, clicked) ||
            !editor_viewport_selection_set(project, state, clicked, true) ||
            state->selected_item_count != 1 ||
            editor_viewport_selection_contains(state, clicked)) return false;
    editor_viewport_selection_clear(state);
    if(!editor_viewport_selection_set(project, state, clicked, false) ||
            !editor_viewport_selection_set(project, state, clicked, true) ||
            state->selected_item_count != 0 ||
            state->selection != EDITOR_SELECTION_NONE) return false;
    return true;
}


static bool picking_pointer_update(EditorProject *project, EditorViewportState *state,
        Position pointer, MouseButtonState button) {
    return editor_viewport_update(state, project, pointer, button,
        MOUSE_BUTTON_STATE_UP, false, 0.0f, false);
}

#define PICK_REQUIRE(condition) do { if(!(condition)) { \
    fprintf(stderr, "render-order picking failed at line %d: %s\n", \
        __LINE__, #condition); return false; } } while(0)

static bool render_order_picking_check(void) {
    EditorProject project;
    EditorViewportState state;
    EditorSelectionRef hit;
    EditorObject *object;
    EditorRigidBody *back;
    EditorRigidBody *front;
    EditorHistory history;
    Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
    Position point = {center.x + 25.0f, center.y};
    Position moved = {point.x + 15.0f, point.y};
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    object = editor_project_object_add(&project, (Position){0});
    PICK_REQUIRE(object != NULL);
    PICK_REQUIRE(editor_project_rigid_body_add(&project, object) != NULL);
    PICK_REQUIRE(editor_project_rigid_body_add(&project, object) != NULL);
    back = &object->rigid_bodies[0];
    front = &object->rigid_bodies[1];
    for(size_t i = 0; i < 2; i += 1) {
        EditorHitbox *box = &object->rigid_bodies[i].hitboxes[0];
        box->vertex_count = 4;
        const Position vertices[] = {{-40, -40}, {40, -40}, {40, 40}, {-40, 40}};
        for(size_t j = 0; j < 4; j += 1) {
            box->vertices[j].position = vertices[j];
            box->vertices[j].id = project.next_vertex_id++;
        }
    }
    /* An already selected body and each of its child modes lose to the
     * same foreground body, including at a covered edge and vertex. */
    const EditorViewportMode modes[] = {EDITOR_VIEWPORT_OBJECT,
        EDITOR_VIEWPORT_RIGID_BODY, EDITOR_VIEWPORT_HITBOX,
        EDITOR_VIEWPORT_LINE, EDITOR_VIEWPORT_VERTEX, EDITOR_VIEWPORT_ORIGIN};
    for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i += 1) {
        state.mode = modes[i];
        state.selection = EDITOR_SELECTION_RIGID_BODY;
        state.selected_rigid_body = back->id;
        state.selected_hitbox = back->hitboxes[0].id;
        state.selected_origin_kind = EDITOR_ORIGIN_RIGID_BODY;
        Position grab = i == 4 ? (Position){center.x + 40, center.y - 40} :
            i == 5 ? center : point;
        PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, grab, &hit));
        PICK_REQUIRE(hit.kind == EDITOR_SELECTION_RIGID_BODY && hit.item == front->id);
        PICK_REQUIRE(picking_pointer_update(&project, &state, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        PICK_REQUIRE(state.selected_rigid_body == front->id &&
            state.mode == EDITOR_VIEWPORT_RIGID_BODY && state.dragged_body &&
            state.dragged_vertex < 0 && !state.dragged_origin);
        (void)picking_pointer_update(&project, &state, grab, MOUSE_BUTTON_STATE_RELEASED);
    }
    /* Captured dragging remains a single undoable interaction. */
    state.mode = EDITOR_VIEWPORT_OBJECT;
    state.selected_rigid_body = back->id;
    PICK_REQUIRE(editor_history_init(&history, &project));
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(editor_navigation_viewport_transform_history_update(
        &project, &state, &history, false));
    PICK_REQUIRE(picking_pointer_update(&project, &state, moved, MOUSE_BUTTON_STATE_DOWN));
    PICK_REQUIRE(editor_navigation_viewport_transform_history_update(
        &project, &state, &history, true));
    PICK_REQUIRE(front->position.x == 15.0f && back->position.x == 0.0f);
    (void)picking_pointer_update(&project, &state, moved, MOUSE_BUTTON_STATE_RELEASED);
    PICK_REQUIRE(editor_navigation_viewport_transform_history_update(
        &project, &state, &history, true));
    PICK_REQUIRE(history.undo_count == 1 && editor_history_undo(&history));
    object = &project.objects[0];
    back = &object->rigid_bodies[0];
    front = &object->rigid_bodies[1];
    PICK_REQUIRE(front->position.x == 0.0f && editor_history_redo(&history));
    object = &project.objects[0];
    back = &object->rigid_bodies[0];
    front = &object->rigid_bodies[1];
    PICK_REQUIRE(front->position.x == 15.0f);
    editor_history_destroy(&history);
    front->position.x = 0.0f;

    /* Hiding and reordering expose a different winner immediately. */
    front->visible = false;
    state.mode = EDITOR_VIEWPORT_OBJECT;
    state.selected_rigid_body = front->id;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.item == back->id);
    front->visible = true;
    EditorRigidBody swap = *back;
    *back = *front;
    *front = swap;
    state.selected_rigid_body = back->id;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.item == front->id);
    /* A sibling hitbox also covers the selected hitbox's child controls. */
    PICK_REQUIRE(editor_project_hitbox_add(&project, front) != NULL);
    EditorHitbox *top_box = &front->hitboxes[1];
    top_box->vertex_count = 4;
    for(size_t i = 0; i < 4; i += 1)
        top_box->vertices[i].position = front->hitboxes[0].vertices[i].position;
    state.mode = EDITOR_VIEWPORT_VERTEX;
    state.selected_rigid_body = front->id;
    state.selected_hitbox = front->hitboxes[0].id;
    PICK_REQUIRE(picking_pointer_update(&project, &state,
        (Position){center.x + 40, center.y - 40}, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.mode == EDITOR_VIEWPORT_HITBOX &&
        state.selected_hitbox == top_box->id && state.dragged_vertex < 0);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);

    /* A higher content layer wins over rigid-body selection and origin controls. */
    EditorSprite *sprite = editor_project_sprite_add(&project, object,
        "cover", "unused.png");
    PICK_REQUIRE(sprite != NULL);
    sprite->size = (Scale){100, 100};
    state.mode = EDITOR_VIEWPORT_ORIGIN;
    state.selection = EDITOR_SELECTION_ORIGIN;
    state.selected_origin_kind = EDITOR_ORIGIN_RIGID_BODY;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, center, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_SPRITE && hit.item == sprite->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, center, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selection == EDITOR_SELECTION_SPRITE && state.dragged_sprite);
    (void)picking_pointer_update(&project, &state, center, MOUSE_BUTTON_STATE_RELEASED);
    sprite->visible = false;

    /* Unselected foreground geometry cannot start a background group drag. */
    EditorSelectionRef group_back = {EDITOR_SELECTION_RIGID_BODY, object->id,
        0, 0, back->id};
    EditorSelectionRef group_sprite = {EDITOR_SELECTION_SPRITE, object->id,
        0, 0, sprite->id};
    PICK_REQUIRE(editor_viewport_selection_set(&project, &state, group_back, false));
    PICK_REQUIRE(editor_viewport_selection_set(&project, &state, group_sprite, true));
    state.mode = EDITOR_VIEWPORT_OBJECT;
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(!state.group_dragging && state.selected_rigid_body == front->id);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);

    /* Modifier presses obey the same winner and never toggle the obscured body. */
    editor_viewport_selection_clear(&state);
    state.mode = EDITOR_VIEWPORT_OBJECT;
    state.selection = EDITOR_SELECTION_NONE;
    state.selection_modifier = true;
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selected_item_count == 1 &&
        state.selected_items[0].item == front->id);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    state.selection_modifier = false;
    editor_viewport_selection_clear(&state);

    /* Particle fills outside polygon geometry select their owner, while
     * standalone particles retain their restricted editor mode. */
    front->particle = true;
    front->particle_radius = 80.0f;
    Position particle_point = {center.x + 60, center.y};
    state.mode = EDITOR_VIEWPORT_OBJECT;
    state.selection = EDITOR_SELECTION_NONE;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, particle_point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_RIGID_BODY && hit.item == front->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, particle_point,
        MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.mode == EDITOR_VIEWPORT_RIGID_BODY && state.dragged_body);
    (void)picking_pointer_update(&project, &state, particle_point, MOUSE_BUTTON_STATE_RELEASED);
    front->standalone_particle = true;
    state.mode = EDITOR_VIEWPORT_OBJECT;
    state.selection = EDITOR_SELECTION_NONE;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, particle_point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_PARTICLE && hit.item == front->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, particle_point,
        MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.mode == EDITOR_VIEWPORT_PARTICLE && state.dragged_body);
    (void)picking_pointer_update(&project, &state, particle_point, MOUSE_BUTTON_STATE_RELEASED);
    front->particle = false;
    front->standalone_particle = false;

    /* Soft nodes and anchors use their rendered layers, including sibling order. */
    EditorSoftBody *soft = editor_project_soft_body_add(&project, object);
    PICK_REQUIRE(soft != NULL);
    PICK_REQUIRE(editor_project_soft_node_add(&project, soft, (Position){25, 0}));
    PICK_REQUIRE(editor_project_soft_node_add(&project, soft, (Position){25, 0}));
    state.mode = EDITOR_VIEWPORT_SOFT_NODE;
    state.selection = EDITOR_SELECTION_SOFT_NODE;
    state.selected_soft_body = soft->id;
    state.selected_soft_node = soft->nodes[0].id;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_SOFT_NODE && hit.item == soft->nodes[1].id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selected_soft_node == soft->nodes[1].id && state.dragged_soft_node);
    editor_viewport_transform_cancel(&state);
    PICK_REQUIRE(!editor_viewport_transform_active_check(&state));
    EditorAnchor *anchor = editor_project_anchor_add(&project, object,
        (Position){25, 0}, 0);
    PICK_REQUIRE(anchor != NULL);
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_ANCHOR && hit.item == anchor->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selection == EDITOR_SELECTION_ANCHOR && state.dragged_anchor);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    anchor->visible = false;
    soft->visible = false;

    /* With the cover hidden, ordinary body-to-child double-click behavior remains. */
    front->visible = false;
    state.mode = EDITOR_VIEWPORT_RIGID_BODY;
    state.selection = EDITOR_SELECTION_RIGID_BODY;
    state.selected_rigid_body = back->id;
    state.last_viewport_click_selection = EDITOR_SELECTION_NONE;
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.dragged_body && state.mode == EDITOR_VIEWPORT_RIGID_BODY);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.mode == EDITOR_VIEWPORT_HITBOX &&
        state.selected_hitbox == back->hitboxes[0].id);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);

    /* Project picking compares content layers first, hierarchy order second. */
    front->visible = true;
    EditorObjectId first_object = object->id;
    EditorObject *second = editor_project_object_add(&project, (Position){0});
    PICK_REQUIRE(second != NULL);
    PICK_REQUIRE(editor_project_rigid_body_add(&project, second));
    state.mode = EDITOR_VIEWPORT_HIERARCHY;
    state.selection = EDITOR_SELECTION_OBJECT;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, center, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_OBJECT && hit.item == first_object);
    PICK_REQUIRE(editor_project_sprite_add(&project, second, "front layer", "unused.png"));
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, center, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_OBJECT && hit.item == second->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, center, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(project.selected == second->id && state.dragged_project_object);
    (void)picking_pointer_update(&project, &state, center, MOUSE_BUTTON_STATE_RELEASED);

    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    return true;
}

static bool layout_render_order_picking_check(void) {
    EditorProject project;
    EditorViewportState state;
    EditorSelectionRef hit;
    Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    EditorCamera *camera = editor_project_camera_add(&project, object);
    EditorLayoutViewport *layout = editor_project_layout_viewport_add(&project);
    PICK_REQUIRE(object != NULL && camera != NULL && layout != NULL);
    PICK_REQUIRE(editor_viewport_camera_add(&project, layout, object->id, camera->id));
    PICK_REQUIRE(editor_viewport_camera_add(&project, layout, object->id, camera->id));
    EditorViewportUiItem *ui = editor_viewport_ui_add(&project, layout,
        EDITOR_VIEWPORT_UI_SHAPE);
    PICK_REQUIRE(ui != NULL);
    layout->config.rectangle = (ViewportRectangle){0, 0, 200, 200};
    layout->camera_items[0].placement.rectangle = (ViewportRectangle){-30, 10, 180, 100};
    layout->camera_items[0].placement.layer = 10;
    layout->camera_items[1].placement.rectangle = (ViewportRectangle){20, 0, 100, 160};
    layout->camera_items[1].placement.orientation = 0.2f;
    layout->camera_items[1].placement.layer = 30;
    ui->position = (Position){40, 40};
    ui->layer = 20;
    Position point = {center.x + 60, center.y + 60};
    state.mode = EDITOR_VIEWPORT_UI_VERTEX_EDITOR;
    state.selected_layout_viewport = layout->id;
    state.selected_viewport_ui_item = ui->id;
    state.selection = EDITOR_SELECTION_UI_VERTEX;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_LAYOUT_VIEWPORT &&
        hit.item == layout->camera_items[1].id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selected_viewport_camera_item == layout->camera_items[1].id &&
        state.dragged_viewport_item && !state.dragged_viewport_vertex);
    /* Capture stays with this screen when its layer changes while dragging. */
    layout->camera_items[1].placement.layer = 0;
    PICK_REQUIRE(picking_pointer_update(&project, &state,
        (Position){point.x + 10, point.y}, MOUSE_BUTTON_STATE_DOWN));
    PICK_REQUIRE(layout->camera_items[1].placement.rectangle.x == 30);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_UI_SHAPE && hit.item == ui->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selected_viewport_ui_item == ui->id && state.dragged_viewport_item);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    ui->visible = false;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.item == layout->camera_items[0].id);
    layout->camera_items[0].placement.visible = false;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.item == layout->camera_items[1].id);
    /* UI follows screens at equal layers; later UI siblings follow earlier ones. */
    ui->visible = true;
    ui->layer = 0;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_UI_SHAPE && hit.item == ui->id);
    EditorViewportUiItem *second_ui = editor_viewport_ui_add(&project, layout,
        EDITOR_VIEWPORT_UI_SHAPE);
    PICK_REQUIRE(second_ui != NULL);
    ui = &layout->ui_items[0];
    second_ui->position = ui->position;
    second_ui->layer = ui->layer;
    state.mode = EDITOR_VIEWPORT_UI_VERTEX_EDITOR;
    state.selected_viewport_ui_item = ui->id;
    PICK_REQUIRE(editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(hit.kind == EDITOR_SELECTION_UI_SHAPE && hit.item == second_ui->id);
    PICK_REQUIRE(picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    PICK_REQUIRE(state.selected_viewport_ui_item == second_ui->id &&
        !state.dragged_viewport_vertex && state.dragged_viewport_item);
    (void)picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_RELEASED);
    layout->enabled = false;
    PICK_REQUIRE(!editor_viewport_selection_at_get(&project, &state, point, &hit));
    PICK_REQUIRE(!picking_pointer_update(&project, &state, point, MOUSE_BUTTON_STATE_PRESSED));
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    return true;
}
#undef PICK_REQUIRE

int main(void) {
    if(!render_order_picking_check() || !layout_render_order_picking_check()) return 1;
    if(!accordion_layout_metrics_check() ||
            !created_name_focus_mapping_check() ||
            !created_name_focus_replacement_check() ||
            !input_key_capture_check() ||
            !hitbox_vertex_zoom_pick_check() || !hitbox_line_zoom_pick_check()) return 1;
    static EditorProject project;
    EditorObject *object;
    EditorRigidBody *body;
    EditorRigidBody *body_b;
    EditorHitbox *hitbox;
    EditorAnchor *anchor;
    EditorJoint *joint;
    EditorSoftBody *soft_body;
    EditorSoftNode *node_a;
    EditorSoftNode *node_b;
    EditorSoftBeam *beam;
    EditorViewportState state = {0};
    EditorHistory history;
    EditorFileBrowser browser = {.mode = EDITOR_FILE_BROWSER_DIRECTORY};
    char path[EDITOR_FILE_BROWSER_PATH_MAX];

    if(!screen_rotation_pointer_check(40.0f, 20.0f, 0.5f) ||
            !screen_rotation_pointer_check(300.0f, 120.0f, 2.0f)) return 1;

    editor_project_init(&project);
    if(!editor_history_init(&history, &project)) return 1;
    object = editor_project_object_add(&project, (Position){0});
    body = editor_project_rigid_body_add(&project, object);
    body_b = editor_project_rigid_body_add(&project, object);
    if(object == NULL || body == NULL || body_b == NULL) return 1;
    anchor = editor_project_anchor_add(&project, object, (Position){0}, body->id);
    joint = editor_project_joint_add(&project, object, EDITOR_JOINT_SPRING);
    soft_body = editor_project_soft_body_add(&project, object);
    if(anchor == NULL || joint == NULL || soft_body == NULL ||
            editor_project_object_hierarchy_index_get(object,
                EDITOR_HIERARCHY_ANCHOR, anchor->id) == SIZE_MAX) return 1;
    node_a = editor_project_soft_node_add(&project, soft_body, (Position){0});
    node_b = editor_project_soft_node_add(&project, soft_body, (Position){30.0f, 0.0f});
    if(node_a == NULL || node_b == NULL) return 1;
    beam = editor_project_soft_beam_add(
        &project, soft_body, node_a->id, node_b->id);
    if(beam == NULL) return 1;
    hitbox = &body->hitboxes[0];
    editor_viewport_state_init(&state);

    {
        EditorSelectionRef first = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body->id};
        EditorSelectionRef second = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body_b->id};
        EditorSelectionRef mixed = {EDITOR_SELECTION_SOFT_BODY,
            object->id, 0, 0, soft_body->id};
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true) ||
                state.selected_item_count != 2 ||
                !editor_viewport_selection_contains(&state, first) ||
                !editor_viewport_selection_contains(&state, second)) return 1;
        if(!editor_viewport_selection_set(&project, &state, second, true) ||
                state.selected_item_count != 1 ||
                state.selected_rigid_body != body->id) return 1;
        state.mode = EDITOR_VIEWPORT_RIGID_BODY;
        if(!editor_viewport_selection_set(&project, &state, mixed, true) ||
                state.selected_item_count != 2 ||
                editor_viewport_selection_homogeneous_check(&state) ||
                state.selection != EDITOR_SELECTION_SOFT_BODY) return 1;
        editor_viewport_multi_selection_dismiss(&project, &state);
        if(state.selected_item_count != 0 ||
                state.mode != EDITOR_VIEWPORT_RIGID_BODY ||
                state.selection != EDITOR_SELECTION_RIGID_BODY ||
                state.selected_rigid_body != body->id) return 1;
        state.selection = EDITOR_SELECTION_RIGID_BODY;
        state.selected_rigid_body = body->id;
        state.mode = EDITOR_VIEWPORT_RIGID_BODY;
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, mixed, true) ||
                state.selected_item_count != 2 ||
                state.selected_items[0].kind != EDITOR_SELECTION_RIGID_BODY ||
                state.selected_items[1].kind != EDITOR_SELECTION_SOFT_BODY)
            return 1;
        editor_viewport_multi_selection_dismiss(&project, &state);
        if(state.mode != EDITOR_VIEWPORT_RIGID_BODY ||
                state.selection != EDITOR_SELECTION_RIGID_BODY ||
                state.selected_rigid_body != body->id) return 1;

        first = (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE,
            object->id, soft_body->id, 0, node_a->id};
        second = (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE,
            object->id, soft_body->id, 0, node_b->id};
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true))
            return 1;
        state.mode = EDITOR_VIEWPORT_SOFT_NODE;
        state.selection_modifier = true;
        if(!editor_viewport_update(&state, &project,
                (Position){editor_viewport_width * 0.5f + 30.0f,
                    EDITOR_MENU_HEIGHT +
                        (editor_viewport_bottom - EDITOR_MENU_HEIGHT) * 0.5f},
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || state.selected_item_count != 1 ||
                editor_viewport_selection_contains(&state, second)) return 1;
        state.selection_modifier = false;
        editor_viewport_selection_clear(&state);
    }
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_OBJECT,
                EDITOR_VIEWPORT_OBJECT)) return 1;
    state.selected_rigid_body = body->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_RIGID_BODY,
                EDITOR_VIEWPORT_RIGID_BODY)) return 1;
    body_b->particle = true;
    state.selected_rigid_body = body_b->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_RIGID_BODY,
                EDITOR_VIEWPORT_RIGID_BODY) ||
            state.selection != EDITOR_SELECTION_RIGID_BODY) return 1;
    body_b->standalone_particle = true;
    body_b->position = (Position){200.0f, 0.0f};
    {
        Position particle_center = {EDITOR_VIEWPORT_WIDTH * 0.5f + 200.0f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
        state.mode = EDITOR_VIEWPORT_PARTICLE;
        state.selection = EDITOR_SELECTION_PARTICLE;
        state.selected_rigid_body = body_b->id;
        if(!editor_viewport_update(&state, &project, particle_center,
                    MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                    false, 0.0f, false) ||
                state.mode == EDITOR_VIEWPORT_HITBOX) return 1;
        (void)editor_viewport_update(&state, &project, particle_center,
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        if(!editor_viewport_update(&state, &project, particle_center,
                    MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                    false, 0.0f, false) ||
                state.mode != EDITOR_VIEWPORT_PARTICLE_RADIUS ||
                state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    }
    body_b->position = (Position){0};
    state.last_viewport_click_selection = EDITOR_SELECTION_NONE;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_RIGID_BODY,
                EDITOR_VIEWPORT_PARTICLE) ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_OBJECT ||
            state.selection != EDITOR_SELECTION_OBJECT) return 1;
    state.selected_rigid_body = body_b->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_PARTICLE,
                EDITOR_VIEWPORT_PARTICLE)) return 1;
    state.selected_hitbox = body_b->hitboxes[0].id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_HITBOX,
                EDITOR_VIEWPORT_PARTICLE) ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    state.selected_vertex = 0;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_VERTEX,
                EDITOR_VIEWPORT_PARTICLE) ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    state.selected_line = 0;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_LINE,
                EDITOR_VIEWPORT_PARTICLE) ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    state.mode = EDITOR_VIEWPORT_HITBOX;
    state.selection = EDITOR_SELECTION_HITBOX;
    (void)editor_viewport_update(&state, &project, (Position){0},
        MOUSE_BUTTON_STATE_UP, MOUSE_BUTTON_STATE_UP, false, 0.0f, false);
    if(state.mode != EDITOR_VIEWPORT_PARTICLE ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    state.mode = EDITOR_VIEWPORT_PARTICLE_RADIUS;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_PARTICLE ||
            state.selection != EDITOR_SELECTION_PARTICLE) return 1;
    body_b->particle = false;
    body_b->standalone_particle = false;
    state.selected_rigid_body = body->id;
    state.selected_hitbox = hitbox->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_HITBOX,
                EDITOR_VIEWPORT_HITBOX)) return 1;
    state.selected_vertex = 0;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_VERTEX,
                EDITOR_VIEWPORT_VERTEX)) return 1;
    state.selected_line = 0;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_LINE,
                EDITOR_VIEWPORT_LINE)) return 1;
    state.selected_anchor = anchor->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_ANCHOR,
                EDITOR_VIEWPORT_ANCHOR)) return 1;
    state.selected_joint = joint->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_JOINT,
                EDITOR_VIEWPORT_JOINT)) return 1;
    state.selected_soft_body = soft_body->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_SOFT_BODY,
                EDITOR_VIEWPORT_SOFT_BODY)) return 1;
    state.selected_soft_node = node_a->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_SOFT_NODE,
                EDITOR_VIEWPORT_SOFT_NODE)) return 1;
    state.selected_soft_beam = beam->id;
    if(!navigation_mode_open_check(&project, &state, EDITOR_SELECTION_SOFT_BEAM,
                EDITOR_VIEWPORT_SOFT_BEAM)) return 1;

    state.selected_vertex = hitbox->vertex_count;
    state.selection = EDITOR_SELECTION_VERTEX;
    if(editor_navigation_selected_open(&project, &state)) return 1;
    state.mode = EDITOR_VIEWPORT_RIGID_BODY;
    state.selection = EDITOR_SELECTION_NONE;
    editor_navigation_current_selection_clear(&project, &state);
    if(state.selection != EDITOR_SELECTION_NONE) return 1;

    state.selection = EDITOR_SELECTION_SOFT_NODE;
    state.selected_soft_body = soft_body->id;
    state.selected_soft_node = soft_body->nodes[0].id;
    if(!editor_viewport_selection_set(&project, &state,
            (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE,
                project.objects[0].id, soft_body->id, 0,
                soft_body->nodes[0].id}, true) ||
            state.selection != EDITOR_SELECTION_NONE ||
            state.selected_item_count != 0) return 1;

    state.mode = EDITOR_VIEWPORT_HIERARCHY;
    editor_navigation_current_selection_clear(&project, &state);
    if(state.selection != EDITOR_SELECTION_NONE || project.selected != 0) return 1;

    state.mode = EDITOR_VIEWPORT_SOFT_BEAM;
    state.selection = EDITOR_SELECTION_NONE;
    if(!editor_navigation_open_item_selection_set(&state) ||
            state.selection != EDITOR_SELECTION_SOFT_BEAM) return 1;
    state.mode = EDITOR_VIEWPORT_HIERARCHY;
    if(editor_navigation_open_item_selection_set(&state)) return 1;

    state.mode = EDITOR_VIEWPORT_ANCHOR;
    state.selection = EDITOR_SELECTION_ANCHOR;
    state.selected_anchor = anchor->id;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_OBJECT ||
            state.selection != EDITOR_SELECTION_NONE ||
            state.selected_anchor != 0) return 1;

    state.mode = EDITOR_VIEWPORT_CAMERA_ENTITY;
    state.selection = EDITOR_SELECTION_CAMERA;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_OBJECT ||
            state.selection != EDITOR_SELECTION_OBJECT) return 1;

    state.mode = EDITOR_VIEWPORT_AUTO_SHAPE;
    state.auto_shape_parent_mode = EDITOR_VIEWPORT_HITBOX;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_HITBOX ||
            state.selection != EDITOR_SELECTION_HITBOX) return 1;
    state.mode = EDITOR_VIEWPORT_AUTO_SHAPE;
    state.auto_shape_parent_mode = EDITOR_VIEWPORT_SOFT_BODY;
    editor_viewport_back(&state);
    if(state.mode != EDITOR_VIEWPORT_SOFT_BODY ||
            state.selection != EDITOR_SELECTION_SOFT_BODY) return 1;

    snprintf(browser.directory, sizeof(browser.directory), "/projects");
    if(!editor_file_browser_directory_path_get(&browser, path, sizeof(path)) ||
            strcmp(path, "/projects") != 0) return 1;
    snprintf(browser.selected_directory, sizeof(browser.selected_directory),
        "/projects/game");
    snprintf(browser.preview_selected_path, sizeof(browser.preview_selected_path),
        "/projects/game/project.rohr.json");
    browser.preview_selected_directory = false;
    if(!editor_file_browser_directory_path_get(&browser, path, sizeof(path)) ||
            strcmp(path, "/projects/game") != 0) return 1;
    snprintf(browser.preview_selected_path, sizeof(browser.preview_selected_path),
        "/projects/game/assets");
    browser.preview_selected_directory = true;
    if(!editor_file_browser_directory_path_get(&browser, path, sizeof(path)) ||
            strcmp(path, "/projects/game/assets") != 0) return 1;
    snprintf(browser.directory, sizeof(browser.directory), "/projects/game/");
    browser.refresh_pending = false;
    if(!editor_file_browser_parent(&browser) ||
            strcmp(browser.directory, "/projects") != 0 ||
            !browser.refresh_pending) return 1;
    snprintf(browser.directory, sizeof(browser.directory), "/");
    browser.refresh_pending = false;
    if(editor_file_browser_parent(&browser) || browser.refresh_pending) return 1;
    snprintf(browser.directory, sizeof(browser.directory), "C:\\projects\\game");
    if(!editor_file_browser_parent(&browser) ||
            strcmp(browser.directory, "C:\\projects") != 0) return 1;
    snprintf(browser.directory, sizeof(browser.directory), "C:\\");
    browser.refresh_pending = false;
    if(editor_file_browser_parent(&browser) || browser.refresh_pending) return 1;
    {
        EditorProject drag_project;
        EditorViewportState drag_state = {0};
        EditorObject *drag_object;
        EditorRigidBody *drag_body;
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};

        editor_project_init(&drag_project);
        editor_viewport_state_init(&drag_state);
        drag_object = editor_project_object_add(&drag_project, (Position){0});
        drag_body = editor_project_rigid_body_add(&drag_project, drag_object);
        if(drag_object == NULL || drag_body == NULL ||
                !editor_project_object_select(&drag_project, drag_object->id)) return 1;
        drag_state.mode = EDITOR_VIEWPORT_HIERARCHY;
        if(!editor_viewport_update(&drag_state, &drag_project, center,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                drag_state.mode != EDITOR_VIEWPORT_HIERARCHY ||
                drag_state.selection != EDITOR_SELECTION_OBJECT ||
                !drag_state.dragged_project_object) return 1;
        if(!editor_viewport_update(&drag_state, &drag_project,
                (Position){center.x + 20.0f, center.y},
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                fabsf(drag_object->overview_position.x - 20.0f) > 0.001f ||
                fabsf(drag_object->position.x) > 0.001f) return 1;
        (void)editor_viewport_update(&drag_state, &drag_project,
            (Position){center.x + 20.0f, center.y},
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        if(!editor_viewport_update(&drag_state, &drag_project,
                (Position){center.x + 20.0f, center.y},
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                drag_state.mode != EDITOR_VIEWPORT_OBJECT) return 1;
        (void)editor_viewport_update(&drag_state, &drag_project,
            (Position){center.x + 20.0f, center.y},
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        if(!editor_viewport_update(&drag_state, &drag_project, center,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || !drag_state.dragged_body ||
                drag_state.mode != EDITOR_VIEWPORT_RIGID_BODY ||
                drag_state.selected_rigid_body != drag_body->id) return 1;
        if(!editor_viewport_update(&drag_state, &drag_project,
                (Position){center.x + 20.0f, center.y}, MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                fabsf(drag_body->position.x - 20.0f) > 0.001f) return 1;
        editor_viewport_state_destroy(&drag_state);
        editor_project_destroy(&drag_project);
    }
    {
        EditorProject camera_overview_project;
        EditorViewportState camera_overview_state = {0};
        EditorObject *camera_overview_object;
        EditorCamera *overview_camera;
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
        Position grab = {center.x + 80.0f, center.y};

        editor_project_init(&camera_overview_project);
        editor_viewport_state_init(&camera_overview_state);
        camera_overview_object = editor_project_object_add(
            &camera_overview_project, (Position){0});
        overview_camera = editor_project_camera_add(&camera_overview_project,
            camera_overview_object);
        if(camera_overview_object == NULL || overview_camera == NULL) return 1;
        overview_camera->position = (Position){80.0f, 0.0f};
        overview_camera->dimensions = (Scale){20.0f, 20.0f};
        camera_overview_state.mode = EDITOR_VIEWPORT_HIERARCHY;
        camera_overview_state.project_elements_hidden = true;
        if(editor_viewport_update(&camera_overview_state,
                &camera_overview_project, grab, MOUSE_BUTTON_STATE_PRESSED,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                !camera_overview_object->visible ||
                camera_overview_state.dragged_project_object) return 1;
        camera_overview_state.project_elements_hidden = false;
        if(!editor_viewport_update(&camera_overview_state,
                &camera_overview_project, grab, MOUSE_BUTTON_STATE_PRESSED,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                camera_overview_state.selection != EDITOR_SELECTION_OBJECT ||
                !camera_overview_state.dragged_project_object) return 1;
        if(!editor_viewport_update(&camera_overview_state,
                &camera_overview_project,
                (Position){grab.x + 20.0f, grab.y}, MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                fabsf(camera_overview_object->overview_position.x - 20.0f) >
                    0.001f ||
                fabsf(overview_camera->position.x - 80.0f) > 0.001f) return 1;
        (void)editor_viewport_update(&camera_overview_state,
            &camera_overview_project, (Position){grab.x + 20.0f, grab.y},
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        if(!editor_viewport_update(&camera_overview_state,
                &camera_overview_project,
                (Position){grab.x + 20.0f, grab.y}, MOUSE_BUTTON_STATE_PRESSED,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                camera_overview_state.mode != EDITOR_VIEWPORT_OBJECT) return 1;
        editor_viewport_state_destroy(&camera_overview_state);
        editor_project_destroy(&camera_overview_project);
    }
    {
        EditorProject camera_project;
        EditorViewportState camera_state = {0};
        EditorObject *camera_object;
        EditorRigidBody *camera_body;
        EditorCamera *camera;
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
        Position grab;

        editor_project_init(&camera_project);
        editor_viewport_state_init(&camera_state);
        camera_object = editor_project_object_add(&camera_project, (Position){0});
        camera_body = editor_project_rigid_body_add(&camera_project, camera_object);
        camera = editor_project_camera_add(&camera_project, camera_object);
        if(camera_object == NULL || camera_body == NULL || camera == NULL ||
                !editor_project_object_select(
                    &camera_project, camera_object->id)) return 1;
        camera_body->rotation = -90.0f;
        camera->position = (Position){20.0f, 0.0f};
        camera->dimensions = (Scale){100.0f, 100.0f};
        camera->attachment_kind = EDITOR_CAMERA_ATTACHMENT_RIGID_BODY;
        camera->attachment = camera_body->id;
        camera_state.mode = EDITOR_VIEWPORT_OBJECT;
        grab = (Position){center.x + 5.0f, center.y - 20.0f};
        camera_state.object_elements_hidden = true;
        if(editor_viewport_update(&camera_state, &camera_project, grab,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || !camera->visible ||
                camera_state.dragged_camera_entity) return 1;
        camera_state.object_elements_hidden = false;
        if(!editor_viewport_update(&camera_state, &camera_project, grab,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || !camera_state.dragged_camera_entity) return 1;
        if(!editor_viewport_update(&camera_state, &camera_project,
                (Position){grab.x + 10.0f, grab.y}, MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                fabsf(camera->position.x - 20.0f) > 0.001f ||
                fabsf(camera->position.y + 10.0f) > 0.001f) return 1;
        editor_viewport_state_destroy(&camera_state);
        editor_project_destroy(&camera_project);
    }
    {
        EditorProject viewport_drag_project;
        EditorViewportState viewport_drag_state = {0};
        EditorObject *previous_object;
        EditorLayoutViewport *dragged_viewport;
        Position origin = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};

        editor_project_init(&viewport_drag_project);
        editor_viewport_state_init(&viewport_drag_state);
        previous_object = editor_project_object_add(&viewport_drag_project,
            (Position){-1000.0f, -1000.0f});
        dragged_viewport = editor_project_layout_viewport_add(
            &viewport_drag_project);
        if(previous_object == NULL || dragged_viewport == NULL ||
                !editor_viewport_selection_set(&viewport_drag_project,
                    &viewport_drag_state,
                    (EditorSelectionRef){EDITOR_SELECTION_OBJECT,
                        previous_object->id, 0, 0, previous_object->id},
                    false)) return 1;
        viewport_drag_state.mode = EDITOR_VIEWPORT_HIERARCHY;
        if(!editor_viewport_update(&viewport_drag_state, &viewport_drag_project,
                (Position){origin.x + 10.0f, origin.y + 10.0f},
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                viewport_drag_state.selection !=
                    EDITOR_SELECTION_LAYOUT_VIEWPORT ||
                !viewport_drag_state.dragged_project_viewport ||
                viewport_drag_state.selected_item_count != 0 ||
                viewport_drag_project.selected != EDITOR_OBJECT_INVALID) return 1;
        if(!editor_viewport_update(&viewport_drag_state, &viewport_drag_project,
                (Position){origin.x + 30.0f, origin.y + 40.0f},
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                fabsf(dragged_viewport->overview_position.x - 20.0f) > 0.001f ||
                fabsf(dragged_viewport->overview_position.y + 30.0f) > 0.001f ||
                dragged_viewport->config.rectangle.x != 0.0f ||
                dragged_viewport->config.rectangle.y != 0.0f) return 1;
        editor_viewport_state_destroy(&viewport_drag_state);
        editor_project_destroy(&viewport_drag_project);
    }
    {
        EditorProject hidden_layout_project;
        EditorViewportState hidden_layout_state = {0};
        EditorLayoutViewport *hidden_layout;
        EditorViewportUiItem *hidden_ui;
        EditorSelectionRef selection;
        Position origin = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};

        editor_project_init(&hidden_layout_project);
        editor_viewport_state_init(&hidden_layout_state);
        hidden_layout = editor_project_layout_viewport_add(
            &hidden_layout_project);
        hidden_ui = hidden_layout == NULL ? NULL : editor_viewport_ui_add(
            &hidden_layout_project, hidden_layout, EDITOR_VIEWPORT_UI_SHAPE);
        if(hidden_layout == NULL || hidden_ui == NULL) return 1;
        hidden_layout_state.mode = EDITOR_VIEWPORT_LAYOUT;
        hidden_layout_state.selected_layout_viewport = hidden_layout->id;
        if(!editor_viewport_selection_at_get(&hidden_layout_project,
                &hidden_layout_state,
                (Position){origin.x + 100.0f, origin.y + 30.0f},
                &selection) || selection.kind != EDITOR_SELECTION_UI_SHAPE)
            return 1;
        hidden_layout_state.layout_elements_hidden = true;
        if(editor_viewport_selection_at_get(&hidden_layout_project,
                &hidden_layout_state,
                (Position){origin.x + 100.0f, origin.y + 30.0f},
                &selection) || !hidden_ui->visible) return 1;
        editor_viewport_state_destroy(&hidden_layout_state);
        editor_project_destroy(&hidden_layout_project);
    }
    {
        EditorProject area_project;
        EditorViewportState area_state = {0};
        EditorObject *area_object;
        EditorSoftBody *area_body;
        EditorSoftNode *area_a;
        EditorSoftNode *area_b;
        EditorSoftNode *area_c;
        EditorSelectionRef area_selection;
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};

        editor_project_init(&area_project);
        editor_viewport_state_init(&area_state);
        area_object = editor_project_object_add(&area_project, (Position){0});
        area_body = editor_project_soft_body_add(&area_project, area_object);
        area_a = editor_project_soft_node_add(
            &area_project, area_body, (Position){-40.0f, -30.0f});
        area_b = editor_project_soft_node_add(
            &area_project, area_body, (Position){40.0f, -30.0f});
        area_c = editor_project_soft_node_add(
            &area_project, area_body, (Position){0.0f, 40.0f});
        if(area_object == NULL || area_body == NULL || area_a == NULL ||
                area_b == NULL || area_c == NULL ||
                editor_project_soft_beam_add(&area_project, area_body,
                    area_a->id, area_b->id) == NULL ||
                editor_project_soft_beam_add(&area_project, area_body,
                    area_b->id, area_c->id) == NULL ||
                editor_project_soft_beam_add(&area_project, area_body,
                    area_c->id, area_a->id) == NULL) return 1;
        editor_project_soft_areas_sync(&area_project, area_body);
        if(area_body->area_count == 0 ||
                !editor_project_object_select(&area_project, area_object->id)) return 1;
        area_selection = (EditorSelectionRef){EDITOR_SELECTION_SOFT_AREA,
            area_object->id, area_body->id, 0, area_body->areas[0].id};
        if(!editor_viewport_selection_set(
                &area_project, &area_state, area_selection, false)) return 1;
        area_state.mode = EDITOR_VIEWPORT_SOFT_AREA;
        if(!editor_viewport_update(&area_state, &area_project,
                (Position){center.x, center.y + 6.0f}, MOUSE_BUTTON_STATE_PRESSED,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                !area_state.dragged_soft_body ||
                area_state.selection != EDITOR_SELECTION_SOFT_AREA ||
                area_state.mode != EDITOR_VIEWPORT_SOFT_AREA) return 1;
        if(!editor_viewport_update(&area_state, &area_project,
                (Position){center.x, center.y + 6.0f}, MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP, false, 0.0f, false) ||
                area_state.selection != EDITOR_SELECTION_SOFT_AREA ||
                area_state.mode != EDITOR_VIEWPORT_SOFT_AREA ||
                fabsf(area_body->position.x) > 0.001f) return 1;
        if(!editor_viewport_update(&area_state, &area_project,
                (Position){center.x + 20.0f, center.y + 6.0f},
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                fabsf(area_body->position.x - 20.0f) > 0.001f ||
                area_state.selection != EDITOR_SELECTION_SOFT_BODY ||
                area_state.mode != EDITOR_VIEWPORT_SOFT_BODY) return 1;
        (void)editor_viewport_update(&area_state, &area_project,
            (Position){center.x + 20.0f, center.y + 6.0f},
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        if(!editor_viewport_update(&area_state, &area_project,
                (Position){center.x + 20.0f,
                    center.y - EDITOR_VIEWPORT_ROTATION_ARM_LENGTH},
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || !area_state.rotated_soft_body) return 1;
        editor_viewport_state_destroy(&area_state);
        editor_project_destroy(&area_project);
    }
    {
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
        state.mode = EDITOR_VIEWPORT_RIGID_BODY;
        state.selection = EDITOR_SELECTION_RIGID_BODY;
        state.selected_rigid_body = body->id;
        if(!editor_project_object_select(&project, object->id)) return 1;
        editor_viewport_marquee_begin(&state,
            (Position){center.x - 100.0f, center.y - 100.0f});
        editor_viewport_marquee_update(&state,
            (Position){center.x + 100.0f, center.y + 100.0f});
        if(!state.marquee_active ||
                !editor_viewport_marquee_finish(&state, &project,
                    (Position){center.x + 100.0f, center.y + 100.0f}) ||
                state.marquee_active || state.selected_item_count != 2 ||
                !editor_viewport_selection_homogeneous_check(&state)) return 1;
        state.mode = EDITOR_VIEWPORT_OBJECT;
        state.selection = EDITOR_SELECTION_NONE;
        editor_viewport_marquee_begin(&state,
            (Position){center.x - 100.0f, center.y - 100.0f});
        if(!editor_viewport_marquee_finish(&state, &project,
                    (Position){center.x + 100.0f, center.y + 100.0f}) ||
                state.selected_item_count != 4 ||
                state.selected_items[0].kind != EDITOR_SELECTION_RIGID_BODY ||
                state.mode != EDITOR_VIEWPORT_ANCHOR ||
                editor_viewport_selection_homogeneous_check(&state)) return 1;
    }
    {
        EditorSelectionRef node_ref = {EDITOR_SELECTION_SOFT_NODE,
            object->id, soft_body->id, 0, node_a->id};
        EditorSelectionRef beam_ref = {EDITOR_SELECTION_SOFT_BEAM,
            object->id, soft_body->id, 0, beam->id};
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_navigation_selection_reorder(&project, &state,
                node_ref, beam_ref, true, &history) ||
                editor_project_soft_body_hierarchy_index_get(soft_body,
                    EDITOR_SOFT_HIERARCHY_NODE, node_a->id) <=
                editor_project_soft_body_hierarchy_index_get(soft_body,
                    EDITOR_SOFT_HIERARCHY_BEAM, beam->id) ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                editor_project_soft_body_hierarchy_index_get(soft_body,
                    EDITOR_SOFT_HIERARCHY_NODE, node_a->id) >=
                editor_project_soft_body_hierarchy_index_get(soft_body,
                    EDITOR_SOFT_HIERARCHY_BEAM, beam->id)) return 1;
    }
    {
        EditorRigidBody *body_c = editor_project_rigid_body_add(&project, object);
        EditorRigidBodyId body_id = body->id;
        EditorRigidBodyId body_b_id = body_b->id;
        EditorRigidBodyId body_c_id;
        size_t body_c_original_index;
        EditorSelectionRef first = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body_id};
        EditorSelectionRef second = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body_b_id};
        EditorSelectionRef third;
        if(body_c == NULL) return 1;
        body_c_id = body_c->id;
        body_c_original_index = editor_project_object_hierarchy_index_get(object,
            EDITOR_HIERARCHY_RIGID_BODY, body_c_id);
        third = (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body_c_id};
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_navigation_selection_reorder(&project, &state,
                second, first, false, &history) ||
                object->hierarchy[0].id != body_b_id ||
                object->hierarchy[1].id != body_id ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                object->hierarchy[0].id != body_id ||
                object->hierarchy[1].id != body_b_id) return 1;
        editor_history_reset(&history);
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true) ||
                !editor_navigation_selection_reorder(&project, &state,
                    first, third, true, &history) ||
                editor_project_object_hierarchy_index_get(object,
                    EDITOR_HIERARCHY_RIGID_BODY, body_c_id) >=
                    editor_project_object_hierarchy_index_get(object,
                        EDITOR_HIERARCHY_RIGID_BODY, body_id) ||
                editor_project_object_hierarchy_index_get(object,
                    EDITOR_HIERARCHY_RIGID_BODY, body_id) + 1 !=
                    editor_project_object_hierarchy_index_get(object,
                        EDITOR_HIERARCHY_RIGID_BODY, body_b_id) ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                object->hierarchy[0].id != body_id ||
                object->hierarchy[1].id != body_b_id ||
                editor_project_object_hierarchy_index_get(object,
                    EDITOR_HIERARCHY_RIGID_BODY, body_c_id) !=
                        body_c_original_index) return 1;
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, second, false) ||
                !editor_viewport_selection_set(&project, &state, third, true) ||
                !editor_navigation_selection_reorder(&project, &state,
                    second, first, false, &history) ||
                object->hierarchy[0].id != body_b_id ||
                object->hierarchy[1].id != body_c_id ||
                object->hierarchy[2].id != body_id ||
                !editor_history_undo(&history)) return 1;
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_navigation_selection_reorder(&project, &state,
                first, third, true, &history) ||
                editor_project_object_hierarchy_index_get(object,
                    EDITOR_HIERARCHY_RIGID_BODY, body_id) + 1 !=
                    object->hierarchy_count ||
                !editor_history_undo(&history)) return 1;
        editor_viewport_selection_clear(&state);
        if(!editor_project_rigid_body_remove(object, body_c_id)) return 1;
        editor_history_reset(&history);
        {
            EditorSelectionRef joint_ref = {EDITOR_SELECTION_JOINT,
                object->id, 0, 0, joint->id};
            EditorSelectionRef soft_ref = {EDITOR_SELECTION_SOFT_BODY,
                object->id, 0, 0, soft_body->id};
            if(!editor_navigation_selection_reorder(&project, &state,
                    joint_ref, soft_ref, true, &history) ||
                    editor_project_object_hierarchy_index_get(object,
                        EDITOR_HIERARCHY_JOINT, joint->id) <=
                    editor_project_object_hierarchy_index_get(object,
                        EDITOR_HIERARCHY_SOFT_BODY, soft_body->id) ||
                    !editor_history_undo(&history)) return 1;
            editor_history_reset(&history);
        }
    }
    {
        EditorInputController *controller =
            editor_project_input_controller_add(&project, "gameplay");
        EditorLayoutViewport *layout =
            editor_project_layout_viewport_add(&project);
        EditorSelectionRef controller_ref;
        EditorSelectionRef object_ref = {EDITOR_SELECTION_OBJECT,
            object->id, 0, 0, object->id};
        if(controller == NULL || layout == NULL) return 1;
        controller_ref = (EditorSelectionRef){EDITOR_SELECTION_INPUT_CONTROLLER,
            0, 0, 0, controller->id};
        editor_project_hierarchy_sync(&project);
        editor_history_reset(&history);
        if(!editor_navigation_selection_reorder(&project, &state,
                controller_ref, object_ref, false, &history) ||
                project.hierarchy[0].kind !=
                    EDITOR_PROJECT_HIERARCHY_INPUT_CONTROLLER ||
                project.hierarchy[0].id != controller->id ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                project.hierarchy[0].kind != EDITOR_PROJECT_HIERARCHY_OBJECT ||
                !editor_history_redo(&history) ||
                project.hierarchy[0].kind !=
                    EDITOR_PROJECT_HIERARCHY_INPUT_CONTROLLER) return 1;
        editor_history_reset(&history);
    }
    {
        EditorInputController *controller = NULL;
        EditorInputAction *action;
        EditorInputAction *second_action;
        EditorInputControllerId controller_id;
        EditorInputActionId action_id;
        EditorInputActionId second_action_id;
        EditorSelectionRef action_ref;
        EditorSelectionRef second_action_ref;
        char name[ROHR_INPUT_NAME_MAX];
        for(size_t i = 0; i < project.input_controller_count; i += 1)
            if(strcmp(project.input_controllers[i].name, "gameplay") == 0)
                controller = &project.input_controllers[i];
        action = controller == NULL ? NULL : editor_project_input_action_add(
            &project, controller->id, "jump", INPUT_ACTION_BUTTON);
        action_id = action == NULL ? 0 : action->id;
        second_action = controller == NULL ? NULL : editor_project_input_action_add(
            &project, controller->id, "pause", INPUT_ACTION_BUTTON);
        if(action == NULL || second_action == NULL) return 1;
        second_action_id = second_action->id;
        controller_id = controller->id;
        action_ref = (EditorSelectionRef){EDITOR_SELECTION_INPUT_ACTION,
            0, controller_id, 0, action_id};
        second_action_ref = (EditorSelectionRef){EDITOR_SELECTION_INPUT_ACTION,
            0, controller_id, 0, second_action_id};
        if(!editor_viewport_selection_set(&project, &state, action_ref, false) ||
                state.selection != EDITOR_SELECTION_INPUT_ACTION ||
                state.selected_input_controller != controller_id ||
                state.selected_input_action != action_id ||
                !editor_navigation_selected_open(&project, &state) ||
                state.mode != EDITOR_VIEWPORT_INPUT_ACTION ||
                !editor_navigation_selection_name_get(&project, action_ref,
                    name, sizeof(name)) || strcmp(name, "jump") != 0 ||
                !editor_navigation_selection_name_set(&project, action_ref,
                    "jump_button") ||
                !editor_navigation_selection_name_get(&project, action_ref,
                    name, sizeof(name)) || strcmp(name, "jump_button") != 0)
            return 1;
        editor_viewport_back(&state);
        if(state.mode != EDITOR_VIEWPORT_INPUT_CONTROLLER ||
                state.selection != EDITOR_SELECTION_INPUT_CONTROLLER)
            return 1;
        editor_history_reset(&history);
        if(!editor_navigation_selection_reorder(&project, &state, action_ref,
                second_action_ref, true, &history) ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[0].id != second_action_id ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[1].id != action_id ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[0].id != action_id ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[1].id != second_action_id ||
                !editor_history_redo(&history) ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[0].id != second_action_id ||
                editor_project_input_controller_get(&project,
                    controller_id)->actions[1].id != action_id)
            return 1;
        editor_history_reset(&history);
        {
            EditorInputAction *binding_action = editor_project_input_action_get(
                &project, controller_id, action_id);
            EditorInputBindingId first_binding;
            EditorInputBindingId second_binding;
            EditorSelectionRef first_binding_ref;
            EditorSelectionRef second_binding_ref;
            if(binding_action == NULL || !editor_project_input_binding_add(
                    &project, controller_id, action_id,
                    (InputBinding){.source = INPUT_BINDING_KEY,
                        .input.key = SDL_SCANCODE_SPACE,
                        .scale = {1.0f, 1.0f}}) ||
                    !editor_project_input_binding_add(&project, controller_id,
                        action_id,
                        (InputBinding){.source = INPUT_BINDING_KEY,
                            .input.key = SDL_SCANCODE_RETURN,
                            .scale = {1.0f, 1.0f}}))
                return 1;
            binding_action = editor_project_input_action_get(&project,
                controller_id, action_id);
            first_binding = binding_action->binding_ids[0];
            second_binding = binding_action->binding_ids[1];
            first_binding_ref = (EditorSelectionRef){
                EDITOR_SELECTION_INPUT_BINDING, 0, controller_id, action_id,
                first_binding};
            second_binding_ref = (EditorSelectionRef){
                EDITOR_SELECTION_INPUT_BINDING, 0, controller_id, action_id,
                second_binding};
            if(!editor_viewport_selection_set(&project, &state,
                    first_binding_ref, false) ||
                    state.selected_input_binding != first_binding ||
                    !editor_navigation_selected_open(&project, &state) ||
                    state.mode != EDITOR_VIEWPORT_INPUT_BINDING ||
                    !editor_navigation_selection_name_set(&project,
                        first_binding_ref, "keyboard_jump") ||
                    !editor_navigation_selection_name_get(&project,
                        first_binding_ref, name, sizeof(name)) ||
                    strcmp(name, "keyboard_jump") != 0) return 1;
            editor_viewport_back(&state);
            if(state.mode != EDITOR_VIEWPORT_INPUT_ACTION ||
                    state.selection != EDITOR_SELECTION_INPUT_ACTION) return 1;
            editor_history_reset(&history);
            if(!editor_navigation_selection_reorder(&project, &state,
                    first_binding_ref, second_binding_ref, true, &history))
                return 1;
            binding_action = editor_project_input_action_get(&project,
                controller_id, action_id);
            if(binding_action->binding_ids[0] != second_binding ||
                    binding_action->binding_ids[1] != first_binding ||
                    strcmp(binding_action->bindings[1].name,
                        "keyboard_jump") != 0 ||
                    binding_action->bindings[1].input.key !=
                        SDL_SCANCODE_SPACE ||
                    history.undo_count != 1 || !editor_history_undo(&history))
                return 1;
            binding_action = editor_project_input_action_get(&project,
                controller_id, action_id);
            if(binding_action->binding_ids[0] != first_binding ||
                    strcmp(binding_action->bindings[0].name,
                        "keyboard_jump") != 0 ||
                    !editor_history_redo(&history)) return 1;
            binding_action = editor_project_input_action_get(&project,
                controller_id, action_id);
            if(binding_action->binding_ids[1] != first_binding) return 1;
            editor_history_reset(&history);
        }
        if(!editor_viewport_selection_set(&project, &state, action_ref, false) ||
                !editor_viewport_selection_set(&project, &state,
                    second_action_ref, true))
            return 1;
        state.mode = EDITOR_VIEWPORT_INPUT_ACTION;
        bool deleted = editor_navigation_multi_selection_delete(
            &project, &state, &history);
        if(!deleted ||
                state.mode != EDITOR_VIEWPORT_INPUT_CONTROLLER ||
                editor_project_input_controller_get(&project,
                    controller_id)->action_count != 0 ||
                editor_project_input_action_get(&project, controller_id,
                    action_ref.item) != NULL || history.undo_count != 1 ||
                !editor_history_undo(&history) ||
                editor_project_input_controller_get(&project,
                    controller_id)->action_count != 2 ||
                editor_project_input_action_get(&project, controller_id,
                    action_ref.item) == NULL)
            return 1;
        editor_history_reset(&history);
    }
    {
        EditorSelectionRef first = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body->id};
        EditorSelectionRef second = {EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body_b->id};
        EditorPropertySetCommand mass_property = {
            .property = EDITOR_PROPERTY_MASS,
            .value_kind = EDITOR_PROPERTY_VALUE_FLOAT,
            .value.number = 7.0f
        };
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true) ||
                !editor_bulk_property_set(&project, &state, &history,
                    &mass_property) ||
                body->mass_value != 7.0f || body_b->mass_value != 7.0f ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                body->mass_value == 7.0f || body_b->mass_value == 7.0f ||
                !editor_history_redo(&history) || body->mass_value != 7.0f ||
                body_b->mass_value != 7.0f)
            return 1;
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state,
                    (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY,
                        object->id, 0, 0, soft_body->id}, true) ||
                editor_viewport_selection_homogeneous_check(&state) ||
                !editor_navigation_multi_selection_delete(
                    &project, &state, &history) ||
                object->rigid_body_count != 1 || object->soft_body_count != 0 ||
                history.undo_count != 1 || !editor_history_undo(&history) ||
                object->rigid_body_count != 2 || object->soft_body_count != 1 ||
                !editor_history_redo(&history) || object->rigid_body_count != 1 ||
                object->soft_body_count != 0)
            return 1;
    }
    {
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        EditorObject *frame_object = editor_project_object_add(&project, (Position){0});
        EditorAnimatedSprite *animation = editor_project_animated_sprite_add(
            &project, frame_object);
        EditorAnimatedSpriteId animation_id;
        EditorSpriteId first_id;
        EditorSpriteId second_id;
        EditorSelectionRef first;
        EditorSelectionRef second;
        if(frame_object == NULL || animation == NULL ||
                !editor_project_animation_frame_add(&project, animation,
                    "first", "assets/first.png", (Scale){16.0f, 16.0f}) ||
                !editor_project_animation_frame_add(&project, animation,
                    "second", "assets/second.png", (Scale){24.0f, 24.0f})) return 1;
        animation_id = animation->id;
        first_id = animation->frames[0].id;
        second_id = animation->frames[1].id;
        first = (EditorSelectionRef){EDITOR_SELECTION_ANIMATION_FRAME,
            frame_object->id, animation_id, 0, first_id};
        second = (EditorSelectionRef){EDITOR_SELECTION_ANIMATION_FRAME,
            frame_object->id, animation_id, 0, second_id};
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true) ||
                state.selected_item_count != 2) return 1;
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, second, false) ||
                !editor_navigation_selection_reorder(&project, &state,
                    second, first, false, &history) ||
                animation->frames[0].id != second_id || history.undo_count != 1 ||
                !editor_history_undo(&history)) return 1;
        frame_object = editor_project_selected_get(&project);
        animation = editor_project_animated_sprite_get(frame_object, animation_id);
        if(animation == NULL || animation->frames[0].id != first_id ||
                !editor_history_redo(&history)) return 1;
        frame_object = editor_project_selected_get(&project);
        animation = editor_project_animated_sprite_get(frame_object, animation_id);
        if(animation == NULL || animation->frames[0].id != second_id) return 1;
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, first, false) ||
                !editor_viewport_selection_set(&project, &state, second, true) ||
                !editor_navigation_multi_selection_delete(&project, &state, &history))
            return 1;
        frame_object = editor_project_selected_get(&project);
        animation = editor_project_animated_sprite_get(frame_object, animation_id);
        if(animation == NULL || animation->frame_count != 0 ||
                history.undo_count != 1 || !editor_history_undo(&history)) return 1;
        frame_object = editor_project_selected_get(&project);
        animation = editor_project_animated_sprite_get(frame_object, animation_id);
        if(animation == NULL || animation->frame_count != 2) return 1;
    }
    {
        EditorObject *sprite_object = editor_project_selected_get(&project);
        size_t sprite_count = sprite_object == NULL ? 0 : sprite_object->sprite_count;
        size_t animation_count = sprite_object == NULL ? 0 :
            sprite_object->animated_sprite_count;
        EditorSprite *sprite = editor_project_sprite_add(&project, sprite_object,
            "standalone", "assets/standalone.png");
        EditorAnimatedSprite *animation = editor_project_animated_sprite_add(
            &project, sprite_object);
        EditorSelectionRef sprite_ref;
        EditorSelectionRef animation_ref;
        if(sprite == NULL || animation == NULL) return 1;
        sprite_ref = (EditorSelectionRef){EDITOR_SELECTION_SPRITE,
            sprite_object->id, 0, 0, sprite->id};
        animation_ref = (EditorSelectionRef){EDITOR_SELECTION_ANIMATED_SPRITE,
            sprite_object->id, 0, 0, animation->id};
        editor_history_reset(&history);
        editor_viewport_selection_clear(&state);
        if(!editor_viewport_selection_set(&project, &state, sprite_ref, false) ||
                !editor_viewport_selection_set(&project, &state,
                    animation_ref, true) || state.selected_item_count != 2 ||
                !editor_navigation_multi_selection_delete(
                    &project, &state, &history) ||
                sprite_object->sprite_count != sprite_count ||
                sprite_object->animated_sprite_count != animation_count ||
                history.undo_count != 1 || !editor_history_undo(&history)) return 1;
        sprite_object = editor_project_selected_get(&project);
        if(sprite_object == NULL ||
                sprite_object->sprite_count != sprite_count + 1 ||
                sprite_object->animated_sprite_count != animation_count + 1) return 1;
    }
    {
        EditorObject *pointer_object = editor_project_object_add(
            &project, (Position){0});
        EditorSprite *sprite;
        EditorAnimatedSprite *animation;
        Position center = {EDITOR_VIEWPORT_WIDTH * 0.5f,
            EDITOR_MENU_HEIGHT +
                (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * 0.5f};
        Position pointer;

        if(pointer_object == NULL) return 1;
        sprite = editor_project_sprite_add(
            &project, pointer_object, "rotated_sprite", "sprite.png");
        animation = editor_project_animated_sprite_add(
            &project, pointer_object);
        if(sprite == NULL || animation == NULL ||
                !editor_project_animation_frame_add(&project, animation,
                    "frame", "frame.png", (Scale){20.0f, 80.0f})) return 1;
        sprite->position = (Position){-100.0f, 0.0f};
        sprite->size = (Scale){80.0f, 20.0f};
        sprite->rotation = -45.0f;
        animation->editor_position = (Position){100.0f, 0.0f};
        animation->editor_rotation = -45.0f;
        project.viewport_local_view = false;
        project.viewport_camera_offset = (Vec2D){0};
        project.viewport_camera_zoom = 1.0f;
        editor_viewport_object_editor_enter(&state);

        pointer = (Position){center.x - 75.2512627f,
            center.y - 24.7487373f};
        if(!editor_viewport_update(&state, &project, pointer,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) || state.mode != EDITOR_VIEWPORT_SPRITE ||
                state.selection != EDITOR_SELECTION_SPRITE ||
                state.selected_sprite != sprite->id) return 1;
        (void)editor_viewport_update(&state, &project, pointer,
            MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        editor_viewport_object_editor_enter(&state);

        pointer = (Position){center.x + 75.2512627f,
            center.y - 24.7487373f};
        if(!editor_viewport_update(&state, &project, pointer,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false) ||
                state.mode != EDITOR_VIEWPORT_ANIMATED_SPRITE ||
                state.selection != EDITOR_SELECTION_ANIMATED_SPRITE ||
                state.selected_animated_sprite != animation->id) return 1;
    }
    {
        EditorProject click_project;
        EditorViewportState click_state = {0};
        EditorObject *clicked_object;
        EditorObject *fallback_object;
        EditorRigidBody *clicked_body;
        EditorHitbox *clicked_hitbox;
        EditorAnchor *clicked_anchor;
        EditorJoint *clicked_joint;
        EditorSoftBody *clicked_soft_body;
        EditorSoftNode *clicked_node;
        EditorSoftNode *clicked_node_b;
        EditorSoftNode *clicked_node_c;
        EditorSoftBeam *clicked_beam;
        EditorSprite *clicked_sprite;
        EditorAnimatedSprite *clicked_animation;
        EditorSelectionRef fallback;
        EditorSelectionRef selections[17];
        EditorSoftNodeId clicked_node_id;
        EditorSoftNodeId clicked_node_b_id;
        EditorSoftNodeId clicked_node_c_id;
        EditorSoftBeamId clicked_beam_id;
        EditorObjectId fallback_object_id;
        size_t selection_count = 0;

        editor_project_init(&click_project);
        editor_viewport_state_init(&click_state);
        fallback_object = editor_project_object_add(
            &click_project, (Position){500.0f, 500.0f});
        if(fallback_object == NULL) return 1;
        fallback_object_id = fallback_object->id;
        clicked_object = editor_project_object_add(&click_project, (Position){0});
        if(clicked_object == NULL || fallback_object == NULL) return 1;
        clicked_body = editor_project_rigid_body_add(
            &click_project, clicked_object);
        if(clicked_body == NULL) return 1;
        clicked_hitbox = &clicked_body->hitboxes[0];
        clicked_anchor = editor_project_anchor_add(&click_project, clicked_object,
            (Position){100.0f, 0.0f}, clicked_body->id);
        clicked_joint = editor_project_joint_add(
            &click_project, clicked_object, EDITOR_JOINT_SPRING);
        clicked_soft_body = editor_project_soft_body_add(
            &click_project, clicked_object);
        if(clicked_anchor == NULL || clicked_joint == NULL ||
                clicked_soft_body == NULL) return 1;
        clicked_node = editor_project_soft_node_add(&click_project,
            clicked_soft_body, (Position){0});
        if(clicked_node == NULL) return 1;
        clicked_node_id = clicked_node->id;
        clicked_node_b = editor_project_soft_node_add(&click_project,
            clicked_soft_body, (Position){20.0f, 0.0f});
        clicked_node_c = editor_project_soft_node_add(&click_project,
            clicked_soft_body, (Position){0.0f, 20.0f});
        if(clicked_node_b == NULL || clicked_node_c == NULL) return 1;
        clicked_node_b_id = clicked_node_b->id;
        clicked_node_c_id = clicked_node_c->id;
        clicked_beam = editor_project_soft_beam_add(&click_project,
            clicked_soft_body, clicked_node_id, clicked_node_b_id);
        if(clicked_beam == NULL) return 1;
        clicked_beam_id = clicked_beam->id;
        if(
                editor_project_soft_beam_add(&click_project, clicked_soft_body,
                    clicked_node_b_id, clicked_node_c_id) == NULL ||
                editor_project_soft_beam_add(&click_project, clicked_soft_body,
                    clicked_node_c_id, clicked_node_id) == NULL) return 1;
        editor_project_soft_areas_sync(&click_project, clicked_soft_body);
        clicked_sprite = editor_project_sprite_add(&click_project, clicked_object,
            "click_sprite", "click_sprite.png");
        clicked_animation = editor_project_animated_sprite_add(
            &click_project, clicked_object);
        if(clicked_soft_body->area_count == 0 || clicked_sprite == NULL ||
                clicked_animation == NULL ||
                !editor_project_animation_frame_add(&click_project,
                    clicked_animation, "click_frame", "click_frame.png",
                    (Scale){16.0f, 16.0f})) return 1;

        fallback = (EditorSelectionRef){EDITOR_SELECTION_OBJECT,
            fallback_object_id, 0, 0, fallback_object_id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_OBJECT, clicked_object->id, 0, 0,
            clicked_object->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_RIGID_BODY, clicked_object->id, 0, 0,
            clicked_body->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_PARTICLE, clicked_object->id, 0, 0,
            clicked_body->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_HITBOX, clicked_object->id, clicked_body->id, 0,
            clicked_hitbox->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_JOINT, clicked_object->id, 0, 0,
            clicked_joint->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_ANCHOR, clicked_object->id, 0, 0,
            clicked_anchor->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_SOFT_BODY, clicked_object->id, 0, 0,
            clicked_soft_body->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_SOFT_NODE, clicked_object->id,
            clicked_soft_body->id, 0, clicked_node_id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_SOFT_BEAM, clicked_object->id,
            clicked_soft_body->id, 0, clicked_beam_id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_SOFT_AREA, clicked_object->id,
            clicked_soft_body->id, 0, clicked_soft_body->areas[0].id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_ORIGIN, clicked_object->id,
            EDITOR_ORIGIN_RIGID_BODY, 0, clicked_body->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_ORIGIN, clicked_object->id,
            EDITOR_ORIGIN_SOFT_BODY, 0, clicked_soft_body->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_LINE, clicked_object->id, clicked_body->id,
            clicked_hitbox->id, 0};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_VERTEX, clicked_object->id, clicked_body->id,
            clicked_hitbox->id, clicked_hitbox->vertices[0].id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_SPRITE, clicked_object->id, 0, 0,
            clicked_sprite->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_ANIMATED_SPRITE, clicked_object->id, 0, 0,
            clicked_animation->id};
        selections[selection_count++] = (EditorSelectionRef){
            EDITOR_SELECTION_ANIMATION_FRAME, clicked_object->id,
            clicked_animation->id, 0, clicked_animation->frames[0].id};
        for(size_t i = 0; i < selection_count; i += 1)
            if(!modifier_click_toggle_check(&click_project, &click_state,
                    fallback, selections[i])) return 1;
        {
            bool visible = false;
            char name[EDITOR_OBJECT_NAME_MAX] = {0};
            EditorSelectionRef object_ref = selections[0];
            clicked_object = editor_object_query_get(&click_project,
                object_ref.object);
            if(clicked_object == NULL) return 1;
            clicked_object->visible = true;
            if(!editor_navigation_selection_visibility_get(&click_project,
                    object_ref, &visible) || !visible) {
                fprintf(stderr, "context visibility get failed\n"); return 1;
            }
            if(!editor_navigation_selection_visibility_set(&click_project,
                    object_ref, false) || clicked_object->visible) {
                fprintf(stderr, "context visibility set failed\n"); return 1;
            }
            if(!editor_navigation_selection_name_get(&click_project,
                    object_ref, name, sizeof(name)) || name[0] == '\0') {
                fprintf(stderr, "context name get failed\n"); return 1;
            }
            if(!editor_navigation_selection_name_set(&click_project,
                    object_ref, "Context Renamed") ||
                    strcmp(clicked_object->name, "ContextRenamed") != 0) {
                fprintf(stderr, "context selection actions failed\n");
                return 1;
            }
            {
                EditorLayoutViewport *layout =
                    editor_project_layout_viewport_add(&click_project);
                EditorViewportUiItem *ui = layout == NULL ? NULL :
                    editor_viewport_ui_add(&click_project, layout,
                        EDITOR_VIEWPORT_UI_SHAPE);
                EditorSelectionRef ui_ref;
                if(ui == NULL) return 1;
                ui_ref = (EditorSelectionRef){EDITOR_SELECTION_UI_SHAPE,
                    layout->id, 0, 0, ui->id};
                ui->visible = true;
                if(!editor_navigation_selection_visibility_get(&click_project,
                        ui_ref, &visible) || !visible ||
                        !editor_navigation_selection_visibility_set(&click_project,
                            ui_ref, false) || ui->visible ||
                        !editor_navigation_selection_visibility_set(&click_project,
                            ui_ref, true) || !ui->visible) {
                    fprintf(stderr, "context UI visibility synchronization failed\n");
                    return 1;
                }
            }
        }
        editor_viewport_state_destroy(&click_state);
        editor_project_destroy(&click_project);
    }
    editor_history_destroy(&history);
    editor_viewport_state_destroy(&state);
    return 0;
}
