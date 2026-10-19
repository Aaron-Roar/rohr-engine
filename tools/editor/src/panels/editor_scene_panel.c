/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_scene_panel.h"
#include "editor_layout.h"
#include "input/input_internal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static uint64_t count_add(uint64_t a, uint64_t b) {
    return UINT64_MAX - a < b ? UINT64_MAX : a + b;
}
static bool diagnostic_same_check(const ReportDiagnostic *a, const ReportDiagnostic *b) {
    return a->report_id == b->report_id && a->internal_stage == b->internal_stage &&
        a->severity == b->severity && a->code == b->code && a->recovery == b->recovery &&
        a->source.kind == b->source.kind && a->source.id == b->source.id &&
        a->target.kind == b->target.kind && a->target.id == b->target.id &&
        !strcmp(a->detail, b->detail);
}
static uint64_t notification_push(EditorScenePanel *p, const char *summary, const char *detail) {
    uint64_t previous = p->notifications->store.next_id;
    editor_notification_panel_push(p->notifications, summary, detail);
    EditorNotificationRecord *record = editor_notification_store_newest_get(&p->notifications->store, 0);
    return record && record->id != previous ? record->id : 0;
}
static void diagnostic_add(EditorScenePanel *p, const ReportDiagnostic *value) {
    char summary[160], detail[1500];
    size_t slot = 0;
    while(slot < p->group_count && !diagnostic_same_check(&p->groups[slot].diagnostic, value)) ++slot;
    bool fresh = slot == p->group_count;
    if(fresh && slot == EDITOR_SCENE_DIAGNOSTIC_GROUPS) {
        /* Reserve the last slot for the first fatal even after ordinary overflow. */
        if(value->severity == ROHR_REPORT_SEVERITY_FATAL &&
                p->groups[slot - 1].diagnostic.severity != ROHR_REPORT_SEVERITY_FATAL) {
            --slot;
            p->omitted = count_add(p->omitted, p->groups[slot].diagnostic.occurrence_count);
            p->groups[slot] = (EditorSceneDiagnosticGroup){0};
        } else {
            p->omitted = count_add(p->omitted, value->occurrence_count);
            return;
        }
    } else if(fresh) ++p->group_count;
    EditorSceneDiagnosticGroup *group = &p->groups[slot];
    if(fresh) group->diagnostic = *value;
    else {
        group->diagnostic.occurrence_count = count_add(group->diagnostic.occurrence_count, value->occurrence_count);
        group->diagnostic.detail_truncated |= value->detail_truncated;
        if(value->has_substep) {
            if(!group->diagnostic.has_substep) group->diagnostic.first_substep = value->first_substep;
            else group->diagnostic.first_substep = SDL_min(group->diagnostic.first_substep, value->first_substep);
            group->diagnostic.last_substep = SDL_max(group->diagnostic.last_substep, value->last_substep);
            group->diagnostic.has_substep = true;
        }
    }
    const ReportDiagnostic *d = &group->diagnostic;
    const char *severity = d->severity == ROHR_REPORT_SEVERITY_FATAL ? "Error" :
        d->severity == ROHR_REPORT_SEVERITY_WARNING ? "Warning" : "Info";
    snprintf(summary, sizeof(summary), "Scene %s %d (%llu%s)", severity, (int)d->code,
        (unsigned long long)d->occurrence_count, d->occurrence_count == UINT64_MAX ? "+" : "");
    snprintf(detail, sizeof(detail),
        "%s\n\nReport: %u  Stage: %u\nSeverity: %s  Code: %d  Recovery: %u\n"
        "Source: %u:%llu  Target: %u:%llu\nOccurrences: %llu%s\n"
        "Substeps: %s %u..%u\nDetail truncated: %s",
        d->detail, (unsigned)d->report_id, d->internal_stage, severity, (int)d->code,
        (unsigned)d->recovery, (unsigned)d->source.kind, (unsigned long long)d->source.id,
        (unsigned)d->target.kind, (unsigned long long)d->target.id,
        (unsigned long long)d->occurrence_count, d->occurrence_count == UINT64_MAX ? " (saturated)" : "",
        d->has_substep ? "yes" : "no", d->first_substep, d->last_substep,
        d->detail_truncated ? "yes" : "no");
    if(fresh) {
        group->notification = notification_push(p, summary, detail);
        char line[1700];
        snprintf(line, sizeof(line), "\n[%s]\n%s\n", summary, detail);
        (void)editor_terminal_panel_output_write(p->terminal, line);
    } else {
        /* Evicted log entries stay evicted; repetition must not create new toasts. */
        (void)editor_notification_panel_update(p->notifications, group->notification, summary, detail);
    }
}
void editor_scene_panel_report(void *context, const Report *report) {
    EditorScenePanel *p = context;
    if(!p || !report || !p->notifications) return;
    bool fatal_in_list = false;
    for(size_t i = 0; i < report->diagnostic_count; ++i) {
        diagnostic_add(p, &report->diagnostics[i]);
        fatal_in_list |= report->has_fatal && diagnostic_same_check(&report->first_fatal, &report->diagnostics[i]);
    }
    if(report->has_fatal && !fatal_in_list) diagnostic_add(p, &report->first_fatal);
    p->omitted = count_add(p->omitted, report->dropped_diagnostic_occurrences);
    if(p->omitted) {
        char detail[192];
        snprintf(detail, sizeof(detail), "Scene diagnostic grouping capacity exceeded. %llu%s occurrences omitted.",
            (unsigned long long)p->omitted, p->omitted == UINT64_MAX ? "+" : "");
        if(!p->overflow_notification) {
            p->overflow_notification = notification_push(p, "Scene diagnostic overflow", detail);
            (void)editor_terminal_panel_output_write(p->terminal, detail);
            (void)editor_terminal_panel_output_write(p->terminal, "\n");
        } else (void)editor_notification_panel_update(p->notifications, p->overflow_notification,
            "Scene diagnostic overflow", detail);
    }
}
/* SDL pipes may split a UTF-8 codepoint between reads. Keep only that partial
 * codepoint, never a line-sized unbounded buffer. Invalid bytes display as '?'. */
void editor_scene_panel_output(void *context, const char *bytes, size_t count) {
    EditorScenePanel *p = context;
    if(!p || !p->terminal) return;
    char output[4096]; size_t used = 0;
    for(size_t i = 0; i < count; ++i) {
        unsigned char c = (unsigned char)bytes[i];
        if(p->utf8_count && (c & 0xc0) != 0x80) {
            output[used++] = '?'; p->utf8_count = p->utf8_expected = 0;
        }
        if(p->utf8_count) {
            p->utf8[p->utf8_count++] = c;
            if(p->utf8_count == p->utf8_expected) {
                bool valid = !(p->utf8[0] == 0xe0 && p->utf8[1] < 0xa0) &&
                    !(p->utf8[0] == 0xed && p->utf8[1] >= 0xa0) &&
                    !(p->utf8[0] == 0xf0 && p->utf8[1] < 0x90) &&
                    !(p->utf8[0] == 0xf4 && p->utf8[1] >= 0x90);
                if(valid) { memcpy(output + used, p->utf8, p->utf8_count); used += p->utf8_count; }
                else output[used++] = '?';
                p->utf8_count = p->utf8_expected = 0;
            }
        } else if(c >= 0xc2 && c <= 0xf4) {
            p->utf8[0] = c; p->utf8_count = 1;
            p->utf8_expected = c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
        } else output[used++] = c && c < 0x80 ? (char)c : '?';
        if(used > sizeof(output) - 6) {
            output[used] = 0; (void)editor_terminal_panel_output_write(p->terminal, output); used = 0;
        }
    }
    if(used) { output[used] = 0; (void)editor_terminal_panel_output_write(p->terminal, output); }
}
static bool text_create(const FontAsset *font, const char *value, TextAsset *text) {
    TextAssetResult result = rohr_graphics_text_create(font, value, (Color){235, 240, 248, 255});
    if(rohr_error_check(result)) return false;
    *text = result.result.value; return true;
}
bool editor_scene_panel_create(EditorScenePanel *p, const FontAsset *font,
        EditorTerminalPanel *terminal, EditorNotificationPanel *notifications) {
    if(!p || !font || !terminal || !notifications) return false;
    *p = (EditorScenePanel){.terminal = terminal, .notifications = notifications, .cleared = true};
    return text_create(font, "Editor", &p->editor_label) &&
        text_create(font, "Scene Simulation", &p->scene_label) &&
        text_create(font, "Run", &p->run_label) && text_create(font, "Pause", &p->pause_label) &&
        text_create(font, "Resume", &p->resume_label) && text_create(font, "Stop", &p->stop_label) &&
        text_create(font, "Please wait", &p->waiting_label) &&
        text_create(font, "Esc releases input", &p->release_label);
}
void editor_scene_panel_focus_set(EditorScenePanel *p, bool focused) {
    if(!p || p->focused == focused) return;
    EngineResult result = editor_scene_session_focus_set(p->session, focused);
    if(focused && rohr_error_check(result)) return;
    p->focused = focused;
    if(focused) {
        rohr_ui_field_focus_clear();
        if(p->terminal) p->terminal->focused = false;
        SDL_Event release = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
        input_event_add(&release);
    } else {
        p->captured_buttons = 0;
        (void)input_mouse_relative_mode_set(false);
        (void)input_text_stop();
    }
}
void editor_scene_panel_attach(EditorScenePanel *p, EditorSceneSession *session) {
    if(!p) return;
    p->session = session;
    editor_scene_session_output_set(session, editor_scene_panel_output, p);
    editor_scene_session_report_output_set(session, editor_scene_panel_report, p);
}
void editor_scene_panel_stop(EditorScenePanel *p) {
    if(!p) return;
    editor_scene_panel_focus_set(p, false);
    p->focus_on_running = p->control_pending = false;
    p->cleared = true; p->sequence = 0;
    graphics_scene_image_destroy(p->image); p->image = NULL;
    editor_scene_session_stop(p->session);
}
void editor_scene_panel_run_begin(EditorScenePanel *p) {
    p->group_count = 0; p->omitted = p->overflow_notification = 0;
    p->utf8_count = p->utf8_expected = 0;
    p->cleared = p->image_failed = false; p->sequence = 0;
    p->focus_on_running = true; p->previous_state = EDITOR_SCENE_STARTING;
}
void editor_scene_panel_update(EditorScenePanel *p) {
    EditorSceneState state = editor_scene_session_state_get(p->session);
    if(state != p->previous_state) p->control_pending = false;
    if(state == EDITOR_SCENE_RUNNING && p->focus_on_running) {
        editor_scene_panel_focus_set(p, true); p->focus_on_running = false;
    }
    if(state == EDITOR_SCENE_FAILED || state == EDITOR_SCENE_STOPPED) {
        if(!p->cleared) editor_scene_panel_stop(p);
        if(p->utf8_count) {
            p->utf8_count = p->utf8_expected = 0;
            editor_scene_panel_output(p, "?", 1);
        }
    }
    p->previous_state = state;
}
static SDL_FRect image_rectangle_get(const EditorSceneFrame *frame, SDL_FRect viewport) {
    viewport.y += EDITOR_SCENE_CONTROLS_HEIGHT; viewport.h -= EDITOR_SCENE_CONTROLS_HEIGHT;
    if(frame->width <= 0 || frame->height <= 0 || viewport.w <= 0 || viewport.h <= 0) return (SDL_FRect){0};
    float scale = SDL_min(viewport.w / frame->width, viewport.h / frame->height);
    return (SDL_FRect){viewport.x + (viewport.w - frame->width * scale) / 2,
        viewport.y + (viewport.h - frame->height * scale) / 2, frame->width * scale, frame->height * scale};
}
bool editor_scene_panel_event_add(EditorScenePanel *p, const SDL_Event *event,
        SDL_FRect viewport, bool blocked) {
    if(!p || !event) return false;
    if(blocked || !p->selected || event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
            event->type == SDL_EVENT_WINDOW_MINIMIZED) {
        editor_scene_panel_focus_set(p, false); p->focus_on_running = false; return false;
    }
    if(editor_scene_session_state_get(p->session) != EDITOR_SCENE_RUNNING || p->control_pending) return false;
    if(p->focused && event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_ESCAPE) {
        editor_scene_panel_focus_set(p, false); return true;
    }
    EditorSceneFrame frame = editor_scene_session_frame_get(p->session);
    SDL_FRect image = image_rectangle_get(&frame, viewport);
    SDL_Event mapped = *event;
    Position point = {0}, delta = {0};
    bool pointer = false;
    switch(event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
            if(event->button.button < INPUT_MOUSE_BUTTON_LEFT || event->button.button >= INPUT_MOUSE_BUTTON_COUNT) return false;
            point = (Position){event->button.x, event->button.y}; pointer = true; break;
        case SDL_EVENT_MOUSE_MOTION:
            point = (Position){event->motion.x, event->motion.y}; pointer = true;
            delta = (Position){event->motion.xrel, event->motion.yrel}; break;
        case SDL_EVENT_MOUSE_WHEEL:
            point = (Position){event->wheel.mouse_x, event->wheel.mouse_y}; pointer = true; break;
        case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT: case SDL_EVENT_TEXT_EDITING: break;
        default: return false;
    }
    if(pointer) {
        Position screen = graphics_window_to_screen_get(point);
        SDL_FPoint logical = {0};
        bool inside = editor_scene_session_pointer_map(&frame, image, (SDL_FPoint){screen.x, screen.y}, &logical);
        if(event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if(!inside) { editor_scene_panel_focus_set(p, false); return false; }
            editor_scene_panel_focus_set(p, true);
            p->captured_buttons |= SDL_BUTTON_MASK(event->button.button);
        }
        if(!p->focused || (!inside && !p->captured_buttons && !input_mouse_relative_mode_check())) return false;
        if(event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
            mapped.button.x = logical.x; mapped.button.y = logical.y;
            if(event->type == SDL_EVENT_MOUSE_BUTTON_UP) p->captured_buttons &= ~SDL_BUTTON_MASK(event->button.button);
        } else if(event->type == SDL_EVENT_MOUSE_MOTION) {
            Position origin = graphics_window_to_screen_get((Position){0});
            delta = graphics_window_to_screen_get(delta);
            mapped.motion.x = logical.x; mapped.motion.y = logical.y;
            mapped.motion.xrel = image.w > 0 ? (delta.x - origin.x) * frame.width / image.w : 0;
            mapped.motion.yrel = image.h > 0 ? (delta.y - origin.y) * frame.height / image.h : 0;
        } else { mapped.wheel.mouse_x = logical.x; mapped.wheel.mouse_y = logical.y; }
    }
    if(!p->focused) return false;
    (void)editor_scene_session_input_add(p->session, &mapped);
    return true;
}
static int window_integer(double value) {
    if(!isfinite(value)) return 0;
    if(value >= INT_MAX) return INT_MAX;
    if(value <= INT_MIN) return INT_MIN;
    return (int)value;
}
void editor_scene_panel_input_mode_apply(EditorScenePanel *p, SDL_FRect viewport) {
    if(!p || !p->focused) return;
    EditorSceneInputMode mode = editor_scene_session_input_mode_get(p->session);
    (void)input_mouse_relative_mode_set(mode.relative_mouse);
    if(!mode.text_active) { (void)input_text_stop(); return; }
    EditorSceneFrame frame = editor_scene_session_frame_get(p->session);
    SDL_FRect image = image_rectangle_get(&frame, viewport);
    if(frame.width <= 0 || image.w <= 0) return;
    float scale = image.w / frame.width;
    SDL_FPoint a = graphics_scene_window_point_get((SDL_FPoint){image.x + mode.text_area.x * scale,
        image.y + mode.text_area.y * scale});
    SDL_FPoint b = graphics_scene_window_point_get((SDL_FPoint){image.x + ((float)mode.text_area.x + mode.text_area.w) * scale,
        image.y + ((float)mode.text_area.y + mode.text_area.h) * scale});
    SDL_FPoint cursor = graphics_scene_window_point_get((SDL_FPoint){image.x + mode.text_cursor * scale, image.y});
    SDL_FPoint origin = graphics_scene_window_point_get((SDL_FPoint){image.x, image.y});
    (void)input_text_start();
    (void)input_text_area_set((SDL_Rect){window_integer(a.x), window_integer(a.y),
        window_integer(fmaxf(0, b.x-a.x)), window_integer(fmaxf(0, b.y-a.y))},
        window_integer(cursor.x-origin.x));
}
EditorSceneAction editor_scene_panel_draw(EditorScenePanel *p, SDL_FRect viewport, bool building) {
    EditorSceneAction action = EDITOR_SCENE_ACTION_NONE;
    rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_VIEWPORT_CONTROL);
    rohr_ui_surface((UIRect){viewport.x, viewport.y - EDITOR_SCENE_TAB_HEIGHT, viewport.w, EDITOR_SCENE_TAB_HEIGHT},
        (Color){26, 31, 40, 255});
    UIButtonStyle selected = {.idle={62,75,94,255}, .hovered={72,85,104,255}, .pressed={42,55,74,255}};
    if(rohr_ui_button("scene.tab.editor", &p->editor_label,
            (UIRect){viewport.x, viewport.y-EDITOR_SCENE_TAB_HEIGHT, 100, EDITOR_SCENE_TAB_HEIGHT},
            p->selected ? NULL : &selected).clicked) {
        p->selected = false; action = EDITOR_SCENE_ACTION_EDITOR;
    }
    if(p->available && rohr_ui_button("scene.tab.simulation", &p->scene_label,
            (UIRect){viewport.x+100, viewport.y-EDITOR_SCENE_TAB_HEIGHT, 200, EDITOR_SCENE_TAB_HEIGHT},
            p->selected ? &selected : NULL).clicked) p->selected = true;
    if(!p->selected) return action;
    rohr_ui_surface((UIRect){viewport.x, viewport.y, viewport.w, viewport.h}, (Color){15,18,24,255});
    EditorSceneFrame frame = editor_scene_session_frame_get(p->session);
    if(!p->cleared && frame.pixels && !p->image_failed) {
        if(!p->image) p->image = graphics_scene_image_create();
        if(!p->image || (p->sequence != frame.sequence && rohr_error_check(
                graphics_scene_image_update(p->image, frame.pixels, frame.width, frame.height, frame.pitch)))) {
            p->image_failed = true;
            notification_push(p, "Scene image upload failed", "Could not upload the composed simulation frame. Stop and Run to retry.");
        } else {
            p->sequence = frame.sequence;
            SDL_FRect image = image_rectangle_get(&frame, viewport);
            if(image.w > 0 && image.h > 0) (void)graphics_scene_image_draw(p->image, image);
        }
    }
    EditorSceneState state = editor_scene_session_state_get(p->session);
    bool waiting = (building && state != EDITOR_SCENE_RUNNING && state != EDITOR_SCENE_PAUSED) ||
        p->control_pending || state == EDITOR_SCENE_STARTING || state == EDITOR_SCENE_STOPPING;
    TextAsset *label = waiting ? &p->waiting_label : state == EDITOR_SCENE_RUNNING ? &p->pause_label :
        state == EDITOR_SCENE_PAUSED ? &p->resume_label : &p->run_label;
    if(rohr_ui_button("scene.play", label, (UIRect){viewport.x+6, viewport.y+3, 125, 28}, NULL).clicked && !waiting) {
        if(state == EDITOR_SCENE_RUNNING || state == EDITOR_SCENE_PAUSED) {
            bool pause = state == EDITOR_SCENE_RUNNING;
            if(!rohr_error_check(editor_scene_session_pause_set(p->session, pause))) {
                p->control_pending = true; p->focus_on_running = !pause;
                editor_scene_panel_focus_set(p, false);
            }
        } else action = EDITOR_SCENE_ACTION_RUN;
    }
    if(rohr_ui_button("scene.stop", &p->stop_label,
            (UIRect){viewport.x+137, viewport.y+3, 80, 28}, NULL).clicked) action = EDITOR_SCENE_ACTION_STOP;
    if(p->focused && viewport.w > 450) rohr_ui_label(&p->release_label,
        (UIRect){viewport.x+230, viewport.y+3, viewport.w-236, 28});
    return action;
}
void editor_scene_panel_destroy(EditorScenePanel *p) {
    if(!p) return;
    editor_scene_panel_stop(p);
    rohr_graphics_text_destroy(&p->editor_label); rohr_graphics_text_destroy(&p->scene_label);
    rohr_graphics_text_destroy(&p->run_label); rohr_graphics_text_destroy(&p->pause_label);
    rohr_graphics_text_destroy(&p->resume_label); rohr_graphics_text_destroy(&p->stop_label);
    rohr_graphics_text_destroy(&p->waiting_label);
    rohr_graphics_text_destroy(&p->release_label);
    *p = (EditorScenePanel){0};
}
