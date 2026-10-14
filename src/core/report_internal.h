/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef REPORT_INTERNAL_H
#define REPORT_INTERNAL_H

#include "report.h"

/* Engine-owned collection only. The caller exclusively owns a valid report
 * with a non-NONE ID and supplies a valid diagnostic. Skips whole-report
 * revalidation so callback collection scans only the bounded diagnostic list.
 * Public report_diagnostic_add remains fully checked. */
void report_diagnostic_collect_internal(Report *report, ReportDiagnostic diagnostic);
/* Completed engine-owned values only. Does not allocate or call other systems.
 * A nonzero version is already published (e.g. retained audio shutdown). */
void report_publish_internal(Report *report);

#endif
