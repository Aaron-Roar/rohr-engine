# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(rohr_center_of_mass LANGUAGES C)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/rohr_example.cmake)
rohr_example_bootstrap()
add_executable(center_of_mass src/main.c
    src/generated/project_objects.c src/generated/project_viewports.c)
target_include_directories(center_of_mass PRIVATE src/generated)
target_link_libraries(center_of_mass PRIVATE ${ROHR_ENGINE_TARGET})
rohr_stage_example_assets(center_of_mass center_of_mass)
