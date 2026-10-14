/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    Report parent = rohr_report_create();
    Report child = rohr_report_create();
    ReportDiagnostic diagnostic = {
        .report_id = ROHR_REPORT_CUSTOM_ID_BEGIN,
        .severity = ROHR_REPORT_SEVERITY_FATAL,
        .code = ERROR_ENGINE_COMPONENT_MISSING,
        .occurrence_count = 1
    };
    if(rohr_error_code_message_get(ERROR_NONE) == NULL ||
            rohr_error_check(rohr_report_header_set(&parent,
                (ReportHeader){ROHR_PHYSICS_REPORT, ROHR_REPORT_COMPLETE, 5})) ||
            rohr_error_check(rohr_report_header_set(&child,
                (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 1})) ||
            rohr_error_check(rohr_report_diagnostic_detail_set(&diagnostic, "SDK-owned detail")) ||
            rohr_error_check(rohr_report_diagnostic_add(&child, diagnostic)) ||
            rohr_error_check(rohr_report_merge(&parent, &child))) return 1;
    memset(&child, 0, sizeof(child));
    if(!rohr_report_valid_check(&parent) || !parent.has_fatal ||
        parent.header.status != ROHR_REPORT_FAILED || parent.header.total_ms != 5 ||
        parent.first_fatal.occurrence_count != 1 || parent.entry_count != 1 ||
        strcmp(parent.first_fatal.detail, "SDK-owned detail") != 0) return 1;
    if(rohr_error_check(rohr_report_json_write(&parent, "sdk_report.json", true)) ||
            rohr_error_check(rohr_report_text_write(&parent, "sdk_report.txt")) ||
            rohr_error_check(rohr_report_console_write(&parent))) return 1;
    if(remove("sdk_report.json") != 0 || remove("sdk_report.txt") != 0) return 1;
    if(rohr_error_check(rohr_engine_start()) || rohr_error_check(rohr_graphics_start())) return 1;
    Report (*pipelines[])(void) = {rohr_graphics_pipeline_cameras_update,
        rohr_graphics_pipeline_commands_execute, rohr_graphics_pipeline_viewports_draw,
        rohr_graphics_pipeline_recording_update, rohr_graphics_pipeline_present, rohr_graphics_show};
    for(size_t i=0; i<sizeof(pipelines)/sizeof(pipelines[0]); i++) {
        Report graphics = pipelines[i]();
        if(!rohr_report_valid_check(&graphics) || graphics.header.status == ROHR_REPORT_FAILED) return 1;
    }
    Report animations=rohr_graphics_sprite_frames_update(0,0);
    if(!rohr_report_valid_check(&animations) || animations.header.id != ROHR_GRAPHICS_ANIMATIONS_REPORT) return 1;
    if(rohr_error_check(rohr_audio_start())) return 1;
    if(rohr_error_check(rohr_audio_volume_set(0.5f))) return 1;
    Report audio = rohr_audio_update();
    if(!rohr_report_valid_check(&audio) || audio.header.id != ROHR_AUDIO_REPORT) return 1;
    Report stopped = rohr_audio_stop();
    Report final_audio = rohr_audio_update();
    if(!rohr_report_valid_check(&stopped) || memcmp(&stopped, &final_audio, sizeof(stopped)) != 0 ||
            rohr_audio_update().header.status != ROHR_REPORT_NOT_RUN) return 1;
    rohr_ui_input_set((UIInput){0});
    (void)rohr_ui_interaction("sdk-ui", (UIRect){0, 0, 30, 30});
    Report ui = rohr_ui_update();
    if(!rohr_report_valid_check(&ui) || ui.header.id != ROHR_UI_REPORT ||
            ui.header.status != ROHR_REPORT_COMPLETE || ui.entry_count != 0) return 1;
    ui_input_set((UIInput){0});
    Report direct_ui = ui_update();
    if(!report_valid_check(&direct_ui) || direct_ui.header.id != ROHR_UI_REPORT ||
            direct_ui.header.status != ROHR_REPORT_COMPLETE) return 1;
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
