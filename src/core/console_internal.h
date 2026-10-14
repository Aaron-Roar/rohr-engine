/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_CONSOLE_INTERNAL_H
#define ROHR_CONSOLE_INTERNAL_H
#include "console.h"
#include "error.h"
#include <stdarg.h>

/** Shared backend; includes the prefix in its capacity and checks stream errors. */
EngineResult console_checked_vwrite(LogSourceType source, const char *fmt, va_list args);
EngineResult console_checked_write(LogSourceType source, const char *fmt, ...);
#endif
