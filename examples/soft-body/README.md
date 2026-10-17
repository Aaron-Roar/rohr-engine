# Authored soft-body car

Open `project.rohr.json` in the editor to inspect the car's chassis, wheels,
soft tires, anchors, and joints. Source definitions live in
`objects/project.rohr.json`. Edit forces through Add Force and Force Properties;
use the CLI for torque definitions until the torque editor is available.

- **A/D:** drive the rear wheel by updating its persistent `rear_drive` torque.
- **Space:** jump with an upward one-tick force at the chassis COM. Both wheels
  must contact external rigid bodies; particle-pit and self-contacts do not count.
  Each press produces at most one jump, and airborne presses are discarded.
- **B:** toggle collision debugging.
- **Escape:** exit.

The source magnitude returns to zero when drive input is released. Jump force is
tuned for the normal 1/240-second physics tick; the existing collision slow-motion
sequence also slows the jump response. An authored zero-magnitude `initial_load`
demonstrates initial-source expiry without changing the car's starting motion.

Regenerate after editing the document:

```sh
rohr-cli --project examples/soft-body generate-c
```

`car_create()` uses the shared object spawner and returns independent entities on
each call. `car_destroy()` deletes the owning instance and its remaining sources.
The normal scene keeps its existing walls, ramp, particle pit, and camera behavior
in developer-owned code. The default project template is unchanged.

Build this example independently with CMake against an installed Rohr SDK. Its
compiled catalog contains the authored data, so launching the executable does
not require the source directory or the JSON document.
