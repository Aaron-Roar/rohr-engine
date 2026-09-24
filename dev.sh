#!/usr/bin/env sh
# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_directory="$project_root/build"
example_build_root="$build_directory/example-build"
example_output_directory="$build_directory/examples"
example_sdk_directory="$build_directory/example-sdk"
asset_sanitizer_build_directory="$build_directory/sanitizers/assets"
operation=${1:-build}

needs_dev_shell=true
case "$operation" in
    sdk|sdk-linux|sdk-nix)
        needs_dev_shell=false
        ;;
esac

if [ "$needs_dev_shell" = true ] && [ -z "${ROHR_DEV_SHELL:-}" ]; then
    if ! command -v nix >/dev/null 2>&1; then
        echo "Error: Nix is required to enter the Rohr development environment." >&2
        echo "Install Nix, then run: $0 $*" >&2
        exit 1
    fi

    exec nix develop "$project_root" -c "$0" "$@"
fi

configure() {
    cmake -S "$project_root" -B "$build_directory" \
        -DROHR_BUILD_EXAMPLES=OFF
}

build_examples() {
    cmake -E remove_directory "$example_sdk_directory"
    cmake --install "$build_directory" --prefix "$example_sdk_directory"
    cmake \
        -DROHR_ROOT="$project_root" \
        -DROHR_EXAMPLE_BUILD_ROOT="$example_build_root" \
        -DROHR_EXAMPLE_OUTPUT_DIRECTORY="$example_output_directory" \
        -DROHR_SDK_PREFIX="$example_sdk_directory" \
        -P "$project_root/cmake/build_examples.cmake"
}

build() {
    configure
    cmake --build "$build_directory"
    build_examples
}

test_assets_sanitized() {
    cmake -S "$project_root" -B "$asset_sanitizer_build_directory" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DROHR_BUILD_EXAMPLES=OFF \
        -DROHR_BUILD_EDITOR=OFF \
        -DROHR_BUILD_TERMINAL=OFF \
        -DROHR_BUILD_CLI=OFF \
        -DROHR_BUILD_TESTS=ON \
        -DROHR_ENABLE_DOCUMENTATION=OFF \
        -DROHR_ENABLE_SANITIZERS=ON
    cmake --build "$asset_sanitizer_build_directory" --parallel \
        --target texture_assets_test animation_assets_test font_assets_test \
            text_assets_test
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
        UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        SDL_VIDEODRIVER=dummy \
        SDL_AUDIODRIVER=dummy \
        ctest --test-dir "$asset_sanitizer_build_directory" \
            --output-on-failure \
            --no-tests=error \
            -R '^(texture_assets|animation_assets|font_assets|text_assets)$'
}

sdk_build() {
    platform=$1
    sdk_build_directory="$project_root/build/sdk/$platform"
    sdk_directory="$project_root/dist/$platform"
    shift

    cmake -E remove_directory "$project_root/dist/rohr"
    cmake -E remove_directory "$sdk_directory"
    cmake -S "$project_root" -B "$sdk_build_directory" \
        -DCMAKE_BUILD_TYPE=Release \
        -DROHR_BUILD_EXAMPLES=OFF \
        -DROHR_BUILD_TESTS=OFF \
        -DROHR_BUILD_SDK_CONSUMER_TESTS=ON \
        -DROHR_ENABLE_DOCUMENTATION=OFF \
        -DROHR_PORTABLE_SDK=ON \
        -DROHR_SDK_INSTALL_PREFIX="$sdk_directory" \
        "$@"
    cmake --build "$sdk_build_directory" --parallel
    cmake --install "$sdk_build_directory" --prefix "$sdk_directory"
    ctest --test-dir "$sdk_build_directory" \
        --output-on-failure -R "^installed_sdk_consumer_${platform}$"
    echo "Rohr $platform SDK: $sdk_directory"
}

sdk_linux_generic() {
    if command -v docker >/dev/null 2>&1; then
        container_runtime=docker
        set -- --user "$(id -u):$(id -g)"
    elif command -v podman >/dev/null 2>&1; then
        container_runtime=podman
        set --
    else
        echo "Error: Docker or Podman is required for sdk-linux." >&2
        exit 1
    fi

    mkdir -p "$project_root/dist"
    "$container_runtime" build \
        -f "$project_root/packaging/linux/Dockerfile" \
        -t rohr-linux-sdk-builder "$project_root"
    "$container_runtime" run --rm \
        "$@" \
        -v "$project_root:/workspace:ro" \
        -v "$project_root/dist:/dist" \
        rohr-linux-sdk-builder
}

sdk_nix() {
    if ! command -v nix >/dev/null 2>&1; then
        echo "Error: Nix is required for sdk-nix." >&2
        exit 1
    fi
    mkdir -p "$project_root/dist"
    nix build "$project_root#sdk" --out-link "$project_root/dist/nix"
    echo "Rohr Nix SDK: $project_root/dist/nix"
}

sdk_windows_cross() {
    if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
        echo "Error: x86_64-w64-mingw32-gcc is required for sdk-windows." >&2
        exit 1
    fi
    sdk_build windows \
        -DCMAKE_TOOLCHAIN_FILE="$project_root/cmake/toolchains/mingw_w64.cmake" \
        -DROHR_HOST_C_COMPILER="$(command -v cc)"
}

sdk_all() {
    sdk_nix
    sdk_linux_generic
    sdk_windows_cross
}

case "$operation" in
    build)
        build
        ;;
    test)
        build
        ctest --test-dir "$build_directory" --output-on-failure
        ;;
    test-assets-sanitized)
        test_assets_sanitized
        ;;
    sdk)
        sdk_all
        ;;
    sdk-linux)
        sdk_linux_generic
        ;;
    sdk-nix)
        sdk_nix
        ;;
    sdk-windows)
        sdk_windows_cross
        ;;
    clean)
        cmake -E remove_directory "$build_directory"
        cmake -E remove_directory "$project_root/dist"
        ;;
    *)
        echo "usage: ./dev.sh [build|test|test-assets-sanitized|sdk|sdk-linux|sdk-nix|sdk-windows|clean]" >&2
        exit 1
        ;;
esac
