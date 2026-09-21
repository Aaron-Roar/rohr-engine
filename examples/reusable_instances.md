# Reusing editor-authored instances

Each object authored in the editor generates a public structure plus create,
draw, and destroy functions. The generated `project_objects_create_all()`
function creates the initial scene, but it is not required for later instances.

For an editor object named `Thing`, game code can create independent instances:

```c
Thing thing_1 = {0};
Thing thing_2 = {0};

if(rohr_error_check(thing_create(&thing_1, (Position){100.0f, 100.0f})) ||
        rohr_error_check(thing_create(&thing_2, (Position){500.0f, 100.0f}))) {
    thing_destroy(&thing_2);
    thing_destroy(&thing_1);
    return 1;
}

/* This changes only thing_1's authored body. */
(void)rohr_physics_position_set(thing_1.body, (Position){200.0f, 200.0f});

thing_draw(&thing_1);
thing_draw(&thing_2);

thing_destroy(&thing_2);
thing_destroy(&thing_1);
```

The generated structure holds the entities, joints, anchors, soft-body parts,
sprites, animations, and cameras belonging to that instance. Internal
references are rebuilt by each call to `thing_create()`, so the two structures
do not share mutable component handles.

UI definitions may also be mounted more than once in the editor. Generated
runtime state stores every mount separately:

```c
ProjectViewports viewports = {0};

if(rohr_error_check(project_viewports_create(&viewports, &objects))) return 1;

/* Each mount has its own mutable visual resource and placement handle. */
(void)rohr_graphics_ui_slider_value_set(viewports.ui_elements[0], 75.0f);
(void)rohr_viewport_item_set(viewports.ui_items[0],
    (ViewportItemConfig){.rectangle = {40.0f, 40.0f, 0.0f, 0.0f},
        .content_scale = {1.0f, 1.0f}, .visible = true});
```

Changing `ui_elements[0]` or `ui_items[0]` does not change another generated
mount. Fonts and immutable source assets may still be shared; mutable runtime
UI configuration and placement are instance-owned.
