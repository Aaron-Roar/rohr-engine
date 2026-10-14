# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.20)

project(rohr_installed_sdk_consumer LANGUAGES C)

find_package(Rohr CONFIG REQUIRED)

add_executable(rohr_installed_sdk_consumer main.c)
target_link_libraries(rohr_installed_sdk_consumer PRIVATE Rohr::Engine)

add_executable(rohr_system_reports ../system_reports.c)
target_link_libraries(rohr_system_reports PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_system_reports PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_system_reports PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
