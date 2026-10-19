/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "../test_png.h"
#include <assert.h>
#include <string.h>

int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    const char *path = "asset_description_sdk.png";
    assert(SDL_SaveFile(path, test_png, sizeof(test_png)));
    TextureDescription texture_description = rohr_graphics_texture_description_default_get();
    texture_description.file = path;
    texture_description.size = (Scale){12, 34};
    TextureAssetResult texture = rohr_graphics_texture_load(texture_description);
    assert(!rohr_error_check(texture));
    TextureDescriptionResult inspected = rohr_graphics_texture_description_get(texture.result.value);
    assert(!rohr_error_check(inspected) && inspected.result.value.size.x == 12);
    assert(inspected.result.value.file != NULL);
    FontAssetResult font = rohr_graphics_font_load(rohr_graphics_font_description_default_get());
    assert(!rohr_error_check(font));
    assert(rohr_graphics_font_description_get(font.result.value).result.value.file == NULL);
    AnimationDescription description = rohr_graphics_animation_description_default_get();
    description.frame_count = 1;
    description.frame_files[0] = path;
    AnimationAssetResult animation = rohr_graphics_animation_load(description);
    assert(!rohr_error_check(animation));
    AnimationDescriptionResult catalog = rohr_graphics_animation_description_get(animation.result.value);
    assert(!rohr_error_check(catalog) && catalog.result.value.frame_count == 1);
    assert(catalog.result.value.frame_ids[0] == 1);
    TextDescription label = rohr_graphics_text_description_default_get();
    label.value = "hello world";
    label.wrap_width = 40;
    TextAssetResult text = rohr_graphics_text_create(label);
    assert(!rohr_error_check(text) && text.result.value.size.y == 16);
    TextDescriptionResult current = rohr_graphics_text_description_get(text.result.value);
    assert(!rohr_error_check(current) && strcmp(current.result.value.value, label.value) == 0);
    current.result.value.wrap_width = 0;
    assert(!rohr_error_check(rohr_graphics_text_description_set(&text.result.value, current.result.value)));
    assert(text.result.value.size.y == 8);
    assert(!rohr_error_check(rohr_graphics_text_destroy(&text.result.value)));
    assert(!rohr_error_check(rohr_graphics_font_release(&font.result.value)));
    assert(!rohr_error_check(rohr_graphics_animation_release(&animation.result.value)));
    assert(!rohr_error_check(rohr_graphics_texture_release(&texture.result.value)));
    assert(SDL_RemovePath(path));
    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
