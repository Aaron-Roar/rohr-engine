/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define OK(call) assert((call).kind == ERROR_RESULT_VALUE)
#define BAD(call) assert((call).kind == ERROR_RESULT_ERROR)

static Report make_report(ReportId id) {
    Report result = rohr_report_create();
    OK(rohr_report_header_set(&result, (ReportHeader){id, ROHR_REPORT_COMPLETE, 2.0}));
    return result;
}

static ReportDiagnostic issue(uint64_t object, ReportSeverity severity) {
    ReportDiagnostic value = {0};
    value.report_id = ROHR_PHYSICS_JOINTS_REPORT_ID;
    value.severity = severity;
    value.code = ERROR_ENGINE_STATE_INVALID;
    value.source = (ReportObjectRef){ROHR_REPORT_OBJECT_ENTITY, object};
    value.occurrence_count = 1;
    return value;
}

static ReportStatistic stat(ReportMetricId metric, ReportAggregation rule, uint64_t value) {
    return (ReportStatistic){.report_id = ROHR_PHYSICS_JOINTS_REPORT_ID, .metric_id = metric, .aggregation = rule, .value = value};
}

static void construction(void) {
    Report value = rohr_report_create();
    ReportDiagnostic diagnostic = {0};
    assert(rohr_report_valid_check(&value));
    assert(value.header.id == ROHR_REPORT_NONE_ID && value.header.status == ROHR_REPORT_NOT_RUN);
    assert(diagnostic.severity == ROHR_REPORT_SEVERITY_NONE);
    assert(!rohr_report_valid_check(NULL));
    BAD(rohr_report_header_set(NULL, (ReportHeader){0}));
    BAD(rohr_report_header_set(&value, (ReportHeader){(ReportId)99, ROHR_REPORT_COMPLETE, 0}));
    OK(rohr_report_header_set(&value, (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_NOT_RUN, 0}));
    OK(rohr_report_header_set(&value, (ReportHeader){ROHR_REPORT_CUSTOM_ID_END, ROHR_REPORT_COMPLETE, 1}));
    BAD(rohr_report_header_set(&value, (ReportHeader){(ReportId)-1, ROHR_REPORT_COMPLETE, 0}));
    BAD(rohr_report_header_set(&value, (ReportHeader){ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_COMPLETE, NAN}));
    BAD(rohr_report_header_set(&value, (ReportHeader){ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_COMPLETE, -1}));
    BAD(rohr_report_diagnostic_add(&value, diagnostic));
    diagnostic = issue(10, ROHR_REPORT_SEVERITY_WARNING);
    diagnostic.occurrence_count = 0;
    BAD(rohr_report_diagnostic_add(&value, diagnostic));
    assert(value.diagnostic_count == 0 && !value.has_fatal);
}

static void details_and_identity(void) {
    Report value = make_report(ROHR_PHYSICS_REPORT_ID);
    ReportDiagnostic diagnostic = issue(10, ROHR_REPORT_SEVERITY_WARNING);
    char text[ROHR_REPORT_DETAIL_CAPACITY + 4];
    memset(text, 'x', sizeof(text)); text[sizeof(text)-1] = '\0';
    OK(rohr_report_diagnostic_detail_set(&diagnostic, text));
    assert(diagnostic.detail_truncated && strlen(diagnostic.detail) == ROHR_REPORT_DETAIL_CAPACITY-1);
    OK(rohr_report_diagnostic_detail_set(&diagnostic, diagnostic.detail + 1));
    assert(!diagnostic.detail_truncated && strlen(diagnostic.detail) == ROHR_REPORT_DETAIL_CAPACITY-2);
    OK(rohr_report_diagnostic_detail_set(&diagnostic, "original"));
    diagnostic.has_substep = true; diagnostic.first_substep = 3; diagnostic.last_substep = 4;
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    OK(rohr_report_diagnostic_detail_set(&diagnostic, "changed"));
    diagnostic.occurrence_count = 4;
    diagnostic.first_substep = 1; diagnostic.last_substep = 9;
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    assert(value.diagnostic_count == 1 && value.diagnostics[0].occurrence_count == 5);
    assert(value.diagnostics[0].first_substep == 1 && value.diagnostics[0].last_substep == 9);
    assert(strcmp(value.diagnostics[0].detail, "original") == 0);
    diagnostic.source.id += UINT64_C(1) << 32; /* Reused slot, different generation. */
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    diagnostic.report_id = ROHR_GRAPHICS_REPORT_ID;
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    diagnostic.internal_stage = 1;
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    diagnostic.recovery = ROHR_REPORT_CONTRIBUTION_SKIPPED;
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    diagnostic.target = (ReportObjectRef){ROHR_REPORT_OBJECT_TEXTURE, 10};
    OK(rohr_report_diagnostic_add(&value, diagnostic));
    assert(value.diagnostic_count == 6);
    Report copy = value;
    OK(rohr_report_diagnostic_detail_set(&diagnostic, NULL));
    assert(diagnostic.detail[0] == '\0' && !diagnostic.detail_truncated);
    memset(&value, 0, sizeof(value));
    assert(rohr_report_valid_check(&copy) && strcmp(copy.diagnostics[0].detail, "original") == 0);
    BAD(rohr_report_diagnostic_detail_set(NULL, "x"));
}

static void aggregation(void) {
    Report a = make_report(ROHR_PHYSICS_REPORT_ID), b = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    a.header.total_ms = 20;
    OK(rohr_report_statistic_add(&a, stat(ROHR_REPORT_PROCESSED_COUNT, ROHR_REPORT_AGGREGATION_SUM, 10)));
    OK(rohr_report_statistic_add(&b, stat(ROHR_REPORT_PROCESSED_COUNT, ROHR_REPORT_AGGREGATION_SUM, 7)));
    OK(rohr_report_statistic_add(&a, stat(ROHR_REPORT_TREE_HEIGHT_PEAK, ROHR_REPORT_AGGREGATION_MAX, 10)));
    OK(rohr_report_statistic_add(&b, stat(ROHR_REPORT_TREE_HEIGHT_PEAK, ROHR_REPORT_AGGREGATION_MAX, 7)));
    OK(rohr_report_statistic_add(&a, stat(ROHR_REPORT_TREE_NODES_CURRENT, ROHR_REPORT_AGGREGATION_LATEST, 10)));
    OK(rohr_report_statistic_add(&b, stat(ROHR_REPORT_TREE_NODES_CURRENT, ROHR_REPORT_AGGREGATION_LATEST, 7)));
    Report before = b;
    OK(rohr_report_merge(&a, &b));
    assert(memcmp(&before, &b, sizeof(b)) == 0);
    assert(a.header.total_ms == 20 && a.header.id == ROHR_PHYSICS_REPORT_ID);
    assert(a.statistics[0].value == 17 && a.statistics[1].value == 10 && a.statistics[2].value == 7);
    assert(a.entry_count == 1 && a.entries[0].invocation_count == 1 && a.entries[0].header.total_ms == 2);
    OK(rohr_report_merge(&a, &b));
    assert(a.entries[0].invocation_count == 2 && a.entries[0].header.total_ms == 4);
    assert(a.statistics[0].value == 24 && a.header.total_ms == 20);

    ReportStatistic custom = stat(ROHR_REPORT_CUSTOM_METRIC_ID_END, ROHR_REPORT_AGGREGATION_NONE, 9);
    BAD(rohr_report_statistic_add(&a, custom));
    custom.aggregation = ROHR_REPORT_AGGREGATION_MAX;
    OK(rohr_report_statistic_add(&a, custom));
    custom.aggregation = ROHR_REPORT_AGGREGATION_SUM;
    OK(rohr_report_statistic_add(&b, custom));
    Report saved = a;
    BAD(rohr_report_merge(&a, &b));
    assert(memcmp(&a, &saved, sizeof(a)) == 0);
    BAD(rohr_report_statistic_add(&a, stat(ROHR_REPORT_CONTACT_COUNT, ROHR_REPORT_AGGREGATION_MAX, 1)));
    BAD(rohr_report_merge(&a, &a));
    BAD(rohr_report_merge(&a, NULL));
    assert(memcmp(&a, &saved, sizeof(a)) == 0);
}

static void critical_capacity(void) {
    Report source = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    for(size_t i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY; i += 1)
        OK(rohr_report_diagnostic_add(&source, issue(i + 1, ROHR_REPORT_SEVERITY_WARNING)));
    ReportDiagnostic fatal = issue(9999, ROHR_REPORT_SEVERITY_FATAL);
    OK(rohr_report_diagnostic_detail_set(&fatal, "first critical failure"));
    OK(rohr_report_diagnostic_add(&source, fatal));
    assert(source.has_fatal && source.header.status == ROHR_REPORT_FAILED);
    assert(source.dropped_diagnostic_occurrences == 0 && source.diagnostic_count == 256);
    OK(rohr_report_diagnostic_detail_set(&fatal, "later text must not replace the first"));
    OK(rohr_report_diagnostic_add(&source, fatal));
    assert(source.first_fatal.occurrence_count == 2);
    assert(strcmp(source.first_fatal.detail, "first critical failure") == 0);
    assert(source.dropped_diagnostic_occurrences == 0);
    fatal.source.id += 1; fatal.occurrence_count = 3;
    OK(rohr_report_diagnostic_add(&source, fatal));
    assert(source.dropped_diagnostic_occurrences == 3);
    assert(source.first_fatal.source.id == 9999 && rohr_report_valid_check(&source));
    Report destination = make_report(ROHR_PHYSICS_REPORT_ID);
    OK(rohr_report_merge(&destination, &source));
    assert(destination.diagnostic_count == 256 && destination.dropped_diagnostic_occurrences == 3);
    assert(destination.first_fatal.occurrence_count == 2);
    OK(rohr_report_merge(&destination, &source));
    assert(destination.first_fatal.occurrence_count == 4);
    assert(destination.dropped_diagnostic_occurrences == 6);
    assert(destination.diagnostics[0].occurrence_count == 2);
    BAD(rohr_report_header_set(&destination, (ReportHeader){ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_COMPLETE, 0}));
    Report success = make_report(ROHR_GRAPHICS_REPORT_ID);
    OK(rohr_report_merge(&destination, &success));
    assert(destination.header.status == ROHR_REPORT_FAILED);
    assert(strcmp(destination.first_fatal.detail, "first critical failure") == 0);
}

static void mirrored_fatal(void) {
    Report a = make_report(ROHR_PHYSICS_REPORT_ID), b = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    ReportDiagnostic fatal = issue(1, ROHR_REPORT_SEVERITY_FATAL);
    OK(rohr_report_diagnostic_detail_set(&fatal, "first"));
    OK(rohr_report_diagnostic_add(&b, fatal));
    OK(rohr_report_merge(&a, &b));
    assert(a.diagnostic_count == 1 && a.diagnostics[0].occurrence_count == 1);
    assert(a.first_fatal.occurrence_count == 1);
    OK(rohr_report_diagnostic_detail_set(&fatal, "second"));
    OK(rohr_report_diagnostic_add(&b, fatal));
    fatal.source.id = 2;
    OK(rohr_report_diagnostic_add(&b, fatal));
    OK(rohr_report_merge(&a, &b));
    assert(a.diagnostic_count == 2 && a.first_fatal.occurrence_count == 3);
    assert(a.diagnostics[0].occurrence_count == 3);
    assert(strcmp(a.first_fatal.detail, "first") == 0 && rohr_report_valid_check(&a));
    Report full = make_report(ROHR_GRAPHICS_REPORT_ID);
    for(size_t i = 0; i < 256; i += 1)
        OK(rohr_report_diagnostic_add(&full, issue(100+i, ROHR_REPORT_SEVERITY_WARNING)));
    OK(rohr_report_merge(&full, &b));
    assert(full.first_fatal.source.id == 1 && full.first_fatal.occurrence_count == 2);
    assert(full.dropped_diagnostic_occurrences == 1); /* Only the later fatal is lost. */
}

static void overflow_and_saturation(void) {
    Report a = make_report(ROHR_PHYSICS_REPORT_ID), b = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    for(size_t i = 0; i < ROHR_REPORT_ENTRY_CAPACITY; i += 1)
        OK(rohr_report_entry_add(&a, (ReportEntry){.header = {(ReportId)(ROHR_REPORT_CUSTOM_ID_BEGIN+i), ROHR_REPORT_COMPLETE, 1}, .invocation_count = 1}));
    for(size_t i = 0; i < ROHR_REPORT_STATISTIC_CAPACITY; i += 1)
        OK(rohr_report_statistic_add(&a, stat((ReportMetricId)(ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN+i),
            ROHR_REPORT_AGGREGATION_SUM, 1)));
    OK(rohr_report_statistic_add(&b, stat(ROHR_REPORT_PROCESSED_COUNT, ROHR_REPORT_AGGREGATION_SUM, 1)));
    b.dropped_entry_count = 2; b.dropped_statistic_count = 3;
    b.dropped_diagnostic_occurrences = 4;
    OK(rohr_report_merge(&a, &b));
    assert(a.dropped_entry_count == 3 && a.dropped_statistic_count == 4);
    assert(a.dropped_diagnostic_occurrences == 4 && a.header.status == ROHR_REPORT_COMPLETE);
    a.entries[0].invocation_count = UINT64_MAX;
    a.entries[0].header.total_ms = DBL_MAX;
    OK(rohr_report_entry_add(&a, (ReportEntry){.header = {ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, DBL_MAX}, .invocation_count = 1}));
    assert(a.entries[0].header.total_ms == DBL_MAX && a.entries[0].invocation_count == UINT64_MAX);
    a.statistics[0].value = UINT64_MAX;
    OK(rohr_report_statistic_add(&a, stat(ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN, ROHR_REPORT_AGGREGATION_SUM, 1)));
    assert(a.statistics[0].value == UINT64_MAX && a.counters_saturated);
    a.dropped_entry_count = UINT64_MAX;
    OK(rohr_report_merge(&a, &b));
    assert(a.dropped_entry_count == UINT64_MAX && rohr_report_valid_check(&a));
    ReportDiagnostic diagnostic = issue(1, ROHR_REPORT_SEVERITY_FATAL);
    diagnostic.occurrence_count = UINT64_MAX;
    OK(rohr_report_diagnostic_add(&b, diagnostic));
    diagnostic.occurrence_count = 1;
    OK(rohr_report_diagnostic_add(&b, diagnostic));
    assert(b.first_fatal.occurrence_count == UINT64_MAX && b.counters_saturated);
}

static void malformed_and_parity(void) {
    Report a = make_report(ROHR_PHYSICS_REPORT_ID), b = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    Report saved = a;
    b.diagnostic_count = SIZE_MAX;
    assert(!rohr_report_valid_check(&b)); BAD(rohr_report_merge(&a, &b));
    assert(memcmp(&a, &saved, sizeof(a)) == 0);
    b = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    ReportDiagnostic diagnostic = issue(1, ROHR_REPORT_SEVERITY_WARNING);
    memset(diagnostic.detail, 'x', sizeof(diagnostic.detail));
    BAD(rohr_report_diagnostic_add(&a, diagnostic));
    diagnostic = issue(1, ROHR_REPORT_SEVERITY_WARNING);
    diagnostic.has_substep = true; diagnostic.first_substep = 3; diagnostic.last_substep = 1;
    BAD(rohr_report_diagnostic_add(&a, diagnostic));
    diagnostic = issue(1, ROHR_REPORT_SEVERITY_FATAL);
    OK(rohr_report_diagnostic_add(&b, diagnostic));
    b.first_fatal.occurrence_count += 1;
    assert(!report_valid_check(&b)); BAD(report_merge(&a, &b));
    assert(memcmp(&a, &saved, sizeof(a)) == 0);
    b = report_create();
    OK(report_header_set(&b, a.header));
    diagnostic = issue(42, ROHR_REPORT_SEVERITY_WARNING);
    OK(report_diagnostic_detail_set(&diagnostic, "same through either API"));
    OK(report_diagnostic_add(&b, diagnostic));
    OK(rohr_report_diagnostic_add(&a, diagnostic));
    assert(a.diagnostic_count == b.diagnostic_count);
    assert(strcmp(a.diagnostics[0].detail, b.diagnostics[0].detail) == 0);
    assert(report_valid_check(&a) && rohr_report_valid_check(&b));
}

/* Independent per-key arithmetic with all three stores full simultaneously. */
static void seeded_full_buffers(void) {
    Report r = make_report(ROHR_PHYSICS_REPORT_ID);
    uint64_t entries[ROHR_REPORT_ENTRY_CAPACITY], stats[ROHR_REPORT_STATISTIC_CAPACITY] = {0};
    uint64_t diagnostics[ROHR_REPORT_DIAGNOSTIC_CAPACITY];
    for(size_t i = 0; i < ROHR_REPORT_ENTRY_CAPACITY; ++i) {
        entries[i] = 1;
        OK(report_entry_add(&r, (ReportEntry){.header = {
            (ReportId)(ROHR_REPORT_CUSTOM_ID_BEGIN+i), ROHR_REPORT_COMPLETE, 1}, .invocation_count = 1}));
    }
    for(size_t i = 0; i < ROHR_REPORT_STATISTIC_CAPACITY; ++i)
        OK(report_statistic_add(&r, stat((ReportMetricId)(ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN+i),
            (ReportAggregation)(ROHR_REPORT_AGGREGATION_SUM+i%3), 0)));
    for(size_t i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY; ++i) {
        diagnostics[i] = 1;
        OK(report_diagnostic_add(&r, issue(i, ROHR_REPORT_SEVERITY_WARNING)));
    }
    uint32_t seed = 0x70421u;
    uint64_t lost_entries = 0, lost_stats = 0, lost_diagnostics = 0;
    for(unsigned step = 0; step < 512; ++step) {
        seed = seed * 1664525u + 1013904223u;
        size_t e = seed % (ROHR_REPORT_ENTRY_CAPACITY+3);
        size_t s = (seed >> 8) % (ROHR_REPORT_STATISTIC_CAPACITY+3);
        size_t d = (seed >> 16) % (ROHR_REPORT_DIAGNOSTIC_CAPACITY+3);
        uint64_t n = 1 + seed % 17;
        OK(report_entry_add(&r, (ReportEntry){.header = {
            (ReportId)(ROHR_REPORT_CUSTOM_ID_BEGIN+e), ROHR_REPORT_COMPLETE, (double)n}, .invocation_count = n}));
        OK(report_statistic_add(&r, stat((ReportMetricId)(ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN+s),
            (ReportAggregation)(ROHR_REPORT_AGGREGATION_SUM+s%3), n)));
        ReportDiagnostic diagnostic = issue(d, ROHR_REPORT_SEVERITY_WARNING);
        diagnostic.occurrence_count = n;
        OK(report_diagnostic_add(&r, diagnostic));
        if(e < ROHR_REPORT_ENTRY_CAPACITY) entries[e] += n; else ++lost_entries;
        if(s >= ROHR_REPORT_STATISTIC_CAPACITY) ++lost_stats;
        else if(s%3 == 0) stats[s] += n;
        else if(s%3 == 2 || n > stats[s]) stats[s] = n;
        if(d < ROHR_REPORT_DIAGNOSTIC_CAPACITY) diagnostics[d] += n; else lost_diagnostics += n;
    }
    for(size_t i = 0; i < ROHR_REPORT_ENTRY_CAPACITY; ++i) {
        assert(r.entries[i].invocation_count == entries[i]);
        assert(r.entries[i].header.total_ms == (double)entries[i]);
    }
    for(size_t i = 0; i < ROHR_REPORT_STATISTIC_CAPACITY; ++i) assert(r.statistics[i].value == stats[i]);
    for(size_t i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY; ++i) assert(r.diagnostics[i].occurrence_count == diagnostics[i]);
    assert(r.dropped_entry_count == lost_entries && lost_entries > 0);
    assert(r.dropped_statistic_count == lost_stats && lost_stats > 0);
    assert(r.dropped_diagnostic_occurrences == lost_diagnostics && lost_diagnostics > 0);
    ReportDiagnostic fatal = issue(UINT64_MAX, ROHR_REPORT_SEVERITY_FATAL);
    char original[ROHR_REPORT_DETAIL_CAPACITY+9];
    memset(original, 'f', sizeof(original)); original[sizeof(original)-1] = 0;
    OK(report_diagnostic_detail_set(&fatal, original));
    OK(report_diagnostic_add(&r, fatal));
    memset(original, 'z', sizeof(original));
    Report source = r;
    for(unsigned i = 0; i < 3; ++i) OK(report_merge(&r, &source));
    assert(r.first_fatal.occurrence_count == 4 && r.first_fatal.detail_truncated);
    assert(r.first_fatal.detail[0] == 'f' && r.first_fatal.source.id == UINT64_MAX);
    assert(r.dropped_diagnostic_occurrences == 4*lost_diagnostics);
    assert(r.dropped_entry_count == 4*lost_entries+3); /* Source roots cannot fit. */
    assert(r.dropped_statistic_count == 4*lost_stats);
    r.dropped_entry_count = r.dropped_statistic_count = r.dropped_diagnostic_occurrences = UINT64_MAX-1;
    r.entries[0].invocation_count = UINT64_MAX-1;
    r.entries[0].header.total_ms = DBL_MAX;
    r.statistics[0].value = UINT64_MAX-1;
    r.diagnostics[0].occurrence_count = UINT64_MAX-1;
    OK(report_merge(&r, &source));
    assert(r.dropped_entry_count == UINT64_MAX && r.dropped_statistic_count == UINT64_MAX);
    assert(r.dropped_diagnostic_occurrences == UINT64_MAX && r.counters_saturated);
    assert(r.entries[0].invocation_count == UINT64_MAX && r.entries[0].header.total_ms == DBL_MAX);
    assert(r.statistics[0].value == UINT64_MAX && r.diagnostics[0].occurrence_count == UINT64_MAX);
    assert(report_valid_check(&r));
}

static void invalid_matrix(void) {
    Report good = make_report(ROHR_PHYSICS_REPORT_ID);
    OK(report_entry_add(&good, (ReportEntry){.header = {ROHR_UI_REPORT_ID, ROHR_REPORT_COMPLETE, 1}, .invocation_count = 1}));
    OK(report_statistic_add(&good, stat(ROHR_REPORT_PROCESSED_COUNT, ROHR_REPORT_AGGREGATION_SUM, 1)));
    OK(report_diagnostic_add(&good, issue(7, ROHR_REPORT_SEVERITY_FATAL)));
    for(unsigned i = 0; i < 20; ++i) {
        Report bad = good;
        switch(i) {
        case 0: bad.entry_count = SIZE_MAX; break;
        case 1: bad.statistic_count = SIZE_MAX; break;
        case 2: bad.diagnostic_count = SIZE_MAX; break;
        case 3: bad.header.total_ms = INFINITY; break;
        case 4: bad.header.status = (ReportStatus)-1; break;
        case 5: bad.entries[0].invocation_count = 0; break;
        case 6: bad.entries[0].header.total_ms = NAN; break;
        case 7: bad.entries[1] = bad.entries[0]; bad.entry_count = 2; break;
        case 8: bad.statistics[0].aggregation = ROHR_REPORT_AGGREGATION_MAX; break;
        case 9: bad.statistics[1] = bad.statistics[0]; bad.statistic_count = 2; break;
        case 10: bad.diagnostics[0].code = ERROR_NONE; break;
        case 11: bad.diagnostics[0].source.kind = (ReportObjectKind)-1; break;
        case 12: bad.diagnostics[0].target.id = 1; break;
        case 13: bad.diagnostics[0].occurrence_count = 0; break;
        case 14: memset(bad.diagnostics[0].detail, 'x', sizeof(bad.diagnostics[0].detail)); break;
        case 15: memset(bad.first_fatal.detail, 'x', sizeof(bad.first_fatal.detail)); break;
        case 16: bad.first_fatal.detail[0] = 'x'; break;
        case 17: bad.has_fatal = false; break;
        case 18: bad.included_versions[ROHR_INPUT_REPORT_ID] = 1; break;
        case 19: bad.diagnostics[0].last_substep = 1; break;
        }
        Report before = bad, destination = good;
        assert(!report_valid_check(&bad) && !rohr_report_valid_check(&bad));
        BAD(report_merge(&destination, &bad));
        assert(memcmp(&destination, &good, sizeof(good)) == 0);
        BAD(rohr_report_merge(&bad, &good));
        BAD(report_header_set(&bad, good.header));
        BAD(rohr_report_entry_add(&bad, good.entries[0]));
        BAD(report_statistic_add(&bad, good.statistics[0]));
        BAD(rohr_report_diagnostic_add(&bad, good.diagnostics[0]));
        assert(memcmp(&bad, &before, sizeof(bad)) == 0);
    }
}

static void copy_and_merge_measurement(void) {
    Report child = make_report(ROHR_PHYSICS_JOINTS_REPORT_ID);
    for(size_t i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY; i += 1)
        OK(rohr_report_diagnostic_add(&child, issue(i+1, ROHR_REPORT_SEVERITY_WARNING)));
    clock_t start = clock();
    uint64_t observed = 0;
    for(unsigned i = 0; i < 100; i += 1) {
        Report parent = make_report(ROHR_PHYSICS_REPORT_ID);
        OK(rohr_report_merge(&parent, &child));
        observed += parent.diagnostic_count;
    }
    assert(observed == 100 * ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    printf("Report: %zu bytes; 100 full child merges: %.3f ms (CPU time, informational)\n",
        sizeof(Report), 1000.0 * (double)(clock()-start) / CLOCKS_PER_SEC);
}

int main(void) {
    seeded_full_buffers();
    invalid_matrix();
    construction();
    details_and_identity();
    aggregation();
    critical_capacity();
    mirrored_fatal();
    overflow_and_saturation();
    malformed_and_parity();
    copy_and_merge_measurement();
    return 0;
}
