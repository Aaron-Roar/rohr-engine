/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <stddef.h>
#include <string.h>

int main(void) {
    Report parent = rohr_report_create();
    Report child = rohr_report_create();
    ReportDiagnostic diagnostic = {
        .report_id = ROHR_REPORT_CUSTOM_ID_BEGIN,
        .severity = ROHR_REPORT_SEVERITY_FATAL,
        .code = ERROR_ENGINE_COMPONENT_MISSING,
        .occurrence_count = 1
    };
    if(rohr_error_code_message_get(ERROR_NONE) == NULL ||
            rohr_error_check(rohr_report_header_set(&parent,
                (ReportHeader){ROHR_PHYSICS_REPORT, ROHR_REPORT_COMPLETE, 5})) ||
            rohr_error_check(rohr_report_header_set(&child,
                (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 1})) ||
            rohr_error_check(rohr_report_diagnostic_detail_set(&diagnostic, "SDK-owned detail")) ||
            rohr_error_check(rohr_report_diagnostic_add(&child, diagnostic)) ||
            rohr_error_check(rohr_report_merge(&parent, &child))) return 1;
    memset(&child, 0, sizeof(child));
    return !rohr_report_valid_check(&parent) || !parent.has_fatal ||
        parent.header.status != ROHR_REPORT_FAILED || parent.header.total_ms != 5 ||
        parent.first_fatal.occurrence_count != 1 || parent.entry_count != 1 ||
        strcmp(parent.first_fatal.detail, "SDK-owned detail") != 0;
}
