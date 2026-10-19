# Rohr Engine Documentation

Rohr Engine is a lightweight C and SDL3 game engine built around explicit
engine systems, stable entity ids, component tables, object pools, and a small
error/result API.

Application code should prefer the public facade:

```c
#include "rohr.h"
```

The facade exposes `rohr_`-prefixed functions while the engine keeps its
smaller internal module headers and implementations.

## Public API

- [Engine API reference](engine_api.md)
- [Description API contract and migration inventory](description_api_contract.md)
- [Entity inspection and snapshot ownership](entity_inspection_contract.md)
- [Tools API reference](tools_api.md)

## Editor

- [Your first Rohr project](getting_started.md)
- [Using the editor](editor.md)
- [Editor architecture](editor_architecture.md)

## Local Generation

From the project root:

```sh
nix develop
cmake --preset linux
cmake --build build/cmake/linux --target docs
```

The generated HTML is written to:

```text
build/docs/html/index.html
```

The `docs` target also refreshes the committed Markdown API references. To
regenerate only those references, run:

```sh
cmake --build build/cmake/linux --target generate_public_api
```

Generated HTML is not committed. The Markdown API references are committed and
are generated from the Doxygen comments in the public headers under `include/`.

## API inventory check

The description API inventory covers every callable declaration and facade alias
in the installed first-party headers, including declarations without Doxygen
comments. From the repository root, run its permanent audit:

```sh
perl scripts/verify_api_inventory.pl
```

From another working directory, use the script's full path. An optional argument
selects another repository root. With Perl available,
`ROHR_BUILD_TESTS` registers `api_inventory` and `api_inventory_validation` in
CTest. The validation test exercises missing/stale entries, duplicates, counts,
configuration exemptions, aliases, and newly added headers. If Perl is unavailable,
CMake explicitly reports that these documentation tests are disabled. No new
runtime or SDK dependency is introduced.

```sh
ctest --test-dir build -R '^api_inventory' --output-on-failure
```

The audit compares names and classifications, not C signatures or ABI compatibility.
Update the inventory deliberately when an API is added, removed, or reclassified;
the audit never edits it automatically. SDK compilation/execution verifies the
compiled interfaces separately.

## Useful Pages

- [Your first Rohr project](getting_started.md)
- [Engine API reference](engine_api.md)
- [Entity inspection and snapshot ownership](entity_inspection_contract.md)
- [Tools API reference](tools_api.md)
- [Architecture](architecture.md)
- [Audio](audio.md)
- [Input](input.md)
- [Physics](physics.md)
- [Shared reports](reports.md)
- [Angles and format migration](angle_convention_contract.md)
- [Building and SDK usage](building.md)
- [Using the editor](editor.md)
- [Editor architecture](editor_architecture.md)
- [Entity ids](entity_ids.md)
- [Error handling](errors.md)
