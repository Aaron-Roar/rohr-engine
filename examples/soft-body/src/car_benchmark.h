/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef CAR_BENCHMARK_H
#define CAR_BENCHMARK_H
#include "rohr.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Example-owned measurement storage; shared scene construction stays in main. */
#define CAR_BENCHMARK_FRAMES 3600
typedef struct CarBenchmark {
    bool enabled, rendered, debug, slow_motion;
    unsigned frames, limit;
    double physics[CAR_BENCHMARK_FRAMES], work[CAR_BENCHMARK_FRAMES];
    double render[CAR_BENCHMARK_FRAMES], simulated, integration, gather, solve;
    double maximum_lag;
    uint64_t started, substeps, candidates, contacts;
} CarBenchmark;

static bool car_benchmark_parse(CarBenchmark *b, int argc, char **argv) {
    b->limit = CAR_BENCHMARK_FRAMES;
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--benchmark=headless") == 0) b->enabled = true;
        else if(strcmp(argv[i], "--benchmark=rendered") == 0)
            b->enabled = b->rendered = true;
        else if(strcmp(argv[i], "--debug") == 0) b->debug = true;
        else if(strcmp(argv[i], "--slow-motion") == 0) b->slow_motion = true;
        else if(strncmp(argv[i], "--frames=", 9) == 0) {
            char *end;
            unsigned long count = strtoul(argv[i] + 9, &end, 10);
            if(*end || count == 0 || count > CAR_BENCHMARK_FRAMES) return false;
            b->limit = (unsigned)count;
        } else return false;
    }
    return b->enabled || argc == 1;
}
static void car_benchmark_report_add(CarBenchmark *b, const Report *r) {
    for(size_t i = 0; i < r->statistic_count; ++i) {
        ReportStatistic s = r->statistics[i];
        if(s.report_id == ROHR_PHYSICS_REPORT_ID && s.metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
            b->substeps += s.value;
        if(s.metric_id == ROHR_REPORT_CANDIDATE_COUNT) b->candidates += s.value;
        if(s.metric_id == ROHR_REPORT_CONTACT_COUNT) b->contacts += s.value;
    }
    for(size_t i = 0; i < r->entry_count; ++i) {
        ReportEntry e = r->entries[i];
        if(e.header.id == ROHR_PHYSICS_INTEGRATION_REPORT_ID) b->integration += e.header.total_ms;
        if(e.header.id == ROHR_PHYSICS_CONTACTS_REPORT_ID) b->gather += e.header.total_ms;
        if(e.header.id == ROHR_PHYSICS_CONSTRAINTS_REPORT_ID) b->solve += e.header.total_ms;
    }
}
static int car_benchmark_compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double car_benchmark_summary(const char *label, double *values, unsigned n) {
    double total = 0;
    for(unsigned i = 0; i < n; ++i) total += values[i];
    qsort(values, n, sizeof(*values), car_benchmark_compare);
    double p95 = n ? values[(n * 95 + 99) / 100 - 1] : 0;
    printf("%s_ms median=%.6f p95=%.6f max=%.6f total=%.6f\n", label,
        n ? values[(n - 1) / 2] : 0, p95, n ? values[n - 1] : 0, total);
    return p95;
}
static bool car_benchmark_finish(CarBenchmark *b) {
    double elapsed = (SDL_GetTicksNS() - b->started) / 1e9;
    car_benchmark_summary("physics_cpu", b->physics, b->frames);
    car_benchmark_summary("render_work", b->render, b->frames);
    double p95 = car_benchmark_summary("frame_work", b->work, b->frames);
    bool complete = b->frames == CAR_BENCHMARK_FRAMES;
    bool realtime = complete && b->rendered && !b->debug && !b->slow_motion &&
        p95 <= 16.67 && fabs(b->simulated / elapsed - 1) <= .01;
    printf("benchmark frames=%u complete=%d rendered=%d debug=%d slow_motion=%d "
        "simulated=%.9f elapsed=%.9f fps=%.3f realtime_pass=%d "
        "substeps=%llu candidates=%llu contacts=%llu\n", b->frames, complete,
        b->rendered, b->debug, b->slow_motion, b->simulated, elapsed,
        elapsed > 0 ? b->frames / elapsed : 0, realtime,
        (unsigned long long)b->substeps, (unsigned long long)b->candidates,
        (unsigned long long)b->contacts);
    printf("profile_ms integration=%.3f gather=%.3f solve=%.3f\n",
        b->integration, b->gather, b->solve);
    printf("schedule_lag_ms final=%.3f maximum=%.3f\n",
        1000 * (elapsed - b->frames / 60.0), 1000 * b->maximum_lag);
    return b->frames == b->limit && (!complete || !b->rendered || b->debug || b->slow_motion || realtime);
}
#endif
