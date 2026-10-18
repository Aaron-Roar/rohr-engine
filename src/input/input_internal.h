/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef INPUT_INTERNAL_H
#define INPUT_INTERNAL_H

#include <SDL3/SDL.h>

void input_start(void);
void input_stop(void);
void input_event_add(const SDL_Event *event);

void input_host_mode_set(bool hosted);
bool input_host_mode_check(void);
SDL_Rect input_host_text_area_get(int *cursor);

#endif
