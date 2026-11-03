# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(physics_measurement LANGUAGES C)
get_filename_component(ROHR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(ROHR_MEASUREMENT_SOURCE_DIR "${ROHR_SOURCE_DIR}" CACHE PATH "Source matching the measured installed SDK")
set(REPO "${ROHR_MEASUREMENT_SOURCE_DIR}")
include("${REPO}/examples/cmake/rohr_example.cmake")
rohr_example_bootstrap()
enable_testing()

# Diagnostic consumers require GNU-compatible --wrap (Linux and MinGW).
# The installed engine archive is neither edited nor recompiled.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/profile_functions.inc" functions REGEX "^(VOID|VALUE|DURATION)\\(")
set(wrapped physics_update physics_dt_update system_physics_update physics_pipeline_update physics_pipeline_substep physics_substep_observer_set physics_report_counter_add)
foreach(line IN LISTS functions)
    string(REGEX REPLACE "^VALUE\\([^,]+, " "VOID(" line "${line}")
    string(REGEX REPLACE "^(VOID|DURATION)\\(([a-z_]+),.*" "\\2" fn "${line}")
    list(APPEND wrapped "${fn}")
endforeach()
function(measurement_target target)
    target_sources(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/profile.c")
    target_include_directories(${target} PRIVATE "${REPO}/src" "${REPO}/tests" "${REPO}/examples/extreme_mass_drop/src")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    target_link_libraries(${target} PRIVATE ${ROHR_ENGINE_TARGET})
    foreach(fn IN LISTS wrapped)
        target_link_options(${target} PRIVATE "LINKER:--wrap=${fn}")
    endforeach()
endfunction()

add_executable(car_measurement "${REPO}/examples/soft-body/src/main.c"
    "${REPO}/examples/soft-body/src/car_controls.c"
    "${REPO}/examples/soft-body/src/generated/project_objects.c")
target_include_directories(car_measurement PRIVATE "${REPO}/examples/soft-body/src/generated")
rohr_add_example_runtime(car_measurement)
measurement_target(car_measurement)
rohr_stage_example_assets(car_measurement soft-body)
add_executable(pit_measurement "${REPO}/tests/particle_pit.c")
measurement_target(pit_measurement)
rohr_stage_example_assets(pit_measurement soft-body)

# Unwrapped playback uses the same scene and installed SDK, without diagnostic
# timers, observers, state scans or per-update CSV output.
add_executable(car_playback "${REPO}/examples/soft-body/src/main.c"
    "${REPO}/examples/soft-body/src/car_controls.c"
    "${REPO}/examples/soft-body/src/generated/project_objects.c")
target_include_directories(car_playback PRIVATE "${REPO}/examples/soft-body/src/generated")
rohr_add_example_runtime(car_playback)
target_link_libraries(car_playback PRIVATE ${ROHR_ENGINE_TARGET})
rohr_stage_example_assets(car_playback soft-body)
add_executable(pit_playback "${REPO}/tests/particle_pit.c")
target_link_libraries(pit_playback PRIVATE ${ROHR_ENGINE_TARGET})
rohr_stage_example_assets(pit_playback soft-body)

foreach(fixture extreme_mass_drop thin_barriers persistent_stack pin_stability weld_stability spring_stability soft_body_stability)
    add_executable(${fixture}_test "${REPO}/tests/${fixture}.c")
    measurement_target(${fixture}_test)
endforeach()
target_sources(extreme_mass_drop_test PRIVATE "${REPO}/examples/extreme_mass_drop/src/drop_fixture.c")
foreach(regression beam_motion particle_sweep physics_reports physics_recovery collision_response soft_body beam_contacts constraint_solver force_preparation contact_gathering joint_gathering joints shared_sources soft_body_frame)
    add_executable(${regression}_test "${REPO}/tests/${regression}.c")
    measurement_target(${regression}_test)
    if(regression STREQUAL "contact_gathering")
        target_compile_definitions(${regression}_test PRIVATE ROHR_TEST_CONTACT_APPEND_WRAP)
        target_link_options(${regression}_test PRIVATE "LINKER:--wrap=contact_constraint_list_append")
    endif()
    add_test(NAME ${regression} COMMAND ${regression}_test)
endforeach()
add_executable(profile_clock_test profile_clock_test.c)
target_compile_options(profile_clock_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
add_test(NAME profile_clock COMMAND profile_clock_test)
add_executable(pit_completion_test pit_completion_test.c)
measurement_target(pit_completion_test)
add_test(NAME pit_completion COMMAND pit_completion_test)
set_tests_properties(pit_completion PROPERTIES PASS_REGULAR_EXPRESSION
    "reached=0 reason=graphics_failed measurements=partial_run")
