/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "report.h"
#include "core/report_internal.h"

#include <SDL3/SDL_atomic.h>
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
    return (id > ROHR_REPORT_NONE && id <= ROHR_REPORT) ||
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
    return report_header_check(entry.header, false) &&
        (entry.group_root ? entry.group_version != 0 : true) &&
        (entry.header.status == ROHR_REPORT_NOT_RUN
            ? entry.group_root && entry.invocation_count == 0 : entry.invocation_count > 0);
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
    return a.report_id == b.report_id && a.metric_id == b.metric_id &&
        a.group_version == b.group_version;
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
        a->target.kind == b->target.kind && a->target.id == b->target.id &&
        a->group_version == b->group_version;
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
    if(report->included_versions[ROHR_REPORT_NONE] != 0 ||
            report->included_versions[ROHR_INPUT_REPORT] != 0) return false;
    if(report->header.id == ROHR_REPORT_NONE) {
        for(size_t i = 1; i < ROHR_REPORT; ++i)
            if(report->included_versions[i] != 0) return false;
        return report->entry_count == 0 && report->statistic_count == 0 &&
            report->diagnostic_count == 0 && !report->has_fatal &&
            report->dropped_entry_count == 0 && report->dropped_statistic_count == 0 &&
            report->dropped_diagnostic_occurrences == 0 && !report->counters_saturated &&
            report->version == 0 && report->invocation_count == 0;
    }
    if(report->has_fatal && (!report_diagnostic_check(&report->first_fatal) ||
            report->first_fatal.severity != ROHR_REPORT_SEVERITY_FATAL ||
            report->header.status != ROHR_REPORT_FAILED)) return false;
    for(size_t i = 0; i < report->entry_count; i += 1) {
        if(!report_entry_check(report->entries[i])) return false;
        if(report->entries[i].header.status == ROHR_REPORT_FAILED &&
                report->header.status != ROHR_REPORT_FAILED) return false;
        for(size_t j = 0; j < i; j += 1)
            if(report->entries[i].header.id == report->entries[j].header.id &&
                    report->entries[i].group_version == report->entries[j].group_version &&
                    report->entries[i].group_root == report->entries[j].group_root) return false;
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
        if(stored->header.id != entry.header.id || stored->group_version != entry.group_version ||
                stored->group_root != entry.group_root) continue;
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

/* Validated public inputs or trusted immutable engine snapshots. */
static void report_merge_collect(Report *destination, const Report *source, uint64_t group) {
    bool fatal_in_list = false;
    report_status_combine(destination, source->header.status);
    if(source->header.status != ROHR_REPORT_NOT_RUN)
        report_entry_collect(destination, (ReportEntry){.header = source->header,
            .invocation_count = source->invocation_count ? source->invocation_count : 1,
            .group_version = group, .group_root = group != 0});
    else if(group != 0)
        report_entry_collect(destination, (ReportEntry){.header = source->header,
            .group_version = group, .group_root = true});
    for(size_t i = 0; i < source->entry_count; i += 1) {
        ReportEntry entry = source->entries[i];
        if(group) entry.group_version = group;
        report_entry_collect(destination, entry);
    }
    for(size_t i = 0; i < source->statistic_count; i += 1) {
        ReportStatistic statistic = source->statistics[i];
        if(group) statistic.group_version = group;
        report_statistic_collect(destination, statistic);
    }

    /* Preserve ordinary order/capacity, then capture any reserved-only fatal. */
    for(size_t i = 0; i < source->diagnostic_count; i += 1) {
        const ReportDiagnostic *value = &source->diagnostics[i];
        if(source->has_fatal && report_diagnostic_same(value, &source->first_fatal)) fatal_in_list = true;
        ReportDiagnostic copy = *value;
        if(group) copy.group_version = group;
        report_diagnostic_collect(destination, &copy);
    }
    if(source->has_fatal && !fatal_in_list) {
        ReportDiagnostic copy = source->first_fatal;
        if(group) copy.group_version = group;
        report_diagnostic_collect(destination, &copy);
    }
    destination->dropped_entry_count = report_sum(destination,
        destination->dropped_entry_count, source->dropped_entry_count);
    destination->dropped_statistic_count = report_sum(destination,
        destination->dropped_statistic_count, source->dropped_statistic_count);
    destination->dropped_diagnostic_occurrences = report_sum(destination,
        destination->dropped_diagnostic_occurrences, source->dropped_diagnostic_occurrences);
    destination->counters_saturated |= source->counters_saturated;
    for(size_t i = 1; i < ROHR_REPORT; ++i)
        if(source->included_versions[i] > destination->included_versions[i])
            destination->included_versions[i] = source->included_versions[i];
}

EngineResult report_merge(Report *destination, const Report *source) {
    if(destination == source || !report_valid_check(destination) || !report_valid_check(source) ||
            destination->header.id == ROHR_REPORT_NONE) return report_invalid();
    for(size_t i = 0; i < source->statistic_count; i += 1)
        if(!report_statistic_compatible(destination, source->statistics[i])) return report_invalid();

    report_merge_collect(destination, source, 0);
    return report_ok();
}

/* Fixed process-lifetime cache; never touched by an audio callback. The lock
 * protects only bounded copies/composition, never system execution or output. */
static SDL_SpinLock report_store_lock;
static Report report_store[ROHR_REPORT];
static uint64_t report_sequence;
static bool report_sequence_exhausted;

static bool report_stored_id_check(ReportId id) {
    return id > ROHR_REPORT_NONE && id < ROHR_REPORT && id != ROHR_INPUT_REPORT;
}

void report_publish_internal(Report *report) {
    if(report->version != 0 || !report_stored_id_check(report->header.id)) return;
    SDL_LockSpinlock(&report_store_lock);
    if(report_sequence == UINT64_MAX) {
        /* Never reuse a version. Retrieval reports failure rather than claiming
         * the old cache is current after sequence exhaustion. */
        report_sequence_exhausted = true;
        report->counters_saturated = true;
    } else {
        report->version = ++report_sequence;
        if(report->header.status != ROHR_REPORT_NOT_RUN && report->invocation_count == 0)
            report->invocation_count = 1;
        report->included_versions[report->header.id] = report->version;
        report_store[report->header.id] = *report;
    }
    SDL_UnlockSpinlock(&report_store_lock);
}

ReportResult report_get(ReportId id) {
    ReportResult result = {.kind = ERROR_RESULT_VALUE};
    if(id != ROHR_REPORT && !report_stored_id_check(id))
        return ERROR_RESULT_MAKE_ERROR(ReportResult, ERROR_ENGINE_REPORT_INVALID);
    SDL_LockSpinlock(&report_store_lock);
    if(report_sequence_exhausted) {
        result.kind = ERROR_RESULT_ERROR;
        result.result.error = ERROR_ENGINE_REPORT_INVALID;
    } else if(id != ROHR_REPORT) {
        result.result.value = report_store[id];
        result.result.value.header.id = id;
    } else {
        Report *combined = &result.result.value;
        ReportId selected[ROHR_REPORT];
        size_t count = 0;
        combined->header.id = ROHR_REPORT;
        combined->version = report_sequence;
        for(ReportId candidate = 1; candidate < ROHR_REPORT; ++candidate) {
            const Report *value = &report_store[candidate];
            if(value->version == 0) continue;
            bool included = false;
            for(ReportId parent = 1; parent < ROHR_REPORT; ++parent)
                if(parent != candidate && report_store[parent].version != 0 &&
                        report_store[parent].included_versions[candidate] == value->version) {
                    included = true;
                    break;
                }
            if(included) continue;
            size_t at = count++;
            while(at > 0 && report_store[selected[at - 1]].version > value->version) {
                selected[at] = selected[at - 1];
                --at;
            }
            selected[at] = candidate;
        }
        for(size_t i = 0; i < count; ++i) {
            const Report *value = &report_store[selected[i]];
            report_merge_collect(combined, value, value->version);
        }
    }
    SDL_UnlockSpinlock(&report_store_lock);
    return result;
}
