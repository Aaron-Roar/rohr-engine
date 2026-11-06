/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define OK(x) assert(!rohr_error_check(x))
static int compare(const void *a,const void *b) { double x=*(const double *)a,y=*(const double *)b; return (x>y)-(x<y); }
int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    OK(rohr_game_state_file_load("assets/flies-in-pit/game.json"));
    double samples[1440],total=0; unsigned diagnostics=0;
    for(unsigned tick=0;tick<1440;++tick) {
        clock_t start=clock(); Report r=rohr_system_physics_update(1.0/120);
        samples[tick]=1000.0*(clock()-start)/CLOCKS_PER_SEC; total+=samples[tick];
        if(r.has_fatal||r.header.status==ROHR_REPORT_FAILED) { printf("flies complete=0 tick=%u\n",tick); return 2; }
        diagnostics+=r.diagnostic_count;
    }
    qsort(samples,1440,sizeof(double),compare);
    printf("flies complete=1 ticks=1440 simulated=12 cpu_ms=%.6f average_tick_ms=%.6f p95_tick_ms=%.6f diagnostics=%u\n",total,total/1440,samples[1367],diagnostics);
    rohr_graphics_stop(); rohr_engine_stop();
    return diagnostics?1:0;
}
