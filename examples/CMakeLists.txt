# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

include(${CMAKE_CURRENT_LIST_DIR}/cmake/rohr_example.cmake)

add_subdirectory(center_of_mass ${CMAKE_BINARY_DIR}/example-build/center_of_mass)
add_subdirectory(audio ${CMAKE_BINARY_DIR}/example-build/audio)
add_subdirectory(view-port ${CMAKE_BINARY_DIR}/example-build/view-port)
add_subdirectory(flies-in-pit ${CMAKE_BINARY_DIR}/example-build/flies-in-pit)
add_subdirectory(flies-around-ball ${CMAKE_BINARY_DIR}/example-build/flies-around-ball)
add_subdirectory(fly-to-finish ${CMAKE_BINARY_DIR}/example-build/fly-to-finish)
add_subdirectory(game-state ${CMAKE_BINARY_DIR}/example-build/game-state)
add_subdirectory(user-interface ${CMAKE_BINARY_DIR}/example-build/user-interface)
add_subdirectory(joints ${CMAKE_BINARY_DIR}/example-build/joints)
add_subdirectory(soft-body ${CMAKE_BINARY_DIR}/example-build/soft-body)
add_subdirectory(pong ${CMAKE_BINARY_DIR}/example-build/pong)
add_subdirectory(player_controller
    ${CMAKE_BINARY_DIR}/example-build/player_controller)
