/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_physics_settings_panel.h"

#include <math.h>

static UIFieldResult physics_field(const char *id, TextAsset *label,
        TextAsset *field, float *value, float x, float y, float width) {
    rohr_ui_label(label, (UIRect){x + 8.0f, y, 190.0f, 28.0f});
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){x + 206.0f, y, width - 216.0f, 28.0f}, NULL);
}

bool editor_physics_settings_panel_create(EditorPhysicsSettingsPanel *panel,
        FontAsset *font) {
    if(panel == NULL || font == NULL) return false;
    *panel = (EditorPhysicsSettingsPanel){0};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &panel->member)) goto fail
    CREATE("Physics", menu_label); CREATE("Project Physics Settings", title);
    CREATE("Close", close_label); CREATE("Engine Timestep", engine_timestep_label);
    CREATE("Override Physics Timestep", override_label);
    CREATE("Physics Timestep", physics_timestep_label);
    CREATE("Substeps", substeps_label); CREATE("Gravity X", gravity_x_label);
    CREATE("Gravity Y", gravity_y_label);
    CREATE("Solver Iterations", solver_iterations_label);
    CREATE("", engine_timestep_field); CREATE("", physics_timestep_field);
    CREATE("", substeps_field); CREATE("", gravity_x_field);
    CREATE("", gravity_y_field); CREATE("", solver_iterations_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&panel->timing_section, font,
            "Timing", true) ||
            !editor_mode_accordion_section_create(&panel->world_section, font,
                "World", true) ||
            !editor_mode_accordion_section_create(&panel->solver_section, font,
                "Solver Settings", true)) goto fail;
    return true;
fail:
    editor_physics_settings_panel_destroy(panel);
    return false;
}

void editor_physics_settings_panel_open(EditorPhysicsSettingsPanel *panel) {
    if(panel != NULL) panel->open = true;
}

void editor_physics_settings_apply(const EditorProject *project) {
    if(project == NULL) return;
    (void)rohr_engine_time_per_tick_set(project->engine_time_per_tick);
    if(project->physics_timestep_override)
        (void)rohr_physics_dt_per_tick_set(project->physics_dt_per_tick);
    else
        rohr_physics_engine_time_per_tick_use();
    (void)rohr_physics_substeps_set(project->physics_substeps);
    (void)rohr_physics_gravity_set(project->physics_gravity);
    (void)rohr_physics_solver_iterations_set(
        project->physics_solver_iterations);
}

void editor_physics_settings_panel_draw(EditorPhysicsSettingsPanel *panel,
        EditorProject *project, UIRect bounds) {
    float engine_dt, physics_dt, substeps, gravity_x, gravity_y, iterations;
    bool override;
    bool changed = false;
    if(panel == NULL || project == NULL || !panel->open) return;
    engine_dt = (float)project->engine_time_per_tick;
    physics_dt = (float)project->physics_dt_per_tick;
    substeps = (float)project->physics_substeps;
    gravity_x = project->physics_gravity.x;
    gravity_y = project->physics_gravity.y;
    iterations = (float)project->physics_solver_iterations;
    override = project->physics_timestep_override;
    rohr_ui_modal_controls_begin();
    rohr_ui_surface(bounds, (Color){37, 42, 52, 255});
    rohr_ui_border(bounds, 2.0f, (Color){5, 6, 8, 255});
    rohr_ui_label(&panel->title,
        (UIRect){bounds.x + 20.0f, bounds.y + 14.0f,
            bounds.width - 40.0f, 32.0f});
    EditorModeAccordionLayoutCursor cursor =
        editor_mode_accordion_layout_cursor_get(
            bounds.x + 18.0f, bounds.width - 36.0f, bounds.y + 62.0f);
    const float timing_rows[] = {28.0f, 28.0f, 28.0f, 28.0f};
    const float world_rows[] = {28.0f, 28.0f};
    const float solver_rows[] = {28.0f};
    size_t timing_count = override ? 4 : 3;
    EditorModeAccordionLayoutResult timing =
        editor_mode_accordion_layout_section(&cursor, &panel->timing_section,
            "editor.settings.physics.timing", timing_rows, timing_count, 10.0f);
    EditorModeAccordionLayoutResult world =
        editor_mode_accordion_layout_section(&cursor, &panel->world_section,
            "editor.settings.physics.world", world_rows, 2, 10.0f);
    EditorModeAccordionLayoutResult solver =
        editor_mode_accordion_layout_section(&cursor, &panel->solver_section,
            "editor.settings.physics.solver", solver_rows, 1, 10.0f);
    if(timing.expanded) {
        float row = timing.content_y;
        UIFieldResult result = physics_field("editor.settings.physics.engine_dt",
            &panel->engine_timestep_label, &panel->engine_timestep_field,
            &engine_dt, bounds.x + 18.0f, row, bounds.width - 36.0f);
        if(result.changed) {
            project->engine_time_per_tick = fmaxf(engine_dt, 0.000001f);
            changed = true;
        }
        row += 38.0f;
        if(editor_mode_checkbox_left("editor.settings.physics.override",
                &panel->override_label,
                (UIRect){bounds.x + 28.0f, row,
                    bounds.width - 56.0f, 28.0f}, &override)) {
            project->physics_timestep_override = override;
            changed = true;
        }
        row += 38.0f;
        if(override) {
            result = physics_field("editor.settings.physics.physics_dt",
                &panel->physics_timestep_label, &panel->physics_timestep_field,
                &physics_dt, bounds.x + 18.0f, row, bounds.width - 36.0f);
            if(result.changed) {
                project->physics_dt_per_tick = fmaxf(physics_dt, 0.000001f);
                changed = true;
            }
            row += 38.0f;
        }
        result = physics_field("editor.settings.physics.substeps",
            &panel->substeps_label, &panel->substeps_field, &substeps,
            bounds.x + 18.0f, row, bounds.width - 36.0f);
        if(result.changed) {
            project->physics_substeps = (uint32_t)fmaxf(1.0f, roundf(substeps));
            changed = true;
        }
    }
    if(world.expanded) {
        UIFieldResult x_result = physics_field("editor.settings.physics.gravity_x",
            &panel->gravity_x_label, &panel->gravity_x_field, &gravity_x,
            bounds.x + 18.0f, world.content_y, bounds.width - 36.0f);
        UIFieldResult y_result = physics_field("editor.settings.physics.gravity_y",
            &panel->gravity_y_label, &panel->gravity_y_field, &gravity_y,
            bounds.x + 18.0f, world.content_y + 38.0f, bounds.width - 36.0f);
        if(x_result.changed || y_result.changed) {
            project->physics_gravity = (Acceleration){gravity_x, gravity_y};
            changed = true;
        }
    }
    if(solver.expanded) {
        UIFieldResult result = physics_field(
            "editor.settings.physics.solver_iterations",
            &panel->solver_iterations_label, &panel->solver_iterations_field,
            &iterations, bounds.x + 18.0f, solver.content_y,
            bounds.width - 36.0f);
        if(result.changed) {
            project->physics_solver_iterations =
                (uint32_t)fmaxf(1.0f, roundf(iterations));
            changed = true;
        }
    }
    if(changed) editor_physics_settings_apply(project);
    if(rohr_ui_button("editor.settings.physics.close", &panel->close_label,
            (UIRect){bounds.x + bounds.width - 124.0f,
                bounds.y + bounds.height - 48.0f, 100.0f, 32.0f}, NULL).clicked)
        panel->open = false;
    rohr_ui_modal_controls_end();
}

void editor_physics_settings_panel_destroy(EditorPhysicsSettingsPanel *panel) {
    if(panel == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&panel->member)
    DESTROY(menu_label); DESTROY(title); DESTROY(close_label);
    DESTROY(engine_timestep_label); DESTROY(override_label);
    DESTROY(physics_timestep_label); DESTROY(substeps_label);
    DESTROY(gravity_x_label); DESTROY(gravity_y_label);
    DESTROY(solver_iterations_label); DESTROY(engine_timestep_field);
    DESTROY(physics_timestep_field); DESTROY(substeps_field);
    DESTROY(gravity_x_field); DESTROY(gravity_y_field);
    DESTROY(solver_iterations_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&panel->timing_section);
    editor_mode_accordion_section_destroy(&panel->world_section);
    editor_mode_accordion_section_destroy(&panel->solver_section);
    *panel = (EditorPhysicsSettingsPanel){0};
}
