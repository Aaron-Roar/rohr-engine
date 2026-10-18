/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "scene_transport.h"
#include "input.h"
#include <math.h>
#include <string.h>

bool scene_input_encode(const SDL_Event *event, unsigned char data[SCENE_INPUT_BYTES]) {
    if(!event || !data) return false;
    memset(data, 0, SCENE_INPUT_BYTES); scene_u32_write(data, event->type);
    switch(event->type) {
        case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
            if(event->key.scancode <= SDL_SCANCODE_UNKNOWN || event->key.scancode >= SDL_SCANCODE_COUNT) return false;
            scene_u32_write(data + 4, event->key.scancode);
            scene_u32_write(data + 8, event->key.mod);
            scene_u32_write(data + 12, event->key.repeat); break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
            if(event->button.button < INPUT_MOUSE_BUTTON_LEFT || event->button.button >= INPUT_MOUSE_BUTTON_COUNT) return false;
            scene_u32_write(data + 4, event->button.button);
            scene_double_write(data + 16, event->button.x); scene_double_write(data + 24, event->button.y); break;
        case SDL_EVENT_MOUSE_MOTION:
            scene_double_write(data + 16, event->motion.x); scene_double_write(data + 24, event->motion.y);
            scene_double_write(data + 32, event->motion.xrel); scene_double_write(data + 40, event->motion.yrel); break;
        case SDL_EVENT_MOUSE_WHEEL:
            scene_double_write(data + 16, event->wheel.x); scene_double_write(data + 24, event->wheel.y);
            scene_double_write(data + 32, event->wheel.mouse_x); scene_double_write(data + 40, event->wheel.mouse_y);
            scene_u32_write(data + 4, event->wheel.direction); break;
        case SDL_EVENT_TEXT_INPUT: case SDL_EVENT_TEXT_EDITING: {
            const char *text = event->type == SDL_EVENT_TEXT_INPUT ? event->text.text : event->edit.text;
            if(!text || strlen(text) >= ROHR_INPUT_TEXT_CAPACITY) return false;
            memcpy(data + 48, text, strlen(text) + 1);
            if(event->type == SDL_EVENT_TEXT_EDITING) {
                scene_u32_write(data + 4, (uint32_t)event->edit.start);
                scene_u32_write(data + 8, (uint32_t)event->edit.length);
            }
            break;
        }
        default: return false;
    }
    SDL_Event validated;
    return scene_input_decode(data, &validated);
}
bool scene_input_decode(const unsigned char data[SCENE_INPUT_BYTES], SDL_Event *event) {
    if(!data || !event) return false;
    SDL_zero(*event); event->type = scene_u32_read(data);
    double x = scene_double_read(data + 16), y = scene_double_read(data + 24);
    double dx = scene_double_read(data + 32), dy = scene_double_read(data + 40);
    if(!isfinite(x) || !isfinite(y) || !isfinite(dx) || !isfinite(dy) ||
            fabs(x) > 1e7 || fabs(y) > 1e7 || fabs(dx) > 1e7 || fabs(dy) > 1e7) return false;
    uint32_t code = scene_u32_read(data + 4);
    switch(event->type) {
        case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
            if(code <= SDL_SCANCODE_UNKNOWN || code >= SDL_SCANCODE_COUNT ||
                    scene_u32_read(data + 8) > UINT16_MAX || scene_u32_read(data + 12) > 1) return false;
            event->key.scancode = (SDL_Scancode)code;
            event->key.mod = (SDL_Keymod)scene_u32_read(data + 8);
            event->key.repeat = scene_u32_read(data + 12) != 0;
            event->key.down = event->type == SDL_EVENT_KEY_DOWN; return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
            if(code < INPUT_MOUSE_BUTTON_LEFT || code >= INPUT_MOUSE_BUTTON_COUNT) return false;
            event->button.button = (Uint8)code; event->button.down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            event->button.x = (float)x; event->button.y = (float)y; return true;
        case SDL_EVENT_MOUSE_MOTION:
            event->motion.x = (float)x; event->motion.y = (float)y;
            event->motion.xrel = (float)dx; event->motion.yrel = (float)dy; return true;
        case SDL_EVENT_MOUSE_WHEEL:
            if(code > SDL_MOUSEWHEEL_FLIPPED) return false;
            event->wheel.x = (float)x; event->wheel.y = (float)y;
            event->wheel.mouse_x = (float)dx; event->wheel.mouse_y = (float)dy;
            event->wheel.direction = (SDL_MouseWheelDirection)code; return true;
        case SDL_EVENT_TEXT_INPUT: case SDL_EVENT_TEXT_EDITING:
            if(!memchr(data + 48, 0, ROHR_INPUT_TEXT_CAPACITY)) return false;
            if(event->type == SDL_EVENT_TEXT_INPUT) event->text.text = (const char *)data + 48;
            else {
                if((int32_t)code < -1 || (int32_t)scene_u32_read(data + 8) < -1) return false;
                event->edit.text = (const char *)data + 48;
                event->edit.start = (int32_t)code; event->edit.length = (int32_t)scene_u32_read(data + 8);
            }
            return true;
        default: return false;
    }
}
