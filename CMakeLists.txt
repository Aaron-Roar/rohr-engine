# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.20)

project(rohr_engine LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ROHR_BUILD_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" CACHE PATH
    "Directory for user-facing Rohr build artifacts")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ROHR_BUILD_OUTPUT_DIRECTORY}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ROHR_BUILD_OUTPUT_DIRECTORY}/lib")

option(ROHR_BUILD_EXAMPLES "Build Rohr example programs" ON)
option(ROHR_BUILD_TESTS "Build Rohr tests" ON)
option(ROHR_BUILD_EDITOR "Build Rohr Editor" ON)
option(ROHR_BUILD_TERMINAL "Build Rohr Terminal tool" ON)
option(ROHR_BUILD_CLI "Build Rohr project CLI" ON)
option(ROHR_ENABLE_DOCUMENTATION "Enable documentation build targets" ON)
option(ROHR_PORTABLE_SDK
    "Avoid development-package dependencies in installed Rohr SDKs" OFF)
option(ROHR_BUILD_SDK_CONSUMER_TESTS
    "Test an external consumer against the installed Rohr SDK" OFF)
option(ROHR_ENABLE_SANITIZERS
    "Enable Linux address and undefined-behavior sanitizers" OFF)
set(ROHR_SDK_INSTALL_PREFIX "" CACHE PATH
    "Installed SDK prefix used by external consumer tests")
if(ROHR_ENABLE_SANITIZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "ROHR_ENABLE_SANITIZERS is supported only on Linux")
    endif()
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR
            "ROHR_ENABLE_SANITIZERS requires a GNU or Clang C compiler")
    endif()
    set(ROHR_SANITIZER_COMPILE_OPTIONS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer)
    set(ROHR_SANITIZER_LINK_OPTIONS
        -fsanitize=address,undefined)
endif()
if(MSVC AND ROHR_PORTABLE_SDK)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING
        "MSVC runtime linked by Rohr SDK targets" FORCE)
endif()

set(ROHR_LUA_SOURCES
    lib/lua/lapi.c
    lib/lua/lauxlib.c
    lib/lua/lbaselib.c
    lib/lua/lcode.c
    lib/lua/lcorolib.c
    lib/lua/lctype.c
    lib/lua/ldebug.c
    lib/lua/ldo.c
    lib/lua/ldump.c
    lib/lua/lfunc.c
    lib/lua/lgc.c
    lib/lua/linit.c
    lib/lua/llex.c
    lib/lua/lmathlib.c
    lib/lua/lmem.c
    lib/lua/lobject.c
    lib/lua/lopcodes.c
    lib/lua/lparser.c
    lib/lua/lstate.c
    lib/lua/lstring.c
    lib/lua/lstrlib.c
    lib/lua/ltable.c
    lib/lua/ltablib.c
    lib/lua/ltm.c
    lib/lua/lundump.c
    lib/lua/lutf8lib.c
    lib/lua/lvm.c
    lib/lua/lzio.c
)
add_library(rohr_lua STATIC ${ROHR_LUA_SOURCES})
target_include_directories(rohr_lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/lua)
if(NOT MSVC)
    target_link_libraries(rohr_lua PUBLIC m)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 as a static library" FORCE)
set(ROHR_SDL_DEPS_SHARED OFF)
if(ROHR_PORTABLE_SDK AND UNIX AND NOT APPLE)
    set(ROHR_SDL_DEPS_SHARED ON)
endif()
set(SDL_DEPS_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL
    "Load SDL platform dependencies dynamically" FORCE)
set(SDL_ALSA_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL "Load ALSA dynamically" FORCE)
set(SDL_PIPEWIRE_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL
    "Load PipeWire dynamically" FORCE)
set(SDL_PULSEAUDIO_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL
    "Load PulseAudio dynamically" FORCE)
if(UNIX AND NOT APPLE)
    set(SDL_WAYLAND ON CACHE BOOL "Build the native Wayland video driver" FORCE)
    set(SDL_WAYLAND_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL
        "Load Wayland dynamically" FORCE)
    set(SDL_X11 ON CACHE BOOL "Build the X11 fallback video driver" FORCE)
endif()
set(SDL_X11_SHARED ${ROHR_SDL_DEPS_SHARED} CACHE BOOL "Load X11 dynamically" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3 test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
set(SDL_INSTALL ON CACHE BOOL "Install SDL3" FORCE)
add_subdirectory(lib/SDL)

set(ROHR_SDL3_TARGET SDL3::SDL3-static)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build bundled libraries statically" FORCE)
set(SDLIMAGE_INSTALL ON CACHE BOOL "Install SDL3_image" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "Build SDL3_image samples" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "Build SDL3_image tests" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "Use SDL_image codec submodules" FORCE)
add_subdirectory(lib/SDL_image)

set(SDLTTF_INSTALL ON CACHE BOOL "Install SDL3_ttf" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "Build SDL3_ttf samples" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "Build SDL_ttf color emoji support" FORCE)
if(WIN32 OR ROHR_PORTABLE_SDK)
    set(SDLTTF_VENDORED ON CACHE BOOL "Use bundled SDL_ttf dependencies" FORCE)
    set(SDLTTF_HARFBUZZ OFF CACHE BOOL "Build SDL_ttf HarfBuzz support" FORCE)
else()
    set(SDLTTF_VENDORED OFF CACHE BOOL "Use bundled SDL_ttf dependencies" FORCE)
    set(SDLTTF_HARFBUZZ ON CACHE BOOL "Build SDL_ttf HarfBuzz support" FORCE)
endif()
add_subdirectory(lib/SDL_ttf)
if(ROHR_PORTABLE_SDK AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(SDL3_ttf-static PRIVATE
        -Wno-unused-local-typedefs)
endif()
if(WIN32 AND SDLTTF_VENDORED)
    install(SCRIPT
        "${CMAKE_CURRENT_BINARY_DIR}/lib/SDL_ttf/external/freetype/cmake_install.cmake")
endif()

set(ROHR_SDK_RUNTIME_LIBRARY_DIRS)
if(ROHR_PORTABLE_SDK AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    foreach(runtime_library_variable
            ALSA_LIBRARY
            PIPEWIRE_0.3_LIB
            PULSE_LIB
            WAYLAND_CLIENT_LIB
            WAYLAND_CURSOR_LIB
            WAYLAND_EGL_LIB
            DECOR_0_LIB
            XKBCOMMON_LIB
            X11_X11_LIB
            X11_Xcursor_LIB
            X11_Xext_LIB
            X11_Xfixes_LIB
            X11_Xi_LIB
            X11_Xrandr_LIB
            X11_Xss_LIB
            X11_Xtst_LIB)
        if(DEFINED ${runtime_library_variable} AND
                "${${runtime_library_variable}}" MATCHES "^/nix/store/")
            get_filename_component(runtime_library_directory
                "${${runtime_library_variable}}" DIRECTORY)
            list(APPEND ROHR_SDK_RUNTIME_LIBRARY_DIRS
                "${runtime_library_directory}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES ROHR_SDK_RUNTIME_LIBRARY_DIRS)
    set(CMAKE_INSTALL_RPATH "${ROHR_SDK_RUNTIME_LIBRARY_DIRS}")
endif()

set(ROHR_SDL3_IMAGE_TARGET SDL3_image::SDL3_image-static)
set(ROHR_SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf-static)

set(ROHR_ENGINE_SOURCES
    src/scene/scene.c
    src/scene/scene_transport.c
    src/scene/scene_input.c
    src/audio/audio.c
    src/core/console.c
    src/core/engine.c
    src/core/error.c
    src/core/platform_process.c
    src/core/report.c
    src/core/report_output.c
    src/core/rohr.c
    src/entity/entity_info.c
    src/entity/entity_components.c
    src/entity/object_source.c
    tools/editor/src/editor_sources.c
    tools/editor/src/editor_sources_json.c
    src/entity/object_description.c
    src/entity/object_document.c
    src/entity/object_spawn.c
    src/entity/object_settings.c
    src/entity/object_visuals.c
    src/graphics/marker_geometry.c
    # Shared checked editor-document model used by runtime object loading.
    tools/editor/src/editor_project.c
    tools/editor/src/editor_numeric.c
    tools/editor/src/editor_entity_settings.c
    tools/editor/src/editor_presentation.c
    tools/editor/src/editor_joint_placement.c
    tools/editor/src/editor_project_json.c
    tools/editor/src/editor_error.c
    tools/editor/src/editor_log.c
    tools/editor_core/src/editor_soft_area.c
    src/entity/entity_pair_set.c
    src/graphics/asset_path.c
    src/graphics/animation_assets.c
    src/graphics/font_assets.c
    src/graphics/graphics.c
    src/graphics/text_assets.c
    src/graphics/texture_assets.c
    src/graphics/graphics_layer_order.c
    src/graphics/window_presentation.c
    src/graphics/ui.c
    src/input/input.c
    src/math/math2d.c
    src/math/node_loop_geometry.c
    src/physics/broadphase/aabb_tree.c
    src/physics/body/body_state.c
    src/physics/body/mass_properties.c
    src/physics/body/force_state.c
    src/physics/body/motion_state.c
    src/physics/auto_shape.c
    src/physics/collision/contact_constraint.c
    src/physics/collision/contact_manifold.c
    src/physics/collision/contact_query.c
    src/physics/collision/hitbox.c
    src/physics/collision/hitbox_binding.c
    src/physics/collision/collision_filter.c
    src/physics/collision/collision_material.c
    src/physics/collision/interaction_query.c
    src/physics/collision/interaction_set.c
    src/physics/collision/interaction_state.c
    src/physics/collision/overlap_query.c
    src/physics/collision/overlap_geometry.c
    src/physics/collision/shape_decomposition.c
    src/physics/constraints/constraint_solver.c
    src/physics/rigid_body/contact_block.c
    src/physics/rigid_body/rigid_sweep.c
    src/physics/constraints/transform_lock.c
    src/physics/joints/joint_constraint.c
    src/physics/joints/joint.c
    src/physics/joints/joint_placement.c
    src/physics/joints/joint_solver.c
    src/physics/joints/joint_motion.c
    src/physics/particles/particle.c
    src/physics/physics.c
    src/physics/physics_entity.c
    src/physics/physics_step.c
    src/physics/physics_operation.c
    src/physics/pipeline/physics_pipeline.c
    src/physics/pipeline/physics_config.c
    src/physics/pipeline/physics_stages.c
    src/physics/rigid_body/rigid_body.c
    src/physics/rigid_body/rigid_body_step.c
    src/physics/soft_body/soft_body.c
    src/physics/soft_body/soft_body_state.c
    src/physics/soft_body/soft_body_mass.c
    src/physics/soft_body/soft_body_motion.c
    src/physics/soft_body/soft_body_frame.c
    src/physics/soft_body/soft_body_area.c
    src/physics/soft_body/soft_body_solver.c
    src/physics/soft_body/beam_pair.c
    src/physics/soft_body/beam_motion.c
    src/state/game_state.c
    src/state/runtime_topology.c
    src/state/runtime_visuals.c
    src/tools/tools.c
    lib/vorbis/stb_vorbis.c
    lib/yyjson/yyjson.c
)

add_library(rohr_engine STATIC ${ROHR_ENGINE_SOURCES})
add_library(Rohr::Engine ALIAS rohr_engine)
target_compile_features(rohr_engine PUBLIC c_std_11)
set_target_properties(rohr_engine PROPERTIES EXPORT_NAME Engine)
target_include_directories(rohr_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/editor/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/editor_core/include
)
target_compile_definitions(rohr_engine PUBLIC SDL_MAIN_HANDLED)
if(ROHR_ENABLE_SANITIZERS)
    target_compile_options(rohr_engine PRIVATE
        ${ROHR_SANITIZER_COMPILE_OPTIONS})
    target_link_options(rohr_engine INTERFACE
        ${ROHR_SANITIZER_LINK_OPTIONS})
endif()
target_link_libraries(rohr_engine
    PUBLIC
        ${ROHR_SDL3_TARGET}
        ${ROHR_SDL3_IMAGE_TARGET}
        ${ROHR_SDL3_TTF_TARGET}
)
if(WIN32)
    target_link_libraries(rohr_engine PUBLIC ws2_32 bcrypt)
endif()
foreach(runtime_library_directory IN LISTS ROHR_SDK_RUNTIME_LIBRARY_DIRS)
    target_link_options(rohr_engine INTERFACE
        "$<INSTALL_INTERFACE:-Wl,-rpath,${runtime_library_directory}>")
endforeach()

if(NOT MSVC)
    target_link_libraries(rohr_engine PUBLIC m)
endif()

add_library(rohr_tools STATIC
    src/tools/component_codegen.c
)
add_library(Rohr::Tools ALIAS rohr_tools)
set_target_properties(rohr_tools PROPERTIES EXPORT_NAME Tools)
target_include_directories(rohr_tools
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS rohr_engine rohr_tools
    EXPORT RohrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT RohrTargets
    FILE RohrTargets.cmake
    NAMESPACE Rohr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rohr)
configure_package_config_file(
    cmake/RohrConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/RohrConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rohr)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/RohrConfigVersion.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/RohrConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/RohrConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rohr)
install(FILES
    config/editor.lua
    DESTINATION ${CMAKE_INSTALL_DATADIR}/rohr)
install(FILES
    config/project_editor.lua
    DESTINATION ${CMAKE_INSTALL_DATADIR}/rohr)
install(FILES
    examples/flies-around-ball/assets/elder-fly/flying/f1.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/rohr/starter-assets
    RENAME tutorial_frame_1.png)
install(FILES
    examples/flies-around-ball/assets/elder-fly/flying/f2.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/rohr/starter-assets
    RENAME tutorial_frame_2.png)
install(FILES
    config/editor_gui_settings.lua
    DESTINATION config)
install(FILES lib/lua/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME Lua-LICENSE)
install(FILES
    COPYING
    COPYING.LESSER
    LICENSE.md
    THIRD_PARTY_LICENSES.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr)
install(FILES lib/SDL/LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME SDL3-LICENSE.txt)
install(FILES lib/SDL_image/LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME SDL3_image-LICENSE.txt)
install(FILES lib/SDL_ttf/LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME SDL3_ttf-LICENSE.txt)
install(FILES
    lib/SDL_ttf/external/freetype/LICENSE.TXT
    lib/SDL_ttf/external/freetype/docs/FTL.TXT
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr/freetype)
install(FILES lib/yyjson/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME yyjson-LICENSE)
install(FILES lib/vorbis/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME stb_vorbis-LICENSE)
install(FILES third_party_licenses/jetbrains_mono_ofl.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rohr
    RENAME JetBrains-Mono-OFL.txt)

include(cmake/rohr_host_tools.cmake)

if(ROHR_BUILD_TESTS OR ROHR_BUILD_SDK_CONSUMER_TESTS)
    include(CTest)
endif()

if(ROHR_BUILD_TERMINAL)
    add_subdirectory(tools/terminal)
endif()

add_subdirectory(tools/editor_core)

if(ROHR_BUILD_CLI)
    add_subdirectory(tools/cli)
endif()

if(ROHR_BUILD_EDITOR)
    if(NOT ROHR_BUILD_TERMINAL)
        message(FATAL_ERROR "ROHR_BUILD_EDITOR requires ROHR_BUILD_TERMINAL")
    endif()
    add_subdirectory(tools/editor)
endif()

if(ROHR_BUILD_EXAMPLES)
    add_subdirectory(examples)

    add_custom_target(examples)
    add_dependencies(examples
        center_of_mass
        audio
        view_port
        flies_in_pit
        flies_around_ball
        fly_to_finish
        game_state
        user_interface
        joints
        soft_body
        pong
        player_controller
    )

endif()

if(ROHR_BUILD_SDK_CONSUMER_TESTS)
    if(NOT ROHR_SDK_INSTALL_PREFIX)
        message(FATAL_ERROR
            "ROHR_BUILD_SDK_CONSUMER_TESTS requires ROHR_SDK_INSTALL_PREFIX")
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(ROHR_SDK_TEST_PLATFORM windows)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(ROHR_SDK_TEST_PLATFORM linux)
    else()
        message(FATAL_ERROR
            "Installed SDK consumer tests do not support ${CMAKE_SYSTEM_NAME}")
    endif()
    add_test(
        NAME installed_sdk_consumer_${ROHR_SDK_TEST_PLATFORM}
        COMMAND ${CMAKE_COMMAND}
            "-DROHR_SDK_PREFIX=${ROHR_SDK_INSTALL_PREFIX}"
            "-DROHR_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/installed_sdk_consumer"
            "-DROHR_CONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/installed_sdk_consumer_${ROHR_SDK_TEST_PLATFORM}"
            "-DROHR_GENERATOR=${CMAKE_GENERATOR}"
            "-DROHR_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
            "-DROHR_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
            "-DROHR_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DROHR_RUN_CONSUMER_TESTS=$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>"
            "-DROHR_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_installed_sdk_consumer.cmake")
    set_tests_properties(installed_sdk_consumer_${ROHR_SDK_TEST_PLATFORM}
        PROPERTIES LABELS "sdk;${ROHR_SDK_TEST_PLATFORM}")
endif()

if(ROHR_BUILD_TESTS)
    # Reuse the documentation toolchain; engine/SDK builds do not require Perl.
    find_package(Perl QUIET)
    if(PERL_FOUND)
        add_test(NAME api_inventory
            COMMAND ${PERL_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_api_inventory.pl
                ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME api_inventory_validation
            COMMAND ${PERL_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/api_inventory.pl)
        set_tests_properties(api_inventory api_inventory_validation
            PROPERTIES LABELS "api;documentation")
    else()
        message(STATUS "API inventory tests disabled: existing documentation tool Perl was not found")
    endif()

    add_executable(audio_test tests/audio.c)
    target_include_directories(audio_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(audio_test PRIVATE rohr_engine)
    set_target_properties(audio_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME audio COMMAND audio_test)

    add_executable(audio_reports_test tests/audio_reports.c)
    target_link_libraries(audio_reports_test PRIVATE rohr_engine)
    set_target_properties(audio_reports_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(audio_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(audio_reports_test PRIVATE ROHR_TEST_AUDIO_WRAP)
        target_link_options(audio_reports_test PRIVATE
            -Wl,--wrap=SDL_OpenAudioDeviceStream -Wl,--wrap=SDL_ResumeAudioStreamDevice
            -Wl,--wrap=SDL_PutAudioStreamData -Wl,--wrap=stb_vorbis_get_samples_float_interleaved
            -Wl,--wrap=SDL_GetTicksNS -Wl,--wrap=SDL_LockMutex -Wl,--wrap=SDL_LockAudioStream
            -Wl,--wrap=stb_vorbis_get_error -Wl,--wrap=stb_vorbis_seek_start
            -Wl,--wrap=SDL_malloc -Wl,--wrap=SDL_free -Wl,--wrap=SDL_LoadWAV
            -Wl,--wrap=stb_vorbis_close)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(audio_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME audio_reports COMMAND audio_reports_test)
    set_tests_properties(audio_reports PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")


    add_executable(engine_lifecycle_test tests/engine_lifecycle.c)
    target_include_directories(engine_lifecycle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(engine_lifecycle_test PRIVATE rohr_engine)
    set_target_properties(engine_lifecycle_test
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )

    add_test(NAME engine_lifecycle COMMAND engine_lifecycle_test)

    add_executable(directory_test tests/directory.c)
    target_link_libraries(directory_test PRIVATE rohr_engine)
    set_target_properties(directory_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME directory COMMAND directory_test)

    add_executable(input_actions_test tests/input_actions.c)
    target_link_libraries(input_actions_test PRIVATE rohr_engine)
    set_target_properties(input_actions_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME input_actions COMMAND input_actions_test)

    add_executable(camera_descriptions_test tests/camera_descriptions.c)
    target_link_libraries(camera_descriptions_test PRIVATE rohr_engine)
    set_target_properties(camera_descriptions_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(camera_descriptions_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(camera_descriptions_test PRIVATE ROHR_TEST_CAMERA_WRAP)
        target_link_options(camera_descriptions_test PRIVATE
            -Wl,--wrap=SDL_GetPerformanceCounter -Wl,--wrap=SDL_GetPerformanceFrequency
            -Wl,--wrap=SDL_SetRenderLogicalPresentation)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(camera_descriptions_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(camera_descriptions_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_test(NAME camera_descriptions COMMAND camera_descriptions_test)
    set_tests_properties(camera_descriptions PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(viewport_descriptions_test tests/viewport_descriptions.c)
    target_link_libraries(viewport_descriptions_test PRIVATE rohr_engine)
    set_target_properties(viewport_descriptions_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(viewport_descriptions_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(viewport_descriptions_test PRIVATE ROHR_TEST_VIEWPORT_WRAP)
        target_link_options(viewport_descriptions_test PRIVATE
            -Wl,--wrap=SDL_CreateTexture -Wl,--wrap=SDL_GetMouseState)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(viewport_descriptions_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(viewport_descriptions_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    add_test(NAME viewport_descriptions COMMAND viewport_descriptions_test)
    set_tests_properties(viewport_descriptions PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(cameras_test tests/cameras.c)
    add_executable(scene_runner_test tests/scene_runner.c)
    target_link_libraries(scene_runner_test PRIVATE rohr_engine)
    if(NOT MSVC)
        target_compile_options(scene_runner_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    else()
        target_compile_options(scene_runner_test PRIVATE /UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(scene_runner_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(scene_runner_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    set_target_properties(scene_runner_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME scene_runner COMMAND scene_runner_test)
    set_tests_properties(scene_runner PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    target_link_libraries(cameras_test PRIVATE rohr_engine)
    set_target_properties(cameras_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME cameras COMMAND cameras_test)

    add_executable(engine_reports_test tests/engine_reports.c)
    target_link_libraries(engine_reports_test PRIVATE rohr_engine)
    set_target_properties(engine_reports_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(engine_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(engine_reports_test PRIVATE ROHR_TEST_ENGINE_WRAP)
        target_link_options(engine_reports_test PRIVATE
            -Wl,--wrap=SDL_GetPerformanceCounter -Wl,--wrap=SDL_GetPerformanceFrequency
            -Wl,--wrap=entity_delete -Wl,--wrap=entity_from_index_get
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(engine_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME engine_reports COMMAND engine_reports_test)

    add_executable(physics_reports_test tests/physics_reports.c)
    target_link_libraries(physics_reports_test PRIVATE rohr_engine)
    target_include_directories(physics_reports_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(physics_reports_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(physics_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(physics_reports_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(physics_reports_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(physics_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME physics_reports COMMAND physics_reports_test)

    add_executable(physics_recovery_test tests/physics_recovery.c)
    target_link_libraries(physics_recovery_test PRIVATE rohr_engine)
    target_include_directories(physics_recovery_test PRIVATE src)
    set_target_properties(physics_recovery_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(physics_recovery_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(physics_recovery_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME physics_recovery COMMAND physics_recovery_test)

    add_executable(graphics_reports_test tests/graphics_reports.c)
    target_link_libraries(graphics_reports_test PRIVATE rohr_engine)
    target_include_directories(graphics_reports_test PRIVATE src)
    set_target_properties(graphics_reports_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(graphics_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(graphics_reports_test PRIVATE ROHR_TEST_GRAPHICS_WRAP)
        target_link_options(graphics_reports_test PRIVATE
            -Wl,--wrap=realloc -Wl,--wrap=SDL_RenderFillRect -Wl,--wrap=SDL_RenderTextureRotated -Wl,--wrap=SDL_SetRenderTarget
            -Wl,--wrap=SDL_RenderPresent -Wl,--wrap=SDL_RenderReadPixels
            -Wl,--wrap=SDL_RenderGeometry -Wl,--wrap=SDL_RenderLine
            -Wl,--wrap=platform_process_command_check -Wl,--wrap=platform_process_open_write
            -Wl,--wrap=platform_process_close -Wl,--wrap=fwrite -Wl,--wrap=fflush)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(graphics_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME graphics_reports COMMAND graphics_reports_test)
    set_tests_properties(graphics_reports PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")

    add_executable(report_output_test tests/report_output.c)
    target_link_libraries(report_output_test PRIVATE rohr_engine)
    target_include_directories(report_output_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    set_target_properties(report_output_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(report_output_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(report_output_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(report_output_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
            -Wl,--wrap=fputs -Wl,--wrap=fwrite -Wl,--wrap=fflush -Wl,--wrap=fclose)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(report_output_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME report_output COMMAND report_output_test)

    add_executable(system_reports_test tests/system_reports.c)
    add_executable(entity_info_test tests/entity_info.c)
    target_link_libraries(entity_info_test PRIVATE rohr_engine)
    target_include_directories(entity_info_test PRIVATE src)
    set_target_properties(entity_info_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(entity_info_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(entity_info_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(entity_info_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(entity_info_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME entity_info COMMAND entity_info_test)
    set_tests_properties(entity_info PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")
    add_executable(entity_info_adversarial_test tests/entity_info_adversarial.c)
    target_link_libraries(entity_info_adversarial_test PRIVATE rohr_engine)
    target_include_directories(entity_info_adversarial_test PRIVATE src)
    set_target_properties(entity_info_adversarial_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(entity_info_adversarial_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(entity_info_adversarial_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(entity_info_adversarial_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(entity_info_adversarial_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME entity_info_adversarial COMMAND entity_info_adversarial_test)
    set_tests_properties(entity_info_adversarial PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")
    add_executable(report_storage_test tests/report_storage.c)
    target_link_libraries(report_storage_test PRIVATE rohr_engine)
    target_include_directories(report_storage_test PRIVATE src)
    set_target_properties(report_storage_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(report_storage_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(report_storage_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(report_storage_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(report_storage_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME report_storage COMMAND report_storage_test)
    set_tests_properties(report_storage PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")
    target_link_libraries(system_reports_test PRIVATE rohr_engine)
    set_target_properties(system_reports_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(system_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(system_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME system_reports COMMAND system_reports_test)
    set_tests_properties(system_reports PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")

    add_executable(report_test tests/report.c)
    target_link_libraries(report_test PRIVATE rohr_engine)
    set_target_properties(report_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    if(NOT MSVC)
        target_compile_options(report_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(report_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    add_test(NAME report COMMAND report_test)

    add_executable(ui_instance_isolation_test tests/ui_instance_isolation.c)
    target_link_libraries(ui_instance_isolation_test PRIVATE rohr_engine)
    set_target_properties(ui_instance_isolation_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME ui_instance_isolation COMMAND ui_instance_isolation_test)

    add_executable(graphics_layer_order_test
        tests/graphics_layer_order.c
        src/graphics/graphics_layer_order.c)
    target_include_directories(graphics_layer_order_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(graphics_layer_order_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME graphics_layer_order COMMAND graphics_layer_order_test)

    add_executable(window_presentation_test tests/window_presentation.c)
    target_link_libraries(window_presentation_test PRIVATE rohr_engine)
    set_target_properties(window_presentation_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME window_presentation COMMAND window_presentation_test)

    add_executable(texture_assets_test tests/texture_assets.c)
    target_include_directories(texture_assets_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(texture_assets_test PRIVATE rohr_engine)
    set_target_properties(texture_assets_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME texture_assets COMMAND texture_assets_test)

    add_executable(animation_assets_test tests/animation_assets.c)
    target_include_directories(animation_assets_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(animation_assets_test PRIVATE rohr_engine)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(animation_assets_test PRIVATE ROHR_TEST_SPRITE_WRAP)
        target_link_options(animation_assets_test PRIVATE -Wl,--wrap=graphics_texture_retain)
    endif()
    if(MSVC)
        target_compile_options(animation_assets_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(animation_assets_test PRIVATE -UNDEBUG)
    endif()
    set_target_properties(animation_assets_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME animation_assets COMMAND animation_assets_test)

    add_executable(font_assets_test tests/font_assets.c)
    target_include_directories(font_assets_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(font_assets_test PRIVATE
        ROHR_TEST_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/examples/user-interface/assets/jetbrains_mono_bold_italic.ttf"
        ROHR_TEST_FONT_ALIAS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/examples/user-interface/assets/../assets/jetbrains_mono_bold_italic.ttf")
    target_link_libraries(font_assets_test PRIVATE rohr_engine)
    set_target_properties(font_assets_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME font_assets COMMAND font_assets_test)

    add_executable(text_assets_test tests/text_assets.c)
    target_include_directories(text_assets_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(text_assets_test PRIVATE
        ROHR_TEST_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/examples/user-interface/assets/jetbrains_mono_bold_italic.ttf")
    target_link_libraries(text_assets_test PRIVATE rohr_engine)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(text_assets_test PRIVATE ROHR_TEST_TEXT_WRAP)
        target_link_options(text_assets_test PRIVATE -Wl,--wrap=calloc)
    endif()
    if(MSVC)
        target_compile_options(text_assets_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(text_assets_test PRIVATE -UNDEBUG)
    endif()
    set_target_properties(text_assets_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME text_assets COMMAND text_assets_test)

    if(ROHR_ENABLE_SANITIZERS)
        foreach(asset_sanitizer_target
                audio_test
                texture_assets_test
                animation_assets_test
                font_assets_test
                text_assets_test)
            target_compile_options(${asset_sanitizer_target} PRIVATE
                ${ROHR_SANITIZER_COMPILE_OPTIONS})
        endforeach()
    endif()

    add_executable(timing_test tests/timing.c)
    target_link_libraries(timing_test PRIVATE rohr_engine)
    set_target_properties(timing_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME timing COMMAND timing_test)

    add_executable(ui_reports_test tests/ui_reports.c)
    target_link_libraries(ui_reports_test PRIVATE rohr_engine)
    set_target_properties(ui_reports_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME ui_reports COMMAND ui_reports_test)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(ui_reports_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(UNIX AND NOT APPLE)
        target_compile_definitions(ui_reports_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(ui_reports_test PRIVATE
            -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(ui_reports_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(ui_field_test tests/ui_field.c)
    target_link_libraries(ui_field_test PRIVATE rohr_engine)
    set_target_properties(ui_field_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME ui_field COMMAND ui_field_test)
    add_executable(ui_field_selection_test tests/ui_field_selection.c)
    target_include_directories(ui_field_selection_test PRIVATE src)
    target_compile_definitions(ui_field_selection_test PRIVATE
        ROHR_TEST_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/examples/user-interface/assets/jetbrains_mono_bold_italic.ttf")
    target_link_libraries(ui_field_selection_test PRIVATE rohr_engine)
    set_target_properties(ui_field_selection_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME ui_field_selection COMMAND ui_field_selection_test)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(ui_field_selection_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(ui_dropdown_test tests/ui_dropdown.c)
    target_link_libraries(ui_dropdown_test PRIVATE rohr_engine)
    set_target_properties(ui_dropdown_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME ui_dropdown COMMAND ui_dropdown_test)

    add_executable(joints_test tests/joints.c)
    target_link_libraries(joints_test PRIVATE rohr_engine)
    set_target_properties(joints_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME joints COMMAND joints_test)

    add_executable(force_preparation_test tests/force_preparation.c)
    target_link_libraries(force_preparation_test PRIVATE rohr_engine)
    target_compile_options(force_preparation_test PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-UNDEBUG>)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(force_preparation_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(force_preparation_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME force_preparation COMMAND force_preparation_test)
    set_tests_properties(force_preparation PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(contact_gathering_test tests/contact_gathering.c)
    target_include_directories(contact_gathering_test PRIVATE src)
    target_link_libraries(contact_gathering_test PRIVATE rohr_engine)
    target_compile_options(contact_gathering_test PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-UNDEBUG>)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
        target_compile_definitions(contact_gathering_test PRIVATE ROHR_TEST_CONTACT_APPEND_WRAP)
        target_link_options(contact_gathering_test PRIVATE "LINKER:--wrap=contact_constraint_list_append")
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(contact_gathering_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(contact_gathering_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME contact_gathering COMMAND contact_gathering_test)
    set_tests_properties(contact_gathering PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(soft_body_test tests/soft_body.c)
    target_link_libraries(soft_body_test PRIVATE rohr_engine)
    set_target_properties(soft_body_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME soft_body COMMAND soft_body_test)

    add_executable(soft_body_frame_test tests/soft_body_frame.c)
    target_include_directories(soft_body_frame_test PRIVATE src)
    target_link_libraries(soft_body_frame_test PRIVATE rohr_engine)
    set_target_properties(soft_body_frame_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME soft_body_frame COMMAND soft_body_frame_test)
    set_tests_properties(soft_body_frame PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(soft_body_frame_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(beam_materials_test tests/beam_materials.c)
    target_link_libraries(beam_materials_test PRIVATE rohr_engine)
    set_target_properties(beam_materials_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME beam_materials COMMAND beam_materials_test)
    set_tests_properties(beam_materials PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(beam_materials_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(soft_body_angular_velocity_test tests/soft_body_angular_velocity.c)
    target_include_directories(soft_body_angular_velocity_test PRIVATE src)
    target_link_libraries(soft_body_angular_velocity_test PRIVATE rohr_engine)
    set_target_properties(soft_body_angular_velocity_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME soft_body_angular_velocity COMMAND soft_body_angular_velocity_test)
    set_tests_properties(soft_body_angular_velocity PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(soft_body_angular_velocity_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(soft_body_mass_test tests/soft_body_mass.c)
    target_include_directories(soft_body_mass_test PRIVATE src)
    target_link_libraries(soft_body_mass_test PRIVATE rohr_engine)
    set_target_properties(soft_body_mass_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME soft_body_mass COMMAND soft_body_mass_test)
    set_tests_properties(soft_body_mass PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(soft_body_mass_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(beam_contacts_test tests/beam_contacts.c)
    target_include_directories(beam_contacts_test PRIVATE src)
    target_link_libraries(beam_contacts_test PRIVATE rohr_engine)
    set_target_properties(beam_contacts_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME beam_contacts COMMAND beam_contacts_test)
    set_tests_properties(beam_contacts PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(beam_contacts_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(soft_body_areas_test tests/soft_body_areas.c
        tests/soft_body_areas_visual.c)
    target_include_directories(soft_body_areas_test PRIVATE src)
    target_link_libraries(soft_body_areas_test PRIVATE rohr_engine)
    set_target_properties(soft_body_areas_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME soft_body_areas COMMAND soft_body_areas_test)
    set_tests_properties(soft_body_areas PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(soft_body_areas_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(node_loop_geometry_test tests/node_loop_geometry.c)
    target_include_directories(node_loop_geometry_test PRIVATE src)
    target_link_libraries(node_loop_geometry_test PRIVATE rohr_engine)
    set_target_properties(node_loop_geometry_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME node_loop_geometry COMMAND node_loop_geometry_test)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(node_loop_geometry_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()

    add_executable(auto_shape_test tests/auto_shape.c)
    target_link_libraries(auto_shape_test PRIVATE rohr_engine)
    set_target_properties(auto_shape_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME auto_shape COMMAND auto_shape_test)

    add_executable(collision_sensor_test tests/collision_sensor.c)
    target_include_directories(collision_sensor_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(collision_sensor_test PRIVATE rohr_engine)
    set_target_properties(collision_sensor_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME collision_sensor COMMAND collision_sensor_test)

    add_executable(collision_response_test tests/collision_response.c)
    target_include_directories(collision_response_test PRIVATE src)
    target_link_libraries(collision_response_test PRIVATE rohr_engine)
    set_target_properties(collision_response_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME collision_response COMMAND collision_response_test)

    add_executable(particle_collision_mode_test tests/particle_collision_mode.c)
    target_link_libraries(particle_collision_mode_test PRIVATE rohr_engine)
    set_target_properties(particle_collision_mode_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME particle_collision_mode COMMAND particle_collision_mode_test)

    add_executable(hitbox_animation_binding_test
        tests/hitbox_animation_binding.c)
    target_link_libraries(hitbox_animation_binding_test PRIVATE rohr_engine)
    set_target_properties(hitbox_animation_binding_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME hitbox_animation_binding COMMAND
        hitbox_animation_binding_test)

    add_executable(entity_pair_set_test tests/entity_pair_set.c)
    target_include_directories(entity_pair_set_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(entity_pair_set_test PRIVATE rohr_engine)
    set_target_properties(entity_pair_set_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME entity_pair_set COMMAND entity_pair_set_test)

    add_executable(aabb_tree_test tests/aabb_tree.c)
    target_include_directories(aabb_tree_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(aabb_tree_test PRIVATE rohr_engine)
    set_target_properties(aabb_tree_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME aabb_tree COMMAND aabb_tree_test)

    add_executable(physics_interaction_set_test tests/physics_interaction_set.c)
    target_include_directories(physics_interaction_set_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(physics_interaction_set_test PRIVATE rohr_engine)
    set_target_properties(physics_interaction_set_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME physics_interaction_set COMMAND physics_interaction_set_test)

    add_executable(constraint_solver_test tests/constraint_solver.c)
    target_include_directories(constraint_solver_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(constraint_solver_test PRIVATE rohr_engine)
    set_target_properties(constraint_solver_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME constraint_solver COMMAND constraint_solver_test)

    add_executable(contact_manifold_test tests/contact_manifold.c)
    target_include_directories(contact_manifold_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(contact_manifold_test PRIVATE rohr_engine)
    set_target_properties(contact_manifold_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME contact_manifold COMMAND contact_manifold_test)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(contact_manifold_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    foreach(contact_test IN ITEMS contact_manifold_test beam_contacts_test)
        if(MSVC)
            target_compile_options(${contact_test} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${contact_test} PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
        endif()
    endforeach()

    add_executable(angles_test tests/angles.c)
    target_link_libraries(angles_test PRIVATE rohr_engine)
    set_target_properties(angles_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME angles COMMAND angles_test)

    foreach(example_angle_case flies soft_body)
        add_executable(example_${example_angle_case}_angles_test
            tests/example_${example_angle_case}_angles.c
            examples/cmake/example_viewport.c)
        target_include_directories(example_${example_angle_case}_angles_test PRIVATE examples/cmake)
        target_link_libraries(example_${example_angle_case}_angles_test PRIVATE rohr_engine)
        set_target_properties(example_${example_angle_case}_angles_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
        add_test(NAME example_${example_angle_case}_angles
            COMMAND example_${example_angle_case}_angles_test)
        set_tests_properties(example_${example_angle_case}_angles PROPERTIES
            WORKING_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endforeach()
    add_custom_command(TARGET example_flies_angles_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/flies-in-pit/assets
            ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests/assets/flies-in-pit)
    target_sources(example_soft_body_angles_test PRIVATE
        examples/soft-body/src/car_controls.c examples/soft-body/src/generated/project_objects.c)
    target_include_directories(example_soft_body_angles_test PRIVATE
        examples/soft-body/src examples/soft-body/src/generated)
    add_executable(authored_car_test tests/authored_car.c
        examples/cmake/example_viewport.c
        examples/soft-body/src/car_controls.c examples/soft-body/src/generated/project_objects.c)
    target_include_directories(authored_car_test PRIVATE examples/cmake examples/soft-body/src examples/soft-body/src/generated)
    target_compile_definitions(authored_car_test PRIVATE CAR_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/examples/soft-body")
    target_link_libraries(authored_car_test PRIVATE rohr_engine)
    if(MSVC)
        target_compile_options(authored_car_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(authored_car_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(authored_car_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        target_link_options(authored_car_test PRIVATE ${ROHR_SANITIZER_LINK_OPTIONS})
    endif()
    set_target_properties(authored_car_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME authored_car COMMAND authored_car_test)
    set_tests_properties(authored_car PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(shared_sources_test tests/shared_sources.c)
    target_link_libraries(shared_sources_test PRIVATE rohr_engine)
    target_include_directories(shared_sources_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(MSVC)
        target_compile_options(shared_sources_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(shared_sources_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(shared_sources_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(shared_sources_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(shared_sources_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(shared_sources_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME shared_sources COMMAND shared_sources_test)

    add_executable(object_instances_test tests/object_instances.c)
    target_link_libraries(object_instances_test PRIVATE rohr_engine)
    target_include_directories(object_instances_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(object_instances_test PRIVATE ROHR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_definitions(object_instances_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(object_instances_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(MSVC)
        target_compile_options(object_instances_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(object_instances_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(object_instances_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(object_instances_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME object_instances COMMAND object_instances_test)

    add_executable(empty_animations_test tests/empty_animations.c)
    target_link_libraries(empty_animations_test PRIVATE rohr_engine)
    target_include_directories(empty_animations_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(MSVC)
        target_compile_options(empty_animations_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(empty_animations_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(empty_animations_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(empty_animations_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME empty_animations COMMAND empty_animations_test)
    set_tests_properties(empty_animations PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(runtime_persistence_test tests/runtime_persistence.c)
    target_link_libraries(runtime_persistence_test PRIVATE rohr_engine)
    target_include_directories(runtime_persistence_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    if(MSVC)
        target_compile_options(runtime_persistence_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(runtime_persistence_test PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(runtime_persistence_test PRIVATE ROHR_TEST_ALLOCATION_WRAP)
        target_link_options(runtime_persistence_test PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    endif()
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(runtime_persistence_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(runtime_persistence_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME runtime_persistence COMMAND runtime_persistence_test)

    add_executable(forces_test tests/forces.c)
    target_link_libraries(forces_test PRIVATE rohr_engine)
    set_target_properties(forces_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME forces COMMAND forces_test)

    add_executable(center_of_mass_test tests/center_of_mass.c)
    target_link_libraries(center_of_mass_test PRIVATE rohr_engine)
    set_target_properties(center_of_mass_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME center_of_mass COMMAND center_of_mass_test)

    add_executable(origin_offsets_test tests/origin_offsets.c)
    target_link_libraries(origin_offsets_test PRIVATE rohr_engine)
    set_target_properties(origin_offsets_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME origin_offsets COMMAND origin_offsets_test)

    add_executable(concave_collision_test tests/concave_collision.c)
    target_include_directories(concave_collision_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(concave_collision_test PRIVATE rohr_engine)
    set_target_properties(concave_collision_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME concave_collision COMMAND concave_collision_test)

    add_executable(rigid_contact_stability_test
        tests/rigid_contact_stability.c)
    target_include_directories(rigid_contact_stability_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(rigid_contact_stability_test PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(rigid_contact_stability_test PRIVATE
            ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(rigid_contact_stability_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests)
    add_test(NAME rigid_contact_stability COMMAND rigid_contact_stability_test)

    add_executable(extreme_mass_drop_test tests/extreme_mass_drop.c
        examples/extreme_mass_drop/src/drop_fixture.c)
    target_include_directories(extreme_mass_drop_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/extreme_mass_drop/src)
    target_link_libraries(extreme_mass_drop_test PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(extreme_mass_drop_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    if(WIN32)
        set(drop_test_platform windows)
    elseif(ROHR_ENABLE_SANITIZERS)
        set(drop_test_platform sanitizers)
    else()
        set(drop_test_platform linux)
    endif()
    set_target_properties(extreme_mass_drop_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_SOURCE_DIR}/sandbox/milestone_6/goal_1/bin/${drop_test_platform})
    if(ROHR_EXAMPLE_OUTPUT_DIRECTORY)
        set_target_properties(extreme_mass_drop_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ROHR_EXAMPLE_OUTPUT_DIRECTORY})
    endif()
    add_test(NAME extreme_mass_drop COMMAND extreme_mass_drop_test)
    set_tests_properties(extreme_mass_drop PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 120)

    add_executable(thin_barriers_test tests/thin_barriers.c)
    target_include_directories(thin_barriers_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(thin_barriers_test PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(thin_barriers_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(thin_barriers_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_SOURCE_DIR}/sandbox/milestone_6/goal_2/bin/${drop_test_platform})
    add_test(NAME thin_barriers COMMAND thin_barriers_test)
    set_tests_properties(thin_barriers PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 120)

    add_executable(persistent_stack_test tests/persistent_stack.c)
    target_include_directories(persistent_stack_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(persistent_stack_test PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(persistent_stack_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(persistent_stack_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_SOURCE_DIR}/sandbox/milestone_6/goal_3/bin/${drop_test_platform})
    add_test(NAME persistent_stack COMMAND persistent_stack_test)
    set_tests_properties(persistent_stack PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 300)

    foreach(joint_kind pin weld spring)
        add_executable(${joint_kind}_stability_test tests/${joint_kind}_stability.c)
        target_include_directories(${joint_kind}_stability_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(${joint_kind}_stability_test PRIVATE rohr_engine)
        if(ROHR_ENABLE_SANITIZERS)
            target_compile_options(${joint_kind}_stability_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
        endif()
        set_target_properties(${joint_kind}_stability_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
            ${CMAKE_SOURCE_DIR}/sandbox/milestone_6/goal_4/bin/${drop_test_platform})
        add_test(NAME ${joint_kind}_stability COMMAND ${joint_kind}_stability_test)
        set_tests_properties(${joint_kind}_stability PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 300)
    endforeach()

    set(ROHR_SOFT_TEST_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/sandbox/milestone_6/goal_5/bin/${drop_test_platform}"
        CACHE PATH "Output directory for soft-body verification executables")
    foreach(soft_test soft_body_stability beam_pair_contacts beam_motion particle_sweep)
    add_executable(${soft_test}_test tests/${soft_test}.c)
    target_include_directories(${soft_test}_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${soft_test}_test PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(${soft_test}_test PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set_target_properties(${soft_test}_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        ${ROHR_SOFT_TEST_OUTPUT_DIRECTORY})
    add_test(NAME ${soft_test} COMMAND ${soft_test}_test)
    set_tests_properties(${soft_test} PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 600)
    endforeach()

    # Opt-in scaling diagnostic; the default invocation opens an interactive view.
    add_executable(particle_pit EXCLUDE_FROM_ALL tests/particle_pit.c)
    target_link_libraries(particle_pit PRIVATE rohr_engine)
    if(ROHR_ENABLE_SANITIZERS)
        target_compile_options(particle_pit PRIVATE ${ROHR_SANITIZER_COMPILE_OPTIONS})
    endif()
    set(ROHR_PHYSICS_DIAGNOSTIC_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/sandbox/milestone_11/goal_1/bin/${drop_test_platform}"
        CACHE PATH "Output directory for opt-in physics diagnostics")
    set_target_properties(particle_pit PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        "${ROHR_PHYSICS_DIAGNOSTIC_OUTPUT_DIRECTORY}")

    add_executable(component_registry_test
        tests/component_registry.c
    )
    target_link_libraries(component_registry_test PRIVATE rohr_tools)
    set_target_properties(component_registry_test
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )

    add_test(
        NAME component_registry
        COMMAND component_registry_test
    )

    set(ROHR_GENERATED_COMPONENT_FIXTURE_DIR
        ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests/generated_components
    )
    set(ROHR_GENERATED_COMPONENT_HEADER
        ${ROHR_GENERATED_COMPONENT_FIXTURE_DIR}/game_components.h
    )
    set(ROHR_GENERATED_COMPONENT_SOURCE
        ${ROHR_GENERATED_COMPONENT_FIXTURE_DIR}/game_components.c
    )

    rohr_add_component_generator(component_fixture_generator
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_component_fixture.c")

    add_custom_command(
        OUTPUT
            ${ROHR_GENERATED_COMPONENT_HEADER}
            ${ROHR_GENERATED_COMPONENT_SOURCE}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            ${ROHR_GENERATED_COMPONENT_FIXTURE_DIR}
        COMMAND
            ${component_fixture_generator_COMMAND}
            ${ROHR_GENERATED_COMPONENT_HEADER}
            ${ROHR_GENERATED_COMPONENT_SOURCE}
        DEPENDS component_fixture_generator
        COMMENT "Generating sparse-set component fixture"
        VERBATIM
    )

    add_executable(generated_components_test
        tests/generated_components_runtime.c
        ${ROHR_GENERATED_COMPONENT_SOURCE}
        ${ROHR_GENERATED_COMPONENT_HEADER}
    )
    target_include_directories(generated_components_test
        PRIVATE ${ROHR_GENERATED_COMPONENT_FIXTURE_DIR}
    )
    target_link_libraries(generated_components_test PRIVATE rohr_engine)
    set_target_properties(generated_components_test
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ROHR_BUILD_OUTPUT_DIRECTORY}/tests
    )

    add_test(NAME generated_components COMMAND generated_components_test)
    add_test(
        NAME generated_components_deterministic
        COMMAND
            ${CMAKE_COMMAND}
            -DGENERATOR=${component_fixture_generator_COMMAND}
            -DOUTPUT_DIRECTORY=${ROHR_BUILD_OUTPUT_DIRECTORY}/tests/deterministic_components
            -P ${CMAKE_SOURCE_DIR}/tests/check_component_generation.cmake
    )

    set_tests_properties(
        physics_recovery
        physics_reports
        audio
        engine_lifecycle
        engine_reports
        directory
        input_actions
        cameras
        ui_instance_isolation
        graphics_layer_order
        window_presentation
        texture_assets
        animation_assets
        font_assets
        text_assets
        timing
        ui_reports
        ui_field
        ui_field_selection
        ui_dropdown
        joints
        soft_body
        auto_shape
        collision_sensor
        collision_response
        particle_collision_mode
        hitbox_animation_binding
        entity_pair_set
        aabb_tree
        physics_interaction_set
        constraint_solver
        contact_manifold
        origin_offsets
        center_of_mass
        forces
        shared_sources
        runtime_persistence
        object_instances
        angles
        rigid_contact_stability
        component_registry
        generated_components
        PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy"
    )

    add_custom_target(rohr_tests)
    add_dependencies(rohr_tests contact_gathering_test)
    add_dependencies(rohr_tests soft_body_stability_test beam_pair_contacts_test beam_motion_test particle_sweep_test)
    add_dependencies(rohr_tests extreme_mass_drop_test)
    add_dependencies(rohr_tests thin_barriers_test)
    add_dependencies(rohr_tests persistent_stack_test)
    add_dependencies(rohr_tests pin_stability_test weld_stability_test spring_stability_test)
    add_dependencies(
        rohr_tests
        entity_info_test
        entity_info_adversarial_test
        report_test
        report_output_test
        system_reports_test
        report_storage_test
        graphics_reports_test
        physics_reports_test
        physics_recovery_test
        audio_reports_test
        audio_test
        engine_lifecycle_test
        engine_reports_test
        directory_test
        input_actions_test
        cameras_test
        camera_descriptions_test
        viewport_descriptions_test
        ui_instance_isolation_test
        graphics_layer_order_test
        window_presentation_test
        texture_assets_test
        animation_assets_test
        font_assets_test
        text_assets_test
        timing_test
        ui_reports_test
        ui_field_test
        ui_field_selection_test
        ui_dropdown_test
        joints_test
        soft_body_test
        beam_materials_test
        beam_contacts_test
        soft_body_mass_test
        soft_body_angular_velocity_test
        soft_body_frame_test
        soft_body_areas_test
        node_loop_geometry_test
        auto_shape_test
        collision_sensor_test
        collision_response_test
        particle_collision_mode_test
        hitbox_animation_binding_test
        entity_pair_set_test
        aabb_tree_test
        physics_interaction_set_test
        constraint_solver_test
        contact_manifold_test
        origin_offsets_test
        center_of_mass_test
        forces_test
        shared_sources_test
        runtime_persistence_test
        object_instances_test
        angles_test
        example_flies_angles_test
        example_soft_body_angles_test
        center_of_mass_persistence_test
        concave_collision_test
        rigid_contact_stability_test
        component_registry_test
        generated_components_test
    )
endif()

if(ROHR_ENABLE_DOCUMENTATION)
    find_package(Doxygen QUIET)
    find_package(Perl QUIET)
    find_program(PANDOC_EXECUTABLE pandoc)

    if(DOXYGEN_FOUND AND PERL_FOUND AND PANDOC_EXECUTABLE)
        set(ROHR_DOCS_OUTPUT ${CMAKE_SOURCE_DIR}/build/docs)
        set(ROHR_STATIC_OUTPUT ${CMAKE_SOURCE_DIR}/build/static)
        set(ROHR_README_HTML ${ROHR_DOCS_OUTPUT}/readme.html)
        set(ROHR_STATIC_README_HTML ${ROHR_STATIC_OUTPUT}/readme.html)

        add_custom_target(generate_engine_api
            COMMAND
                ${PERL_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/generate_public_api_markdown.pl
                ${CMAKE_SOURCE_DIR}/include/rohr.h
                ${CMAKE_SOURCE_DIR}/docs/engine_api.md
                rohr_
            DEPENDS
                ${CMAKE_SOURCE_DIR}/include/rohr.h
                ${CMAKE_SOURCE_DIR}/scripts/generate_public_api_markdown.pl
            COMMENT "Generating engine API Markdown"
            VERBATIM
        )

        add_custom_target(generate_tools_api
            COMMAND
                ${PERL_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/generate_public_api_markdown.pl
                ${CMAKE_SOURCE_DIR}/include/rohr_tools.h
                ${CMAKE_SOURCE_DIR}/docs/tools_api.md
                rohr_tools_
            DEPENDS
                ${CMAKE_SOURCE_DIR}/include/rohr_tools.h
                ${CMAKE_SOURCE_DIR}/scripts/generate_public_api_markdown.pl
            COMMENT "Generating tools API Markdown"
            VERBATIM
        )

        add_custom_target(generate_public_api)
        add_dependencies(generate_public_api generate_engine_api generate_tools_api)

        add_custom_command(
            OUTPUT ${ROHR_README_HTML}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ROHR_DOCS_OUTPUT}
            COMMAND
                ${PANDOC_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/README.md
                --standalone
                --embed-resources
                --metadata title=Rohr\ Engine
                -o ${ROHR_README_HTML}
            DEPENDS
                ${CMAKE_SOURCE_DIR}/README.md
                ${CMAKE_SOURCE_DIR}/docs/README.md
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_in_pit.gif
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_around_ball.gif
            COMMENT "Rendering README HTML"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${ROHR_STATIC_README_HTML}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ROHR_STATIC_OUTPUT}/docs/assets
            COMMAND
                ${PANDOC_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/README.md
                --standalone
                --embed-resources
                --metadata title=Rohr\ Engine
                -o ${ROHR_STATIC_README_HTML}
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_in_pit.mp4
                ${ROHR_STATIC_OUTPUT}/docs/assets/flies_in_pit.mp4
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_around_ball.mp4
                ${ROHR_STATIC_OUTPUT}/docs/assets/flies_around_ball.mp4
            DEPENDS
                ${CMAKE_SOURCE_DIR}/README.md
                ${CMAKE_SOURCE_DIR}/docs/README.md
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_in_pit.gif
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_around_ball.gif
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_in_pit.mp4
                ${CMAKE_SOURCE_DIR}/docs/assets/flies_around_ball.mp4
            COMMENT "Rendering standalone README preview"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM
        )

        add_custom_target(readme_html DEPENDS ${ROHR_README_HTML})
        add_custom_target(static_readme DEPENDS ${ROHR_STATIC_README_HTML})

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS generate_public_api readme_html static_readme
            COMMENT "Generating Doxygen documentation"
            VERBATIM
        )
    else()
        message(WARNING
            "Documentation targets are disabled because Doxygen, Perl, or Pandoc was not found"
        )
    endif()
endif()
