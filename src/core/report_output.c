/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "report.h"
#include "physics.h"
#include "graphics.h"
#include "core/console_internal.h"
#include "yyjson/yyjson.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Names are explicit wire-format labels, not inferred from numeric ranges. */
static const char *report_id_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_NONE: return "ROHR_REPORT_NONE";
        case ROHR_REPORT: return "ROHR_REPORT";
        case ROHR_PHYSICS_REPORT: return "ROHR_PHYSICS_REPORT";
        case ROHR_PHYSICS_SUBSTEP_REPORT: return "ROHR_PHYSICS_SUBSTEP_REPORT";
        case ROHR_PHYSICS_LOADS_REPORT: return "ROHR_PHYSICS_LOADS_REPORT";
        case ROHR_PHYSICS_INTEGRATION_REPORT: return "ROHR_PHYSICS_INTEGRATION_REPORT";
        case ROHR_PHYSICS_CONTACTS_REPORT: return "ROHR_PHYSICS_CONTACTS_REPORT";
        case ROHR_PHYSICS_JOINTS_REPORT: return "ROHR_PHYSICS_JOINTS_REPORT";
        case ROHR_PHYSICS_CONSTRAINTS_REPORT: return "ROHR_PHYSICS_CONSTRAINTS_REPORT";
        case ROHR_GRAPHICS_REPORT: return "ROHR_GRAPHICS_REPORT";
        case ROHR_AUDIO_REPORT: return "ROHR_AUDIO_REPORT";
        case ROHR_INPUT_REPORT: return "ROHR_INPUT_REPORT";
        case ROHR_UI_REPORT: return "ROHR_UI_REPORT";
        case ROHR_GRAPHICS_CAMERAS_REPORT: return "ROHR_GRAPHICS_CAMERAS_REPORT";
        case ROHR_GRAPHICS_COMMANDS_REPORT: return "ROHR_GRAPHICS_COMMANDS_REPORT";
        case ROHR_GRAPHICS_VIEWPORTS_REPORT: return "ROHR_GRAPHICS_VIEWPORTS_REPORT";
        case ROHR_GRAPHICS_RECORDING_REPORT: return "ROHR_GRAPHICS_RECORDING_REPORT";
        case ROHR_GRAPHICS_PRESENTATION_REPORT: return "ROHR_GRAPHICS_PRESENTATION_REPORT";
        case ROHR_GRAPHICS_ANIMATIONS_REPORT: return "ROHR_GRAPHICS_ANIMATIONS_REPORT";
        case ROHR_AUDIO_COMMANDS_REPORT: return "ROHR_AUDIO_COMMANDS_REPORT";
        case ROHR_AUDIO_SOUNDS_REPORT: return "ROHR_AUDIO_SOUNDS_REPORT";
        case ROHR_AUDIO_MUSIC_REPORT: return "ROHR_AUDIO_MUSIC_REPORT";
        case ROHR_AUDIO_OUTPUT_REPORT: return "ROHR_AUDIO_OUTPUT_REPORT";
        case ROHR_ENGINE_LIFECYCLE_REPORT: return "ROHR_ENGINE_LIFECYCLE_REPORT";
        case ROHR_ENGINE_REPORT: return "ROHR_ENGINE_REPORT";
        default: return NULL;
    }
}

static const char *status_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_NOT_RUN: return "ROHR_REPORT_NOT_RUN";
        case ROHR_REPORT_COMPLETE: return "ROHR_REPORT_COMPLETE";
        case ROHR_REPORT_FAILED: return "ROHR_REPORT_FAILED";
        default: return NULL;
    }
}

static const char *severity_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_SEVERITY_NONE: return "ROHR_REPORT_SEVERITY_NONE";
        case ROHR_REPORT_SEVERITY_WARNING: return "ROHR_REPORT_SEVERITY_WARNING";
        case ROHR_REPORT_SEVERITY_FATAL: return "ROHR_REPORT_SEVERITY_FATAL";
        default: return NULL;
    }
}

static const char *aggregation_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_AGGREGATION_NONE: return "ROHR_REPORT_AGGREGATION_NONE";
        case ROHR_REPORT_AGGREGATION_SUM: return "ROHR_REPORT_AGGREGATION_SUM";
        case ROHR_REPORT_AGGREGATION_MAX: return "ROHR_REPORT_AGGREGATION_MAX";
        case ROHR_REPORT_AGGREGATION_LATEST: return "ROHR_REPORT_AGGREGATION_LATEST";
        default: return NULL;
    }
}

static const char *metric_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT: return "ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT";
        case ROHR_REPORT_UI_EVENT_RECEIVED_COUNT: return "ROHR_REPORT_UI_EVENT_RECEIVED_COUNT";
        case ROHR_REPORT_UI_EVENT_DROPPED_COUNT: return "ROHR_REPORT_UI_EVENT_DROPPED_COUNT";
        case ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK: return "ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK";
        case ROHR_REPORT_UI_TEXT_BYTES_PEAK: return "ROHR_REPORT_UI_TEXT_BYTES_PEAK";
        case ROHR_REPORT_UI_KEY_EVENT_SLOTS_PEAK: return "ROHR_REPORT_UI_KEY_EVENT_SLOTS_PEAK";
        case ROHR_REPORT_UI_TEXT_EVENT_SLOTS_PEAK: return "ROHR_REPORT_UI_TEXT_EVENT_SLOTS_PEAK";
        case ROHR_REPORT_UI_SCROLL_RECORDS_PEAK: return "ROHR_REPORT_UI_SCROLL_RECORDS_PEAK";
        case ROHR_REPORT_UI_SCROLL_SCOPE_DEPTH_PEAK: return "ROHR_REPORT_UI_SCROLL_SCOPE_DEPTH_PEAK";
        case ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK: return "ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK";
        case ROHR_REPORT_TICKS_ADVANCED_COUNT: return "ROHR_REPORT_TICKS_ADVANCED_COUNT";
        case ROHR_REPORT_EXPIRED_ENTITY_COUNT: return "ROHR_REPORT_EXPIRED_ENTITY_COUNT";
        case ROHR_REPORT_METRIC_NONE: return "ROHR_REPORT_METRIC_NONE";
        case ROHR_REPORT_PROCESSED_COUNT: return "ROHR_REPORT_PROCESSED_COUNT";
        case ROHR_REPORT_SKIPPED_COUNT: return "ROHR_REPORT_SKIPPED_COUNT";
        case ROHR_REPORT_CANDIDATE_COUNT: return "ROHR_REPORT_CANDIDATE_COUNT";
        case ROHR_REPORT_OVERLAP_COUNT: return "ROHR_REPORT_OVERLAP_COUNT";
        case ROHR_REPORT_CONTACT_COUNT: return "ROHR_REPORT_CONTACT_COUNT";
        case ROHR_REPORT_QUARANTINED_COUNT: return "ROHR_REPORT_QUARANTINED_COUNT";
        case ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT: return "ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT";
        case ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT: return "ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT";
        case ROHR_REPORT_COLLIDER_COUNT: return "ROHR_REPORT_COLLIDER_COUNT";
        case ROHR_REPORT_NARROWPHASE_TEST_COUNT: return "ROHR_REPORT_NARROWPHASE_TEST_COUNT";
        case ROHR_REPORT_BROADPHASE_BUILD_NS: return "ROHR_REPORT_BROADPHASE_BUILD_NS";
        case ROHR_REPORT_BROADPHASE_QUERY_NS: return "ROHR_REPORT_BROADPHASE_QUERY_NS";
        case ROHR_REPORT_NARROWPHASE_NS: return "ROHR_REPORT_NARROWPHASE_NS";
        case ROHR_REPORT_TREE_HEIGHT_PEAK: return "ROHR_REPORT_TREE_HEIGHT_PEAK";
        case ROHR_REPORT_QUEUED_COMMAND_COUNT: return "ROHR_REPORT_QUEUED_COMMAND_COUNT";
        case ROHR_REPORT_FRAME_LIMIT_WAIT_NS: return "ROHR_REPORT_FRAME_LIMIT_WAIT_NS";
        case ROHR_REPORT_AUDIO_FRAMES_COUNT: return "ROHR_REPORT_AUDIO_FRAMES_COUNT";
        case ROHR_REPORT_CANCELLED_COMMAND_COUNT: return "ROHR_REPORT_CANCELLED_COMMAND_COUNT";
        case ROHR_REPORT_TREE_NODES_CURRENT: return "ROHR_REPORT_TREE_NODES_CURRENT";
        default: return NULL;
    }
}

static const char *object_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_OBJECT_NONE: return "ROHR_REPORT_OBJECT_NONE";
        case ROHR_REPORT_OBJECT_ENTITY: return "ROHR_REPORT_OBJECT_ENTITY";
        case ROHR_REPORT_OBJECT_TEXTURE: return "ROHR_REPORT_OBJECT_TEXTURE";
        case ROHR_REPORT_OBJECT_ANIMATION: return "ROHR_REPORT_OBJECT_ANIMATION";
        case ROHR_REPORT_OBJECT_FONT: return "ROHR_REPORT_OBJECT_FONT";
        case ROHR_REPORT_OBJECT_TEXT: return "ROHR_REPORT_OBJECT_TEXT";
        case ROHR_REPORT_OBJECT_SOUND: return "ROHR_REPORT_OBJECT_SOUND";
        case ROHR_REPORT_OBJECT_MUSIC: return "ROHR_REPORT_OBJECT_MUSIC";
        case ROHR_REPORT_OBJECT_CAMERA: return "ROHR_REPORT_OBJECT_CAMERA";
        case ROHR_REPORT_OBJECT_VIEWPORT: return "ROHR_REPORT_OBJECT_VIEWPORT";
        case ROHR_REPORT_OBJECT_SCREEN: return "ROHR_REPORT_OBJECT_SCREEN";
        case ROHR_REPORT_OBJECT_UI: return "ROHR_REPORT_OBJECT_UI";
        case ROHR_REPORT_OBJECT_INPUT_CONTROLLER: return "ROHR_REPORT_OBJECT_INPUT_CONTROLLER";
        case ROHR_REPORT_OBJECT_INPUT_ACTION: return "ROHR_REPORT_OBJECT_INPUT_ACTION";
        case ROHR_REPORT_OBJECT_CUSTOM: return "ROHR_REPORT_OBJECT_CUSTOM";
        default: return NULL;
    }
}

static const char *recovery_name(uint32_t value) {
    switch(value) {
        case ROHR_REPORT_RECOVERY_NONE: return "ROHR_REPORT_RECOVERY_NONE";
        case ROHR_REPORT_CONTRIBUTION_SKIPPED: return "ROHR_REPORT_CONTRIBUTION_SKIPPED";
        case ROHR_REPORT_ENTITY_QUARANTINED: return "ROHR_REPORT_ENTITY_QUARANTINED";
        case ROHR_REPORT_UPDATE_ABORTED: return "ROHR_REPORT_UPDATE_ABORTED";
        default: return NULL;
    }
}

static const char *code_name(uint32_t value) {
    switch(value) {
        case ERROR_ENGINE_UI_CAPACITY_EXCEEDED: return "ERROR_ENGINE_UI_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_UI_SCOPE_UNBALANCED: return "ERROR_ENGINE_UI_SCOPE_UNBALANCED";
        case ERROR_NONE: return "ERROR_NONE";
        case ERROR_MEMORY_POOL_NULL_POINTER: return "ERROR_MEMORY_POOL_NULL_POINTER";
        case ERROR_MEMORY_POOL_CAPACITY_OVERFLOW: return "ERROR_MEMORY_POOL_CAPACITY_OVERFLOW";
        case ERROR_MEMORY_POOL_ALLOCATION_FAILED: return "ERROR_MEMORY_POOL_ALLOCATION_FAILED";
        case ERROR_MEMORY_POOL_FULL: return "ERROR_MEMORY_POOL_FULL";
        case ERROR_MEMORY_POOL_INVALID_OBJECT: return "ERROR_MEMORY_POOL_INVALID_OBJECT";
        case ERROR_MEMORY_POOL_OBJECT_NOT_USED: return "ERROR_MEMORY_POOL_OBJECT_NOT_USED";
        case ERROR_MEMORY_POOL_SHRINK_WOULD_REMOVE_USED_OBJECT: return "ERROR_MEMORY_POOL_SHRINK_WOULD_REMOVE_USED_OBJECT";
        case ERROR_ENGINE_ALREADY_RUNNING: return "ERROR_ENGINE_ALREADY_RUNNING";
        case ERROR_ENGINE_SDL_INIT_FAILED: return "ERROR_ENGINE_SDL_INIT_FAILED";
        case ERROR_ENGINE_ENTITY_TABLES_INIT_FAILED: return "ERROR_ENGINE_ENTITY_TABLES_INIT_FAILED";
        case ERROR_ENGINE_PHYSICS_TABLES_INIT_FAILED: return "ERROR_ENGINE_PHYSICS_TABLES_INIT_FAILED";
        case ERROR_ENGINE_GRAPHICS_TABLES_INIT_FAILED: return "ERROR_ENGINE_GRAPHICS_TABLES_INIT_FAILED";
        case ERROR_ENGINE_GRAPHICS_INIT_FAILED: return "ERROR_ENGINE_GRAPHICS_INIT_FAILED";
        case ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED: return "ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED";
        case ERROR_ENGINE_GRAPHICS_VSYNC_SET_FAILED: return "ERROR_ENGINE_GRAPHICS_VSYNC_SET_FAILED";
        case ERROR_ENGINE_GRAPHICS_WINDOW_PRESENTATION_FAILED: return "ERROR_ENGINE_GRAPHICS_WINDOW_PRESENTATION_FAILED";
        case ERROR_ENGINE_INVALID_FRAME_LIMIT: return "ERROR_ENGINE_INVALID_FRAME_LIMIT";
        case ERROR_ENGINE_MAX_ENTITIES_EXCEEDED: return "ERROR_ENGINE_MAX_ENTITIES_EXCEEDED";
        case ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED: return "ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED";
        case ERROR_ENGINE_TABLE_EXPANSION_FAILED: return "ERROR_ENGINE_TABLE_EXPANSION_FAILED";
        case ERROR_ENGINE_INVALID_ENTITY: return "ERROR_ENGINE_INVALID_ENTITY";
        case ERROR_ENGINE_ENTITY_NOT_FOUND: return "ERROR_ENGINE_ENTITY_NOT_FOUND";
        case ERROR_ENGINE_COMPONENT_MISSING: return "ERROR_ENGINE_COMPONENT_MISSING";
        case ERROR_ENGINE_INDEX_OUT_OF_RANGE: return "ERROR_ENGINE_INDEX_OUT_OF_RANGE";
        case ERROR_ENGINE_INVALID_SHAPE: return "ERROR_ENGINE_INVALID_SHAPE";
        case ERROR_ENGINE_POSITION_OUT_OF_RANGE: return "ERROR_ENGINE_POSITION_OUT_OF_RANGE";
        case ERROR_ENGINE_TEXTURE_LOAD_FAILED: return "ERROR_ENGINE_TEXTURE_LOAD_FAILED";
        case ERROR_ENGINE_TEXTURE_NOT_FOUND: return "ERROR_ENGINE_TEXTURE_NOT_FOUND";
        case ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED: return "ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_FONT_LOAD_FAILED: return "ERROR_ENGINE_FONT_LOAD_FAILED";
        case ERROR_ENGINE_FONT_NOT_FOUND: return "ERROR_ENGINE_FONT_NOT_FOUND";
        case ERROR_ENGINE_FONT_CAPACITY_EXCEEDED: return "ERROR_ENGINE_FONT_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_TEXT_CREATE_FAILED: return "ERROR_ENGINE_TEXT_CREATE_FAILED";
        case ERROR_ENGINE_TEXT_NOT_FOUND: return "ERROR_ENGINE_TEXT_NOT_FOUND";
        case ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED: return "ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_ANIMATION_LOAD_FAILED: return "ERROR_ENGINE_ANIMATION_LOAD_FAILED";
        case ERROR_ENGINE_ANIMATION_NOT_FOUND: return "ERROR_ENGINE_ANIMATION_NOT_FOUND";
        case ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED: return "ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_INVALID_ENTITY_NAME: return "ERROR_ENGINE_INVALID_ENTITY_NAME";
        case ERROR_ENGINE_ENTITY_NAME_TOO_LONG: return "ERROR_ENGINE_ENTITY_NAME_TOO_LONG";
        case ERROR_ENGINE_DUPLICATE_ENTITY_NAME: return "ERROR_ENGINE_DUPLICATE_ENTITY_NAME";
        case ERROR_ENGINE_STATE_IO_FAILED: return "ERROR_ENGINE_STATE_IO_FAILED";
        case ERROR_ENGINE_STATE_INVALID: return "ERROR_ENGINE_STATE_INVALID";
        case ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND: return "ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND";
        case ERROR_ENGINE_INVALID_GROUP_NAME: return "ERROR_ENGINE_INVALID_GROUP_NAME";
        case ERROR_ENGINE_GROUP_NAME_TOO_LONG: return "ERROR_ENGINE_GROUP_NAME_TOO_LONG";
        case ERROR_ENGINE_DUPLICATE_GROUP_NAME: return "ERROR_ENGINE_DUPLICATE_GROUP_NAME";
        case ERROR_ENGINE_GROUP_NOT_FOUND: return "ERROR_ENGINE_GROUP_NOT_FOUND";
        case ERROR_ENGINE_STATE_TEMPLATE_DOCUMENT_LIMIT_EXCEEDED: return "ERROR_ENGINE_STATE_TEMPLATE_DOCUMENT_LIMIT_EXCEEDED";
        case ERROR_ENGINE_STATE_DUPLICATE_ASSET_DEFINITION: return "ERROR_ENGINE_STATE_DUPLICATE_ASSET_DEFINITION";
        case ERROR_ENGINE_STATE_ASSET_REFERENCE_NOT_FOUND: return "ERROR_ENGINE_STATE_ASSET_REFERENCE_NOT_FOUND";
        case ERROR_ENGINE_UI_DEFINITION_NOT_FOUND: return "ERROR_ENGINE_UI_DEFINITION_NOT_FOUND";
        case ERROR_ENGINE_GRAPHICS_UI_IN_USE: return "ERROR_ENGINE_GRAPHICS_UI_IN_USE";
        case ERROR_ENGINE_INVALID_GRAPHICS_LAYER_NAME: return "ERROR_ENGINE_INVALID_GRAPHICS_LAYER_NAME";
        case ERROR_ENGINE_GRAPHICS_LAYER_NAME_TOO_LONG: return "ERROR_ENGINE_GRAPHICS_LAYER_NAME_TOO_LONG";
        case ERROR_ENGINE_DUPLICATE_GRAPHICS_LAYER_NAME: return "ERROR_ENGINE_DUPLICATE_GRAPHICS_LAYER_NAME";
        case ERROR_ENGINE_GRAPHICS_LAYER_NOT_FOUND: return "ERROR_ENGINE_GRAPHICS_LAYER_NOT_FOUND";
        case ERROR_ENGINE_INVALID_INPUT_NAME: return "ERROR_ENGINE_INVALID_INPUT_NAME";
        case ERROR_ENGINE_INPUT_NAME_TOO_LONG: return "ERROR_ENGINE_INPUT_NAME_TOO_LONG";
        case ERROR_ENGINE_DUPLICATE_INPUT_NAME: return "ERROR_ENGINE_DUPLICATE_INPUT_NAME";
        case ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED: return "ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_INPUT_NOT_FOUND: return "ERROR_ENGINE_INPUT_NOT_FOUND";
        case ERROR_ENGINE_INPUT_TYPE_MISMATCH: return "ERROR_ENGINE_INPUT_TYPE_MISMATCH";
        case ERROR_ENGINE_INPUT_BINDING_INVALID: return "ERROR_ENGINE_INPUT_BINDING_INVALID";
        case ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND: return "ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND";
        case ERROR_ENGINE_INPUT_OPERATION_FAILED: return "ERROR_ENGINE_INPUT_OPERATION_FAILED";
        case ERROR_ENGINE_INPUT_CONFIGURATION_INVALID: return "ERROR_ENGINE_INPUT_CONFIGURATION_INVALID";
        case ERROR_ENGINE_DIRECTORY_WORKING_SET_FAILED: return "ERROR_ENGINE_DIRECTORY_WORKING_SET_FAILED";
        case ERROR_ENGINE_AUDIO_ALREADY_STARTED: return "ERROR_ENGINE_AUDIO_ALREADY_STARTED";
        case ERROR_ENGINE_AUDIO_NOT_STARTED: return "ERROR_ENGINE_AUDIO_NOT_STARTED";
        case ERROR_ENGINE_AUDIO_INIT_FAILED: return "ERROR_ENGINE_AUDIO_INIT_FAILED";
        case ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID: return "ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID";
        case ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED: return "ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED";
        case ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND: return "ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND";
        case ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED: return "ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID: return "ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID";
        case ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED: return "ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED";
        case ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND: return "ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND";
        case ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED: return "ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE: return "ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE";
        case ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED: return "ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED";
        case ERROR_ENGINE_AUDIO_OPERATION_FAILED: return "ERROR_ENGINE_AUDIO_OPERATION_FAILED";
        case ERROR_ENGINE_REPORT_INVALID: return "ERROR_ENGINE_REPORT_INVALID";
        case ERROR_ENGINE_OUTPUT_IO_FAILED: return "ERROR_ENGINE_OUTPUT_IO_FAILED";
        case ERROR_ENGINE_OUTPUT_TRUNCATED: return "ERROR_ENGINE_OUTPUT_TRUNCATED";
        case ERROR_ENGINE_GRAPHICS_DRAW_FAILED: return "ERROR_ENGINE_GRAPHICS_DRAW_FAILED";
        case ERROR_ENGINE_GRAPHICS_STATE_FAILED: return "ERROR_ENGINE_GRAPHICS_STATE_FAILED";
        case ERROR_ENGINE_GRAPHICS_RECORDING_FAILED: return "ERROR_ENGINE_GRAPHICS_RECORDING_FAILED";
        case ERROR_ENGINE_OUTPUT_FORMAT_FAILED: return "ERROR_ENGINE_OUTPUT_FORMAT_FAILED";
        default: return NULL;
    }
}

static const char *physics_stage_name(uint32_t value) {
    switch(value) {
        case ROHR_PHYSICS_ARGUMENTS: return "ROHR_PHYSICS_ARGUMENTS";
        case ROHR_PHYSICS_PREPARATION: return "ROHR_PHYSICS_PREPARATION";
        case ROHR_PHYSICS_INTEGRATION: return "ROHR_PHYSICS_INTEGRATION";
        case ROHR_PHYSICS_BROADPHASE: return "ROHR_PHYSICS_BROADPHASE";
        case ROHR_PHYSICS_CONTACT_GATHER: return "ROHR_PHYSICS_CONTACT_GATHER";
        case ROHR_PHYSICS_JOINT_GATHER: return "ROHR_PHYSICS_JOINT_GATHER";
        case ROHR_PHYSICS_FINALIZATION: return "ROHR_PHYSICS_FINALIZATION";
        case ROHR_PHYSICS_CONSTRAINT_RESPONSE: return "ROHR_PHYSICS_CONSTRAINT_RESPONSE";
        case ROHR_PHYSICS_JOINT_TARGET: return "ROHR_PHYSICS_JOINT_TARGET";
        default: return NULL;
    }
}

static const char *graphics_stage_name(uint32_t value) {
    switch(value) {
        case ROHR_GRAPHICS_SUBMISSION: return "ROHR_GRAPHICS_SUBMISSION";
        case ROHR_GRAPHICS_CAMERA_CALLBACK: return "ROHR_GRAPHICS_CAMERA_CALLBACK";
        case ROHR_GRAPHICS_COMMAND_EXECUTION: return "ROHR_GRAPHICS_COMMAND_EXECUTION";
        case ROHR_GRAPHICS_VIEWPORT_COMPOSITION: return "ROHR_GRAPHICS_VIEWPORT_COMPOSITION";
        case ROHR_GRAPHICS_RECORDING_CAPTURE: return "ROHR_GRAPHICS_RECORDING_CAPTURE";
        case ROHR_GRAPHICS_RECORDING_WRITE: return "ROHR_GRAPHICS_RECORDING_WRITE";
        case ROHR_GRAPHICS_PRESENTATION: return "ROHR_GRAPHICS_PRESENTATION";
        case ROHR_GRAPHICS_ANIMATION: return "ROHR_GRAPHICS_ANIMATION";
        case ROHR_GRAPHICS_RESTORE: return "ROHR_GRAPHICS_RESTORE";
        default: return NULL;
    }
}

static const char *stage_name(ReportId owner, uint32_t stage) {
    if(owner >= ROHR_PHYSICS_REPORT && owner <= ROHR_PHYSICS_CONSTRAINTS_REPORT)
        return physics_stage_name(stage);
    if(owner == ROHR_GRAPHICS_REPORT || (owner >= ROHR_GRAPHICS_CAMERAS_REPORT &&
            owner <= ROHR_GRAPHICS_ANIMATIONS_REPORT)) return graphics_stage_name(stage);
    return NULL;
}

static EngineResult output_result(EngineError code) {
    return code == ERROR_NONE ? ERROR_RESULT_MAKE_VALUE(EngineResult, true)
        : ERROR_RESULT_MAKE_ERROR(EngineResult, code);
}

/* Enough for every escaped detail byte (\xNN), labels, and finite double times. */
#define REPORT_TEXT_LINE_CAPACITY (4u * ROHR_REPORT_DETAIL_CAPACITY + 512u)

typedef struct ReportTextOutput {
    FILE *file; /* NULL selects the engine console. Borrowed for the whole write. */
    EngineError error;
} ReportTextOutput;

static void text_line(ReportTextOutput *output, const char *format, ...) {
    if(output->error != ERROR_NONE && output->error != ERROR_ENGINE_OUTPUT_TRUNCATED) return;
    char line[REPORT_TEXT_LINE_CAPACITY];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if(length < 0 || (size_t)length >= sizeof(line)) {
        output->error = ERROR_ENGINE_OUTPUT_FORMAT_FAILED;
        return;
    }
    if(output->file != NULL) {
        if(fputs(line, output->file) == EOF) output->error = ERROR_ENGINE_OUTPUT_IO_FAILED;
    } else {
        char *cursor = line;
        while(*cursor != '\0') {
            char *next = strchr(cursor, '\n');
            if(next == NULL) next = cursor + strlen(cursor);
            else next++;
            char saved = *next;
            *next = '\0';
            EngineResult result = console_checked_write(LOG_ENGINE, "%s", cursor);
            *next = saved;
            if(error_check(result)) {
                output->error = result.result.error;
                if(output->error != ERROR_ENGINE_OUTPUT_TRUNCATED) return;
            }
            cursor = next;
        }
    }
}

static const char *label(const char *name, uint32_t value, const char *prefix,
        bool report_suffix, char fallback[64]) {
    if(name == NULL) {
        snprintf(fallback, 64, "custom/unknown (%" PRIu32 ")", value);
        return fallback;
    }
    size_t prefix_length = strlen(prefix);
    if(strncmp(name, prefix, prefix_length) == 0) name += prefix_length;
    size_t length = strlen(name);
    if(report_suffix && length >= 7 && strcmp(name + length - 7, "_REPORT") == 0) {
        snprintf(fallback, 64, "%.*s", (int)(length - 7), name);
        return fallback;
    }
    return name;
}

static const char *report_label(ReportId id, char buffer[64]) {
    return label(report_id_name(id), id, "ROHR_", true, buffer);
}

static void detail_escape(const char *source, char escaped[4 * ROHR_REPORT_DETAIL_CAPACITY]) {
    static const char hex[] = "0123456789abcdef";
    char *out = escaped;
    for(const unsigned char *p = (const unsigned char *)source; *p; p++) {
        if(*p == '\n' || *p == '\r' || *p == '\t' || *p == '\\') {
            *out++ = '\\';
            *out++ = *p == '\n' ? 'n' : *p == '\r' ? 'r' : *p == '\t' ? 't' : '\\';
        } else if(*p < 32 || *p == 127) {
            *out++ = '\\'; *out++ = 'x';
            *out++ = hex[*p >> 4]; *out++ = hex[*p & 15];
        } else *out++ = (char)*p;
    }
    *out = '\0';
}

static bool diagnostic_key_same(const ReportDiagnostic *a, const ReportDiagnostic *b) {
    return a->report_id == b->report_id && a->internal_stage == b->internal_stage &&
        a->severity == b->severity && a->code == b->code && a->recovery == b->recovery &&
        a->source.kind == b->source.kind && a->source.id == b->source.id &&
        a->target.kind == b->target.kind && a->target.id == b->target.id &&
        a->group_version == b->group_version;
}

static void diagnostic_text(ReportTextOutput *out, const ReportDiagnostic *d) {
    char owner[64], stage[64], kind[64];
    char detail[4 * ROHR_REPORT_DETAIL_CAPACITY];
    detail_escape(d->detail, detail);
    text_line(out, "  %s: %s (%u)\n", severity_name(d->severity) + strlen("ROHR_REPORT_SEVERITY_"),
        code_name(d->code), (unsigned)d->code);
    text_line(out, "  Message: %s\n", error_code_message_get(d->code));
    text_line(out, "  Pipeline: %s (%u)\n", report_label(d->report_id, owner), (unsigned)d->report_id);
    if(d->group_version) text_line(out, "  Snapshot version: %" PRIu64 "\n", d->group_version);
    if(d->internal_stage != 0)
        text_line(out, "  Stage: %s (%" PRIu32 ")\n",
            label(stage_name(d->report_id, d->internal_stage), d->internal_stage,
                "ROHR_PHYSICS_", false, stage), d->internal_stage);
    if(d->source.kind != ROHR_REPORT_OBJECT_NONE)
        text_line(out, "  Source: %s %" PRIu64 "\n",
            label(object_name(d->source.kind), d->source.kind, "ROHR_REPORT_OBJECT_", false, kind), d->source.id);
    if(d->target.kind != ROHR_REPORT_OBJECT_NONE)
        text_line(out, "  Target: %s %" PRIu64 "\n",
            label(object_name(d->target.kind), d->target.kind, "ROHR_REPORT_OBJECT_", false, kind), d->target.id);
    if(d->has_substep) text_line(out, "  Substeps: %" PRIu32 "-%" PRIu32 "\n", d->first_substep, d->last_substep);
    text_line(out, "  Occurrences: %" PRIu64 "\n", d->occurrence_count);
    text_line(out, "  Recovery: %s\n", recovery_name(d->recovery) + strlen("ROHR_REPORT_"));
    text_line(out, "  Detail: %s%s\n", detail, d->detail_truncated ? " [detail truncated]" : "");
}

static void report_text(ReportTextOutput *out, const Report *report) {
    char owner[64], metric[64];
    text_line(out, "%s (%u) - %s\n", report_label(report->header.id, owner), (unsigned)report->header.id,
        status_name(report->header.status) + strlen("ROHR_REPORT_"));
    text_line(out, "Version: %" PRIu64 "  Root invocations: %" PRIu64 "\n", report->version, report->invocation_count);
    text_line(out, "Total time: %.3f ms\n\nPipelines:\n", report->header.total_ms);
    if(report->entry_count == 0) text_line(out, "  None.\n");
    for(size_t i = 0; i < report->entry_count; i++) {
        const ReportEntry *e = &report->entries[i];
        text_line(out, "  %s (%u)  %s  %.3f ms  %" PRIu64 " invocation(s)\n",
            report_label(e->header.id, owner), (unsigned)e->header.id,
            status_name(e->header.status) + strlen("ROHR_REPORT_"), e->header.total_ms, e->invocation_count);
        if(e->group_version) text_line(out, "    Snapshot version: %" PRIu64 "%s\n",
            e->group_version, e->group_root ? " (selected root)" : "");
    }
    text_line(out, "\nStatistics:\n");
    if(report->statistic_count == 0) text_line(out, "  None.\n");
    for(size_t i = 0; i < report->statistic_count; i++) {
        const ReportStatistic *s = &report->statistics[i];
        text_line(out, "  %s (%u)  %s (%u): %" PRIu64 " [%s]\n", report_label(s->report_id, owner),
            (unsigned)s->report_id, label(metric_name(s->metric_id), s->metric_id,
                "ROHR_REPORT_", false, metric), (unsigned)s->metric_id, s->value,
            aggregation_name(s->aggregation) + strlen("ROHR_REPORT_AGGREGATION_"));
        if(s->group_version) text_line(out, "    Snapshot version: %" PRIu64 "\n", s->group_version);
    }
    text_line(out, "\nFirst fatal:\n");
    if(report->has_fatal) diagnostic_text(out, &report->first_fatal);
    else text_line(out, "  None.\n");
    text_line(out, "\nDiagnostics:\n");
    size_t additional = 0;
    for(size_t i = 0; i < report->diagnostic_count; i++) {
        const ReportDiagnostic *d = &report->diagnostics[i];
        if(report->has_fatal && diagnostic_key_same(d, &report->first_fatal)) continue;
        if(additional++) text_line(out, "\n");
        diagnostic_text(out, d);
    }
    if(additional == 0) text_line(out, "  No additional diagnostics.\n");
    text_line(out, "\nOverflow:\n");
    text_line(out, "  Dropped entries: %" PRIu64 "\n", report->dropped_entry_count);
    text_line(out, "  Dropped statistics: %" PRIu64 "\n", report->dropped_statistic_count);
    text_line(out, "  Dropped diagnostic occurrences: %" PRIu64 "\n", report->dropped_diagnostic_occurrences);
    text_line(out, "  Counters saturated: %s\n", report->counters_saturated ? "yes" : "no");
}

EngineResult report_console_write(const Report *report) {
    if(!report_valid_check(report)) return output_result(ERROR_ENGINE_REPORT_INVALID);
    ReportTextOutput output = {0};
    report_text(&output, report);
    return output_result(output.error);
}

EngineResult report_text_write(const Report *report, const char *path) {
    if(!report_valid_check(report) || path == NULL || path[0] == '\0')
        return output_result(ERROR_ENGINE_REPORT_INVALID);
    FILE *file = fopen(path, "wb");
    if(file == NULL) return output_result(ERROR_ENGINE_OUTPUT_IO_FAILED);
    ReportTextOutput output = {.file = file};
    report_text(&output, report);
    if(fflush(file) == EOF) output.error = ERROR_ENGINE_OUTPUT_IO_FAILED;
    if(fclose(file) == EOF) output.error = ERROR_ENGINE_OUTPUT_IO_FAILED;
    return output_result(output.error);
}

typedef struct ReportJson {
    yyjson_mut_doc *doc;
    bool failed;
} ReportJson;

static void json_put(ReportJson *j, yyjson_mut_val *object, const char *key, yyjson_mut_val *value) {
    if(!yyjson_mut_obj_add_val(j->doc, object, key, value)) j->failed = true;
}

static yyjson_mut_val *json_enum(ReportJson *j, uint32_t value, const char *name) {
    yyjson_mut_val *object = yyjson_mut_obj(j->doc);
    json_put(j, object, "value", yyjson_mut_uint(j->doc, value));
    json_put(j, object, "name", name ? yyjson_mut_str(j->doc, name) : yyjson_mut_null(j->doc));
    return object;
}

static yyjson_mut_val *json_u64(ReportJson *j, uint64_t value) {
    char digits[21]; /* UINT64_MAX plus NUL. */
    snprintf(digits, sizeof(digits), "%" PRIu64, value);
    return yyjson_mut_strcpy(j->doc, digits);
}

static yyjson_mut_val *json_header(ReportJson *j, ReportHeader header) {
    yyjson_mut_val *object = yyjson_mut_obj(j->doc);
    json_put(j, object, "id", json_enum(j, header.id, report_id_name(header.id)));
    json_put(j, object, "status", json_enum(j, header.status, status_name(header.status)));
    json_put(j, object, "total_ms", yyjson_mut_real(j->doc, header.total_ms));
    return object;
}

static yyjson_mut_val *json_object(ReportJson *j, ReportObjectRef ref) {
    yyjson_mut_val *object = yyjson_mut_obj(j->doc);
    json_put(j, object, "kind", json_enum(j, ref.kind, object_name(ref.kind)));
    json_put(j, object, "id", json_u64(j, ref.id));
    return object;
}

static yyjson_mut_val *json_diagnostic(ReportJson *j, const ReportDiagnostic *d) {
    yyjson_mut_val *object = yyjson_mut_obj(j->doc);
    json_put(j, object, "report_id", json_enum(j, d->report_id, report_id_name(d->report_id)));
    json_put(j, object, "group_version", json_u64(j, d->group_version));
    json_put(j, object, "internal_stage", json_enum(j, d->internal_stage, stage_name(d->report_id, d->internal_stage)));
    json_put(j, object, "severity", json_enum(j, d->severity, severity_name(d->severity)));
    json_put(j, object, "code", json_enum(j, d->code, code_name(d->code)));
    json_put(j, object, "message", yyjson_mut_str(j->doc, error_code_message_get(d->code)));
    json_put(j, object, "recovery", json_enum(j, d->recovery, recovery_name(d->recovery)));
    json_put(j, object, "source", json_object(j, d->source));
    json_put(j, object, "target", json_object(j, d->target));
    json_put(j, object, "occurrence_count", json_u64(j, d->occurrence_count));
    json_put(j, object, "has_substep", yyjson_mut_bool(j->doc, d->has_substep));
    json_put(j, object, "first_substep", yyjson_mut_uint(j->doc, d->first_substep));
    json_put(j, object, "last_substep", yyjson_mut_uint(j->doc, d->last_substep));
    json_put(j, object, "detail", yyjson_mut_str(j->doc, d->detail));
    json_put(j, object, "detail_truncated", yyjson_mut_bool(j->doc, d->detail_truncated));
    return object;
}

EngineResult report_json_write(const Report *report, const char *path, bool pretty) {
    if(!report_valid_check(report) || path == NULL || path[0] == '\0')
        return output_result(ERROR_ENGINE_REPORT_INVALID);
    ReportJson j = {.doc = yyjson_mut_doc_new(NULL)};
    if(j.doc == NULL) return output_result(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    yyjson_mut_val *root = yyjson_mut_obj(j.doc);
    yyjson_mut_doc_set_root(j.doc, root);
    json_put(&j, root, "format_version", yyjson_mut_uint(j.doc, 2));
    json_put(&j, root, "header", json_header(&j, report->header));
    json_put(&j, root, "version", json_u64(&j, report->version));
    json_put(&j, root, "invocation_count", json_u64(&j, report->invocation_count));
    yyjson_mut_val *coverage = yyjson_mut_arr(j.doc);
    json_put(&j, root, "included_versions", coverage);
    for(size_t i = 1; i < ROHR_REPORT; ++i) {
        if(!report->included_versions[i]) continue;
        yyjson_mut_val *object = yyjson_mut_obj(j.doc);
        json_put(&j, object, "report_id", json_enum(&j, (uint32_t)i, report_id_name((uint32_t)i)));
        json_put(&j, object, "version", json_u64(&j, report->included_versions[i]));
        if(!yyjson_mut_arr_append(coverage, object)) j.failed = true;
    }
    yyjson_mut_val *entries = yyjson_mut_arr(j.doc);
    json_put(&j, root, "entries", entries);
    for(size_t i = 0; i < report->entry_count; i++) {
        yyjson_mut_val *object = yyjson_mut_obj(j.doc);
        json_put(&j, object, "header", json_header(&j, report->entries[i].header));
        json_put(&j, object, "invocation_count", json_u64(&j, report->entries[i].invocation_count));
        json_put(&j, object, "group_version", json_u64(&j, report->entries[i].group_version));
        json_put(&j, object, "group_root", yyjson_mut_bool(j.doc, report->entries[i].group_root));
        if(!yyjson_mut_arr_append(entries, object)) j.failed = true;
    }
    yyjson_mut_val *statistics = yyjson_mut_arr(j.doc);
    json_put(&j, root, "statistics", statistics);
    for(size_t i = 0; i < report->statistic_count; i++) {
        const ReportStatistic *s = &report->statistics[i];
        yyjson_mut_val *object = yyjson_mut_obj(j.doc);
        json_put(&j, object, "report_id", json_enum(&j, s->report_id, report_id_name(s->report_id)));
        json_put(&j, object, "metric_id", json_enum(&j, s->metric_id, metric_name(s->metric_id)));
        json_put(&j, object, "group_version", json_u64(&j, s->group_version));
        json_put(&j, object, "aggregation", json_enum(&j, s->aggregation, aggregation_name(s->aggregation)));
        json_put(&j, object, "value", json_u64(&j, s->value));
        if(!yyjson_mut_arr_append(statistics, object)) j.failed = true;
    }
    yyjson_mut_val *diagnostics = yyjson_mut_arr(j.doc);
    json_put(&j, root, "diagnostics", diagnostics);
    for(size_t i = 0; i < report->diagnostic_count; i++)
        if(!yyjson_mut_arr_append(diagnostics, json_diagnostic(&j, &report->diagnostics[i]))) j.failed = true;
    json_put(&j, root, "first_fatal", report->has_fatal
        ? json_diagnostic(&j, &report->first_fatal) : yyjson_mut_null(j.doc));
    json_put(&j, root, "dropped_entry_count", json_u64(&j, report->dropped_entry_count));
    json_put(&j, root, "dropped_statistic_count", json_u64(&j, report->dropped_statistic_count));
    json_put(&j, root, "dropped_diagnostic_occurrences", json_u64(&j, report->dropped_diagnostic_occurrences));
    json_put(&j, root, "counters_saturated", yyjson_mut_bool(j.doc, report->counters_saturated));
    if(j.failed) {
        yyjson_mut_doc_free(j.doc);
        return output_result(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    size_t length;
    yyjson_write_err error;
    char *data = yyjson_mut_write_opts(j.doc, pretty ? YYJSON_WRITE_PRETTY : 0, NULL, &length, &error);
    yyjson_mut_doc_free(j.doc);
    if(data == NULL) return output_result(error.code == YYJSON_WRITE_ERROR_MEMORY_ALLOCATION
        ? ERROR_MEMORY_POOL_ALLOCATION_FAILED : ERROR_ENGINE_OUTPUT_FORMAT_FAILED);
    FILE *file = fopen(path, "wb");
    EngineError result = ERROR_NONE;
    if(file == NULL) result = ERROR_ENGINE_OUTPUT_IO_FAILED;
    else {
        if(fwrite(data, 1, length, file) != length) result = ERROR_ENGINE_OUTPUT_IO_FAILED;
        if(fflush(file) == EOF) result = ERROR_ENGINE_OUTPUT_IO_FAILED;
        if(fclose(file) == EOF) result = ERROR_ENGINE_OUTPUT_IO_FAILED;
    }
    free(data);
    return output_result(result);
}
