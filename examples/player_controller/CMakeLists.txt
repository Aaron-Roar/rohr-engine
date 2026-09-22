cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(PlayerController LANGUAGES C)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(ROHR_PLAYER_CONTROLLER_IN_TREE OFF)
if(TARGET rohr_engine)
    set(ROHR_ENGINE_TARGET rohr_engine)
    set(ROHR_PLAYER_CONTROLLER_IN_TREE ON)
elseif(DEFINED ROHR_ENGINE_SOURCE_ROOT AND
        EXISTS "${ROHR_ENGINE_SOURCE_ROOT}/CMakeLists.txt")
    set(ROHR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ROHR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ROHR_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
    set(ROHR_BUILD_CLI OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ROHR_ENGINE_SOURCE_ROOT}" rohr-engine)
    set(ROHR_ENGINE_TARGET rohr_engine)
else()
    if(DEFINED ROHR_ENGINE_SOURCE_ROOT)
        list(PREPEND CMAKE_PREFIX_PATH "${ROHR_ENGINE_SOURCE_ROOT}")
    endif()
    find_package(Rohr CONFIG REQUIRED)
    set(ROHR_ENGINE_TARGET Rohr::Engine)
endif()

file(GLOB ROHR_GENERATED_SOURCES CONFIGURE_DEPENDS src/generated/*.c)
add_executable(player_controller src/main.c ${ROHR_GENERATED_SOURCES})
set_target_properties(player_controller PROPERTIES OUTPUT_NAME PlayerController)
if(ROHR_PLAYER_CONTROLLER_IN_TREE)
    set_target_properties(player_controller PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ROHR_BUILD_OUTPUT_DIRECTORY}/examples")
endif()
target_include_directories(player_controller PRIVATE src/generated)
target_link_libraries(player_controller PRIVATE ${ROHR_ENGINE_TARGET})
add_custom_target(player_controller_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:player_controller>/assets"
    VERBATIM)
add_dependencies(player_controller player_controller_assets)
