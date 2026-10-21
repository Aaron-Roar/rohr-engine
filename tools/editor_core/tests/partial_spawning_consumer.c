/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_objects.h"
#include "project_viewports.h"
#include <assert.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))
int main(int argc, char **argv) {
    assert(argc == 2);
    bool full = !strcmp(argv[1], "full"), missing = !strcmp(argv[1], "missing");
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    ProjectObjects instances = {0}; ProjectViewports presentation = {0};
    if(full) OK(project_objects_create_all(&instances));
    else if(!missing) OK(starter_create(&instances.starter, (Position){0}));
    else OK(tug_create(&instances.tug, (Position){0}));
    OK(project_viewports_create(&presentation, &instances));
    assert(presentation.screen_count == 1);
    for(unsigned frame = 0; frame < 3; ++frame) {
        Report report = rohr_graphics_show();
        assert(report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
        assert(report.diagnostic_count == (missing ? 1u : 0u));
        if(missing) {
            assert(report.diagnostics[0].report_id == ROHR_GRAPHICS_VIEWPORTS_REPORT_ID);
            assert(report.diagnostics[0].source.id == presentation.screens[0]);
            assert(report.diagnostics[0].target.kind == ROHR_REPORT_OBJECT_CAMERA);
        }
    }
    if(missing) {
        OK(starter_create(&instances.starter, (Position){0}));
        /* Generated callbacks are installed when presentation is created. Recreate
         * presentation after adding the missing authored camera owner. */
        project_viewports_destroy(&presentation);
        OK(project_viewports_create(&presentation, &instances));
        Report report = rohr_graphics_show();
        assert(!report.has_fatal && report.diagnostic_count == 0);
    }
    project_viewports_destroy(&presentation);
    if(!full && !missing) {
        /* Nonzero stale entities must not be mistaken for intentionally absent ones. */
        instances.tug.box_1 = UINT32_MAX;
        assert(rohr_error_check(project_viewports_create(&presentation, &instances)));
        instances.tug.box_1 = ENTITY_INVALID;
    }
    project_objects_destroy_all(&instances);
    rohr_engine_stop();
    return 0;
}
