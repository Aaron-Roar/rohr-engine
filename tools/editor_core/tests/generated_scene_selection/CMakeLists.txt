# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(rohr_scene_selection LANGUAGES C)
find_package(Rohr CONFIG REQUIRED)
set(SCENE_PROJECT_DIR "" CACHE PATH "Generated simulation project")
add_executable(scene_selection probe.c
    "${SCENE_PROJECT_DIR}/src/generated/scene/project_objects.c")
target_include_directories(scene_selection PRIVATE "${SCENE_PROJECT_DIR}/src/generated"
    "${SCENE_PROJECT_DIR}/src/generated/scene")
target_link_libraries(scene_selection PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(scene_selection PRIVATE /UNDEBUG)
else()
    target_compile_options(scene_selection PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
add_custom_command(TARGET scene_selection POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SCENE_PROJECT_DIR}/assets"
        "$<TARGET_FILE_DIR:scene_selection>/assets")
