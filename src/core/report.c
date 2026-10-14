/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "report.h"
#include "core/report_internal.h"

#include <float.h>
#include <math.h>
#include <string.h>

static EngineResult report_ok(void) {
    return ERROR_RESULT_MAKE_VALUE(EngineResult, true);
}

static EngineResult report_invalid(void) {
    return ERROR_RESULT_MAKE_ERROR(EngineResult, ERROR_ENGINE_REPORT_INVALID);
}

static bool report_id_check(ReportId id) {
    return (id > ROHR_REPORT_NONE && id <= ROHR_ENGINE_LIFECYCLE_REPORT) ||
        (id >= ROHR_REPORT_CUSTOM_ID_BEGIN && id <= ROHR_REPORT_CUSTOM_ID_END);
}

static bool report_header_check(ReportHeader header, bool empty_allowed) {
    return (report_id_check(header.id) ||
            (empty_allowed && header.id == ROHR_REPORT_NONE &&
             header.status == ROHR_REPORT_NOT_RUN && header.total_ms == 0.0)) &&
        header.status >= ROHR_REPORT_NOT_RUN && header.status <= ROHR_REPORT_FAILED &&
        isfinite(header.total_ms) && header.total_ms >= 0.0;
}

static bool report_object_check(ReportObjectRef ref) {
    return ref.kind >= ROHR_REPORT_OBJECT_NONE && ref.kind <= ROHR_REPORT_OBJECT_CUSTOM &&
        (ref.kind != ROHR_REPORT_OBJECT_NONE || ref.id == 0);
}

static bool report_entry_check(ReportEntry entry) {
    return report_header_check(entry.header, false) && entry.invocation_count > 0 &&
        entry.header.status != ROHR_REPORT_NOT_RUN;
}

static bool report_statistic_check(ReportStatistic value) {
    ReportAggregation expected;
    if(!report_id_check(value.report_id)) return false;
    if(value.metric_id >= ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN &&
            value.metric_id <= ROHR_REPORT_CUSTOM_METRIC_ID_END)
        return value.aggregation >= ROHR_REPORT_AGGREGATION_SUM &&
            value.aggregation <= ROHR_REPORT_AGGREGATION_LATEST;
    if(value.metric_id >= ROHR_REPORT_PROCESSED_COUNT &&
            value.metric_id <= ROHR_REPORT_NARROWPHASE_NS)
        expected = ROHR_REPORT_AGGREGATION_SUM;
    else if(value.metric_id == ROHR_REPORT_TREE_HEIGHT_PEAK)
        expected = ROHR_REPORT_AGGREGATION_MAX;
    else if(value.metric_id == ROHR_REPORT_TREE_NODES_CURRENT)
        expected = ROHR_REPORT_AGGREGATION_LATEST;
    else if(value.metric_id == ROHR_REPORT_QUEUED_COMMAND_COUNT ||
            value.metric_id == ROHR_REPORT_FRAME_LIMIT_WAIT_NS ||
            value.metric_id == ROHR_REPORT_AUDIO_FRAMES_COUNT ||
            value.metric_id == ROHR_REPORT_CANCELLED_COMMAND_COUNT ||
            value.metric_id == ROHR_REPORT_TICKS_ADVANCED_COUNT ||
            value.metric_id == ROHR_REPORT_EXPIRED_ENTITY_COUNT)
        expected = ROHR_REPORT_AGGREGATION_SUM;
    else if(value.metric_id >= ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT &&
            value.metric_id <= ROHR_REPORT_UI_EVENT_DROPPED_COUNT)
        expected = ROHR_REPORT_AGGREGATION_SUM;
    else if(value.metric_id >= ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK &&
            value.metric_id <= ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK)
        expected = ROHR_REPORT_AGGREGATION_MAX;
    else return false;
    return value.aggregation == expected;
}

static bool report_statistic_same(ReportStatistic a, ReportStatistic b) {
    return a.report_id == b.report_id && a.metric_id == b.metric_id;
}

static bool report_diagnostic_check(const ReportDiagnostic *value) {
    return report_id_check(value->report_id) &&
        (value->severity == ROHR_REPORT_SEVERITY_WARNING ||
         value->severity == ROHR_REPORT_SEVERITY_FATAL) &&
        value->code > ERROR_NONE && value->code <= ERROR_ENGINE_UI_SCOPE_UNBALANCED &&
        value->recovery >= ROHR_REPORT_RECOVERY_NONE &&
        value->recovery <= ROHR_REPORT_UPDATE_ABORTED &&
        report_object_check(value->source) && report_object_check(value->target) &&
        value->occurrence_count > 0 &&
        (value->has_substep ? value->first_substep <= value->last_substep :
         value->first_substep == 0 && value->last_substep == 0) &&
        memchr(value->detail, '\0', sizeof(value->detail)) != NULL;
}

static bool report_diagnostic_same(const ReportDiagnostic *a, const ReportDiagnostic *b) {
    return a->report_id == b->report_id && a->internal_stage == b->internal_stage &&
        a->severity == b->severity && a->code == b->code && a->recovery == b->recovery &&
        a->source.kind == b->source.kind && a->source.id == b->source.id &&
        a->target.kind == b->target.kind && a->target.id == b->target.id;
}

static bool report_diagnostic_equal(const ReportDiagnostic *a, const ReportDiagnostic *b) {
    return report_diagnostic_same(a, b) && a->occurrence_count == b->occurrence_count &&
        a->has_substep == b->has_substep && a->first_substep == b->first_substep &&
        a->last_substep == b->last_substep && a->detail_truncated == b->detail_truncated &&
        strcmp(a->detail, b->detail) == 0;
}

bool report_valid_check(const Report *report) {
    bool fatal_seen = false;
    if(report == NULL || !report_header_check(report->header, true) ||
            report->entry_count > ROHR_REPORT_ENTRY_CAPACITY ||
            report->statistic_count > ROHR_REPORT_STATISTIC_CAPACITY ||
            report->diagnostic_count > ROHR_REPORT_DIAGNOSTIC_CAPACITY) return false;
    if(report->header.id == ROHR_REPORT_NONE)
        return report->entry_count == 0 && report->statistic_count == 0 &&
            report->diagnostic_count == 0 && !report->has_fatal &&
            report->dropped_entry_count == 0 && report->dropped_statistic_count == 0 &&
            report->dropped_diagnostic_occurrences == 0 && !report->counters_saturated;
    if(report->has_fatal && (!report_diagnostic_check(&report->first_fatal) ||
            report->first_fatal.severity != ROHR_REPORT_SEVERITY_FATAL ||
            report->header.status != ROHR_REPORT_FAILED)) return false;
    for(size_t i = 0; i < report->entry_count; i += 1) {
        if(!report_entry_check(report->entries[i])) return false;
        if(report->entries[i].header.status == ROHR_REPORT_FAILED &&
                report->header.status != ROHR_REPORT_FAILED) return false;
        for(size_t j = 0; j < i; j += 1)
            if(report->entries[i].header.id == report->entries[j].header.id) return false;
    }
    for(size_t i = 0; i < report->statistic_count; i += 1) {
        if(!report_statistic_check(report->statistics[i])) return false;
        for(size_t j = 0; j < i; j += 1)
            if(report_statistic_same(report->statistics[i], report->statistics[j])) return false;
    }
    for(size_t i = 0; i < report->diagnostic_count; i += 1) {
        const ReportDiagnostic *value = &report->diagnostics[i];
        if(!report_diagnostic_check(value)) return false;
        for(size_t j = 0; j < i; j += 1)
            if(report_diagnostic_same(value, &report->diagnostics[j])) return false;
        if(value->severity == ROHR_REPORT_SEVERITY_FATAL) {
            if(!report->has_fatal) return false;
            if(!fatal_seen && !report_diagnostic_same(value, &report->first_fatal)) return false;
            fatal_seen = true;
        }
        if(report->has_fatal && report_diagnostic_same(value, &report->first_fatal) &&
                !report_diagnostic_equal(value, &report->first_fatal)) return false;
    }
    return true;
}

static uint64_t report_sum(Report *report, uint64_t a, uint64_t b) {
    if(UINT64_MAX - a < b) {
        report->counters_saturated = true;
        return UINT64_MAX;
    }
    return a + b;
}

static double report_time_sum(Report *report, double a, double b) {
    if(DBL_MAX - a < b) {
        report->counters_saturated = true;
        return DBL_MAX;
    }
    return a + b;
}

static void report_status_combine(Report *report, ReportStatus status) {
    if(status > report->header.status) report->header.status = status;
}

Report report_create(void) {
    return (Report){0};
}

EngineResult report_header_set(Report *report, ReportHeader header) {
    if(!report_valid_check(report) || !report_header_check(header, true) ||
            (header.id == ROHR_REPORT_NONE && report->header.id != ROHR_REPORT_NONE) ||
            (report->header.status == ROHR_REPORT_FAILED && header.status != ROHR_REPORT_FAILED))
        return report_invalid();
    report->header = header;
    return report_ok();
}

static void report_entry_collect(Report *report, ReportEntry entry) {
    report_status_combine(report, entry.header.status);
    for(size_t i = 0; i < report->entry_count; i += 1) {
        ReportEntry *stored = &report->entries[i];
        if(stored->header.id != entry.header.id) continue;
        if(entry.header.status > stored->header.status) stored->header.status = entry.header.status;
        stored->header.total_ms = report_time_sum(report, stored->header.total_ms, entry.header.total_ms);
        stored->invocation_count = report_sum(report, stored->invocation_count, entry.invocation_count);
        return;
    }
    if(report->entry_count < ROHR_REPORT_ENTRY_CAPACITY)
        report->entries[report->entry_count++] = entry;
    else report->dropped_entry_count = report_sum(report, report->dropped_entry_count, 1);
}

EngineResult report_entry_add(Report *report, ReportEntry entry) {
    if(!report_valid_check(report) || !report_entry_check(entry) || report->header.id == ROHR_REPORT_NONE)
        return report_invalid();
    report_entry_collect(report, entry);
    return report_ok();
}

static bool report_statistic_compatible(const Report *report, ReportStatistic value) {
    for(size_t i = 0; i < report->statistic_count; i += 1)
        if(report_statistic_same(report->statistics[i], value) &&
                report->statistics[i].aggregation != value.aggregation) return false;
    return true;
}

static void report_statistic_collect(Report *report, ReportStatistic value) {
    for(size_t i = 0; i < report->statistic_count; i += 1) {
        ReportStatistic *stored = &report->statistics[i];
        if(!report_statistic_same(*stored, value)) continue;
        if(value.aggregation == ROHR_REPORT_AGGREGATION_SUM)
            stored->value = report_sum(report, stored->value, value.value);
        else if(value.aggregation == ROHR_REPORT_AGGREGATION_LATEST || value.value > stored->value)
            stored->value = value.value;
        return;
    }
    if(report->statistic_count < ROHR_REPORT_STATISTIC_CAPACITY)
        report->statistics[report->statistic_count++] = value;
    else report->dropped_statistic_count = report_sum(report, report->dropped_statistic_count, 1);
}

EngineResult report_statistic_add(Report *report, ReportStatistic statistic) {
    if(!report_valid_check(report) || !report_statistic_check(statistic) ||
            report->header.id == ROHR_REPORT_NONE ||
            !report_statistic_compatible(report, statistic)) return report_invalid();
    report_statistic_collect(report, statistic);
    return report_ok();
}

EngineResult report_diagnostic_detail_set(ReportDiagnostic *diagnostic, const char *detail) {
    char copy[ROHR_REPORT_DETAIL_CAPACITY] = {0};
    size_t count = 0;
    bool truncated = false;
    if(diagnostic == NULL) return report_invalid();
    if(detail != NULL) {
        while(count + 1 < sizeof(copy) && detail[count] != '\0') {
            copy[count] = detail[count];
            count += 1;
        }
        truncated = detail[count] != '\0';
    }
    memcpy(diagnostic->detail, copy, sizeof(copy));
    diagnostic->detail_truncated = truncated;
    return report_ok();
}

static void report_diagnostic_accumulate(Report *report, ReportDiagnostic *stored,
        const ReportDiagnostic *value) {
    stored->occurrence_count = report_sum(report, stored->occurrence_count, value->occurrence_count);
    if(value->has_substep) {
        if(!stored->has_substep || value->first_substep < stored->first_substep)
            stored->first_substep = value->first_substep;
        if(!stored->has_substep || value->last_substep > stored->last_substep)
            stored->last_substep = value->last_substep;
        stored->has_substep = true;
    }
}

static void report_diagnostic_collect(Report *report, const ReportDiagnostic *value) {
    bool first_fatal = false;
    bool reserved_match = false;
    if(value->severity == ROHR_REPORT_SEVERITY_FATAL) {
        report->header.status = ROHR_REPORT_FAILED;
        if(!report->has_fatal) {
            report->has_fatal = true;
            report->first_fatal = *value;
            first_fatal = true;
        }
        reserved_match = report_diagnostic_same(value, &report->first_fatal);
    }
    for(size_t i = 0; i < report->diagnostic_count; i += 1) {
        ReportDiagnostic *stored = &report->diagnostics[i];
        if(!report_diagnostic_same(stored, value)) continue;
        report_diagnostic_accumulate(report, stored, value);
        if(reserved_match) report->first_fatal = *stored;
        return;
    }
    if(reserved_match && !first_fatal)
        report_diagnostic_accumulate(report, &report->first_fatal, value);
    if(report->diagnostic_count < ROHR_REPORT_DIAGNOSTIC_CAPACITY) {
        report->diagnostics[report->diagnostic_count++] = reserved_match ? report->first_fatal : *value;
    } else if(!reserved_match) {
        report->dropped_diagnostic_occurrences = report_sum(report,
            report->dropped_diagnostic_occurrences, value->occurrence_count);
    }
}

void report_diagnostic_collect_internal(Report *report, ReportDiagnostic diagnostic) {
    report_diagnostic_collect(report, &diagnostic);
}

EngineResult report_diagnostic_add(Report *report, ReportDiagnostic diagnostic) {
    if(!report_valid_check(report) || !report_diagnostic_check(&diagnostic) ||
            report->header.id == ROHR_REPORT_NONE) return report_invalid();
    report_diagnostic_collect(report, &diagnostic);
    return report_ok();
}

EngineResult report_merge(Report *destination, const Report *source) {
    bool fatal_in_list = false;
    if(destination == source || !report_valid_check(destination) || !report_valid_check(source) ||
            destination->header.id == ROHR_REPORT_NONE) return report_invalid();
    for(size_t i = 0; i < source->statistic_count; i += 1)
        if(!report_statistic_compatible(destination, source->statistics[i])) return report_invalid();

    report_status_combine(destination, source->header.status);
    if(source->header.status != ROHR_REPORT_NOT_RUN)
        report_entry_collect(destination, (ReportEntry){source->header, 1});
    for(size_t i = 0; i < source->entry_count; i += 1)
        report_entry_collect(destination, source->entries[i]);
    for(size_t i = 0; i < source->statistic_count; i += 1)
        report_statistic_collect(destination, source->statistics[i]);

    /* Preserve ordinary order/capacity, then capture any reserved-only fatal. */
    for(size_t i = 0; i < source->diagnostic_count; i += 1) {
        const ReportDiagnostic *value = &source->diagnostics[i];
        if(source->has_fatal && report_diagnostic_same(value, &source->first_fatal)) fatal_in_list = true;
        report_diagnostic_collect(destination, value);
    }
    if(source->has_fatal && !fatal_in_list)
        report_diagnostic_collect(destination, &source->first_fatal);
    destination->dropped_entry_count = report_sum(destination,
        destination->dropped_entry_count, source->dropped_entry_count);
    destination->dropped_statistic_count = report_sum(destination,
        destination->dropped_statistic_count, source->dropped_statistic_count);
    destination->dropped_diagnostic_occurrences = report_sum(destination,
        destination->dropped_diagnostic_occurrences, source->dropped_diagnostic_occurrences);
    destination->counters_saturated |= source->counters_saturated;
    return report_ok();
}
