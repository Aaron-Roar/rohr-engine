/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "rohr.h"
#include "core/console_internal.h"
#include "yyjson/yyjson.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#define dup _dup
#define dup2 _dup2
#define close _close
#define fileno _fileno
#else
#include <unistd.h>
#endif

#define OK(call) assert(!error_check(call))
#define FAIL(call, code) do { EngineResult failed_result = (call); assert(error_check(failed_result) && failed_result.result.error == (code)); } while(0)
static const char *json_path = "report_output_test.json";
static const char *pretty_path = "report_output_pretty_test.json";
static const char *text_path = "report_output_test.txt";

#ifdef ROHR_TEST_ALLOCATION_WRAP
static int allocation_limit = -1;
static bool fail_puts, fail_write, fail_flush, fail_close;
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
int __real_fputs(const char *, FILE *);
size_t __real_fwrite(const void *, size_t, size_t, FILE *);
int __real_fflush(FILE *);
int __real_fclose(FILE *);
static bool allocation_failure(void) {
    if(allocation_limit < 0) return false;
    if(allocation_limit == 0) return true;
    allocation_limit--;
    return false;
}
void *__wrap_malloc(size_t n) { return allocation_failure() ? NULL : __real_malloc(n); }
void *__wrap_realloc(void *p, size_t n) { return allocation_failure() ? NULL : __real_realloc(p, n); }
void *__wrap_calloc(size_t n, size_t s) { return allocation_failure() ? NULL : __real_calloc(n, s); }
int __wrap_fputs(const char *s, FILE *f) { return fail_puts ? EOF : __real_fputs(s, f); }
size_t __wrap_fwrite(const void *p, size_t s, size_t n, FILE *f) { return fail_write ? 0 : __real_fwrite(p, s, n, f); }
int __wrap_fflush(FILE *f) { return fail_flush ? EOF : __real_fflush(f); }
int __wrap_fclose(FILE *f) { int r = __real_fclose(f); return fail_close ? EOF : r; }
#endif

typedef struct Capture { FILE *file; int saved; } Capture;
static Capture capture_begin(void) {
    Capture c = {.file = tmpfile(), .saved = dup(fileno(stdout))};
    assert(c.file != NULL && c.saved >= 0);
    assert(fflush(stdout) == 0 && dup2(fileno(c.file), fileno(stdout)) >= 0);
    return c;
}
static char *stream_read(FILE *file) {
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file); assert(size >= 0);
    rewind(file);
    char *s = malloc((size_t)size + 1); assert(s);
    assert(fread(s, 1, (size_t)size, file) == (size_t)size);
    s[size] = '\0'; return s;
}
static char *capture_end(Capture c) {
    clearerr(stdout);
    assert(fflush(stdout) == 0 && dup2(c.saved, fileno(stdout)) >= 0);
    close(c.saved);
    char *s = stream_read(c.file);
    assert(fclose(c.file) == 0); return s;
}
static char *file_read(const char *path) {
    FILE *f = fopen(path, "rb"); assert(f);
    char *s = stream_read(f); assert(fclose(f) == 0); return s;
}
static void sentinel_write(void) {
    FILE *f = fopen(json_path, "wb"); assert(f);
    assert(fputs("unchanged", f) != EOF && fclose(f) == 0);
}
static void sentinel_check(void) {
    char *s = file_read(json_path); assert(strcmp(s, "unchanged") == 0); free(s);
}
static yyjson_doc *json_read(const char *path) {
    yyjson_doc *doc = yyjson_read_file(path, 0, NULL, NULL); assert(doc); return doc;
}
static yyjson_val *get(yyjson_val *v, const char *name) {
    yyjson_val *found = yyjson_obj_get(v, name); assert(found); return found;
}
static void string_is(yyjson_val *v, const char *s) {
    assert(yyjson_is_str(v) && strcmp(yyjson_get_str(v), s) == 0);
}
static Report sample(void) {
    Report r = report_create();
    OK(report_header_set(&r, (ReportHeader){ROHR_PHYSICS_REPORT, ROHR_REPORT_COMPLETE, .42}));
    OK(report_entry_add(&r, (ReportEntry){.header = {ROHR_PHYSICS_INTEGRATION_REPORT, ROHR_REPORT_COMPLETE, .12}, .invocation_count = UINT64_MAX}));
    OK(report_statistic_add(&r, (ReportStatistic){.report_id = ROHR_PHYSICS_INTEGRATION_REPORT, .metric_id = ROHR_REPORT_PROCESSED_COUNT, .aggregation = ROHR_REPORT_AGGREGATION_SUM, .value = UINT64_MAX}));
    OK(report_statistic_add(&r, (ReportStatistic){.report_id = ROHR_REPORT_CUSTOM_ID_BEGIN, .metric_id = ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN, .aggregation = ROHR_REPORT_AGGREGATION_LATEST, .value = 9007199254740993ULL}));
    ReportDiagnostic d = {
        .report_id = ROHR_PHYSICS_INTEGRATION_REPORT, .internal_stage = ROHR_PHYSICS_INTEGRATION,
        .severity = ROHR_REPORT_SEVERITY_WARNING, .code = ERROR_ENGINE_POSITION_OUT_OF_RANGE,
        .recovery = ROHR_REPORT_CONTRIBUTION_SKIPPED,
        .source = {ROHR_REPORT_OBJECT_ENTITY, UINT64_MAX}, .target = {ROHR_REPORT_OBJECT_ENTITY, 9007199254740993ULL},
        .occurrence_count = UINT64_MAX, .has_substep = true, .first_substep = 0, .last_substep = 3
    };
    OK(report_diagnostic_detail_set(&d, "quote \" and slash \\ and newline\n\t\033 utf8: café"));
    OK(report_diagnostic_add(&r, d));
    d.severity = ROHR_REPORT_SEVERITY_FATAL;
    d.recovery = ROHR_REPORT_ENTITY_QUARANTINED;
    OK(report_diagnostic_detail_set(&d, "original critical message"));
    OK(report_diagnostic_add(&r, d));
    r.dropped_entry_count = UINT64_MAX;
    r.dropped_statistic_count = UINT64_MAX;
    r.dropped_diagnostic_occurrences = UINT64_MAX;
    r.counters_saturated = true;
    return r;
}

static void json_and_ownership(void) {
    Report r = sample(), saved = r;
    OK(rohr_report_json_write(&r, json_path, false));
    OK(report_json_write(&r, pretty_path, true));
    yyjson_doc *compact = json_read(json_path), *pretty = json_read(pretty_path);
    yyjson_val *root = yyjson_doc_get_root(compact);
    assert(yyjson_equals(root, yyjson_doc_get_root(pretty)));
    assert(yyjson_get_uint(get(root, "format_version")) == 2);
    assert(yyjson_obj_get(root, "diagnostic_count") == NULL);
    yyjson_val *header = get(root, "header");
    string_is(get(get(header, "id"), "name"), "ROHR_PHYSICS_REPORT");
    assert(yyjson_get_real(get(header, "total_ms")) == .42);
    string_is(get(get(header, "status"), "name"), "ROHR_REPORT_FAILED");
    string_is(get(yyjson_arr_get(get(root, "entries"), 0), "invocation_count"), "18446744073709551615");
    yyjson_val *stats = get(root, "statistics");
    string_is(get(yyjson_arr_get(stats, 0), "value"), "18446744073709551615");
    yyjson_val *custom = yyjson_arr_get(stats, 1);
    assert(yyjson_is_null(get(get(custom, "report_id"), "name")));
    assert(yyjson_is_null(get(get(custom, "metric_id"), "name")));
    string_is(get(custom, "value"), "9007199254740993");
    yyjson_val *diags = get(root, "diagnostics"); assert(yyjson_arr_size(diags) == 2);
    yyjson_val *warning = yyjson_arr_get(diags, 0), *fatal = get(root, "first_fatal");
    string_is(get(warning, "detail"), r.diagnostics[0].detail);
    string_is(get(get(warning, "source"), "id"), "18446744073709551615");
    string_is(get(get(warning, "target"), "id"), "9007199254740993");
    string_is(get(warning, "occurrence_count"), "18446744073709551615");
    string_is(get(get(warning, "internal_stage"), "name"), "ROHR_PHYSICS_INTEGRATION");
    assert(yyjson_get_bool(get(warning, "has_substep")) && yyjson_get_uint(get(warning, "last_substep")) == 3);
    string_is(get(fatal, "detail"), "original critical message");
    string_is(get(fatal, "message"), error_code_message_get(ERROR_ENGINE_POSITION_OUT_OF_RANGE));
    assert(yyjson_equals(fatal, yyjson_arr_get(diags, 1)));
    string_is(get(root, "dropped_diagnostic_occurrences"), "18446744073709551615");
    assert(yyjson_get_bool(get(root, "counters_saturated")));
    assert(memcmp(&r, &saved, sizeof(r)) == 0);
    yyjson_doc_free(compact); yyjson_doc_free(pretty);
    char *a = file_read(json_path), *b = file_read(pretty_path);
    assert(strchr(a, '\n') == NULL && strchr(b, '\n') != NULL); free(a); free(b);
}

static void empty_and_invalid(void) {
    Report r = report_create();
    OK(report_json_write(&r, json_path, false));
    yyjson_doc *doc = json_read(json_path); yyjson_val *root = yyjson_doc_get_root(doc);
    assert(yyjson_is_null(get(root, "first_fatal")) && yyjson_arr_size(get(root, "diagnostics")) == 0);
    string_is(get(get(get(root, "header"), "status"), "name"), "ROHR_REPORT_NOT_RUN");
    yyjson_doc_free(doc);
    OK(report_header_set(&r, (ReportHeader){ROHR_GRAPHICS_REPORT, ROHR_REPORT_COMPLETE, 0}));
    OK(report_text_write(&r, text_path));
    char *s = file_read(text_path); assert(strstr(s, "GRAPHICS") && strstr(s, "COMPLETE")); free(s);
    sentinel_write();
    r.diagnostic_count = ROHR_REPORT_DIAGNOSTIC_CAPACITY + 1;
    FAIL(report_json_write(&r, json_path, true), ERROR_ENGINE_REPORT_INVALID);
    FAIL(report_text_write(&r, json_path), ERROR_ENGINE_REPORT_INVALID);
    Capture capture = capture_begin();
    FAIL(report_console_write(&r), ERROR_ENGINE_REPORT_INVALID);
    s = capture_end(capture); assert(s[0] == '\0'); free(s);
    sentinel_check();
    FAIL(report_console_write(NULL), ERROR_ENGINE_REPORT_INVALID);
    r = sample();
    FAIL(report_json_write(&r, NULL, true), ERROR_ENGINE_REPORT_INVALID);
    FAIL(report_text_write(&r, ""), ERROR_ENGINE_REPORT_INVALID);
    FAIL(report_json_write(&r, "missing_report_dir/no/report.json", true), ERROR_ENGINE_OUTPUT_IO_FAILED);
    FAIL(report_text_write(&r, "missing_report_dir/no/report.txt"), ERROR_ENGINE_OUTPUT_IO_FAILED);
    r.diagnostics[0].detail[0] = (char)0xff; r.diagnostics[0].detail[1] = '\0';
    sentinel_write();
    FAIL(report_json_write(&r, json_path, false), ERROR_ENGINE_OUTPUT_FORMAT_FAILED);
    sentinel_check();
}

static void text_and_history(void) {
    Report r = sample(), saved = r;
    OK(rohr_report_text_write(&r, text_path));
    char *plain = file_read(text_path);
    assert(strstr(plain, "First fatal:") && strstr(plain, "Counters saturated: yes"));
    char *first = strstr(plain, "original critical message"); assert(first && !strstr(first + 1, "original critical message"));
    assert(strstr(plain, "\\n\\t\\x1b") && strstr(plain, "custom/unknown (65536)"));
    Capture c = capture_begin(); OK(rohr_report_console_write(&r)); char *console = capture_end(c);
    const char *p = plain, *q = console;
    while(*p) {
        assert(strncmp(q, "[#]", 3) == 0); q += 3;
        const char *end = strchr(p, '\n'); assert(end);
        size_t length = (size_t)(end - p) + 1;
        assert(strncmp(p, q, length) == 0); p += length; q += length;
    }
    assert(*q == '\0' && memcmp(&r, &saved, sizeof(r)) == 0);
    free(plain); free(console);

    for(unsigned i = 0; i < 40; i++) {
        ReportDiagnostic d = {.report_id = ROHR_UI_REPORT, .severity = ROHR_REPORT_SEVERITY_WARNING,
            .code = ERROR_ENGINE_STATE_INVALID, .occurrence_count = 1, .internal_stage = i};
        OK(report_diagnostic_add(&r, d));
    }
    c = capture_begin(); OK(report_console_write(&r)); console = capture_end(c);
    size_t lines = 0; for(char *s = console; *s; s++) if(*s == '\n') lines++;
    assert(lines > 100);
    c = capture_begin(); console_logs_print(); char *history = capture_end(c);
    char *last = console;
    for(size_t i = 0; i < lines - 100; i++) last = strchr(last, '\n') + 1;
    assert(strcmp(last, history) == 0); free(console); free(history);
}

static void capacities_and_truncation(void) {
    Report r = report_create();
    OK(report_header_set(&r, (ReportHeader){ROHR_PHYSICS_REPORT, ROHR_REPORT_COMPLETE, DBL_MAX}));
    for(unsigned i = 0; i < ROHR_REPORT_ENTRY_CAPACITY + 1; i++)
        OK(report_entry_add(&r, (ReportEntry){.header = {ROHR_REPORT_CUSTOM_ID_BEGIN + i, ROHR_REPORT_COMPLETE, .1}, .invocation_count = 1}));
    for(unsigned i = 0; i < ROHR_REPORT_STATISTIC_CAPACITY + 1; i++)
        OK(report_statistic_add(&r, (ReportStatistic){.report_id = ROHR_PHYSICS_REPORT, .metric_id = ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN + i, .aggregation = ROHR_REPORT_AGGREGATION_MAX, .value = i}));
    ReportDiagnostic d = {.report_id = ROHR_PHYSICS_REPORT, .severity = ROHR_REPORT_SEVERITY_WARNING,
        .code = ERROR_ENGINE_STATE_INVALID, .occurrence_count = 1};
    for(unsigned i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY; i++) {
        d.internal_stage = i; OK(report_diagnostic_add(&r, d));
    }
    d.severity = ROHR_REPORT_SEVERITY_FATAL;
    char detail[ROHR_REPORT_DETAIL_CAPACITY + 2]; memset(detail, 1, sizeof(detail)); detail[sizeof(detail)-1] = 0;
    OK(report_diagnostic_detail_set(&d, detail)); OK(report_diagnostic_add(&r, d));
    Report saved = r;
    OK(report_json_write(&r, json_path, false));
    yyjson_doc *doc = json_read(json_path); yyjson_val *root = yyjson_doc_get_root(doc);
    assert(yyjson_arr_size(get(root, "entries")) == ROHR_REPORT_ENTRY_CAPACITY);
    assert(yyjson_arr_size(get(root, "statistics")) == ROHR_REPORT_STATISTIC_CAPACITY);
    assert(yyjson_arr_size(get(root, "diagnostics")) == ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    assert(yyjson_get_bool(get(get(root, "first_fatal"), "detail_truncated")));
    string_is(get(root, "dropped_entry_count"), "1");
    string_is(get(root, "dropped_statistic_count"), "1");
    string_is(get(root, "dropped_diagnostic_occurrences"), "0");
    yyjson_doc_free(doc);
    OK(report_text_write(&r, text_path)); char *text = file_read(text_path);
    assert(strstr(text, "[detail truncated]")); free(text);
    Capture c = capture_begin(); FAIL(report_console_write(&r), ERROR_ENGINE_OUTPUT_TRUNCATED);
    text = capture_end(c); assert(strstr(text, "[truncated]") && strstr(text, "Counters saturated:"));
    free(text); assert(memcmp(&r, &saved, sizeof(r)) == 0);
    char boundary[MAX_LOG_STR + 1]; memset(boundary, 'a', sizeof(boundary));
    boundary[MAX_LOG_STR - 4] = 0;
    c = capture_begin(); OK(console_checked_write(LOG_ENGINE, "%s", boundary)); text = capture_end(c);
    assert(strlen(text) == MAX_LOG_STR - 1); free(text);
    boundary[MAX_LOG_STR - 4] = 'a'; boundary[MAX_LOG_STR - 3] = 0;
    c = capture_begin(); FAIL(console_checked_write(LOG_ENGINE, "%s", boundary), ERROR_ENGINE_OUTPUT_TRUNCATED);
    text = capture_end(c); assert(strlen(text) == MAX_LOG_STR - 1 && strstr(text, "[truncated]")); free(text);
}

#ifdef ROHR_TEST_ALLOCATION_WRAP
static void failures(void) {
    Report r = sample(), saved = r;
    int limit;
    for(limit = 0; limit < 100; limit++) {
        sentinel_write(); allocation_limit = limit;
        EngineResult result = report_json_write(&r, json_path, true);
        allocation_limit = -1;
        if(!error_check(result)) break;
        assert(result.result.error == ERROR_MEMORY_POOL_ALLOCATION_FAILED);
        sentinel_check();
    }
    assert(limit > 2 && limit < 100);
    fail_write = true; FAIL(report_json_write(&r, json_path, false), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_write = false;
    fail_puts = true; FAIL(report_text_write(&r, text_path), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_puts = false;
    fail_flush = true; FAIL(report_json_write(&r, json_path, false), ERROR_ENGINE_OUTPUT_IO_FAILED);
    FAIL(report_text_write(&r, text_path), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_flush = false;
    fail_close = true; FAIL(report_json_write(&r, json_path, false), ERROR_ENGINE_OUTPUT_IO_FAILED);
    FAIL(report_text_write(&r, text_path), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_close = false;
    Capture c = capture_begin();
    fail_puts = true; FAIL(report_console_write(&r), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_puts = false;
    fail_flush = true; FAIL(report_console_write(&r), ERROR_ENGINE_OUTPUT_IO_FAILED); fail_flush = false;
    char *text = capture_end(c); free(text);
    assert(memcmp(&r, &saved, sizeof(r)) == 0);
}
#endif

/* Cover the complete built-in vocabulary, including reserved input IDs.
 * Naming an ID does not claim that its subsystem produces reports. */
static void builtin_names(void) {
    static const struct { ReportId id; const char *name; } owners[] = {
        {ROHR_PHYSICS_REPORT, "ROHR_PHYSICS_REPORT"},
        {ROHR_PHYSICS_SUBSTEP_REPORT, "ROHR_PHYSICS_SUBSTEP_REPORT"},
        {ROHR_PHYSICS_LOADS_REPORT, "ROHR_PHYSICS_LOADS_REPORT"},
        {ROHR_PHYSICS_INTEGRATION_REPORT, "ROHR_PHYSICS_INTEGRATION_REPORT"},
        {ROHR_PHYSICS_CONTACTS_REPORT, "ROHR_PHYSICS_CONTACTS_REPORT"},
        {ROHR_PHYSICS_JOINTS_REPORT, "ROHR_PHYSICS_JOINTS_REPORT"},
        {ROHR_PHYSICS_CONSTRAINTS_REPORT, "ROHR_PHYSICS_CONSTRAINTS_REPORT"},
        {ROHR_GRAPHICS_REPORT, "ROHR_GRAPHICS_REPORT"},
        {ROHR_AUDIO_REPORT, "ROHR_AUDIO_REPORT"},
        {ROHR_INPUT_REPORT, "ROHR_INPUT_REPORT"},
        {ROHR_UI_REPORT, "ROHR_UI_REPORT"},
        {ROHR_ENGINE_REPORT, "ROHR_ENGINE_REPORT"},
        {ROHR_GRAPHICS_CAMERAS_REPORT, "ROHR_GRAPHICS_CAMERAS_REPORT"},
        {ROHR_GRAPHICS_COMMANDS_REPORT, "ROHR_GRAPHICS_COMMANDS_REPORT"},
        {ROHR_GRAPHICS_VIEWPORTS_REPORT, "ROHR_GRAPHICS_VIEWPORTS_REPORT"},
        {ROHR_GRAPHICS_RECORDING_REPORT, "ROHR_GRAPHICS_RECORDING_REPORT"},
        {ROHR_GRAPHICS_PRESENTATION_REPORT, "ROHR_GRAPHICS_PRESENTATION_REPORT"},
        {ROHR_GRAPHICS_ANIMATIONS_REPORT, "ROHR_GRAPHICS_ANIMATIONS_REPORT"},
        {ROHR_AUDIO_COMMANDS_REPORT, "ROHR_AUDIO_COMMANDS_REPORT"},
        {ROHR_AUDIO_SOUNDS_REPORT, "ROHR_AUDIO_SOUNDS_REPORT"},
        {ROHR_AUDIO_MUSIC_REPORT, "ROHR_AUDIO_MUSIC_REPORT"},
        {ROHR_AUDIO_OUTPUT_REPORT, "ROHR_AUDIO_OUTPUT_REPORT"},
        {ROHR_ENGINE_LIFECYCLE_REPORT, "ROHR_ENGINE_LIFECYCLE_REPORT"},
        {ROHR_REPORT, "ROHR_REPORT"},
    };
    static const struct { ReportMetricId id; const char *name; ReportAggregation rule; } metrics[] = {
        {ROHR_REPORT_PROCESSED_COUNT, "ROHR_REPORT_PROCESSED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_SKIPPED_COUNT, "ROHR_REPORT_SKIPPED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_CANDIDATE_COUNT, "ROHR_REPORT_CANDIDATE_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_OVERLAP_COUNT, "ROHR_REPORT_OVERLAP_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_CONTACT_COUNT, "ROHR_REPORT_CONTACT_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_QUARANTINED_COUNT, "ROHR_REPORT_QUARANTINED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT, "ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT, "ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_COLLIDER_COUNT, "ROHR_REPORT_COLLIDER_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_NARROWPHASE_TEST_COUNT, "ROHR_REPORT_NARROWPHASE_TEST_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_BROADPHASE_BUILD_NS, "ROHR_REPORT_BROADPHASE_BUILD_NS", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_BROADPHASE_QUERY_NS, "ROHR_REPORT_BROADPHASE_QUERY_NS", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_NARROWPHASE_NS, "ROHR_REPORT_NARROWPHASE_NS", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_TREE_HEIGHT_PEAK, "ROHR_REPORT_TREE_HEIGHT_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_TREE_NODES_CURRENT, "ROHR_REPORT_TREE_NODES_CURRENT", ROHR_REPORT_AGGREGATION_LATEST},
        {ROHR_REPORT_QUEUED_COMMAND_COUNT, "ROHR_REPORT_QUEUED_COMMAND_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_FRAME_LIMIT_WAIT_NS, "ROHR_REPORT_FRAME_LIMIT_WAIT_NS", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_AUDIO_FRAMES_COUNT, "ROHR_REPORT_AUDIO_FRAMES_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_CANCELLED_COMMAND_COUNT, "ROHR_REPORT_CANCELLED_COMMAND_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT, "ROHR_REPORT_UI_INTERACTION_EVALUATION_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_UI_EVENT_RECEIVED_COUNT, "ROHR_REPORT_UI_EVENT_RECEIVED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_UI_EVENT_DROPPED_COUNT, "ROHR_REPORT_UI_EVENT_DROPPED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK, "ROHR_REPORT_UI_NAVIGATION_ENTRIES_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_TEXT_BYTES_PEAK, "ROHR_REPORT_UI_TEXT_BYTES_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_KEY_EVENT_SLOTS_PEAK, "ROHR_REPORT_UI_KEY_EVENT_SLOTS_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_TEXT_EVENT_SLOTS_PEAK, "ROHR_REPORT_UI_TEXT_EVENT_SLOTS_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_SCROLL_RECORDS_PEAK, "ROHR_REPORT_UI_SCROLL_RECORDS_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_SCROLL_SCOPE_DEPTH_PEAK, "ROHR_REPORT_UI_SCROLL_SCOPE_DEPTH_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK, "ROHR_REPORT_UI_TRANSLATION_SCOPE_DEPTH_PEAK", ROHR_REPORT_AGGREGATION_MAX},
        {ROHR_REPORT_TICKS_ADVANCED_COUNT, "ROHR_REPORT_TICKS_ADVANCED_COUNT", ROHR_REPORT_AGGREGATION_SUM},
        {ROHR_REPORT_EXPIRED_ENTITY_COUNT, "ROHR_REPORT_EXPIRED_ENTITY_COUNT", ROHR_REPORT_AGGREGATION_SUM},
    };
    assert(sizeof(owners) / sizeof(owners[0]) == ROHR_REPORT);
    assert(sizeof(metrics) / sizeof(metrics[0]) == ROHR_REPORT_EXPIRED_ENTITY_COUNT);
    Report r = report_create();
    OK(report_header_set(&r, (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 0}));
    for(size_t i = 0; i < sizeof(owners) / sizeof(owners[0]); ++i)
        OK(report_entry_add(&r, (ReportEntry){.header = {owners[i].id, ROHR_REPORT_COMPLETE, 0}, .invocation_count = 1}));
    for(size_t i = 0; i < sizeof(metrics) / sizeof(metrics[0]); ++i) {
        ReportStatistic s = {.report_id = ROHR_REPORT_CUSTOM_ID_BEGIN, .metric_id = metrics[i].id, .aggregation = metrics[i].rule, .value = 1};
        OK(report_statistic_add(&r, s));
        s.aggregation = s.aggregation == ROHR_REPORT_AGGREGATION_SUM ?
            ROHR_REPORT_AGGREGATION_MAX : ROHR_REPORT_AGGREGATION_SUM;
        FAIL(report_statistic_add(&r, s), ERROR_ENGINE_REPORT_INVALID);
    }
    OK(report_text_write(&r, text_path));
    OK(report_json_write(&r, json_path, false));
    char *text = file_read(text_path);
    yyjson_doc *doc = json_read(json_path);
    yyjson_val *root = yyjson_doc_get_root(doc);
    for(size_t i = 0; i < sizeof(owners) / sizeof(owners[0]); ++i) {
        char readable[128];
        if(owners[i].id == ROHR_REPORT) snprintf(readable, sizeof(readable), "  REPORT (%u)  COMPLETE", (unsigned)ROHR_REPORT);
        else snprintf(readable, sizeof(readable), "  %.*s (%u)  COMPLETE",
            (int)(strlen(owners[i].name) - strlen("ROHR_") - strlen("_REPORT")),
            owners[i].name + strlen("ROHR_"), (unsigned)owners[i].id);
        assert(strstr(text, readable));
        yyjson_val *entry = yyjson_arr_get(get(root, "entries"), i);
        string_is(get(get(get(entry, "header"), "id"), "name"), owners[i].name);
    }
    for(size_t i = 0; i < sizeof(metrics) / sizeof(metrics[0]); ++i) {
        assert(strstr(text, metrics[i].name + strlen("ROHR_REPORT_")));
        yyjson_val *stat = yyjson_arr_get(get(root, "statistics"), i);
        string_is(get(get(stat, "metric_id"), "name"), metrics[i].name);
    }
    free(text); yyjson_doc_free(doc);
}

static void versioned_output(void) {
    Report r = {.header = {ROHR_REPORT, ROHR_REPORT_COMPLETE, 0}, .version = 9};
    for(uint64_t v = 7; v <= 8; ++v) {
        OK(report_entry_add(&r, (ReportEntry){.header = {ROHR_UI_REPORT, ROHR_REPORT_COMPLETE, 1},
            .invocation_count = 1, .group_version = v, .group_root = true}));
        OK(report_statistic_add(&r, (ReportStatistic){.report_id = ROHR_UI_REPORT,
            .metric_id = ROHR_REPORT_PROCESSED_COUNT, .aggregation = ROHR_REPORT_AGGREGATION_SUM,
            .value = v, .group_version = v}));
        ReportDiagnostic d = {.report_id = ROHR_UI_REPORT, .severity = ROHR_REPORT_SEVERITY_FATAL,
            .code = ERROR_ENGINE_STATE_INVALID, .occurrence_count = 1, .group_version = v};
        OK(report_diagnostic_add(&r, d));
    }
    r.included_versions[ROHR_UI_REPORT] = 8;
    OK(report_json_write(&r, json_path, false));
    yyjson_doc *doc = json_read(json_path);
    yyjson_val *root = yyjson_doc_get_root(doc);
    string_is(get(root, "version"), "9");
    assert(yyjson_arr_size(get(root, "diagnostics")) == 2);
    string_is(get(get(root, "first_fatal"), "group_version"), "7");
    yyjson_val *entry = yyjson_arr_get(get(root, "entries"), 1);
    string_is(get(entry, "group_version"), "8");
    assert(yyjson_get_bool(get(entry, "group_root")));
    string_is(get(yyjson_arr_get(get(root, "statistics"), 0), "group_version"), "7");
    string_is(get(yyjson_arr_get(get(root, "included_versions"), 0), "version"), "8");
    yyjson_doc_free(doc);
    OK(report_text_write(&r, text_path));
    char *text = file_read(text_path);
    assert(strstr(text, "Snapshot version: 7") && strstr(text, "Snapshot version: 8"));
    free(text);
    ReportEntry invalid = {.header = {ROHR_UI_REPORT, ROHR_REPORT_NOT_RUN, 0}, .group_root = true};
    FAIL(report_entry_add(&r, invalid), ERROR_ENGINE_REPORT_INVALID);
    Report empty = report_create(); empty.version = 1;
    assert(!report_valid_check(&empty));
}

int main(void) {
#ifdef _WIN32
    assert(_setmode(fileno(stdout), _O_BINARY) != -1);
#endif
    versioned_output();
    builtin_names();
    json_and_ownership();
    empty_and_invalid();
    text_and_history();
    capacities_and_truncation();
#ifdef ROHR_TEST_ALLOCATION_WRAP
    failures();
#endif
    remove(json_path); remove(pretty_path); remove(text_path);
    return 0;
}
