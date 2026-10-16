# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.20)

project(rohr_installed_sdk_consumer LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Rohr CONFIG REQUIRED)

set(object_fixture ${CMAKE_CURRENT_SOURCE_DIR}/../../examples/center_of_mass/src/generated)
add_executable(rohr_object_descriptions object_descriptions.c ${object_fixture}/project_objects.c)
target_include_directories(rohr_object_descriptions PRIVATE ${object_fixture})
target_link_libraries(rohr_object_descriptions PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_object_descriptions PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_object_descriptions PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_installed_sdk_consumer main.c)
target_link_libraries(rohr_installed_sdk_consumer PRIVATE Rohr::Engine)

add_executable(rohr_system_reports ../system_reports.c)
target_link_libraries(rohr_system_reports PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_system_reports PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_system_reports PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_entity_info ../entity_info.c)
target_link_libraries(rohr_entity_info PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_entity_info PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_entity_info PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_entity_info_adversarial ../entity_info_adversarial.c)
target_link_libraries(rohr_entity_info_adversarial PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_entity_info_adversarial PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_entity_info_adversarial PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_forces ../forces.c)
target_link_libraries(rohr_forces PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_forces PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_forces PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_runtime_persistence runtime_persistence.c)
target_link_libraries(rohr_runtime_persistence PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_runtime_persistence PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_runtime_persistence PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
