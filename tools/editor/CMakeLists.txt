# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

add_executable(rohr_editor_gui
    src/browser/editor_file_browser.c
    src/editor_history.c
    src/editor_shortcuts.c
    src/editor_navigation.c
    src/panels/editor_panel_lifecycle.c
    src/editor_viewport.c
    src/editors/multi/editor_bulk_panel.c
    src/panels/editor_build_settings_panel.c
    src/panels/editor_build_notifications.c
    src/panels/editor_generation_report.c
    src/panels/editor_notification_panel.c
    src/panels/editor_physics_settings_panel.c
    src/panels/editor_scene_panel.c
    src/panels/editor_application_settings_panel.c
    src/panels/editor_scene_settings_panel.c
    src/panels/editor_input_settings_panel.c
    src/panels/editor_notification_store.c
    src/editors/geometry/editor_origin_panel.c
    src/panels/editor_terminal_panel.c
    src/panels/editor_visual_settings_panel.c
    src/panels/editor_error_notifications.c
    src/states/editor_app_state.c
    src/states/editor_project_launcher_state.c
    src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
    src/editors/editor_collision_controls.c
    src/editors/geometry/editor_vertex.c
    src/editors/geometry/editor_line.c
    src/editors/geometry/editor_auto_shape.c
    src/editors/geometry/editor_hitbox.c
    src/editors/physics/editor_center_of_mass.c
    src/editors/physics/editor_particle.c
    src/editors/physics/editor_force.c
    src/editors/physics/editor_anchor.c
    src/editors/physics/editor_rigid_body.c
    src/editors/physics/editor_joint.c
    src/editors/soft_body/editor_soft_beam.c
    src/editors/soft_body/editor_soft_node.c
    src/editors/soft_body/editor_soft_area.c
    src/editors/soft_body/editor_soft_body.c
    src/editors/render/editor_animation_frame.c
    src/editors/render/editor_sprite.c
    src/editors/render/editor_camera.c
    src/editors/editor_entity_settings_panel.c
    src/editors/render/editor_animated_sprite.c
    src/editors/object/editor_object.c
    src/editors/object/editor_hierarchy.c
    src/editors/layout/editor_layout_viewport.c
    src/viewport/controls/editor_rotation_control.c
    src/viewport/controls/editor_coordinate_toggle.c
    src/viewport/menus/editor_viewport_context_menu.c
    src/main.c
)

target_include_directories(rohr_editor_gui PRIVATE
    src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(rohr_editor_gui PRIVATE
    ROHR_DEVELOPMENT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(rohr_editor_gui PRIVATE rohr_engine rohr_terminal rohr_editor_core)

set_target_properties(rohr_editor_gui PROPERTIES
    OUTPUT_NAME rohr-gui
    RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tools/rohr-gui
)

add_custom_command(TARGET rohr_editor_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:rohr_editor_gui>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/jetbrains_mono_bold_italic.ttf
        $<TARGET_FILE_DIR:rohr_editor_gui>/assets/jetbrains_mono_bold_italic.ttf
    VERBATIM
)
install(TARGETS rohr_editor_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_BINDIR}/assets)

if(ROHR_BUILD_TESTS)
    # Compile the actual GUI controller into the regression test without exposing
    # an application-internal API solely for testing.
    get_target_property(editor_diagnostics_sources rohr_editor_gui SOURCES)
    list(REMOVE_ITEM editor_diagnostics_sources src/main.c)
    add_executable(editor_build_diagnostics_test tests/editor_build_diagnostics.c
        ${editor_diagnostics_sources})
    target_include_directories(editor_build_diagnostics_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(editor_build_diagnostics_test PRIVATE
        ROHR_DEVELOPMENT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_link_libraries(editor_build_diagnostics_test PRIVATE rohr_engine rohr_terminal rohr_editor_core)
    set_target_properties(editor_build_diagnostics_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(editor_build_diagnostics_test PRIVATE -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_build_diagnostics_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(editor_build_diagnostics_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_executable(editor_error_presentation_test tests/editor_error_presentation.c ${editor_diagnostics_sources})
    target_include_directories(editor_error_presentation_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(editor_error_presentation_test PRIVATE ROHR_DEVELOPMENT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_link_libraries(editor_error_presentation_test PRIVATE rohr_engine rohr_terminal rohr_editor_core)
    target_compile_options(editor_error_presentation_test PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:-UNDEBUG>)
    set_target_properties(editor_error_presentation_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_error_presentation_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(editor_error_presentation_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_test(NAME editor_error_presentation COMMAND editor_error_presentation_test ${CMAKE_CURRENT_BINARY_DIR}/error_presentation_project)
    set_tests_properties(editor_error_presentation PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_panel_lifecycle_test tests/editor_panel_lifecycle.c
        ${editor_diagnostics_sources})
    target_include_directories(editor_panel_lifecycle_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(editor_panel_lifecycle_test PRIVATE
        ROHR_DEVELOPMENT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_link_libraries(editor_panel_lifecycle_test PRIVATE rohr_engine rohr_terminal rohr_editor_core)
    set_target_properties(editor_panel_lifecycle_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(editor_panel_lifecycle_test PRIVATE -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_panel_lifecycle_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(editor_panel_lifecycle_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_test(NAME editor_panel_lifecycle COMMAND editor_panel_lifecycle_test)
    set_tests_properties(editor_panel_lifecycle PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(NOT CMAKE_CROSSCOMPILING)
        add_test(NAME editor_build_diagnostics COMMAND editor_build_diagnostics_test
            ${CMAKE_CURRENT_BINARY_DIR}/diagnostics_project)
        set_tests_properties(editor_build_diagnostics PROPERTIES TIMEOUT 90
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endif()
    add_executable(editor_scene_panel_test tests/editor_scene_panel.c
        src/panels/editor_scene_panel.c src/panels/editor_notification_panel.c
        src/panels/editor_notification_store.c src/panels/editor_terminal_panel.c)
    target_include_directories(editor_scene_panel_test PRIVATE src ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_scene_panel_test PRIVATE rohr_editor_core rohr_terminal)
    set_target_properties(editor_scene_panel_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(editor_scene_panel_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_scene_panel_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(editor_scene_panel_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_test(NAME editor_scene_panel COMMAND editor_scene_panel_test $<TARGET_FILE:scene_child_test>)
    set_tests_properties(editor_scene_panel PROPERTIES TIMEOUT 30 ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_scene_settings_test tests/editor_scene_settings.c
        src/editor_history.c src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c src/panels/editor_scene_settings_panel.c
        src/editor_viewport.c src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_scene_settings_test PRIVATE src ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/lib)
    target_link_libraries(editor_scene_settings_test PRIVATE rohr_editor_core)
    set_target_properties(editor_scene_settings_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_scene_settings_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME editor_scene_settings COMMAND editor_scene_settings_test)
    set_tests_properties(editor_scene_settings PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_log_test tests/editor_log.c src/editors/editor_mode_controls.c)
    target_include_directories(editor_log_test PRIVATE src ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/lib)
    target_link_libraries(editor_log_test PRIVATE rohr_editor_core)
    set_target_properties(editor_log_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_log_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME editor_log COMMAND editor_log_test)
    set_tests_properties(editor_log PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_text_input_test
        tests/editor_text_input.c
        src/browser/editor_file_browser.c
        src/panels/editor_terminal_panel.c)
    target_include_directories(editor_text_input_test PRIVATE src)
    target_compile_definitions(editor_text_input_test PRIVATE
        ROHR_TEST_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/jetbrains_mono_bold_italic.ttf"
        ROHR_TEST_BROWSER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    target_link_libraries(editor_text_input_test PRIVATE rohr_engine rohr_terminal rohr_editor_core)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(editor_text_input_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(editor_text_input_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_text_input COMMAND editor_text_input_test)
    set_tests_properties(editor_text_input PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_test(NAME editor_gui_startup
        COMMAND rohr_editor_gui --startup-check)
    set_tests_properties(editor_gui_startup PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        TIMEOUT 15)

    add_executable(editor_app_state_test
        tests/editor_app_state.c
        src/states/editor_app_state.c
    )
    target_include_directories(editor_app_state_test PRIVATE src)
    set_target_properties(editor_app_state_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_app_state COMMAND editor_app_state_test)

    add_executable(editor_project_test
        tests/editor_project.c
    )
    target_include_directories(editor_project_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib)
    target_link_libraries(editor_project_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_project_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_project COMMAND editor_project_test)

    add_executable(editor_navigation_test
        tests/editor_navigation.c
        tests/editor_layers.c
        src/browser/editor_file_browser.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/viewport/controls/editor_rotation_control.c
        src/editors/multi/editor_bulk_panel.c
        src/panels/editor_input_settings_panel.c
    )
    target_include_directories(editor_navigation_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_navigation_test PRIVATE rohr_engine rohr_editor_core)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(editor_navigation_test PRIVATE ROHR_TEST_PREVIEW_WRAP)
        target_link_options(editor_navigation_test PRIVATE
            -Wl,--wrap=rohr_graphics_screen_quad_draw
            -Wl,--wrap=SDL_RenderFillRect -Wl,--wrap=SDL_RenderTextureRotated)
    endif()
    set_target_properties(editor_navigation_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_navigation COMMAND editor_navigation_test)
    set_tests_properties(editor_navigation PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_auto_shape_test
        tests/editor_auto_shape.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/geometry/editor_hitbox.c
        src/editors/soft_body/editor_soft_area.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/soft_body/editor_soft_body.c
        src/editors/multi/editor_bulk_panel.c
        src/viewport/controls/editor_rotation_control.c
    )
    target_include_directories(editor_auto_shape_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_auto_shape_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_auto_shape_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_auto_shape COMMAND editor_auto_shape_test)
    set_tests_properties(editor_auto_shape PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_area_authoring_test
        tests/editor_area_authoring.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/soft_body/editor_soft_node.c
        src/editors/soft_body/editor_soft_area.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_area_authoring_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_area_authoring_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_area_authoring_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_area_authoring COMMAND editor_area_authoring_test)
    set_tests_properties(editor_area_authoring PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_com_authoring_test
        tests/editor_com_authoring.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/physics/editor_center_of_mass.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_com_authoring_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_com_authoring_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_com_authoring_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_com_authoring COMMAND editor_com_authoring_test)
    set_tests_properties(editor_com_authoring PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_soft_mass_authoring_test
        tests/editor_soft_mass_authoring.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/soft_body/editor_soft_body.c
        src/editors/soft_body/editor_soft_area.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_soft_mass_authoring_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_soft_mass_authoring_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_soft_mass_authoring_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_soft_mass_authoring COMMAND editor_soft_mass_authoring_test)
    set_tests_properties(editor_soft_mass_authoring PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_particle_authoring_test
        tests/editor_particle_authoring.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/physics/editor_particle.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_particle_authoring_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_particle_authoring_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_particle_authoring_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_particle_authoring COMMAND editor_particle_authoring_test)
    set_tests_properties(editor_particle_authoring PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_beam_materials_test
        tests/editor_beam_materials.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/soft_body/editor_soft_beam.c
        src/editors/multi/editor_bulk_panel.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_beam_materials_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_beam_materials_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_beam_materials_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_beam_materials COMMAND editor_beam_materials_test)
    set_tests_properties(editor_beam_materials PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_configuration_controls_test
        tests/editor_configuration_controls.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/editor_entity_settings_panel.c
        src/panels/editor_application_settings_panel.c
        src/editors/multi/editor_bulk_panel.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_configuration_controls_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_configuration_controls_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_configuration_controls_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_configuration_controls COMMAND editor_configuration_controls_test)
    set_tests_properties(editor_configuration_controls PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_collision_controls_test
        tests/editor_collision_controls.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_viewport.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editors/editor_collision_controls.c
        src/editors/geometry/editor_auto_shape.c
        src/editors/multi/editor_bulk_panel.c
        src/editors/physics/editor_center_of_mass.c
        src/editors/physics/editor_rigid_body.c
        src/editors/physics/editor_particle.c
        src/editors/soft_body/editor_soft_node.c
        src/editors/soft_body/editor_soft_beam.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_collision_controls_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_collision_controls_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_collision_controls_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_collision_controls COMMAND editor_collision_controls_test)
    set_tests_properties(editor_collision_controls PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    get_target_property(motion_test_sources editor_collision_controls_test SOURCES)
    list(REMOVE_ITEM motion_test_sources tests/editor_collision_controls.c)
    add_executable(editor_motion_authoring_test tests/editor_motion_authoring.c ${motion_test_sources}
        src/editors/soft_body/editor_soft_body.c src/editors/soft_body/editor_soft_area.c)
    target_include_directories(editor_motion_authoring_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_motion_authoring_test PRIVATE rohr_engine rohr_editor_core)
    if(NOT MSVC)
        target_compile_options(editor_motion_authoring_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    set_target_properties(editor_motion_authoring_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_motion_authoring COMMAND editor_motion_authoring_test)
    set_tests_properties(editor_motion_authoring PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    get_target_property(joint_test_sources editor_motion_authoring_test SOURCES)
    list(REMOVE_ITEM joint_test_sources tests/editor_motion_authoring.c)
    add_executable(editor_joint_authoring_test tests/editor_joint_authoring.c ${joint_test_sources}
        src/editors/physics/editor_joint.c)
    target_include_directories(editor_joint_authoring_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_joint_authoring_test PRIVATE rohr_engine rohr_editor_core)
    target_compile_options(editor_joint_authoring_test PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-UNDEBUG>)
    set_target_properties(editor_joint_authoring_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_joint_authoring COMMAND editor_joint_authoring_test)
    set_tests_properties(editor_joint_authoring PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    if(ROHR_SDK_INSTALL_PREFIX AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME editor_joint_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_joint_authoring_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/joint generated project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/joint_generation.cmake")
        set_tests_properties(editor_joint_generated PROPERTIES LABELS "sdk;linux")
    endif()

    get_target_property(soft_physics_test_sources editor_motion_authoring_test SOURCES)
    list(REMOVE_ITEM soft_physics_test_sources tests/editor_motion_authoring.c)
    add_executable(editor_soft_physics_test tests/editor_soft_physics.c ${soft_physics_test_sources})
    target_include_directories(editor_soft_physics_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_soft_physics_test PRIVATE rohr_engine rohr_editor_core)
    if(NOT MSVC)
        target_compile_options(editor_soft_physics_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    set_target_properties(editor_soft_physics_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_soft_physics COMMAND editor_soft_physics_test)
    set_tests_properties(editor_soft_physics PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_SDK_INSTALL_PREFIX AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME editor_soft_physics_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_soft_physics_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/soft physics generated project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/soft_physics_generation.cmake")
        set_tests_properties(editor_soft_physics_generated PROPERTIES LABELS "sdk;linux")
    endif()

    if(ROHR_SDK_INSTALL_PREFIX AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME editor_motion_generated COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:editor_motion_authoring_test>
            "-DFIXTURE=${CMAKE_CURRENT_BINARY_DIR}/motion generated project"
            "-DSDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/motion_generation.cmake")
        set_tests_properties(editor_motion_generated PROPERTIES LABELS "sdk;linux")
    endif()

    add_executable(editor_viewport_context_menu_test
        tests/editor_viewport_context_menu.c
        src/viewport/menus/editor_viewport_context_menu.c
    )
    target_include_directories(editor_viewport_context_menu_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib)
    target_link_libraries(editor_viewport_context_menu_test PRIVATE
        rohr_engine rohr_editor_core)
    set_target_properties(editor_viewport_context_menu_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_viewport_context_menu
        COMMAND editor_viewport_context_menu_test)
    set_tests_properties(editor_viewport_context_menu PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_visibility_test
        tests/editor_visibility.c src/editor_history.c src/editor_navigation.c
        src/editors/render/editor_camera.c src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editor_viewport.c src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_visibility_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_visibility_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_visibility_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_visibility COMMAND editor_visibility_test)
    set_tests_properties(editor_visibility PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_force_test
        tests/editor_force.c src/editors/physics/editor_force.c src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editor_history.c src/editor_navigation.c src/editor_viewport.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_force_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_force_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_force_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_force COMMAND editor_force_test)
    set_tests_properties(editor_force PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_reference_picker_test
        tests/editor_reference_picker.c src/editors/physics/editor_force.c
        src/panels/editor_scene_settings_panel.c
        src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editor_history.c src/editor_navigation.c src/editor_viewport.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_reference_picker_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_reference_picker_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_reference_picker_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_reference_picker COMMAND editor_reference_picker_test)
    set_tests_properties(editor_reference_picker PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(editor_torque_test
        tests/editor_torque.c src/editors/physics/editor_force.c src/editors/editor_mode_controls.c src/editors/editor_reference_picker.c
        src/editor_history.c src/editor_navigation.c src/editor_viewport.c
        src/viewport/controls/editor_rotation_control.c)
    target_include_directories(editor_torque_test PRIVATE src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_torque_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_torque_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME editor_torque COMMAND editor_torque_test)
    set_tests_properties(editor_torque PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(editor_history_test
        tests/editor_history.c
        src/editor_history.c
        src/editor_navigation.c
        src/editor_shortcuts.c
        src/editor_viewport.c
        src/viewport/controls/editor_rotation_control.c
    )
    target_include_directories(editor_history_test PRIVATE
        src ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(editor_history_test PRIVATE rohr_engine rohr_editor_core)
    set_target_properties(editor_history_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_history COMMAND editor_history_test)

    add_executable(editor_notification_store_test
        tests/editor_notification_store.c
        src/panels/editor_notification_store.c
    )
    target_include_directories(editor_notification_store_test PRIVATE src)
    set_target_properties(editor_notification_store_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )
    add_test(NAME editor_notification_store COMMAND editor_notification_store_test)
endif()

if(ROHR_BUILD_TESTS AND ROHR_ENABLE_SANITIZERS)
    foreach(area_editor_target editor_joint_authoring_test editor_soft_physics_test editor_motion_authoring_test editor_visibility_test editor_torque_test editor_force_test editor_reference_picker_test editor_project_test editor_navigation_test editor_history_test editor_area_authoring_test editor_auto_shape_test editor_collision_controls_test editor_com_authoring_test editor_particle_authoring_test editor_beam_materials_test editor_configuration_controls_test)
        target_compile_options(${area_editor_target} PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endforeach()
endif()
