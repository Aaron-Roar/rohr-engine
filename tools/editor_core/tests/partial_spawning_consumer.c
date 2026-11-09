/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include <assert.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))
static Entity take(EntityResult result) { OK(result); return result.result.value; }
int main(int argc, char **argv) {
    assert(argc == 2);
    bool full = !strcmp(argv[1], "full"), missing = !strcmp(argv[1], "missing");
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    Entity root = ENTITY_INVALID, example = ENTITY_INVALID, tug = ENTITY_INVALID;
    if(full) {
        root = take(rohr_object_create(project));
        example = take(rohr_object_member_get(root, project.example.name));
    } else if(!missing) example = take(rohr_object_create(project.example));
    else tug = take(rohr_object_create(project.tug));
    ViewportObjectBinding binding = {&project.example._description, example};
    ViewportCreateDescription description = {.template=&project.initial_viewport._description,
        .objects=&binding,.object_count=1};
    ViewportIdResult view = rohr_viewport_create(description);
    if(missing) {
        assert(rohr_error_check(view)); /* An explicitly supplied invalid owner still fails. */
        example = take(rohr_object_create(project.example));
        binding.instance = example;
        view = rohr_viewport_create(description);
    }
    OK(view);
    for(unsigned frame=0;frame<3;++frame) {
        Report report = rohr_graphics_viewport_show(view.result.value);
        assert(report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
        assert(report.diagnostic_count == 0);
    }
    OK(rohr_viewport_destroy(view.result.value));
    if(root) OK(rohr_entity_delete(root));
    else OK(rohr_entity_delete(example));
    assert(rohr_error_check(rohr_viewport_create(description))); /* Stale camera owner. */
    if(tug) OK(rohr_entity_delete(tug));
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
