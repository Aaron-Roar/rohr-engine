/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "platform_process.h"
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

bool platform_process_command_check(const char *command) {
    char check_command[256];

    if(command == NULL) {
        return false;
    }

#ifdef _WIN32
    snprintf(
        check_command,
        sizeof(check_command),
        "where %s >nul 2>nul",
        command
    );
#else
    snprintf(
        check_command,
        sizeof(check_command),
        "command -v %s >/dev/null 2>&1",
        command
    );
#endif

    return system(check_command) == 0;
}

FILE *platform_process_open_write(const char *command) {
#ifdef _WIN32
    return _popen(command, "wb");
#else
    return popen(command, "w");
#endif
}

int platform_process_close(FILE *process) {
#ifdef _WIN32
    return _pclose(process);
#else
    return pclose(process);
#endif
}

bool platform_process_working_directory_set(const char *directory) {
    if(directory == NULL || directory[0] == '\0') return false;
#ifdef _WIN32
    return _chdir(directory) == 0;
#else
    return chdir(directory) == 0;
#endif
}
