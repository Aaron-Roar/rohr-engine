/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int enabled=-1;
static unsigned samples,invalid;
static double max_beam_absolute,max_beam_relative,peak_speed;
static uint64_t hash=UINT64_C(1469598103934665603);
static void finish(void) {
    if(enabled) printf("audit samples=%u invalid=%u beam_absolute=%.9g beam_relative=%.9g peak_speed=%.9g hash=%llu\n",
        samples,invalid,max_beam_absolute,max_beam_relative,peak_speed,(unsigned long long)hash);
}
static bool enabled_check(void) {
    if(enabled<0) { const char *v=getenv("ROHR_COLLISION_AUDIT"); enabled=v&&atoi(v); atexit(finish); }
    return enabled!=0;
}
static void hash_add(const void *p,size_t size) {
    const unsigned char *bytes=p;
    for(size_t i=0;i<size;++i) { hash^=bytes[i]; hash*=UINT64_C(1099511628211); }
}
Report __real_physics_pipeline_constraints_solve(uint32_t iterations);
Report __wrap_physics_pipeline_constraints_solve(uint32_t iterations) {
    Report r=__real_physics_pipeline_constraints_solve(iterations);
    if(!enabled_check()) return r;
    ++samples;
    for(unsigned n=0;n<rohr_entity_alive_count_get();++n) {
        EntityResult e=rohr_entity_alive_at_get(n);
        if(rohr_error_check(e)) { ++invalid; continue; }
        EntityIndexResult result=rohr_entity_index_get(e.result.value);
        if(rohr_error_check(result)) { ++invalid; continue; }
        EntityIndex i=result.result.value;
        if((entity_mask[i]&ROHR_DYNAMIC) && (entity_mask[i]&ROHR_HIT_BOX)) {
            invalid+=!isfinite(positions[i].x)||!isfinite(positions[i].y)||!isfinite(velocities[i].x)||!isfinite(velocities[i].y)||!isfinite(orientations[i]);
            peak_speed=fmax(peak_speed,hypot(velocities[i].x,velocities[i].y));
            hash_add(&positions[i],sizeof(positions[i])); hash_add(&velocities[i],sizeof(velocities[i]));
            hash_add(&orientations[i],sizeof(orientations[i]));
        }
        if(entity_mask[i]&ROHR_SOFT_BODY_BEAM) {
            SoftBodyBeam beam=soft_body_beams[i];
            EntityIndexResult a=rohr_entity_index_get(beam.node_a),b=rohr_entity_index_get(beam.node_b);
            if(rohr_error_check(a)||rohr_error_check(b)) { ++invalid; continue; }
            Position pa=positions[a.result.value],pb=positions[b.result.value];
            double error=fabs(hypot(pa.x-pb.x,pa.y-pb.y)-beam.rest_length);
            max_beam_absolute=fmax(max_beam_absolute,error);
            if(beam.rest_length>0) max_beam_relative=fmax(max_beam_relative,error/beam.rest_length);
        }
    }
    return r;
}
