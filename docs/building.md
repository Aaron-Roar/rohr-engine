# Building and using Rohr

## Build an SDK first

Game developers should build and use an SDK. The normal workflow is:

```text
clone Rohr -> build the SDK for your system -> run rohr-gui -> make your game
```

The SDK contains the editor, CLI, headers, libraries, CMake package, project
templates, configuration, and license notices. You do not need to install SDL
separately.

If Git is new to you, `git clone` downloads the repository and `cd` enters the
downloaded directory:

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
```

Then follow exactly one of the platform sections below.

### Generic Linux SDK

Use this on an x86_64 Linux system when you do not want the Nix package. Docker
or Podman must be installed and running.

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
./dev.sh sdk-linux
./dist/linux/bin/rohr-gui
```

The result is available in both forms:

```text
dist/linux/
dist/rohr-sdk-linux-x86_64.tar.gz
```

The SDK is built inside the older Rocky Linux 8 baseline. Building against an
older glibc avoids accidentally requiring the newer glibc from the developer's
computer and improves compatibility with newer x86_64 glibc-based Linux
distributions.

The unpacked SDK is relocatable as one directory. It still requires normal
Linux facilities such as glibc, Wayland or X11, and working GPU/OpenGL or Vulkan
drivers. It is not intended for musl-only systems.

To test the archive itself:

```sh
mkdir rohr-sdk-test
tar -xzf dist/rohr-sdk-linux-x86_64.tar.gz -C rohr-sdk-test
./rohr-sdk-test/linux/bin/rohr-gui
```

### Nix SDK on Linux

Use this on NixOS or another Linux system with Nix installed. It supports
`x86_64-linux` and `aarch64-linux`.

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
./dev.sh sdk-nix
./dist/nix/bin/rohr-gui
```

The result is:

```text
dist/nix -> /nix/store/...-rohr-sdk-...
```

Nix supplies and wraps the runtime libraries, making this the most reproducible
Linux option. `dist/nix` is a link to the Nix store, not an independent archive.
Do not copy only `rohr-gui` out of it; run it through the package so its wrapped
dependencies remain available.

### Windows SDK

On Windows, install Git, CMake, and Visual Studio 2022 with **Desktop development
with C++**. Open a Visual Studio Developer PowerShell and run:

```powershell
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
dev.bat sdk
dist\windows\bin\rohr-gui.exe
```

The result is:

```text
dist\windows\
```

Keep its `bin`, `include`, `lib`, and `share` directories together. Rohr and SDL
are included, so SDL does not need to be installed separately.

Linux developers preparing a MinGW Windows SDK can instead run:

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
./dev.sh sdk-windows
```

That requires `x86_64-w64-mingw32-gcc` and a native host C compiler. MinGW and
MSVC SDKs use different compiler/runtime families, so games should use the SDK
matching their compiler.

### Build every SDK

On a Linux release machine with Nix, Docker or Podman, MinGW-w64, and a host C
compiler installed:

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
./dev.sh sdk
```

This attempts the generic Linux, Nix Linux, and Windows distributions. Most
developers only need the single SDK command for their system.

Every SDK build installs the exported CMake package, installs the license
notices, and builds a separate consumer project to verify that an external game
can link against the finished SDK.
Nix and generic Linux packaging execute the installed consumers as well;
cross-built Windows consumers are compiled without claiming native execution.

## Make a game with the SDK

For a screenshot-guided walkthrough written for a first-time programmer, follow
[Your first Rohr project](getting_started.md).

Start the `rohr-gui` binary inside the SDK, create a project, save it, and choose
**Build > Build Project**. This generates game C under `src/generated/`,
configures the project, and compiles it.

Generated game output is not LGPL-covered Rohr source. It may be used, modified,
and distributed without restriction.

The CLI equivalent is:

```sh
/path/to/sdk/bin/rohr-cli --project /path/to/game generate-c
/path/to/sdk/bin/rohr-cli --project /path/to/game build
```

If the terminal is already inside the game project:

```sh
cd /path/to/game
/path/to/sdk/bin/rohr-cli build
```

`generate-c` replaces only `src/generated/`. `compile` configures when needed
and invokes the configured compiler command. `build` generates C and then
compiles.

New projects include an **Example** scene and three complete alternatives in
`src/main.c`, separated by numbered comment dividers:

1. **Project creation** is active by default. `rohr_project_create(project)`
   creates the authored hierarchy, binds its viewports, creates its input
   controllers, and loads its audio without playing it. Destroy the project
   instance once; handles retrieved from it are borrowed.
2. **Object creation** creates the chosen hierarchy. The example explicitly
   binds its camera to a separately owned viewport and cleans up both.
3. **Entity creation** creates selected members individually. Optional links to
   members outside the creation scope stay unbound; required physics dependencies
   must still be supplied. The example binds its camera and cleans up each owner.

To try an alternative, comment out the active main and uncomment one complete
alternative. Each main shows startup, checks, the frame loop, and cleanup directly,
without local utility functions or wrapper macros. Apply authored engine/window
settings once at startup using `project_settings_apply()`, as each path shows.
Project creation does not reset global settings. Runtime
screens and buttons can still be added to any created viewport.

Edit `src/main.c` for your game; keep editor-authored recipes in `src/generated/`.
Regeneration preserves developer-edited main and build files. Multiple project
instances have independent owned resources but share the engine's physics world
and presentation services; they are not isolated worlds. See
[creation scope and ownership](scoped_creation_contract.md).

When upgrading to the shared creation APIs, rebuild the SDK and regenerate the
project catalogs together. Developer-edited mains require manual source migration;
regeneration preserves them. See [shared creation and migration](shared_creation.md)
for value-style calls, named resources, ownership, and the verified SDK matrix.

### Hand-written CMake game

The SDK exports `Rohr::Engine`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_game C)

find_package(Rohr CONFIG REQUIRED)
add_executable(my_game src/main.c)
target_link_libraries(my_game PRIVATE Rohr::Engine)
```

Point CMake at the SDK directory:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/sdk
cmake --build build
```

## Build and test the engine repository

The commands below are for people changing Rohr itself. Game developers should
use one of the SDK workflows above.

### Linux contributor build

Install Git and Nix, then run:

```sh
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
./dev.sh build
./dev.sh test
```

`dev.sh` enters `nix develop` automatically. The output is organized as:

```text
build/
├── example-build/       # independent CMake build tree for each example
├── example-sdk/         # locally staged Rohr SDK consumed by examples
├── examples/
├── tests/
└── tools/
    ├── rohr-cli/rohr-cli
    └── rohr-gui/rohr-gui
```

Run the focused texture, animation, font, text, and audio ownership tests with
Linux address, leak, and undefined-behavior checks using:

```sh
./dev.sh test-assets-sanitized
```

This command uses the isolated `build/sanitizers/assets/` build tree and does
not replace the normal test suite. It is supported on Linux with GCC or Clang.
The tests' deterministic registry counters verify logical ownership transitions,
while the sanitizers independently detect native memory leaks, invalid memory
access, and undefined behavior.

The public-header inventory is checked automatically when the existing Perl
documentation tool is available. Run just its audit and validation cases with
`ctest --test-dir build -R '^api_inventory' --output-on-failure`.
See the [inventory workflow](README.md#api-inventory-check) for direct use.

Installed-SDK consumer builds also register their twelve executables with CTest
on native platforms. Configure `tests/installed_sdk_consumer` against an installed
SDK, build it, then run `ctest --test-dir <consumer-build> --output-on-failure`.
Tests use isolated build-tree working directories and SDL dummy drivers. The
engine's `ROHR_BUILD_SDK_CONSUMER_TESTS` workflow now builds and executes these
checks for native targets; cross-builds compile them without claiming execution.

The engine is built once and installed into `build/example-sdk/`. Each example
is then configured independently through its own `CMakeLists.txt` and resolves
`Rohr::Engine` from that staged SDK. This validates the standalone consumer
configuration without recompiling the engine for every example.

The independent generated-project fixture checks that editor/CLI-authored
degrees survive JSON and C generation, then produce the same COM motion as
direct API calls. With the development build configured in `build/`, run:

```sh
nix develop
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure -R '^center_of_mass_persistence$'
cmake --install build --prefix "$PWD/build/example-sdk"
cmake -S tools/editor_core/tests/generated_com -B build/generated_com_sdk \
  -DCMAKE_PREFIX_PATH="$PWD/build/example-sdk" \
  -DCOM_GENERATED_DIR="$PWD/build/tools/editor_core/com_persistence_generated/src/generated"
cmake --build build/generated_com_sdk --parallel 4
ctest --test-dir build/generated_com_sdk --output-on-failure
```

The fixture verifies an initial -450-degree orientation, 810 degrees/second,
and a full second of COM-based motion without wrapping. For Windows
cross-compilation, use the Windows SDK prefix and toolchain file with the same
generated directory; execute its test only on a compatible runtime.

Run development binaries from the repository root:

```sh
./build/tools/rohr-gui/rohr-gui
./build/examples/pong
./build/examples/joints
./build/examples/soft_body
./build/examples/audio
```

Remove generated development and SDK output with:

```sh
./dev.sh clean
```

### Windows contributor build

Open a Visual Studio 2022 Developer PowerShell:

```powershell
git clone https://github.com/Aaron-Roar/rohr-engine.git
cd rohr-engine
dev.bat build
dev.bat test
```

### Direct CMake workflow

Contributors who need direct control can use the Linux presets:

```sh
nix develop
cmake --preset linux
cmake --build --preset linux --parallel
ctest --preset linux
```

Preset CMake state is under `build/cmake/linux`, while runnable artifacts remain
under `build/`.

Useful focused commands are:

```sh
cmake --build --preset linux-examples --parallel
cmake --build --preset linux-tests --parallel
cmake --build build/cmake/linux --target pong
cmake --build build/cmake/linux --target docs
cmake --build build/cmake/linux --target help
```

Do not run `cmake --build build` unless `build/` was configured by
`./dev.sh build` or `cmake -S . -B build`.

On native Windows, the equivalent preset workflow is:

```powershell
cmake --preset windows
cmake --build --preset windows-debug --parallel
ctest --preset windows
```

## Optional tools

`ffmpeg` is only required for recording and video-generation workflows. It is
not required to run the editor, build a normal game, or link against Rohr.
