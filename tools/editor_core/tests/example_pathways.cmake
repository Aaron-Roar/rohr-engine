# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
foreach(required CLI FIXTURE SDK_PREFIX)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Example pathway failed (${result}): ${ARGV}")
    endif()
endfunction()
file(REMOVE_RECURSE "${FIXTURE}")
run("${CLI}" --project "${FIXTURE}" create)
file(READ "${FIXTURE}/src/main.c" original)
set(marker "// ====================")
string(FIND "${original}" "${marker} 1." first)
string(FIND "${original}" "${marker} 2." second)
string(FIND "${original}" "${marker} 3." third)
if(first LESS 0 OR second LESS 0 OR third LESS 0)
    message(FATAL_ERROR "Missing complete example alternatives")
endif()
string(SUBSTRING "${original}" 0 ${first} shared)
# A bounded functional run; production main remains unchanged and user-owned.
string(REPLACE "    for(;;) {" "    for(unsigned frame = 0;; ++frame) {\n        if(frame == 120) return 0;\n        if(frame == 40) rohr_engine_pause();\n        if(frame == 80) rohr_engine_resume();" shared "${shared}")
math(EXPR length "${second} - ${first}")
string(SUBSTRING "${original}" ${first} ${length} project)
math(EXPR length "${third} - ${second}")
string(SUBSTRING "${original}" ${second} ${length} object)
string(SUBSTRING "${original}" ${third} -1 entity)
foreach(pathway project object entity)
    set(body "${${pathway}}")
    if(NOT pathway STREQUAL "project")
        # Keep the divider; mechanically uncomment every following line.
        string(FIND "${body}" "\n" line)
        string(SUBSTRING "${body}" ${line} -1 body)
        string(REPLACE "\n// " "\n" body "${body}")
        string(REPLACE "\n//\n" "\n\n" body "${body}")
    endif()
    file(WRITE "${FIXTURE}/src/${pathway}.c" "${shared}${body}")
endforeach()
file(WRITE "${FIXTURE}/src/report_policy.c" [=[
#define main example_main
#include "project.c"
#undef main

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    rohr_engine_pause();
    if(!rohr_report_check(rohr_engine_update())) return 2;
    Report skipped = rohr_physics_update(rohr_engine_tick_delta_get());
    if(skipped.header.status != ROHR_REPORT_NOT_RUN || rohr_report_check(skipped) ||
            !report_ok(skipped)) return 3;
    /* An unrelated skipped system is not silently treated as success. */
    skipped.header.id = ROHR_GRAPHICS_REPORT_ID;
    if(report_ok(skipped)) return 4;
    skipped.header.id = ROHR_PHYSICS_REPORT_ID;
    skipped.diagnostic_count = ROHR_REPORT_DIAGNOSTIC_CAPACITY + 1;
    if(report_ok(skipped)) return 5;
    Report failed = rohr_report_create();
    if(rohr_error_check(rohr_report_header_set(&failed,
            (ReportHeader){ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_FAILED, 0})) ||
            report_ok(failed)) return 6;
    rohr_engine_stop();
    return 0;
}
]=])
file(WRITE "${FIXTURE}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.20)
project(example_pathways LANGUAGES C)
set(CMAKE_C_STANDARD 11)
find_package(Rohr CONFIG REQUIRED)
foreach(pathway project object entity report_policy)
    add_executable(${pathway} src/${pathway}.c src/generated/project_objects.c)
    target_include_directories(${pathway} PRIVATE src/generated)
    target_link_libraries(${pathway} PRIVATE Rohr::Engine)
    if(NOT MSVC)
        target_compile_options(${pathway} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_custom_command(TARGET ${pathway} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${pathway}>/assets")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pathway}.path" CONTENT "$<TARGET_FILE:${pathway}>")
endforeach()
]=])
file(SHA256 "${FIXTURE}/CMakeLists.txt" build_before)
run("${CLI}" --project "${FIXTURE}" generate-c)
file(READ "${FIXTURE}/src/main.c" regenerated)
file(SHA256 "${FIXTURE}/CMakeLists.txt" build_after)
if(NOT original STREQUAL regenerated OR NOT build_before STREQUAL build_after)
    message(FATAL_ERROR "Regeneration changed example main or custom build file")
endif()
set(configure "${CMAKE_COMMAND}" -S "${FIXTURE}" -B "${FIXTURE}/build"
    "-DCMAKE_PREFIX_PATH=${SDK_PREFIX}" -DCMAKE_BUILD_TYPE=Release)
if(DEFINED TOOLCHAIN)
    list(APPEND configure "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}")
endif()
run(${configure})
run("${CMAKE_COMMAND}" --build "${FIXTURE}/build" --parallel 4)
if(NOT DEFINED TOOLCHAIN)
    file(MAKE_DIRECTORY "${FIXTURE}/outside_source")
    foreach(pathway project object entity report_policy)
        file(READ "${FIXTURE}/build/${pathway}.path" executable)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
            "${executable}" WORKING_DIRECTORY "${FIXTURE}/outside_source" RESULT_VARIABLE result TIMEOUT 30)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "${pathway} example failed: ${result}")
        endif()
    endforeach()
endif()
