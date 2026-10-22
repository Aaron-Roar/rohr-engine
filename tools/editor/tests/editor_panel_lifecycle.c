/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#define main editor_application_main
#include "../src/main.c"
#undef main
#include <assert.h>
#define OK(call) assert(!rohr_error_check(call))

static void frame_end(void) {
    Report report = rohr_ui_update();
    assert(!report.has_fatal && report.diagnostic_count == 0);
    (void)rohr_graphics_pipeline_commands_execute();
}
static void wheel(Position pointer) {
    SDL_Event event = {.type = SDL_EVENT_MOUSE_WHEEL}; event.wheel.y = -1;
    rohr_ui_event_add(&event);
    rohr_ui_input_set((UIInput){.pointer = pointer});
}

static void navigation_checks(EditorProject *project, EditorViewportState *state) {
    EditorObject *object = &project->objects[0];
    EditorPanelLifecycle panel = {0};
    assert(editor_panel_lifecycle_update(&panel, project, state));
    panel.current.offset = 91;
    state->mode = EDITOR_VIEWPORT_OBJECT;
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0);
    panel.current.offset = 231; panel.current.measured_height = 900;
    /* Merely selecting a child row must not change the open panel. */
    state->selection = EDITOR_SELECTION_SOFT_BODY;
    state->selected_soft_body = object->soft_body_items[0].id;
    assert(!editor_panel_lifecycle_update(&panel, project, state));
    assert(panel.current.key.item.kind == EDITOR_SELECTION_OBJECT);
    state->mode = EDITOR_VIEWPORT_SOFT_BODY;
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0);
    panel.current.offset = 177;
    state->mode = EDITOR_VIEWPORT_SOFT_NODE;
    state->selected_soft_node = object->soft_body_items[0].nodes[0].id;
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0);
    panel.current.offset = 45;
    /* A sibling of the same kind starts fresh. */
    state->selected_soft_node = object->soft_body_items[0].nodes[1].id;
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0);
    editor_panel_back(&panel, state);
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 177);
    editor_panel_back(&panel, state);
    assert(editor_panel_lifecycle_update(&panel, project, state));
    assert(panel.current.offset == 231 && panel.current.measured_height == 900);
    state->mode = EDITOR_VIEWPORT_SOFT_BODY;
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0);
    editor_panel_back(&panel, state); assert(editor_panel_lifecycle_update(&panel, project, state));
    editor_panel_back(&panel, state); assert(editor_panel_lifecycle_update(&panel, project, state));
    assert(panel.current.offset == 91);
    /* Deletion prunes ancestor entries, and resetting a project has no history. */
    state->mode = EDITOR_VIEWPORT_JOINT; state->selected_joint = object->joint_items[0].id;
    assert(editor_panel_lifecycle_update(&panel, project, state));
    state->mode = EDITOR_VIEWPORT_OBJECT; assert(editor_panel_lifecycle_update(&panel, project, state));
    assert(editor_project_joint_remove(object, state->selected_joint));
    (void)editor_panel_lifecycle_update(&panel, project, state);
    for(size_t i = 0; i < panel.count; ++i) assert(panel.previous[i].key.mode != EDITOR_VIEWPORT_JOINT);
    panel = (EditorPanelLifecycle){0};
    assert(editor_panel_lifecycle_update(&panel, project, state)); assert(panel.current.offset == 0 && panel.count == 0);
}

static void hitbox_return_checks(EditorProject *project, EditorViewportState *state, FontAsset *font) {
    EditorObject *object = &project->objects[0];
    EditorRigidBody *body = editor_project_rigid_body_add(project, object); assert(body && body->hitbox_count);
    state->selected_rigid_body = body->id; state->selected_hitbox = body->hitboxes[0].id;
    state->selection = EDITOR_SELECTION_RIGID_BODY;
    EditorPanelLifecycle panel = {0};
    EditorObjectEditor objects; assert(editor_object_editor_create(&objects, font));
    EditorRigidBodyEditor bodies; assert(editor_rigid_body_editor_create(&bodies, font));
    EditorHitboxEditor hitboxes; assert(editor_hitbox_editor_create(&hitboxes, font));
    EditorAutoShapeEditor shapes; assert(editor_auto_shape_editor_create(&shapes, font));
    EditorEntitySettingsPanel settings; assert(editor_entity_settings_panel_create(&settings, font));
    EditorViewportMode modes[] = {EDITOR_VIEWPORT_OBJECT, EDITOR_VIEWPORT_RIGID_BODY,
        EDITOR_VIEWPORT_HITBOX, EDITOR_VIEWPORT_RIGID_BODY, EDITOR_VIEWPORT_OBJECT};
    for(size_t step = 0; step < sizeof(modes) / sizeof(modes[0]); ++step) {
        if(step >= 3) editor_panel_back(&panel, state);
        else state->mode = modes[step];
        assert(state->mode == modes[step]);
        assert(editor_panel_lifecycle_update(&panel, project, state));
        assert(panel.current.offset == (step >= 3 ? 42 : 0));
        ui_interaction_cancel(); frame_end();
        float top = editor_panel_body_top_get(state->mode, true);
        for(unsigned frame = 0; frame < 2; ++frame) {
            if(frame) wheel((Position){40,220});
            else rohr_ui_input_set((UIInput){0});
            UIScrollRegionResult scroll = rohr_ui_scroll_region_begin("editor.tools.scroll",
                (UIRect){0,124,400,220}, panel.current.measured_height > 0 ?
                    panel.current.measured_height - top : 2000, panel.current.offset, 42);
            if(frame) assert(scroll.offset == panel.current.offset + 42);
            panel.current.offset = scroll.offset;
            rohr_ui_translation_y_push(124 - top);
            editor_mode_external_name_set(editor_mode_name_field_id_get(state->mode));
            editor_mode_accordion_layout_measure_reset();
            EditorModeContext context = {.project=project,.viewport=state,.width=400,.delete_footer=true};
            if(state->mode == EDITOR_VIEWPORT_OBJECT)
                (void)editor_object_editor_draw(&objects, &context, NULL, NULL, false);
            else if(state->mode == EDITOR_VIEWPORT_RIGID_BODY)
                (void)editor_rigid_body_editor_draw(&bodies, &context);
            else (void)editor_hitbox_editor_draw(&hitboxes, &shapes, &context);
            float bottom = fmaxf(editor_mode_accordion_layout_measure_get(), ui_scroll_content_height_get() + top);
            (void)editor_entity_settings_panel_draw(&settings, &context, bottom + 8);
            panel.current.measured_height = fmaxf(editor_mode_accordion_layout_measure_get(), ui_scroll_content_height_get() + top);
            editor_mode_external_name_set(NULL); rohr_ui_translation_y_pop();
            rohr_ui_scroll_region_end(); frame_end();
        }
    }
    ui_interaction_cancel(); frame_end();
    /* The editor text cache borrows these panel-owned slots. Clear it while
     * the local panels are still alive, as main does at project teardown. */
    editor_mode_named_text_clear_all();
    editor_entity_settings_panel_destroy(&settings); editor_auto_shape_editor_destroy(&shapes);
    editor_hitbox_editor_destroy(&hitboxes); editor_rigid_body_editor_destroy(&bodies);
    editor_object_editor_destroy(&objects);
}

static void input_checks(FontAsset *font) {
    TextAsset label; assert(editor_mode_text_create(font, "Option", &label));
    const TextAsset *options[] = {&label, &label};
    UIRect dropdown_bounds = {20, 200, 160, 30};
    for(unsigned phase = 0; phase < 2; ++phase) {
        rohr_ui_input_set((UIInput){.pointer = {40, 210}, .primary_button = phase ? MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_PRESSED});
        UIDropdownResult r = rohr_ui_dropdown("old.dropdown", options, 2, 0,
            (UIDropdownConfig){0}, dropdown_bounds, NULL);
        if(phase) assert(r.open);
        (void)rohr_ui_scroll_region_begin("old.child", (UIRect){0, 160, 250, 200}, 800, 0, 42);
        rohr_ui_scroll_region_end(); frame_end();
    }
    /* A fixed-header click dismisses the dropdown and acts on the first click. */
    for(unsigned phase = 0; phase < 2; ++phase) {
        rohr_ui_input_set((UIInput){.pointer = {40,100}, .primary_button = phase ?
            MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_PRESSED});
        if(!phase) ui_dropdown_dismiss();
        UIButtonResult back = rohr_ui_button("header.back", &label, (UIRect){20,86,100,30}, NULL);
        if(phase) assert(back.clicked);
        frame_end();
    }
    ui_interaction_cancel(); frame_end();
    wheel((Position){40, 220});
    UIScrollRegionResult scroll = rohr_ui_scroll_region_begin("editor.tools.scroll",
        (UIRect){0, 160, 300, 220}, 900, 0, 42);
    assert(scroll.offset == 42); rohr_ui_scroll_region_end(); frame_end();
    /* Back/header remains stationary and clickable above a scrolled body. */
    for(unsigned phase = 0; phase < 2; ++phase) {
        rohr_ui_input_set((UIInput){.pointer = {40, 100}, .primary_button = phase ? MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_PRESSED});
        UIButtonResult back = rohr_ui_button("header.back", &label, (UIRect){20, 86, 100, 30}, NULL);
        if(phase) assert(back.clicked);
        scroll = rohr_ui_scroll_region_begin("editor.tools.scroll", (UIRect){0, 160, 300, 220}, 900, 200, 42);
        UIButtonResult clipped = rohr_ui_button("body.row", &label, (UIRect){20, 286, 100, 30}, NULL);
        assert(!clipped.clicked && !clipped.hovered);
        assert(ui_scroll_content_height_get() == 156);
        rohr_ui_scroll_region_end(); frame_end();
    }
    /* Cancel pressed controls and text focus without leaking release/typing. */
    char name[32] = "saved";
    ui_field_focus_set("old.name", (UIFieldBinding){.kind=UI_FIELD_STRING, .string=name, .string_capacity=sizeof(name)}, &label, true);
    ui_interaction_cancel();
    rohr_ui_input_set((UIInput){.pointer={40,100}, .primary_button=MOUSE_BUTTON_STATE_RELEASED});
    assert(!rohr_ui_button("header.back", &label, (UIRect){20,86,100,30}, NULL).clicked);
    frame_end(); assert(!strcmp(name, "saved"));
    /* Clamped Back offsets and nested wheel routing use the current geometry. */
    rohr_ui_input_set((UIInput){0});
    scroll = rohr_ui_scroll_region_begin("editor.tools.scroll", (UIRect){0,160,300,220}, 260, 500, 42);
    assert(scroll.offset == 40);
    (void)rohr_ui_scroll_region_begin("nested", (UIRect){10,240,200,100}, 500, 0, 42);
    rohr_ui_scroll_region_end(); rohr_ui_scroll_region_end(); frame_end();
    wheel((Position){40,230});
    scroll = rohr_ui_scroll_region_begin("editor.tools.scroll", (UIRect){0,160,300,220}, 900, 40, 42);
    assert(scroll.offset == 40);
    scroll = rohr_ui_scroll_region_begin("nested", (UIRect){10,240,200,100}, 500, 0, 42);
    assert(scroll.offset == 42);
    rohr_ui_scroll_region_end(); rohr_ui_scroll_region_end(); frame_end();
    ui_interaction_cancel(); frame_end();
    rohr_ui_input_set((UIInput){0});
    (void)rohr_ui_button("header.back", &label, (UIRect){20,86,100,30}, NULL);
    (void)rohr_ui_scroll_region_begin("editor.tools.scroll", (UIRect){0,160,300,220}, 900, 0, 42);
    (void)rohr_ui_button("body.bottom", &label, (UIRect){20,600,100,30}, NULL);
    rohr_ui_scroll_region_end(); frame_end();
    assert(rohr_ui_navigation_move(UI_NAVIGATION_DOWN));
    assert(ui_navigation_scroll_delta_get("editor.tools.scroll") == 0);
    assert(rohr_ui_navigation_move(UI_NAVIGATION_DOWN));
    assert(ui_navigation_scroll_delta_get("editor.tools.scroll") == 250);
    ui_interaction_cancel(); frame_end();
    rohr_graphics_text_destroy(&label);
}

int main(void) {
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    FontAssetResult loaded = rohr_graphics_font_load((FontDescription){0}); OK(loaded);
    FontAsset font = loaded.result.value;
    static EditorProject project; editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){0}); assert(object);
    project.selected = object->id;
    EditorSoftBody *soft = editor_project_soft_body_add(&project, object); assert(soft);
    assert(editor_project_soft_node_add(&project, soft, (Position){0}));
    assert(editor_project_soft_node_add(&project, soft, (Position){4,0}));
    assert(editor_project_joint_add(&project, object, EDITOR_JOINT_REVOLUTE));
    EditorViewportState state; editor_viewport_state_init(&state);
    navigation_checks(&project, &state);
    input_checks(&font);
    hitbox_return_checks(&project, &state, &font);
    /* The parent list reports all rows before shared settings are appended. */
    EditorObjectEditor object_editor; assert(editor_object_editor_create(&object_editor, &font));
    EditorEntitySettingsPanel settings; assert(editor_entity_settings_panel_create(&settings, &font));
    state.mode = EDITOR_VIEWPORT_OBJECT; state.selection = EDITOR_SELECTION_SOFT_BODY;
    state.selected_soft_body = soft->id;
    EditorModeContext context = {.project=&project, .viewport=&state, .x=0, .width=400, .delete_footer=true};
    rohr_ui_input_set((UIInput){0}); editor_mode_accordion_layout_measure_reset();
    (void)editor_object_editor_draw(&object_editor, &context, NULL, NULL, false);
    float list_bottom = editor_mode_accordion_layout_measure_get();
    assert(list_bottom > 400);
    (void)editor_entity_settings_panel_draw(&settings, &context, list_bottom + 8);
    assert(editor_mode_accordion_layout_measure_get() > list_bottom);
    frame_end();
    editor_entity_settings_panel_destroy(&settings); editor_object_editor_destroy(&object_editor);
    editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    editor_mode_named_text_clear_all(); OK(rohr_graphics_font_release(&font)); rohr_engine_stop();
    return 0;
}
