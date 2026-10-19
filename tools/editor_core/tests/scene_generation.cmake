# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED FIXTURE)
    message(FATAL_ERROR "Missing scene generation test paths")
endif()
file(REMOVE_RECURSE "${FIXTURE}")
execute_process(COMMAND "${TEST_EXECUTABLE}" "${FIXTURE}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Scene generation test failed: ${result}")
endif()
