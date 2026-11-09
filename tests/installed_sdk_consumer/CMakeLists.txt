# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.20)

project(rohr_installed_sdk_consumer LANGUAGES C)
include(CTest)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Rohr CONFIG REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckPIESupported)
    check_pie_supported(LANGUAGES C)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(object_fixture ${CMAKE_CURRENT_SOURCE_DIR}/../../examples/center_of_mass/src/generated)
include(creation_compile_contract.cmake)
add_executable(rohr_creation_dispatch creation_dispatch.c ${object_fixture}/project_objects.c)
target_include_directories(rohr_creation_dispatch PRIVATE ${object_fixture})
target_link_libraries(rohr_creation_dispatch PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_creation_dispatch PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_creation_dispatch PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
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

add_executable(rohr_asset_descriptions asset_descriptions.c)
target_link_libraries(rohr_asset_descriptions PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_asset_descriptions PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_asset_descriptions PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_audio_descriptions audio_descriptions.c)
target_link_libraries(rohr_audio_descriptions PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_audio_descriptions PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_audio_descriptions PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_input_descriptions ../input_actions.c)
target_link_libraries(rohr_input_descriptions PRIVATE Rohr::Engine)
if(NOT MSVC)
    target_compile_options(rohr_input_descriptions PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(rohr_camera_descriptions ../camera_descriptions.c)
target_link_libraries(rohr_camera_descriptions PRIVATE Rohr::Engine)
if(NOT MSVC)
    target_compile_options(rohr_camera_descriptions PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(rohr_camera_descriptions PRIVATE ROHR_TEST_CAMERA_WRAP)
    target_link_options(rohr_camera_descriptions PRIVATE
        -Wl,--wrap=SDL_GetPerformanceCounter -Wl,--wrap=SDL_GetPerformanceFrequency
        -Wl,--wrap=SDL_SetRenderLogicalPresentation)
endif()

add_executable(rohr_viewport_descriptions ../viewport_descriptions.c)
target_link_libraries(rohr_viewport_descriptions PRIVATE Rohr::Engine)
if(NOT MSVC)
    target_compile_options(rohr_viewport_descriptions PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(rohr_viewport_descriptions PRIVATE ROHR_TEST_VIEWPORT_WRAP)
    target_link_options(rohr_viewport_descriptions PRIVATE
        -Wl,--wrap=SDL_CreateTexture -Wl,--wrap=SDL_GetMouseState)
endif()

add_executable(rohr_automatic_viewports ../automatic_viewports.c
    ../../examples/cmake/example_viewport.c)
target_include_directories(rohr_automatic_viewports PRIVATE ../../examples/cmake)
target_link_libraries(rohr_automatic_viewports PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_automatic_viewports PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_automatic_viewports PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_generated_workflow generated_workflow.c
    ${object_fixture}/project_objects.c ../../examples/cmake/example_viewport.c)
target_include_directories(rohr_generated_workflow PRIVATE ${object_fixture} ../../examples/cmake)
target_link_libraries(rohr_generated_workflow PRIVATE Rohr::Engine)
if(MSVC)
    target_compile_options(rohr_generated_workflow PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_generated_workflow PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_executable(rohr_project_creation ../project_creation.c)
target_link_libraries(rohr_project_creation PRIVATE Rohr::Engine)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(rohr_project_creation PRIVATE ROHR_TEST_ALLOCATION_WRAP)
    target_link_options(rohr_project_creation PRIVATE
        -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
endif()
if(MSVC)
    target_compile_options(rohr_project_creation PRIVATE /UNDEBUG)
else()
    target_compile_options(rohr_project_creation PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
endif()

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    foreach(consumer
            rohr_installed_sdk_consumer rohr_object_descriptions rohr_creation_dispatch
            rohr_system_reports rohr_entity_info rohr_entity_info_adversarial
            rohr_forces rohr_runtime_persistence rohr_asset_descriptions
            rohr_audio_descriptions rohr_input_descriptions
            rohr_camera_descriptions rohr_viewport_descriptions rohr_automatic_viewports
            rohr_generated_workflow rohr_project_creation)
        # Each consumer owns its temporary fixtures and runs outside the source tree.
        set(runtime_directory "${CMAKE_CURRENT_BINARY_DIR}/test_work/${consumer}")
        file(MAKE_DIRECTORY "${runtime_directory}")
        add_test(NAME ${consumer} COMMAND ${consumer})
        set_tests_properties(${consumer} PROPERTIES
            WORKING_DIRECTORY "${runtime_directory}"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
            TIMEOUT 30)
    endforeach()
    # This consumer renders two frames in each of 1,000 complete object cycles.
    set_tests_properties(rohr_generated_workflow rohr_project_creation PROPERTIES TIMEOUT 180)
endif()
