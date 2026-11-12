# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

add_library(rohr_editor_core STATIC
    src/editor_collision_migration.c
    src/editor_object_catalog.c
    src/editor_scene_session.c
    src/editor_build.c
    src/editor_workspace_paths.c
    src/editor_build_process.c
    ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_workspace.c
    src/editor_force_geometry.c
    src/editor_mass_properties.c
    src/editor_source_commands.c
    src/editor_diagnostics.c
    src/editor_command.c
    src/editor_auto_shape.c
    src/editor_cli_selectors.c
    src/editor_cli_standard.c
    src/editor_document.c
    src/editor_config.c
    src/editor_object_commands.c
)

target_compile_definitions(rohr_editor_core PRIVATE
    ROHR_DEVELOPMENT_SHARE_DIR="${PROJECT_SOURCE_DIR}/config"
    ROHR_DEVELOPMENT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

target_include_directories(rohr_editor_core
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PUBLIC
        include
        ${PROJECT_SOURCE_DIR}/tools/editor/src
        ${PROJECT_SOURCE_DIR}/lib
)
target_link_libraries(rohr_editor_core PUBLIC rohr_engine PRIVATE rohr_lua)
if(ROHR_ENABLE_SANITIZERS)
    target_compile_options(rohr_editor_core PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
endif()

if(ROHR_BUILD_TESTS)
    add_executable(editor_numeric_locks_test
        ${PROJECT_SOURCE_DIR}/tools/editor/tests/editor_numeric_locks.c
        ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_history.c)
    add_test(NAME editor_numeric_locks COMMAND editor_numeric_locks_test)
    add_executable(editor_collision_migration_test tests/editor_collision_migration.c)
    add_test(NAME editor_collision_migration COMMAND editor_collision_migration_test)
    set_tests_properties(editor_collision_migration PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(UNIX AND NOT APPLE)
        target_compile_definitions(editor_collision_migration_test PRIVATE ROHR_TEST_RENAME_WRAP)
        target_link_options(editor_collision_migration_test PRIVATE -Wl,--wrap=SDL_RenamePath)
    endif()
    add_executable(editor_configuration_parity_test tests/editor_configuration_parity.c
        ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_history.c)
    add_executable(editor_construction_parity_test tests/editor_construction_parity.c)
    add_executable(editor_incomplete_joints_test tests/editor_incomplete_joints.c)
    add_executable(editor_interaction_fixture tests/editor_interaction_fixture.c)
    add_executable(scene_child_test tests/scene_child.c)
    add_executable(editor_scene_session_test tests/editor_scene_session.c)
    add_executable(editor_build_test tests/editor_build.c)
    add_executable(editor_workspace_paths_test tests/editor_workspace_paths.c)
    add_test(NAME editor_workspace_paths COMMAND editor_workspace_paths_test "${CMAKE_CURRENT_BINARY_DIR}/path fixtures")
    add_executable(editor_scene_generation_test tests/editor_scene_generation.c)
    add_executable(editor_animation_attachments_test tests/editor_animation_attachments.c)
    foreach(scene_target editor_animation_attachments_test editor_numeric_locks_test editor_collision_migration_test editor_workspace_paths_test scene_child_test editor_scene_session_test editor_build_test editor_scene_generation_test editor_configuration_parity_test editor_construction_parity_test editor_incomplete_joints_test editor_interaction_fixture)
        target_include_directories(${scene_target} PRIVATE ${PROJECT_SOURCE_DIR}/src)
        target_link_libraries(${scene_target} PRIVATE rohr_editor_core)
        if(NOT MSVC)
            target_compile_options(${scene_target} PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
        else()
            target_compile_options(${scene_target} PRIVATE /UNDEBUG)
        endif()
        if(ROHR_ENABLE_SANITIZERS)
            target_compile_options(${scene_target} PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
            target_link_options(${scene_target} PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
        endif()
        set_target_properties(${scene_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    endforeach()
    add_test(NAME editor_construction_parity COMMAND editor_construction_parity_test)
    add_test(NAME editor_incomplete_joints COMMAND editor_incomplete_joints_test)
    set_tests_properties(editor_incomplete_joints PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    set_tests_properties(editor_construction_parity PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME editor_configuration_parity COMMAND editor_configuration_parity_test)
    set_tests_properties(editor_configuration_parity PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_BUILD_SDK_CONSUMER_TESTS AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME editor_animation_attachments_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_animation_attachments_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/animation attachments project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/animation_attachments_generation.cmake")
        set_tests_properties(editor_animation_attachments_generated PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
        if(ROHR_BUILD_CLI)
            add_test(NAME editor_example_pathways COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/example pathways"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/example_pathways.cmake")
            set_tests_properties(editor_example_pathways PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
            add_test(NAME editor_collision_migration_generated COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli>
                -DTEST_EXECUTABLE=$<TARGET_FILE:editor_collision_migration_test>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/collision migration fixture"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/collision_migration_generation.cmake")
            set_tests_properties(editor_collision_migration_generated PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
            add_test(NAME editor_workspace_paths_build COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli> -DPROBE=$<TARGET_FILE:editor_build_test>
                -DPATHS=$<TARGET_FILE:editor_workspace_paths_test>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/configured path fixtures"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/workspace_paths.cmake")
            set_tests_properties(editor_workspace_paths_build PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
            add_test(NAME editor_build_relocation COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli> -DPROBE=$<TARGET_FILE:editor_build_test>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/relocation fixtures"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/build_relocation.cmake")
            set_tests_properties(editor_build_relocation PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
            add_test(NAME editor_build_scaffolding COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli> -DPROBE=$<TARGET_FILE:editor_build_test>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/scaffolding fixtures"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/build_scaffolding.cmake")
            set_tests_properties(editor_build_scaffolding PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
            add_test(NAME editor_partial_spawning COMMAND ${CMAKE_COMMAND}
                -DCLI=$<TARGET_FILE:rohr_editor_cli> -DPROBE=$<TARGET_FILE:editor_build_test>
                "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/partial spawning fixture"
                "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/partial_spawning.cmake")
            set_tests_properties(editor_partial_spawning PROPERTIES LABELS "sdk;linux" TIMEOUT 180)
        endif()
        add_test(NAME editor_construction_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_construction_parity_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/construction generated project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/construction_generation.cmake")
        set_tests_properties(editor_construction_generated PROPERTIES LABELS "sdk;linux")
        add_test(NAME editor_incomplete_joints_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_incomplete_joints_test>
            -DPROBE=$<TARGET_FILE:editor_build_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/incomplete joints generated project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/incomplete_joints_generation.cmake")
        set_tests_properties(editor_incomplete_joints_generated PROPERTIES LABELS "sdk;linux")
        add_test(NAME editor_configuration_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_configuration_parity_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/configuration generated project"
            "-DFONT=${PROJECT_SOURCE_DIR}/examples/user-interface/assets/jetbrains_mono_bold_italic.ttf"
            "-DCONSUMER_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/configuration_generated_consumer.c"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/configuration_generation.cmake")
        set_tests_properties(editor_configuration_generated PROPERTIES LABELS "sdk;linux")
    endif()
    add_test(NAME editor_scene_generation COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:editor_scene_generation_test>
        "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/scene generation project"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/scene_generation.cmake)
    add_test(NAME editor_scene_session COMMAND editor_scene_session_test $<TARGET_FILE:scene_child_test>)
    set_tests_properties(editor_scene_session PROPERTIES TIMEOUT 60
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME editor_build COMMAND editor_build_test $<TARGET_FILE:editor_build_test>
        $<TARGET_FILE:scene_child_test> "${CMAKE_CURRENT_BINARY_DIR}/build test project")
    set_tests_properties(editor_build PROPERTIES TIMEOUT 40
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_sources_test tests/editor_sources.c ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_history.c)
    target_include_directories(editor_sources_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_sources_test PRIVATE rohr_editor_core)
    if(MSVC)
        target_compile_options(editor_sources_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(editor_sources_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(UNIX AND NOT APPLE)
        target_compile_definitions(editor_sources_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(editor_sources_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_sources_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(editor_sources_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    set_target_properties(editor_sources_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_sources COMMAND editor_sources_test)
    set_tests_properties(editor_sources PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    set(COM_EXAMPLE_SOURCE ${PROJECT_SOURCE_DIR}/examples/center_of_mass)
    add_executable(center_of_mass_example_test tests/center_of_mass_example.c
        ${COM_EXAMPLE_SOURCE}/src/generated/project_objects.c)
    target_include_directories(center_of_mass_example_test PRIVATE
        ${COM_EXAMPLE_SOURCE}/src/generated)
    target_compile_definitions(center_of_mass_example_test PRIVATE
        COM_EXAMPLE_SOURCE="${COM_EXAMPLE_SOURCE}")
    target_link_libraries(center_of_mass_example_test PRIVATE rohr_editor_core)
    set_target_properties(center_of_mass_example_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME center_of_mass_example COMMAND center_of_mass_example_test)
    set_tests_properties(center_of_mass_example PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_mass_properties_test tests/editor_mass_properties.c)
    target_link_libraries(editor_mass_properties_test PRIVATE rohr_editor_core)
    set_target_properties(editor_mass_properties_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_mass_properties COMMAND editor_mass_properties_test)
    set_tests_properties(editor_mass_properties PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(center_of_mass_persistence_test tests/center_of_mass_persistence.c)
    target_link_libraries(center_of_mass_persistence_test PRIVATE rohr_editor_core)
    set_target_properties(center_of_mass_persistence_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME center_of_mass_persistence COMMAND center_of_mass_persistence_test)
    set_tests_properties(center_of_mass_persistence PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_core_test tests/editor_core.c)
    target_link_libraries(editor_core_test PRIVATE rohr_editor_core)
    set_target_properties(editor_core_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_core COMMAND editor_core_test)

    add_executable(editor_config_test tests/editor_config.c)
    target_link_libraries(editor_config_test PRIVATE rohr_editor_core)
    target_compile_definitions(editor_config_test PRIVATE
        EDITOR_CONFIG_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    set_target_properties(editor_config_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_config COMMAND editor_config_test
        $<TARGET_FILE:editor_config_test>)
endif()

if(ROHR_BUILD_TESTS)
    add_executable(editor_soft_areas_test tests/editor_soft_areas.c)
    target_link_libraries(editor_soft_areas_test PRIVATE rohr_editor_core)
    set_target_properties(editor_soft_areas_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_soft_areas_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME editor_soft_areas COMMAND editor_soft_areas_test)
    # Keep assertion-based setup and checks active in Release verification.
    get_property(editor_core_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(editor_core_target IN LISTS editor_core_targets)
        if(editor_core_target MATCHES "_test$")
            if(MSVC)
                target_compile_options(${editor_core_target} PRIVATE /UNDEBUG)
            else()
                target_compile_options(${editor_core_target} PRIVATE -UNDEBUG)
            endif()
        endif()
    endforeach()
endif()
