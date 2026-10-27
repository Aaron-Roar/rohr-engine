/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#define main editor_application_main
#include "../src/main.c"
#undef main
#include "editor_soft_area.h"
#include <assert.h>
#define OK(call) assert(!rohr_error_check(call))

static EditorObject *pair(EditorProject *p, const char *name) {
    EditorObject *o = editor_project_object_add(p, (Position){0}); assert(o);
    snprintf(o->name, sizeof(o->name), "%s", name);
    EditorRigidBody *a = editor_project_rigid_body_add(p, o); assert(a); uint32_t aid = a->id;
    EditorRigidBody *b = editor_project_rigid_body_add(p, o); assert(b);
    a = editor_project_rigid_body_get(o, aid);
    snprintf(a->name, sizeof(a->name), "left"); snprintf(b->name, sizeof(b->name), "right");
    a->position = (Position){-100, 0}; b->position = (Position){80, 0};
    EditorAnchor *aa = editor_project_anchor_add(p, o, (Position){90, 0}, aid); assert(aa); uint32_t aa_id = aa->id;
    EditorAnchor *bb = editor_project_anchor_add(p, o, (Position){-90, 0}, b->id); assert(bb);
    EditorJoint *joint = editor_project_joint_add(p, o, EDITOR_JOINT_WELD); assert(joint);
    snprintf(joint->name, sizeof(joint->name), "connection");
    joint->anchor_a = aa_id; joint->anchor_b = bb->id;
    a->rotation_locked = true;
    return o;
}
static EditorCommand rest(EditorObject *o, float angle) {
    return (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET, .data.property_set = {
        .kind = EDITOR_ITEM_JOINT, .object = o->id, .item = o->joint_items[0].id,
        .property = EDITOR_PROPERTY_REST_ANGLE, .value_kind = EDITOR_PROPERTY_VALUE_FLOAT, .value.number = angle}};
}
static void lifecycle(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = pair(&p, "Conflict");
    EditorDiagnostics d = {0}; editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    EditorHistory history; assert(editor_history_init(&history, &p));
    EditorNotificationPanel notifications = {0}; FontAsset font = rohr_graphics_font_default_get();
    assert(editor_notification_panel_create(&notifications, &font, &font));
    editor_operation_project = &p; editor_operation_history = &history;
    editor_command_executing_callback_set(editor_operation_command_executing, &history);
    editor_command_finished_callback_set(editor_operation_command_finished, &notifications);
    EditorCommand c = rest(o, 45); EditorCommandResult result = editor_command_execute(&p, &c);
    assert(result.kind == ERROR_RESULT_VALUE && result.placement.blocked);
    EditorSelectionRef joint = {EDITOR_SELECTION_JOINT, o->id, 0, 0, o->joint_items[0].id};
    EditorSelectionRef body = {EDITOR_SELECTION_RIGID_BODY, o->id, 0, 0, o->rigid_bodies[0].id};
    EditorSelectionRef parent = {EDITOR_SELECTION_OBJECT, o->id, 0, 0, o->id};
    editor_diagnostics_refresh(&d, &p, NULL);
    assert(editor_diagnostics_item_get(&d, joint, false));
    assert(editor_diagnostics_item_get(&d, body, false));
    assert(editor_diagnostics_item_get(&d, parent, true));
    assert(!editor_diagnostics_item_get(&d, parent, false));
    assert(editor_diagnostics_field_check(&d, joint, "editor.joint.rest_angle"));
    assert(editor_diagnostics_section_check(&d, joint, "editor.joint.section.parameters"));
    assert(editor_diagnostics_section_check(&d, joint, "editor.mode.properties"));
    assert(!editor_diagnostics_section_check(&d, joint, "editor.joint.section.appearance"));
    assert(editor_diagnostics_section_check(&d, body, "editor.entity_settings.section.3"));
    assert(!editor_diagnostics_section_check(&d, body, "editor.entity_settings.section.0"));
    EditorModeAccordionSection section = {0};
    assert(editor_mode_accordion_section_create(&section, &font, "Parameters", false));
    editor_mode_diagnostics_set(&d, joint);
    rohr_ui_input_set((UIInput){0});
    assert(!editor_mode_accordion_section_draw(&section, "editor.joint.section.parameters", (UIRect){0, 0, 300, 30}, 50));
    editor_mode_accordion_section_expanded_set(&section, true);
    assert(editor_mode_accordion_section_draw(&section, "editor.joint.section.parameters", (UIRect){0, 40, 300, 30}, 50));
    rohr_ui_update();
    Report rendered = rohr_graphics_pipeline_commands_execute(); assert(!rendered.has_fatal);
    editor_mode_accordion_section_destroy(&section);
    editor_mode_diagnostics_set(NULL, (EditorSelectionRef){0});
    assert(!editor_diagnostics_field_check(&d, joint, "editor.joint.name"));
    EditorNotificationRecord *record = editor_notification_store_newest_get(&notifications.store, 0);
    assert(record && strstr(record->detail, "Conflict") && strstr(record->detail, "connection"));
    assert(strstr(record->detail, "left") && strstr(record->detail, "right"));
    size_t notification_count = notifications.store.entry_count;
    assert(editor_history_undo(&history)); editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    assert(editor_history_redo(&history)); editor_diagnostics_refresh(&d, &p, NULL); assert(d.count > 0);
    assert(notifications.store.entry_count == notification_count);
    o = &p.objects[0]; c = rest(o, 0); OK(editor_command_execute(&p, &c));
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    /* Rejected numeric edit leaves valid stored state and no red indicators. */
    c = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET, .data.property_set = {
        .kind = EDITOR_ITEM_RIGID_BODY, .object = o->id, .item = o->rigid_bodies[0].id,
        .property = EDITOR_PROPERTY_MASS, .value_kind = EDITOR_PROPERTY_VALUE_FLOAT, .value.number = -1}};
    assert(editor_command_execute(&p, &c).kind == ERROR_RESULT_ERROR);
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    assert(notifications.store.entry_count == notification_count + 1);
    /* Impossible pointer motion never adds notifications or poisons diagnostics. */
    c = (EditorCommand){.type = EDITOR_COMMAND_RIGID_BODY_TRANSFORM, .pointer_transform = true,
        .data.rigid_body_transform = {o->id, o->rigid_bodies[1].id, {80, 0}, 45}};
    assert(editor_command_execute(&p, &c).kind == ERROR_RESULT_ERROR);
    assert(notifications.store.entry_count == notification_count + 1);
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    /* Independent errors survive repairing another item. */
    o->joint_items[0].anchor_b = 0;
    o->rigid_bodies[0].entity_settings.transform_driver = (ObjectEntityReference){OBJECT_BODY, 9999, 0};
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 2);
    assert(editor_diagnostics_field_check(&d, joint, "editor.joint.anchor_b"));
    assert(!editor_diagnostics_field_check(&d, joint, "editor.joint.anchor_a"));
    o->joint_items[0].anchor_b = o->anchors[1].id;
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 1);
    o->rigid_bodies[0].entity_settings.transform_driver = (ObjectEntityReference){0};
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    /* Draft rejection reports once and never becomes a document diagnostic. */
    editor_mode_diagnostics_set(&d, body);
    editor_mode_field_rejected_callback_set(editor_operation_field_rejected, &notifications);
    TextAsset field = {0}; assert(editor_mode_text_create(&font, "", &field));
    EditorMixedNumberField draft = {.text = "invalid", .active = true}; float value = 12;
    rohr_ui_input_set((UIInput){0});
    UIFieldResult edit = editor_mode_number_blur_field(&draft, "editor.entity_settings.axis-angle", &field,
        (UIRect){0, 0, 200, 28}, &value);
    assert(!edit.changed && value == 12);
    record = editor_notification_store_newest_get(&notifications.store, 0);
    assert(record && strstr(record->detail, "left") && strstr(record->detail, "axis-angle"));
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    rohr_ui_update(); rohr_graphics_text_destroy(&field);
    editor_mode_field_rejected_callback_set(NULL, NULL);
    editor_mode_diagnostics_set(NULL, (EditorSelectionRef){0});
    editor_diagnostics_refresh(&d, NULL, NULL); assert(d.count == 0);
    editor_command_executing_callback_set(NULL, NULL); editor_command_finished_callback_set(NULL, NULL);
    editor_operation_project = NULL; editor_operation_history = NULL;
    editor_history_destroy(&history); editor_notification_panel_destroy(&notifications);
    editor_diagnostics_destroy(&d); editor_project_destroy(&p);
}
static void identities(void) {
    EditorDiagnostics d = {0};
    EditorDiagnostic items[] = {
        {.target = {EDITOR_SELECTION_SOFT_NODE, 1, 2, 0, 3}},
        {.target = {EDITOR_SELECTION_SOFT_HOLE, 1, 2, 4, 5}},
        {.target = {EDITOR_SELECTION_LAYOUT_VIEWPORT, 1, 1, 0, 6}},
        {.target = {EDITOR_SELECTION_UI_TEXT, 1, 0, 0, 7}}
    };
    d.items = items; d.count = sizeof(items) / sizeof(items[0]);
    assert(editor_diagnostics_item_get(&d, (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY, 1, 0, 0, 2}, true));
    assert(editor_diagnostics_item_get(&d, (EditorSelectionRef){EDITOR_SELECTION_SOFT_AREA, 1, 2, 0, 4}, true));
    assert(editor_diagnostics_item_get(&d, (EditorSelectionRef){EDITOR_SELECTION_LAYOUT_VIEWPORT, 0, 0, 0, 1}, true));
    assert(!editor_diagnostic_target_check(items[3].target, (EditorSelectionRef){EDITOR_SELECTION_OBJECT, 1, 0, 0, 1}, true));
    assert(!editor_diagnostics_item_get(&d, (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY, 9, 0, 0, 2}, true));
}
static void startup_notifications(void) {
    static EditorProject p; editor_project_init(&p); EditorObject *o = pair(&p, "OnLoad");
    EditorCommand c = rest(o, 45); assert(editor_command_execute(&p, &c).placement.blocked);
    EditorDiagnostics current = {0}, previous = {0};
    EditorNotificationPanel panel = {0}; FontAsset font = rohr_graphics_font_default_get();
    assert(editor_notification_panel_create(&panel, &font, &font));
    editor_diagnostics_refresh(&current, &p, NULL);
    editor_diagnostic_notifications_update(&panel, &p, NULL, &current, &previous);
    assert(panel.store.entry_count == current.count);
    size_t count = panel.store.entry_count;
    EditorDiagnostic saved = current.items[0];
    previous = current; current = (EditorDiagnostics){0};
    editor_diagnostics_refresh(&current, &p, NULL);
    editor_diagnostic_notifications_update(&panel, &p, NULL, &current, &previous);
    assert(panel.store.entry_count == count); /* unchanged frame, no repeat */
    c = rest(o, 0); OK(editor_command_execute(&p, &c));
    editor_diagnostics_refresh(&previous, &p, NULL); assert(previous.count == 0);
    editor_diagnostic_notifications_update(&panel, &p, NULL, &previous, &current);
    assert(panel.store.entry_count == count); /* history retained after repair */
    c = rest(o, 45); assert(editor_command_execute(&p, &c).placement.blocked);
    editor_diagnostics_refresh(&current, &p, NULL);
    editor_diagnostic_notifications_update(&panel, &p, NULL, &current, &previous);
    assert(panel.store.entry_count == count * 2); /* reappearance is a new event */
    assert(editor_diagnostics_contains_check(&current, &saved));
    /* New project/open uses an empty comparison snapshot even with equal IDs. */
    editor_diagnostic_notifications_update(&panel, &p, NULL, &current, NULL);
    assert(panel.store.entry_count == count * 3);
    editor_diagnostics_destroy(&current); editor_diagnostics_destroy(&previous);
    editor_notification_panel_destroy(&panel); editor_project_destroy(&p);
}
static void other_types(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){0}); assert(o);
    EditorSoftBody *soft = editor_project_soft_body_add(&p, o); assert(soft);
    EditorSoftNode *node = editor_project_soft_node_add(&p, soft, (Position){0}); assert(node);
    node->node_mass = 0;
    EditorSoftArea *area = editor_soft_area_add(soft); assert(area);
    EditorDiagnostics d = {0}; editor_diagnostics_refresh(&d, &p, NULL);
    EditorSelectionRef n = {EDITOR_SELECTION_SOFT_NODE, o->id, soft->id, 0, node->id};
    EditorSelectionRef a = {EDITOR_SELECTION_SOFT_AREA, o->id, soft->id, 0, area->id};
    assert(editor_diagnostics_field_check(&d, n, "editor.soft_node.mass"));
    assert(editor_diagnostics_item_get(&d, a, false));
    assert(editor_diagnostics_item_get(&d, (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY, o->id, 0, 0, soft->id}, true));
    node->node_mass = 1;
    assert(editor_soft_area_remove(soft, area->id));
    editor_diagnostics_refresh(&d, &p, NULL); assert(d.count == 0);
    EditorCamera *camera = editor_project_camera_add(&p, o); assert(camera);
    camera->zoom = 0; editor_diagnostics_refresh(&d, &p, NULL);
    assert(editor_diagnostics_field_check(&d, (EditorSelectionRef){EDITOR_SELECTION_CAMERA, o->id, 0, 0, camera->id}, "editor.camera.zoom"));
    EditorViewportState state = {0}; editor_viewport_state_init(&state); state.diagnostics = &d;
    p.selected = o->id; state.mode = EDITOR_VIEWPORT_OBJECT;
    editor_viewport_draw(&p, &state, false);
    Report report = rohr_graphics_pipeline_commands_execute(); assert(!report.has_fatal);
    editor_viewport_state_destroy(&state);
    editor_viewport_assets_destroy(); editor_diagnostics_destroy(&d); editor_project_destroy(&p);
}
int main(int argc, char **argv) {
    OK(rohr_engine_start()); OK(rohr_graphics_start()); lifecycle(); identities(); other_types(); startup_notifications();
    if(argc >= 2) {
        char generated_path[EDITOR_WORKSPACE_PATH_MAX];
        const char *path = argv[argc-1];
        if(argc == 2) {
            snprintf(generated_path, sizeof(generated_path), "%s_%llu", path, (unsigned long long)SDL_GetPerformanceCounter());
            path = generated_path;
        }
        static EditorProject p; EditorWorkspace w = {0};
        OK(editor_workspace_create(&w, &p, path));
        EditorObject *o = pair(&p, "BlockedWeld"); EditorCommand c = rest(o, 45);
        assert(editor_command_execute(&p, &c).placement.blocked);
        o->overview_position = (Position){-300, 0};
        o = pair(&p, "MissingEndpoint"); o->joint_items[0].anchor_b = 0; o->overview_position = (Position){300, 0};
        p.selected = p.objects[0].id;
        assert(editor_workspace_save(&w, &p));
        EditorDiagnostics d = {0}; editor_diagnostics_refresh(&d, &p, &w); assert(d.count > 0);
        size_t count = d.count; EditorProject loaded; editor_project_init(&loaded);
        OK(editor_workspace_load(&w, &loaded, path));
        editor_diagnostics_refresh(&d, &loaded, &w); assert(d.count == count);
        editor_diagnostics_destroy(&d); editor_project_destroy(&loaded); editor_project_destroy(&p);
    }
    editor_mode_named_text_clear_all(); rohr_graphics_stop(); rohr_engine_stop();
    puts("editor error presentation passed"); return 0;
}
