# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(frame_pacing LANGUAGES C)
include("${CMAKE_CURRENT_LIST_DIR}/../../examples/cmake/rohr_example.cmake")
rohr_example_bootstrap()
add_executable(frame_pacing main.c)
target_link_libraries(frame_pacing PRIVATE ${ROHR_ENGINE_TARGET})
if(NOT MSVC)
    target_compile_options(frame_pacing PRIVATE -Wall -Wextra -Wpedantic)
endif()
rohr_stage_example_assets(frame_pacing user-interface)
