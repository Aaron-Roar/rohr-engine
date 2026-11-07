/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Link-only presentation instrumentation; no physics wrappers or timed I/O.
 */
#include "rohr.h"
#include <stdio.h>
#include <time.h>

typedef struct Sample {
    double interval, work, cpu, backend, wait, whole;
} Sample;
static Sample samples[7202];
static unsigned count, depth, presents, failures;
static Uint64 launched, started, previous, presented, backend_ns;
static clock_t cpu_start, cpu_end;
static double startup;
static void begin(void) {
    started=SDL_GetTicksNS(); cpu_start=clock(); cpu_end=cpu_start;
    presents=0; backend_ns=0;
    if(!count) startup=(started-launched)/1e6;
}
static void end(Report report) {
    Uint64 end=SDL_GetTicksNS(), wait=0;
    for(size_t i=0;i<report.statistic_count;++i)
        if(report.statistics[i].report_id==ROHR_GRAPHICS_PRESENTATION_REPORT_ID &&
           report.statistics[i].metric_id==ROHR_REPORT_FRAME_LIMIT_WAIT_NS) wait+=report.statistics[i].value;
    if(presents!=1 || report.header.status==ROHR_REPORT_FAILED || report.diagnostic_count) ++failures;
    if(count<sizeof(samples)/sizeof(*samples)) samples[count++]=(Sample){
        previous ? (presented-previous)/1e6 : 0,
        (double)(end-started)/1e6-wait/1e6-backend_ns/1e6,
        (double)(cpu_end-cpu_start)*1000/CLOCKS_PER_SEC,
        backend_ns/1e6,wait/1e6,(end-started)/1e6};
    else ++failures;
    previous=presented;
}
bool __real_SDL_RenderPresent(SDL_Renderer *renderer);
bool __wrap_SDL_RenderPresent(SDL_Renderer *renderer) {
    Uint64 start=SDL_GetTicksNS();
    if(depth) { cpu_end=clock(); presented=start; ++presents; }
    bool result=__real_SDL_RenderPresent(renderer);
    if(depth) backend_ns+=SDL_GetTicksNS()-start;
    return result;
}
Report __real_rohr_graphics_show(void);
Report __wrap_rohr_graphics_show(void) {
    if(depth++==0) begin();
    Report r=__real_rohr_graphics_show();
    if(--depth==0) end(r);
    return r;
}
Report __real_graphics_viewport_show(ViewportId viewport);
Report __wrap_graphics_viewport_show(ViewportId viewport) {
    if(depth++==0) begin();
    Report r=__real_graphics_viewport_show(viewport);
    if(--depth==0) end(r);
    return r;
}
Report __real_graphics_viewports_show(const ViewportId *views,size_t amount);
Report __wrap_graphics_viewports_show(const ViewportId *views,size_t amount) {
    if(depth++==0) begin();
    Report r=__real_graphics_viewports_show(views,amount);
    if(--depth==0) end(r);
    return r;
}
int __real_main(int argc,char **argv);
int __wrap_main(int argc,char **argv) {
    launched=SDL_GetTicksNS();
    int result=__real_main(argc,argv);
    printf("render_samples count=%u failures=%u startup_ms=%.6f\n",count,failures,startup);
    puts("sample,index,present_interval_ms,render_work_ms,render_cpu_before_present_ms,backend_present_ms,software_wait_ms,show_ms");
    for(unsigned i=0;i<count;++i) {
        Sample s=samples[i];
        printf("sample,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",i,s.interval,s.work,s.cpu,s.backend,s.wait,s.whole);
    }
    return result ? result : failures ? 1 : 0;
}
