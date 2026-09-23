# Player controller JSON project

This is a complete Rohr editor project demonstrating a JSON-authored player,
floor, viewport, and logical input controller.

- Hold `A` or `D` to apply horizontal force.
- Press `Space` to apply one upward force while the player has a rigid-body
  contact below it.
- Press `Escape` to exit.

Rohr world coordinates are Y-up, so this project uses negative-Y gravity and
positive-Y jump force.

The player has gravity and locked rotation. Developer-owned gameplay code in
`src/main.c` limits horizontal and vertical speed, applies held movement force,
and implements contact-gated jumping. Static scene data and input declarations
remain editor-owned in `objects/project.rohr.json`.

From the repository root, build it with the other examples:

```sh
./dev.sh build
./build/examples/PlayerController
```

From this directory, generate, build, and run against the source checkout:

```sh
../../build/tools/rohr-cli/rohr-cli --project . generate-c
cmake -S . -B build -DCMAKE_PREFIX_PATH=../../build/example-sdk
cmake --build build
./build/PlayerController
```
