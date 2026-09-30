# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

add_library(rohr_editor_core STATIC
    ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_error.c
    ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_project.c
    ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_project_json.c
    ${PROJECT_SOURCE_DIR}/tools/editor/src/editor_workspace.c
    src/editor_mass_properties.c
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

    add_executable(stable_soft_areas_test tests/stable_soft_areas.c)
    target_link_libraries(stable_soft_areas_test PRIVATE rohr_editor_core)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(stable_soft_areas_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(stable_soft_areas_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME stable_soft_areas COMMAND stable_soft_areas_test)
    set_tests_properties(stable_soft_areas PROPERTIES
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
