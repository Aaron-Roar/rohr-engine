# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
foreach(required CLI PROBE PATHS FIXTURE SDK_PREFIX)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${ARGV}\n${result}\n${output}\n${error}")
    endif()
endfunction()
file(REMOVE_RECURSE "${FIXTURE}")
set(root "${FIXTURE}/project")
run("${CLI}" --project "${root}" create)
file(WRITE "${root}/src/main.c" [=[
#include "rohr.h"
#include "project_objects.h"
#include <assert.h>
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    assert(!rohr_error_check(rohr_directory_working_set(rohr_directory_base_get())));
    Example example = {0};
    assert(!rohr_error_check(example_create(&example, (Position){0})));
    example_destroy(&example);
    rohr_engine_stop();
    return 0;
}
]=])
file(SHA256 "${root}/src/main.c" original_main)
run("${PATHS}" configure "${root}" "${SDK_PREFIX}")
set(root "${root} moved")
file(SHA256 "${root}/code files/main.c" current_main)
if(NOT original_main STREQUAL current_main)
    message(FATAL_ERROR "Path configuration rewrote developer main.c")
endif()
run("${CLI}" --project "${root}" generate-c)
run("${CLI}" --project "${root}" compile)
set(build "${FIXTURE}/shared build")
set(app "${FIXTURE}/output app/demo")
set(scene "${FIXTURE}/output scene/demo")
foreach(executable "${app}" "${scene}")
    if(NOT EXISTS "${executable}")
        message(FATAL_ERROR "Missing configured output ${executable}")
    endif()
endforeach()
foreach(directory "output app" "output scene")
    if(NOT EXISTS "${FIXTURE}/${directory}/assets/example_frame_1.png")
        message(FATAL_ERROR "Runtime assets not staged for ${directory}")
    endif()
endforeach()
file(APPEND "${build}/CMakeCache.txt" "\nC7_CACHE_ONLY:STRING=retained\n")
file(TIMESTAMP "${app}" before "%s")
run("${CMAKE_COMMAND}" -E sleep 1)
run("${PROBE}" project "${root}" app "${SDK_PREFIX}")
run("${PROBE}" project "${root}" scene "${SDK_PREFIX}")
file(TIMESTAMP "${app}" after "%s")
file(READ "${build}/CMakeCache.txt" cache)
if(NOT cache MATCHES "C7_CACHE_ONLY" OR NOT before STREQUAL after)
    message(FATAL_ERROR "Unchanged custom-path build discarded valid cache or recompiled")
endif()
foreach(target app scene)
    run("${CMAKE_COMMAND}" -E env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
        "${PROBE}" launch "${root}" "${target}" "${SDK_PREFIX}")
endforeach()
# Direct manifest edits are shared with CLI. Changed output settings invalidate
# the configured-path record before launching and force a fresh configure.
file(READ "${root}/project.rohr.json" manifest)
string(JSON manifest SET "${manifest}" application_executable "\"../new output/demo\"")
file(WRITE "${root}/project.rohr.json" "${manifest}\n")
run("${CLI}" --project "${root}" generate-c)
execute_process(COMMAND "${PROBE}" resolve "${root}" app "${SDK_PREFIX}" RESULT_VARIABLE blocked
    OUTPUT_QUIET ERROR_QUIET)
if(blocked EQUAL 0)
    message(FATAL_ERROR "Changed output launched stale application")
endif()
run("${CLI}" --project "${root}" compile)
file(READ "${build}/CMakeCache.txt" cache)
if(cache MATCHES "C7_CACHE_ONLY" OR NOT EXISTS "${FIXTURE}/new output/demo")
    message(FATAL_ERROR "Changed output did not reconfigure")
endif()
run("${CMAKE_COMMAND}" -E env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
    "${PROBE}" launch "${root}" app "${SDK_PREFIX}")
