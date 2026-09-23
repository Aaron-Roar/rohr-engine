@echo off
rem Copyright 2026 Aaron Rohrer
rem SPDX-License-Identifier: LGPL-3.0-only

setlocal

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "BUILD_DIRECTORY=%PROJECT_ROOT%\build"
set "EXAMPLE_BUILD_ROOT=%BUILD_DIRECTORY%\example-build"
set "EXAMPLE_OUTPUT_DIRECTORY=%BUILD_DIRECTORY%\examples"
set "EXAMPLE_SDK_DIRECTORY=%BUILD_DIRECTORY%\example-sdk"
set "SDK_BUILD_DIRECTORY=%PROJECT_ROOT%\build\sdk\windows"
set "SDK_DIRECTORY=%PROJECT_ROOT%\dist\windows"
set "OPERATION=%~1"
if "%OPERATION%"=="" set "OPERATION=build"

if "%OPERATION%"=="clean" goto clean
if "%OPERATION%"=="sdk" goto sdk
if not "%OPERATION%"=="build" if not "%OPERATION%"=="test" if not "%OPERATION%"=="sdk" goto usage

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIRECTORY%" -DROHR_BUILD_EXAMPLES=OFF
if errorlevel 1 exit /b %errorlevel%
cmake --build "%BUILD_DIRECTORY%" --config Debug
if errorlevel 1 exit /b %errorlevel%
cmake -E remove_directory "%EXAMPLE_SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake --install "%BUILD_DIRECTORY%" --config Debug --prefix "%EXAMPLE_SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake ^
    "-DROHR_ROOT=%PROJECT_ROOT%" ^
    "-DROHR_EXAMPLE_BUILD_ROOT=%EXAMPLE_BUILD_ROOT%" ^
    "-DROHR_EXAMPLE_OUTPUT_DIRECTORY=%EXAMPLE_OUTPUT_DIRECTORY%" ^
    "-DROHR_SDK_PREFIX=%EXAMPLE_SDK_DIRECTORY%" ^
    -DROHR_BUILD_CONFIG=Debug ^
    -P "%PROJECT_ROOT%\cmake\build_examples.cmake"
if errorlevel 1 exit /b %errorlevel%

if "%OPERATION%"=="test" (
    ctest --test-dir "%BUILD_DIRECTORY%" --build-config Debug --output-on-failure
    exit /b %errorlevel%
)
exit /b 0

:sdk
cmake -E remove_directory "%PROJECT_ROOT%\dist\rohr"
if errorlevel 1 exit /b %errorlevel%
cmake -E remove_directory "%SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake -E make_directory "%SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake -S "%PROJECT_ROOT%" -B "%SDK_BUILD_DIRECTORY%" -DCMAKE_BUILD_TYPE=Release -DROHR_BUILD_EXAMPLES=OFF -DROHR_BUILD_TESTS=OFF -DROHR_BUILD_SDK_CONSUMER_TESTS=ON -DROHR_ENABLE_DOCUMENTATION=OFF -DROHR_PORTABLE_SDK=ON -DROHR_SDK_INSTALL_PREFIX="%SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake --build "%SDK_BUILD_DIRECTORY%" --config Release --parallel
if errorlevel 1 exit /b %errorlevel%
cmake --install "%SDK_BUILD_DIRECTORY%" --config Release --prefix "%SDK_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir "%SDK_BUILD_DIRECTORY%" --build-config Release --output-on-failure -R "^installed_sdk_consumer_windows$"
if errorlevel 1 exit /b %errorlevel%
echo Rohr windows SDK: %SDK_DIRECTORY%
exit /b 0

:clean
cmake -E remove_directory "%BUILD_DIRECTORY%"
if errorlevel 1 exit /b %errorlevel%
cmake -E remove_directory "%PROJECT_ROOT%\dist"
exit /b %errorlevel%

:usage
echo usage: dev.bat [build^|test^|sdk^|clean] 1>&2
exit /b 1
