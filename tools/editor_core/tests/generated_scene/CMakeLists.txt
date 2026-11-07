# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(rohr_generated_scene LANGUAGES C)
find_package(Rohr CONFIG REQUIRED)
set(SCENE_PROJECT_DIR "" CACHE PATH "New project created by rohr-cli")
if(NOT EXISTS "${SCENE_PROJECT_DIR}/src/main.c")
    message(FATAL_ERROR "Create a default project with rohr-cli and set SCENE_PROJECT_DIR")
endif()
add_executable(generated_scene probe.c
    "${SCENE_PROJECT_DIR}/src/generated/project_objects.c")
target_include_directories(generated_scene PRIVATE "${SCENE_PROJECT_DIR}/src"
    "${SCENE_PROJECT_DIR}/src/generated")
target_link_libraries(generated_scene PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(generated_scene PRIVATE /UNDEBUG)
else()
    target_compile_options(generated_scene PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
add_custom_command(TARGET generated_scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SCENE_PROJECT_DIR}/assets"
        "$<TARGET_FILE_DIR:generated_scene>/assets")
enable_testing()
add_test(NAME generated_scene COMMAND generated_scene)
set_tests_properties(generated_scene PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
