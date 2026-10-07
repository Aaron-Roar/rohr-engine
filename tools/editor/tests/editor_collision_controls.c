/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/editor_collision_controls.h"
#include "editors/physics/editor_rigid_body.h"
#include "editors/physics/editor_particle.h"
#include "editors/soft_body/editor_soft_node.h"
#include "editors/soft_body/editor_soft_beam.h"
#include "editors/multi/editor_bulk_panel.h"
#include <assert.h>
#include <string.h>

float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static void begin(const EditorProject *project, const EditorCommand *command, void *context) {
    editor_history_command_begin(context, project, command);
}
static void finish(const EditorCommand *command, const EditorCommandResult *result, void *context) {
    editor_history_command_finish(context, command, result);
}
#define OK(expr) assert(!editor_result_check(expr))
static void frame(EditorCollisionControls *ui, EditorProject *project, EditorHistory *history,
        EditorSelectionRef *refs, size_t count, Position pointer, MouseButtonState button, float shift) {
    rohr_ui_frame_begin((UIInput){pointer,button});
    rohr_ui_clip_begin((UIRect){0,0,400,720});
    rohr_ui_translation_y_push(shift);
    (void)editor_collision_controls_draw(ui, "test.collision", project, history, refs, count, 10,40,350,true);
    rohr_ui_translation_y_pop(); rohr_ui_clip_end(); rohr_ui_frame_end();
}
static void click(EditorCollisionControls *ui, EditorProject *project, EditorHistory *history,
        EditorSelectionRef *refs, size_t count, Position pointer, float shift) {
    frame(ui,project,history,refs,count,pointer,MOUSE_BUTTON_STATE_PRESSED,shift);
    frame(ui,project,history,refs,count,pointer,MOUSE_BUTTON_STATE_RELEASED,shift);
    frame(ui,project,history,refs,count,pointer,MOUSE_BUTTON_STATE_UP,shift);
}
static void layout_check(EditorCollisionControls *ui) {
    assert(ui->open[0] && ui->open[1]);
    assert(ui->list_bounds[0].y == ui->header_bounds[0].y + 32);
    assert(ui->header_bounds[1].y == ui->list_bounds[0].y + ui->list_bounds[0].height);
    assert(ui->list_bounds[1].y == ui->header_bounds[1].y + 32);
    assert(ui->list_bounds[1].height > 0);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorProject project; EditorHistory history; EditorViewportState state;
    editor_project_init(&project); editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    assert(object != NULL);
    EditorRigidBodyId rigid = editor_project_rigid_body_add(&project, object)->id;
    EditorRigidBody *particle = editor_project_rigid_body_add(&project, object);
    EditorRigidBodyId particle_id = particle->id;
    particle->particle = true; particle->standalone_particle = true;
    EditorCommand add = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id}};
    assert(editor_command_execute(&project, &add).kind == ERROR_RESULT_VALUE);
    EditorSoftBody *soft = &object->soft_body_items[0];
    EditorSelectionRef refs[] = {
        {EDITOR_SELECTION_RIGID_BODY,object->id,0,0,rigid},
        {EDITOR_SELECTION_PARTICLE,object->id,0,0,particle_id},
        {EDITOR_SELECTION_SOFT_NODE,object->id,soft->id,0,soft->nodes[0].id},
        {EDITOR_SELECTION_SOFT_BEAM,object->id,soft->id,0,soft->beams[0].id}};
    assert(editor_history_init(&history, &project));
    editor_command_executing_callback_set(begin, &history);
    editor_command_finished_callback_set(finish, &history);
    EditorCollisionValues values;
    assert(editor_collision_values_get(&project, refs, 4, &values));
    OK(editor_collision_edit(&project,&history,refs,4,-1,NULL,false,false));
    assert(history.undo_count == 1);
    assert(editor_collision_values_get(&project,refs,4,&values) && !values.enabled_any);
    /* Disabled items retain fully editable filters, without enabling collision. */
    size_t masks_before = project.collision_mask_count;
    OK(editor_collision_edit(&project,&history,refs,4,0,"friends",true,true));
    assert(history.undo_count == 2 && project.collision_mask_count == masks_before + 1);
    uint64_t bit = UINT64_C(1) << masks_before;
    assert(editor_collision_values_get(&project,refs,4,&values) && (values.category_all & bit) && !values.enabled_any);
    assert(editor_history_undo(&history) && project.collision_mask_count == masks_before);
    assert(editor_collision_values_get(&project,refs,4,&values) && !(values.category_any & bit));
    assert(editor_history_redo(&history) && project.collision_mask_count == masks_before + 1);
    OK(editor_collision_edit(&project,&history,refs,1,0,"friends",false,false));
    assert(editor_collision_values_get(&project,refs,4,&values) && !(values.category_all & bit) && (values.category_any & bit));
    uint64_t unrelated = values.category_any & ~bit;
    OK(editor_collision_edit(&project,&history,refs,4,0,"friends",true,false));
    assert(editor_collision_values_get(&project,refs,4,&values) && (values.category_all & bit));
    assert((values.category_any & ~bit) == unrelated);
    uint64_t categories = values.category_all;
    uint64_t with_unrelated = values.with_any & ~bit;
    OK(editor_collision_edit(&project,&history,refs,4,1,"friends",true,false));
    assert(editor_collision_values_get(&project,refs,4,&values));
    assert(values.category_all == categories && (values.with_all & bit));
    assert((values.with_any & ~bit) == with_unrelated);
    size_t rejected_undo = history.undo_count;
    assert(editor_result_check(editor_collision_edit(&project,&history,refs,4,0,"",true,true)));
    assert(history.undo_count == rejected_undo && project.collision_mask_count == masks_before + 1);
    /* One invalid beam rejects the entire enable operation, including earlier targets. */
    soft = &project.objects[0].soft_body_items[0];
    soft->beams[0].node_a = 0;
    size_t undo_before = history.undo_count;
    assert(editor_result_check(editor_collision_edit(&project,&history,refs,4,-1,NULL,true,false)));
    assert(history.undo_count == undo_before);
    assert(editor_collision_values_get(&project,refs,4,&values) && !values.enabled_any);
    soft = &project.objects[0].soft_body_items[0]; soft->beams[0].node_a = soft->nodes[0].id;
    EditorSelectionRef incompatible[] = {refs[0],{EDITOR_SELECTION_SOFT_BODY,refs[0].object,0,0,soft->id}};
    assert(!editor_collision_values_get(&project,incompatible,2,&values));
    assert(editor_result_check(editor_collision_edit(&project,&history,incompatible,2,-1,NULL,true,false)));
    /* Real pointer input: own-section expansion, mixed bit click, independent fields, scrolling. */
    EditorCollisionControls ui;
    assert(editor_collision_controls_create(&ui,&font));
    editor_collision_controls_selection_set(&ui,refs,4);
    click(&ui,&project,&history,refs,4,(Position){100,80},0);
    assert(ui.open[0] && !ui.open[1]);
    click(&ui,&project,&history,refs,4,(Position){100,ui.header_bounds[1].y + 10},0);
    layout_check(&ui);
    OK(editor_collision_edit(&project,&history,refs,1,-1,NULL,true,false));
    assert(editor_collision_values_get(&project,refs,4,&values) && values.enabled_any && !values.enabled_all);
    click(&ui,&project,&history,refs,4,(Position){100,50},0);
    assert(editor_collision_values_get(&project,refs,4,&values) && values.enabled_all);
    click(&ui,&project,&history,refs,4,(Position){100,50},0);
    assert(editor_collision_values_get(&project,refs,4,&values) && !values.enabled_any);
    OK(editor_collision_edit(&project,&history,refs,1,0,"friends",false,false));
    float row_y = ui.list_bounds[0].y + 32 * (masks_before + 1) + 10;
    click(&ui,&project,&history,refs,4,(Position){100,row_y - 32},-32);
    assert(editor_collision_values_get(&project,refs,4,&values) && (values.category_all & bit));
    layout_check(&ui);
    click(&ui,&project,&history,refs,4,(Position){100,row_y},0);
    assert(editor_collision_values_get(&project,refs,4,&values) && !(values.category_any & bit));
    ui.names[0][0] = 'a'; ui.names[1][0] = 'b';
    editor_collision_controls_selection_set(&ui,refs+1,1);
    assert(!ui.open[0] && !ui.open[1] && ui.names[0][0] == 0 && ui.names[1][0] == 0);
    editor_collision_controls_destroy(&ui);
    /* Each actual editor is wired to the same measured controls while disabled. */
    EditorRigidBodyEditor rb; EditorSoftNodeEditor node; EditorSoftBeamEditor beam;
    EditorParticleEditor pe; EditorBulkPanel bulk;
    assert(editor_rigid_body_editor_create(&rb,&font));
    assert(editor_soft_node_editor_create(&node,&font));
    assert(editor_soft_beam_editor_create(&beam,&font));
    assert(editor_particle_editor_create(&pe,&font));
    assert(editor_bulk_panel_create(&bulk,&font));
    EditorCollisionControls *panels[] = {&rb.collision,&pe.collision,&node.collision,&beam.collision,&bulk.collision};
    for(size_t p = 0; p < 5; p += 1) {
        editor_viewport_selection_clear(&state);
        if(p < 4) {
            assert(editor_viewport_selection_set(&project,&state,refs[p],false));
            state.mode = p == 0 ? EDITOR_VIEWPORT_RIGID_BODY : p == 1 ? EDITOR_VIEWPORT_PARTICLE :
                p == 2 ? EDITOR_VIEWPORT_SOFT_NODE : EDITOR_VIEWPORT_SOFT_BEAM;
        } else {
            assert(editor_viewport_selection_set(&project,&state,refs[0],false));
            assert(editor_viewport_selection_set(&project,&state,refs[3],true));
        }
        editor_collision_controls_selection_set(panels[p],p < 4 ? &refs[p] : state.selected_items,p < 4 ? 1 : 2);
        panels[p]->open[0] = panels[p]->open[1] = true;
        rb.collision_section.expanded = node.collision_section.expanded = true;
        rohr_ui_frame_begin((UIInput){0});
        EditorModeContext context = {.project=&project,.history=&history,.viewport=&state,.width=400};
        if(p == 0) (void)editor_rigid_body_editor_draw(&rb,&context);
        if(p == 1) (void)editor_particle_editor_draw(&pe,&context);
        if(p == 2) (void)editor_soft_node_editor_draw(&node,&context);
        if(p == 3) (void)editor_soft_beam_editor_draw(&beam,&context);
        if(p == 4) (void)editor_bulk_panel_draw(&bulk,&project,&state,&history,NULL,0,400,650,NULL,NULL);
        rohr_ui_frame_end();
        layout_check(panels[p]);
        if(p == 4) {
            size_t prior_undo = history.undo_count;
            Position pointer = {50, panels[p]->header_bounds[0].y - 22};
            MouseButtonState buttons[] = {MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_RELEASED};
            for(size_t i = 0; i < 2; i += 1) {
                rohr_ui_frame_begin((UIInput){pointer,buttons[i]});
                (void)editor_bulk_panel_draw(&bulk,&project,&state,&history,NULL,0,400,650,NULL,NULL);
                rohr_ui_frame_end();
            }
            assert(history.undo_count == prior_undo + 1);
            assert(editor_collision_values_get(&project,state.selected_items,2,&values) && values.enabled_all);
            assert(editor_history_undo(&history));
            assert(editor_collision_values_get(&project,state.selected_items,2,&values) && !values.enabled_any);
            assert(state.selected_item_count == 2 && state.mode == EDITOR_VIEWPORT_SOFT_BEAM);
        }
    }
    editor_bulk_panel_destroy(&bulk); editor_particle_editor_destroy(&pe);
    editor_soft_beam_editor_destroy(&beam); editor_soft_node_editor_destroy(&node);
    editor_rigid_body_editor_destroy(&rb);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    editor_viewport_assets_destroy(); rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
