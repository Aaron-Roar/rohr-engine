# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

foreach(required_variable
        ROHR_ROOT
        ROHR_EXAMPLE_BUILD_ROOT
        ROHR_EXAMPLE_OUTPUT_DIRECTORY
        ROHR_SDK_PREFIX)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

set(example_directories
    audio
    view-port
    flies-in-pit
    flies-around-ball
    fly-to-finish
    game-state
    user-interface
    joints
    soft-body
    pong
    player_controller)

file(MAKE_DIRECTORY "${ROHR_EXAMPLE_OUTPUT_DIRECTORY}")

foreach(example_directory IN LISTS example_directories)
    set(source_directory "${ROHR_ROOT}/examples/${example_directory}")
    set(binary_directory "${ROHR_EXAMPLE_BUILD_ROOT}/${example_directory}")

    set(configure_command
        "${CMAKE_COMMAND}"
        -S "${source_directory}"
        -B "${binary_directory}"
        "-DCMAKE_PREFIX_PATH=${ROHR_SDK_PREFIX}"
        "-DROHR_EXAMPLE_OUTPUT_DIRECTORY=${ROHR_EXAMPLE_OUTPUT_DIRECTORY}")
    if(DEFINED ROHR_BUILD_TYPE AND NOT "${ROHR_BUILD_TYPE}" STREQUAL "")
        list(APPEND configure_command "-DCMAKE_BUILD_TYPE=${ROHR_BUILD_TYPE}")
    endif()
    execute_process(COMMAND ${configure_command} RESULT_VARIABLE configure_result)
    if(NOT configure_result EQUAL 0)
        message(FATAL_ERROR
            "Could not configure standalone example: ${example_directory}")
    endif()

    set(build_command
        "${CMAKE_COMMAND}" --build "${binary_directory}" --parallel)
    if(DEFINED ROHR_BUILD_CONFIG AND NOT "${ROHR_BUILD_CONFIG}" STREQUAL "")
        list(APPEND build_command --config "${ROHR_BUILD_CONFIG}")
    endif()
    execute_process(COMMAND ${build_command} RESULT_VARIABLE build_result)
    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR
            "Could not build standalone example: ${example_directory}")
    endif()
endforeach()
