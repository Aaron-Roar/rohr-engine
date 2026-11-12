# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
foreach(required CLI PROBE FIXTURE SDK_PREFIX)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${ARGV}\n${result}\n${output}\n${error}")
    endif()
endfunction()
function(blocked root)
    execute_process(COMMAND "${PROBE}" resolve "${root}" app "${SDK_PREFIX}" RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0)
        message(FATAL_ERROR "Relocated application was runnable before reconfiguration: ${output}")
    endif()
endfunction()
function(check_cache root)
    file(READ "${root}/build/CMakeCache.txt" cache)
    if(NOT cache MATCHES "CMAKE_HOME_DIRECTORY:INTERNAL=${root}\n" OR
            NOT cache MATCHES "CMAKE_BUILD_TYPE:STRING=Debug\n" OR cache MATCHES "C2_CACHE_ONLY")
        message(FATAL_ERROR "Cache did not recover saved settings at ${root}")
    endif()
    if(EXISTS "${root}/build/rohr_reconfigure_required.txt")
        message(FATAL_ERROR "Recovery marker was not cleared")
    endif()
endfunction()
file(REMOVE_RECURSE "${FIXTURE}")
set(original "${FIXTURE}/original project")
set(copied "${FIXTURE}/copied project")
set(moved "${FIXTURE}/moved project")
run("${CLI}" --project "${original}" create)
# A developer-owned entry point exercises generated objects and exits predictably.
file(WRITE "${original}/src/main.c" [=[
#include "rohr.h"
#include "project.h"
#include <assert.h>
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    assert(!rohr_error_check(rohr_directory_working_set(rohr_directory_base_get())));
    EntityResult example = rohr_object_create(project.example);
    assert(!rohr_error_check(example));
    assert(!rohr_error_check(rohr_entity_delete(example.result.value)));
    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
]=])
file(WRITE "${original}/editor.lua"
    "return {build={project={configure={'cmake','-S','{project}','-B','{build}','-DCMAKE_BUILD_TYPE=Debug','-DCMAKE_PREFIX_PATH=${SDK_PREFIX}'}}}}\n")
file(APPEND "${original}/CMakeLists.txt" "\n# Developer customization must survive relocation.\n")
run("${PROBE}" project "${original}" app "${SDK_PREFIX}")
run("${PROBE}" project "${original}" scene "${SDK_PREFIX}")
file(WRITE "${original}/build/keep_user_file.txt" "keep this file\n")
file(APPEND "${original}/build/CMakeCache.txt" "\nC2_CACHE_ONLY:STRING=preserve_until_moved\n")
file(READ "${original}/build/rohr_application_Debug.path" executable)
file(TIMESTAMP "${executable}" before "%s")
run("${CMAKE_COMMAND}" -E sleep 1)
run("${CLI}" --project "${original}" compile)
file(READ "${original}/build/CMakeCache.txt" unchanged_cache)
if(NOT unchanged_cache MATCHES "C2_CACHE_ONLY")
    message(FATAL_ERROR "Unchanged build discarded cache-only options")
endif()
file(TIMESTAMP "${executable}" after "%s")
if(NOT before STREQUAL after)
    message(FATAL_ERROR "Unchanged build discarded valid cache or recompiled the application")
endif()
# A real source edit rebuilds successfully while retaining the cache-only setting.
file(APPEND "${original}/src/main.c" "\n/* developer edit */\n")
run("${CLI}" --project "${original}" compile)
file(READ "${original}/build/CMakeCache.txt" cache)
if(NOT cache MATCHES "C2_CACHE_ONLY")
    message(FATAL_ERROR "Ordinary source edit cleared the cache")
endif()
# An authored scene edit regenerates and builds through the same retained cache.
file(READ "${original}/objects/project.rohr.json" document)
string(JSON document SET "${document}" objects 0 position x 5)
file(WRITE "${original}/objects/project.rohr.json" "${document}\n")
run("${CLI}" --project "${original}" generate-c)
run("${PROBE}" project "${original}" app "${SDK_PREFIX}")
run("${PROBE}" project "${original}" scene "${SDK_PREFIX}")
file(READ "${original}/build/CMakeCache.txt" cache)
if(NOT cache MATCHES "C2_CACHE_ONLY")
    message(FATAL_ERROR "Authored scene edit cleared the cache")
endif()
file(GLOB_RECURSE asset_files RELATIVE "${original}" "${original}/assets/*")
set(preserved_files src/main.c editor.lua CMakeLists.txt objects/project.rohr.json build/keep_user_file.txt ${asset_files})
foreach(name IN LISTS preserved_files)
    if(EXISTS "${original}/${name}")
        file(SHA256 "${original}/${name}" "hash_${name}")
    endif()
endforeach()
file(SHA256 "${original}/build/CMakeCache.txt" original_cache)
file(COPY "${original}/" DESTINATION "${copied}")
blocked("${copied}")
run("${PROBE}" project "${copied}" app "${SDK_PREFIX}")
run("${PROBE}" project "${copied}" scene "${SDK_PREFIX}")
check_cache("${copied}")
file(SHA256 "${original}/build/CMakeCache.txt" original_cache_after)
if(NOT original_cache STREQUAL original_cache_after)
    message(FATAL_ERROR "Building a copy modified the original cache")
endif()
file(RENAME "${copied}" "${moved}")
blocked("${moved}")
run("${CLI}" --project "${moved}" compile)
check_cache("${moved}")
foreach(name IN LISTS preserved_files)
    if(DEFINED "hash_${name}")
        file(SHA256 "${moved}/${name}" actual)
        if(NOT "${hash_${name}}" STREQUAL actual)
            message(FATAL_ERROR "Relocation changed developer file ${name}")
        endif()
    endif()
endforeach()
foreach(root "${original}" "${moved}")
    foreach(target app scene)
        run("${CMAKE_COMMAND}" -E env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
            "${PROBE}" launch "${root}" "${target}" "${SDK_PREFIX}")
    endforeach()
endforeach()
# Relative CLI project paths resolve from the caller, not a cached old root.
execute_process(COMMAND "${CLI}" --project "moved project" compile WORKING_DIRECTORY "${FIXTURE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Relative project build failed: ${output}${error}")
endif()
