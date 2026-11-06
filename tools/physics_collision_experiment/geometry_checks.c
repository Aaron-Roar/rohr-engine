/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define OK(x) assert(!rohr_error_check(x))
int main(void) {
    OK(rohr_engine_start());
    EntityResult first=rohr_entity_add(),second=rohr_entity_add(); OK(first); OK(second);
    OK(rohr_physics_hitbox_set(first.result.value,rohr_math_square_create(1,1)));
    OK(rohr_physics_hitbox_set(second.result.value,rohr_math_square_create(.2f,2)));
    EntityIndexResult ar=rohr_entity_index_get(first.result.value),br=rohr_entity_index_get(second.result.value); OK(ar); OK(br);
    EntityIndex a=ar.result.value,b=br.result.value;
    ExperimentPose start={{-2,0},0},end={{2,0},0},wall={{0,0},0};
    double t,reverse; ExperimentContact c,d; unsigned iterations; bool limited;
    assert(experiment_cast_check(a,b,start,end,wall,wall,&t,&c,&iterations,&limited));
    assert(!limited&&fabs(t-.35)<.0001&&c.normal.x>.999f);
    assert(experiment_cast_check(b,a,wall,wall,start,end,&reverse,&d,&iterations,&limited));
    assert(!limited&&fabs(t-reverse)<.000001&&d.normal.x<-.999f);
    assert(!experiment_cast_check(a,b,(ExperimentPose){{-3,.6f},0},
        (ExperimentPose){{2,5.6f},0},wall,wall,&t,&c,&iterations,&limited));
    assert(!limited);
    assert(experiment_cast_check(a,b,wall,wall,wall,wall,&t,&c,&iterations,&limited)&&t==0);
    OK(rohr_physics_hitbox_set(first.result.value,rohr_math_square_create(.2f,.2f)));
    OK(rohr_physics_hitbox_set(second.result.value,rohr_math_square_create(8,.2f)));
    ExperimentPose target={{2,2},0},turned={{0,0},-90};
    assert(experiment_cast_check(a,b,target,target,wall,turned,&t,&c,&iterations,&limited));
    assert(!limited&&t>.3&&t<.6);
    rohr_engine_stop();
    puts("PASS convex cast geometry: translation, symmetry, miss, initial overlap and rotation");
    return 0;
}
