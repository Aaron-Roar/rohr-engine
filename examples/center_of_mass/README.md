# Center of mass and origin offsets

An editable project comparing automatic COM (left) with explicit COM (right).
Both bodies use the same rectangle, authored from local (20, 10) to (100, 50),
mass 12, initial rotation -20.053523 degrees, angular velocity -45.836624 degrees/second,
zero linear velocity, and an anchor at local (100, 30).

| Body | Local COM | Derived inertia |
| --- | --- | --- |
| Automatic | (60, 30) | 8000 |
| Explicit | (30, 15) | 21500 |

White squares mark origins, blue squares mark COMs, and yellow squares mark
anchors. During free rotation the COMs stay fixed while origins and anchors
orbit. Collisions and gravity are disabled so the coordinate behavior is easy
to observe. Escape closes the demo.

The runtime and editor use clockwise degrees. Open this directory in the
editor to change the body Physics properties.
Origin edits preserve all local offsets. Explicit COM edits move only COM;
switching back to Automatic restores the geometry centroid. Editor orientation
edits rotate around the origin; simulated rotation rotates around COM.

All static geometry, mass, COM, anchors, initial motion, camera, and viewport
data are in `objects/project.rohr.json`. The generated modules apply them.
Developer-owned `src/main.c` runs the loop and draws markers from runtime queries.

From the repository root:

```sh
build/tools/rohr-cli/rohr-cli --project examples/center_of_mass generate-c
cmake --install build --prefix "$PWD/build/example-sdk"
cmake -S examples/center_of_mass -B build/example-build/center_of_mass \
  -DCMAKE_PREFIX_PATH="$PWD/build/example-sdk"
cmake --build build/example-build/center_of_mass
build/example-build/center_of_mass/center_of_mass
```

The standalone build needs the adjacent shared example CMake helper, as do
the other bundled examples. It has no source-relative runtime asset dependency.
The root `examples` target and `./dev.sh build` also include this example.

CTest `center_of_mass_example` loads and round-trips this project, compares all
four regenerated modules with the checked-in files, and checks editor inertia
against analytic values. It compares generated and direct-runtime simulation,
anchor motion, invalid geometry rollback, teleports, and fixed-origin orientation
edits. Runtime JSON save/reload is also demonstrated by the `game-state` example.
