/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

#define ROHR_REPORT_ENTRY_CAPACITY 32u
#define ROHR_REPORT_STATISTIC_CAPACITY 64u
#define ROHR_REPORT_DIAGNOSTIC_CAPACITY 256u
/** Includes the terminating NUL. */
#define ROHR_REPORT_DETAIL_CAPACITY 256u

/** Engine identifiers and the inclusive application-owned range. */
typedef enum ReportId {
    ROHR_REPORT_NONE = 0,
    ROHR_PHYSICS_REPORT,
    ROHR_PHYSICS_SUBSTEP_REPORT,
    ROHR_PHYSICS_LOADS_REPORT,
    ROHR_PHYSICS_INTEGRATION_REPORT,
    ROHR_PHYSICS_CONTACTS_REPORT,
    ROHR_PHYSICS_JOINTS_REPORT,
    ROHR_PHYSICS_CONSTRAINTS_REPORT,
    ROHR_GRAPHICS_REPORT,
    ROHR_AUDIO_REPORT,
    ROHR_INPUT_REPORT,
    ROHR_UI_REPORT,
    ROHR_ENGINE_TICK_REPORT,
    ROHR_GRAPHICS_CAMERAS_REPORT,
    ROHR_GRAPHICS_COMMANDS_REPORT,
    ROHR_GRAPHICS_VIEWPORTS_REPORT,
    ROHR_GRAPHICS_RECORDING_REPORT,
    ROHR_GRAPHICS_PRESENTATION_REPORT,
    ROHR_GRAPHICS_ANIMATIONS_REPORT,
    ROHR_AUDIO_COMMANDS_REPORT,
    ROHR_AUDIO_SOUNDS_REPORT,
    ROHR_AUDIO_MUSIC_REPORT,
    ROHR_AUDIO_OUTPUT_REPORT,
    ROHR_REPORT_CUSTOM_ID_BEGIN = 0x10000,
    ROHR_REPORT_CUSTOM_ID_END = 0x7fffffff
} ReportId;

typedef enum ReportStatus {
    ROHR_REPORT_NOT_RUN = 0,
    ROHR_REPORT_COMPLETE,
    ROHR_REPORT_FAILED
} ReportStatus;

typedef enum ReportSeverity {
    ROHR_REPORT_SEVERITY_NONE = 0,
    ROHR_REPORT_SEVERITY_WARNING,
    ROHR_REPORT_SEVERITY_FATAL
} ReportSeverity;

typedef enum ReportAggregation {
    ROHR_REPORT_AGGREGATION_NONE = 0,
    ROHR_REPORT_AGGREGATION_SUM,
    ROHR_REPORT_AGGREGATION_MAX,
    ROHR_REPORT_AGGREGATION_LATEST
} ReportAggregation;

/** COUNT metrics sum; PEAK metrics take max; CURRENT metrics take latest. */
typedef enum ReportMetricId {
    ROHR_REPORT_METRIC_NONE = 0,
    ROHR_REPORT_PROCESSED_COUNT,
    ROHR_REPORT_SKIPPED_COUNT,
    ROHR_REPORT_CANDIDATE_COUNT,
    ROHR_REPORT_OVERLAP_COUNT,
    ROHR_REPORT_CONTACT_COUNT,
    ROHR_REPORT_QUARANTINED_COUNT,
    ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT,
    ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT,
    ROHR_REPORT_COLLIDER_COUNT,
    ROHR_REPORT_NARROWPHASE_TEST_COUNT,
    ROHR_REPORT_BROADPHASE_BUILD_NS,
    ROHR_REPORT_BROADPHASE_QUERY_NS,
    ROHR_REPORT_NARROWPHASE_NS,
    ROHR_REPORT_TREE_HEIGHT_PEAK,
    ROHR_REPORT_TREE_NODES_CURRENT,
    ROHR_REPORT_QUEUED_COMMAND_COUNT,
    ROHR_REPORT_FRAME_LIMIT_WAIT_NS,
    ROHR_REPORT_AUDIO_FRAMES_COUNT,
    ROHR_REPORT_CANCELLED_COMMAND_COUNT,
    ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN = 0x10000,
    ROHR_REPORT_CUSTOM_METRIC_ID_END = 0x7fffffff
} ReportMetricId;

typedef enum ReportObjectKind {
    ROHR_REPORT_OBJECT_NONE = 0,
    ROHR_REPORT_OBJECT_ENTITY,
    ROHR_REPORT_OBJECT_TEXTURE,
    ROHR_REPORT_OBJECT_ANIMATION,
    ROHR_REPORT_OBJECT_FONT,
    ROHR_REPORT_OBJECT_TEXT,
    ROHR_REPORT_OBJECT_SOUND,
    ROHR_REPORT_OBJECT_MUSIC,
    ROHR_REPORT_OBJECT_CAMERA,
    ROHR_REPORT_OBJECT_VIEWPORT,
    ROHR_REPORT_OBJECT_SCREEN,
    ROHR_REPORT_OBJECT_UI,
    ROHR_REPORT_OBJECT_INPUT_CONTROLLER,
    ROHR_REPORT_OBJECT_INPUT_ACTION,
    ROHR_REPORT_OBJECT_CUSTOM
} ReportObjectKind;

/** Evidence only: does not own, resolve, or retain the referenced object. */
typedef struct ReportObjectRef {
    ReportObjectKind kind;
    uint64_t id;
} ReportObjectRef;

typedef enum ReportRecovery {
    ROHR_REPORT_RECOVERY_NONE = 0,
    ROHR_REPORT_CONTRIBUTION_SKIPPED,
    ROHR_REPORT_ENTITY_QUARANTINED,
    ROHR_REPORT_UPDATE_ABORTED
} ReportRecovery;

typedef struct ReportHeader {
    ReportId id;
    ReportStatus status;
    double total_ms;
} ReportHeader;

typedef struct ReportEntry {
    ReportHeader header;
    uint64_t invocation_count;
} ReportEntry;

typedef struct ReportStatistic {
    ReportId report_id;
    ReportMetricId metric_id;
    ReportAggregation aggregation;
    uint64_t value;
} ReportStatistic;

typedef struct ReportDiagnostic {
    ReportId report_id;
    /** Optional pipeline-local stage identifier; zero means unspecified. */
    uint32_t internal_stage;
    ReportSeverity severity;
    EngineError code;
    ReportRecovery recovery;
    ReportObjectRef source;
    ReportObjectRef target;
    uint64_t occurrence_count;
    bool has_substep;
    uint32_t first_substep;
    uint32_t last_substep;
    char detail[ROHR_REPORT_DETAIL_CAPACITY];
    bool detail_truncated;
} ReportDiagnostic;

/** Self-contained value. No allocation, retained resources, or engine lifetime. */
typedef struct Report {
    ReportHeader header;
    size_t entry_count;
    ReportEntry entries[ROHR_REPORT_ENTRY_CAPACITY];
    size_t statistic_count;
    ReportStatistic statistics[ROHR_REPORT_STATISTIC_CAPACITY];
    size_t diagnostic_count;
    ReportDiagnostic diagnostics[ROHR_REPORT_DIAGNOSTIC_CAPACITY];
    uint64_t dropped_entry_count;
    uint64_t dropped_statistic_count;
    uint64_t dropped_diagnostic_occurrences;
    bool counters_saturated;
    bool has_fatal;
    ReportDiagnostic first_fatal;
} Report;

/** Empty NOT_RUN builder; set its header before publishing a completed report. */
Report report_create(void);
/** Validate populated slots and reserved fatal consistency; ignores unused slots. */
bool report_valid_check(const Report *report);
/** Set identity/status/measured parent timing. A failed report cannot be cleared. */
EngineResult report_header_set(Report *report, ReportHeader header);
/** Accumulate matching child IDs; never change measured parent time. */
EngineResult report_entry_add(Report *report, ReportEntry entry);
/** Collect by report/metric ID. Rules must agree, including custom metrics. */
EngineResult report_statistic_add(Report *report, ReportStatistic statistic);
/** Copy optional text (NULL clears it), with visible truncation in the value. */
EngineResult report_diagnostic_detail_set(ReportDiagnostic *diagnostic, const char *detail);
/** Append/aggregate an issue. NONE and zero occurrences are invalid. */
EngineResult report_diagnostic_add(Report *report, ReportDiagnostic diagnostic);
/**
 * Combine a child and its descendants, adding the child's header as one entry
 * when it ran. Do not merge both a child and its descendants separately.
 * Source is unchanged. Self-merge is invalid. Parent identity/time are preserved;
 * status propagates (failure is sticky). Latest and first fatal follow merge order.
 * Malformed inputs/conflicting metric rules leave the destination unchanged.
 * Capacity exhaustion succeeds with visible loss counts; reserved first fatal
 * is retained and is not lost. Matching repeats can still accumulate when full.
 * Integer and elapsed-time accumulation saturate and set counters_saturated.
 */
EngineResult report_merge(Report *destination, const Report *source);

/** Write a validated report through the console; no engine startup required. */
EngineResult report_console_write(const Report *report);
/** Replace a file with versioned JSON; pretty changes whitespace only.
 * Uses temporary allocation. Invalid reports/paths fail before opening the file.
 * An I/O failure can leave a partial file. The source report is never modified.
 */
EngineResult report_json_write(const Report *report, const char *path, bool pretty);
/** Replace a file with readable report text, without console prefixes.
 * Does not create directories. An I/O failure can leave a partial file.
 */
EngineResult report_text_write(const Report *report, const char *path);

#endif
