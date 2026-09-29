/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "graphics/text_assets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "field selection line %d: %s\n", __LINE__, #c); return false; } } while(0)
static FontAsset test_font;
static SDL_Window *test_window;
/* The bundled SDL dummy backend has no window manager to grant focus. */
extern bool SDL_SetKeyboardFocus(SDL_Window *window);

typedef struct FieldFixture {
    char value[512];
    float number;
    UIFieldBinding binding;
    TextAsset text;
    UIRect bounds;
    bool multiline;
} FieldFixture;

static UIFieldResult field_draw(FieldFixture *f) {
    return f->multiline ? rohr_ui_multiline_field("selection", f->binding,
        &f->text, f->bounds, NULL) : rohr_ui_field("selection", f->binding,
        &f->text, f->bounds, NULL);
}

static UIFieldResult frame(FieldFixture *f, Position p, MouseButtonState button) {
    rohr_ui_frame_begin((UIInput){.pointer=p, .primary_button=button});
    UIFieldResult result = field_draw(f);
    rohr_ui_frame_end();
    return result;
}

static void key(SDL_Keycode code, SDL_Keymod modifiers) {
    SDL_Event event = {.type=SDL_EVENT_KEY_DOWN};
    event.key.key = code;
    event.key.mod = modifiers;
    rohr_ui_field_event_add(&event);
}

static void text_input(const char *value) {
    SDL_Event event = {.type=SDL_EVENT_TEXT_INPUT};
    event.text.text = value;
    rohr_ui_field_event_add(&event);
}

static bool fixture_start(FieldFixture *f, const char *value, bool multiline) {
    memset(f, 0, sizeof(*f));
    snprintf(f->value, sizeof(f->value), "%s", value);
    f->binding = (UIFieldBinding){.kind=UI_FIELD_STRING, .string=f->value,
        .string_capacity=sizeof(f->value)};
    f->bounds = (UIRect){100,100,300,80};
    f->multiline = multiline;
    TextAssetResult made = rohr_graphics_text_create(&test_font, value, (Color){255,255,255,255});
    CHECK(!rohr_error_check(made));
    f->text = made.result.value;
    rohr_ui_field_focus_clear();
    (void)frame(f, (Position){0}, MOUSE_BUTTON_STATE_UP);
    return true;
}

static void fixture_stop(FieldFixture *f) {
    rohr_ui_field_focus_clear();
    (void)rohr_graphics_text_destroy(&f->text);
}

/* Use the actual font metrics, so whitespace and variable-width text are tested. */
static Position point(FieldFixture *f, size_t offset) {
    TTF_Text *text = graphics_text_native_get(f->text);
    if(f->multiline) {
        TTF_SubString substring = {0};
        (void)TTF_GetTextSubString(text, (int)offset, &substring);
        return (Position){f->bounds.x + 6 + substring.rect.x,
            f->bounds.y + 6 + substring.rect.y + substring.rect.h * 0.5f};
    }
    int width = 0, height = 0;
    (void)TTF_GetStringSize(TTF_GetTextFont(text), f->value, offset, &width, &height);
    if(offset == 0) width = 0;
    float left = f->text.size.x > f->bounds.width - 12 ? f->bounds.x + 6 :
        f->bounds.x + (f->bounds.width - f->text.size.x) * 0.5f;
    return (Position){left + width, f->bounds.y + f->bounds.height * 0.5f};
}

static bool select_range(FieldFixture *f, size_t a, size_t b) {
    Position start = point(f, a), end = point(f, b);
    CHECK(!frame(f, start, MOUSE_BUTTON_STATE_PRESSED).changed);
    CHECK(!frame(f, end, MOUSE_BUTTON_STATE_DOWN).changed);
    CHECK(!frame(f, end, MOUSE_BUTTON_STATE_RELEASED).changed);
    return true;
}

static bool editing_check(void) {
    for(int reverse=0; reverse<2; reverse+=1) {
        FieldFixture f;
        CHECK(fixture_start(&f, "ab cd ef", false));
        CHECK(select_range(&f, reverse ? 5 : 2, reverse ? 2 : 5));
        text_input("X");
        CHECK(frame(&f, (Position){0}, MOUSE_BUTTON_STATE_UP).changed);
        CHECK(strcmp(f.value, "abX ef") == 0);
        fixture_stop(&f);
    }
    for(int action=0; action<4; action+=1) {
        FieldFixture f;
        CHECK(fixture_start(&f, "abcdef", false));
        CHECK(select_range(&f, 1, 4));
        key(action==0 ? SDLK_BACKSPACE : action==1 ? SDLK_DELETE :
            action==2 ? SDLK_LEFT : SDLK_RIGHT, action==3 ? SDL_KMOD_SHIFT : SDL_KMOD_NONE);
        if(action>=2) text_input("X");
        CHECK(frame(&f, (Position){0}, MOUSE_BUTTON_STATE_UP).changed);
        CHECK(strcmp(f.value, action<2 ? "aef" : action==2 ? "aXbcdef" : "abcdXef") == 0);
        fixture_stop(&f);
    }
    FieldFixture f;
    CHECK(fixture_start(&f, "abcdef", false));
    Position a=point(&f,3), b=point(&f,5), c=point(&f,1);
    (void)frame(&f,a,MOUSE_BUTTON_STATE_PRESSED);
    (void)frame(&f,b,MOUSE_BUTTON_STATE_DOWN);
    (void)frame(&f,c,MOUSE_BUTTON_STATE_DOWN);
    (void)frame(&f,c,MOUSE_BUTTON_STATE_RELEASED);
    text_input("X");
    CHECK(frame(&f,c,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"aXdef")==0);
    key(SDLK_A,SDL_KMOD_CTRL);
    text_input("\xc3\xa9\xf0\x9f\x98\x80");
    key(SDLK_LEFT,SDL_KMOD_NONE);
    key(SDLK_BACKSPACE,SDL_KMOD_NONE);
    CHECK(frame(&f,c,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"\xf0\x9f\x98\x80")==0);
    key(SDLK_DELETE,SDL_KMOD_NONE);
    CHECK(frame(&f,c,MOUSE_BUTTON_STATE_UP).changed && f.value[0]=='\0');
    fixture_stop(&f);
    return true;
}

static bool clicks_check(void) {
    for(int multiline=0; multiline<2; multiline+=1) {
        FieldFixture f;
        CHECK(fixture_start(&f, multiline ? "abc\ndef" : "abcdef", multiline!=0));
        Position p=point(&f,2);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
        text_input("X");
        CHECK(frame(&f,p,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"X")==0);
        fixture_stop(&f);
        CHECK(fixture_start(&f,"abcdef",false));
        p=point(&f,2);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
        p=point(&f,3);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
        (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
        text_input("X");
        CHECK(frame(&f,p,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abcXdef")==0);
        fixture_stop(&f);
    }
    FieldFixture f;
    CHECK(fixture_start(&f,"1234",false));
    f.number=1234;
    f.binding=(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=&f.number};
    Position p=point(&f,2);
    (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
    (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
    key('9',SDL_KMOD_NONE);
    CHECK(frame(&f,p,MOUSE_BUTTON_STATE_UP).changed && f.number==12934);
    fixture_stop(&f);
    return true;
}

static bool rejection_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f,"abcdef",false));
    f.binding.string_capacity=7;
    CHECK(select_range(&f,2,4));
    text_input("TOO LONG");
    CHECK(!frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abcdef")==0);
    text_input("X");
    CHECK(frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abXef")==0);
    fixture_stop(&f);
    CHECK(fixture_start(&f,"1234",false));
    f.number=1234;
    f.binding=(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=&f.number};
    CHECK(select_range(&f,1,3));
    key('-',SDL_KMOD_NONE);
    CHECK(!frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && f.number==1234);
    key('9',SDL_KMOD_NONE);
    CHECK(frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && f.number==194);
    fixture_stop(&f);
    return true;
}

static bool capture_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f,"abcdef",false));
    Position p=point(&f,2), outside={450,140};
    (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
    for(int release=0;release<2;release+=1) {
        rohr_ui_frame_begin((UIInput){.pointer=outside,.primary_button=release ?
            MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_DOWN});
        CHECK(!rohr_ui_button("neighbor",NULL,(UIRect){410,100,100,80},NULL).hovered);
        CHECK(!field_draw(&f).changed);
        CHECK(rohr_ui_pointer_consumed_get());
        CHECK(!rohr_ui_button("later-neighbor",NULL,(UIRect){410,100,100,80},NULL).hovered);
        rohr_ui_frame_end();
    }
    text_input("X");
    CHECK(frame(&f,outside,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abX")==0);
    fixture_stop(&f);
    CHECK(fixture_start(&f,"abcdef",false));
    (void)frame(&f,point(&f,2),MOUSE_BUTTON_STATE_PRESSED);
    rohr_ui_frame_begin((UIInput){.pointer=outside,.primary_button=MOUSE_BUTTON_STATE_DOWN});
    rohr_ui_modal_set((UIRect){400,90,150,100});
    CHECK(!field_draw(&f).active);
    rohr_ui_frame_end();
    text_input("X");
    CHECK(!frame(&f,outside,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abcdef")==0);
    (void)frame(&f,point(&f,2),MOUSE_BUTTON_STATE_PRESSED);
    rohr_ui_frame_begin((UIInput){.primary_button=MOUSE_BUTTON_STATE_DOWN});
    rohr_ui_frame_end(); /* Field disappeared. */
    text_input("X");
    CHECK(!frame(&f,outside,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abcdef")==0);
    fixture_stop(&f);
    return true;
}

static bool multiline_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f,"abc\ndef\nghi",true));
    CHECK(select_range(&f,1,6));
    text_input("X");
    CHECK(frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"aXf\nghi")==0);
    fixture_stop(&f);
    CHECK(fixture_start(&f,"one two three four five six seven eight nine ten eleven twelve",true));
    f.bounds.width=90;
    f.bounds.height=45;
    (void)frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP);
    (void)frame(&f,point(&f,0),MOUSE_BUTTON_STATE_PRESSED);
    Position bottom={f.bounds.x+f.bounds.width-10,f.bounds.y+f.bounds.height+20};
    for(int i=0;i<100;i+=1) {
        SDL_Delay(10);
        (void)frame(&f,bottom,MOUSE_BUTTON_STATE_DOWN);
    }
    (void)frame(&f,bottom,MOUSE_BUTTON_STATE_RELEASED);
    text_input("X");
    CHECK(frame(&f,bottom,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"X")==0);
    fixture_stop(&f);
    return true;
}

static bool overflow_and_translation_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f,"abcdefghijklmnopqrstuvwxyz0123456789",false));
    f.bounds.width=90;
    (void)frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP);
    (void)frame(&f,point(&f,1),MOUSE_BUTTON_STATE_PRESSED);
    Position outside={f.bounds.x+f.bounds.width+20,140};
    for(int i=0;i<100;i+=1) {
        SDL_Delay(10);
        (void)frame(&f,outside,MOUSE_BUTTON_STATE_DOWN);
    }
    (void)frame(&f,outside,MOUSE_BUTTON_STATE_RELEASED);
    key(SDLK_DELETE,SDL_KMOD_NONE);
    CHECK(frame(&f,outside,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"a")==0);
    fixture_stop(&f);

    CHECK(fixture_start(&f,"abcdef",false));
    f.bounds.height=40;
    Position a=point(&f,1), b=point(&f,4);
    a.y-=5; b.y-=5;
    for(int step=0;step<3;step+=1) {
        rohr_ui_frame_begin((UIInput){.pointer=step ? b : a,
            .primary_button=step==0 ? MOUSE_BUTTON_STATE_PRESSED :
                step==1 ? MOUSE_BUTTON_STATE_DOWN : MOUSE_BUTTON_STATE_RELEASED});
        (void)rohr_ui_scroll_region_begin("panel",(UIRect){100,90,300,45},200,20,10);
        rohr_ui_translation_y_push(15);
        CHECK(!field_draw(&f).changed);
        rohr_ui_translation_y_pop();
        rohr_ui_scroll_region_end();
        rohr_ui_frame_end();
    }
    text_input("X");
    CHECK(frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"aXef")==0);
    rohr_ui_field_focus_clear();
    rohr_ui_frame_begin((UIInput){.pointer={250,137},.primary_button=MOUSE_BUTTON_STATE_PRESSED});
    (void)rohr_ui_scroll_region_begin("panel",(UIRect){100,90,300,45},200,20,10);
    rohr_ui_translation_y_push(15);
    CHECK(!field_draw(&f).active);
    rohr_ui_translation_y_pop();
    rohr_ui_scroll_region_end();
    rohr_ui_frame_end();
    fixture_stop(&f);
    return true;
}

static bool explicit_focus_and_empty_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f,"rename me",false));
    ui_field_focus_set("selection",f.binding,&f.text,true);
    text_input("new name");
    CHECK(frame(&f,(Position){0},MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"new name")==0);
    fixture_stop(&f);
    CHECK(fixture_start(&f,"",false));
    Position p={250,140};
    (void)frame(&f,p,MOUSE_BUTTON_STATE_PRESSED);
    (void)frame(&f,p,MOUSE_BUTTON_STATE_RELEASED);
    text_input("abc");
    CHECK(frame(&f,p,MOUSE_BUTTON_STATE_UP).changed && strcmp(f.value,"abc")==0);
    fixture_stop(&f);
    return true;
}

static bool text_input_focus_check(void) {
    for(int multiline = 0; multiline < 2; multiline += 1) {
        FieldFixture f;
        CHECK(fixture_start(&f, "abcdef", multiline != 0));
        Position p = point(&f, 2);
        CHECK(SDL_StopTextInput(test_window));
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_PRESSED).active);
        CHECK(SDL_TextInputActive(test_window));
        (void)frame(&f, p, MOUSE_BUTTON_STATE_RELEASED);
        CHECK(SDL_StopTextInput(test_window));
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_PRESSED).active);
        CHECK(SDL_TextInputActive(test_window));
        (void)frame(&f, p, MOUSE_BUTTON_STATE_RELEASED);
        text_input("X");
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_UP).changed);
        CHECK(strcmp(f.value, "X") == 0);

        /* Recover even without another click, including focus granted late. */
        CHECK(SDL_StopTextInput(test_window));
        CHECK(SDL_SetKeyboardFocus(NULL));
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_UP).active);
        CHECK(!SDL_TextInputActive(test_window));
        CHECK(SDL_SetKeyboardFocus(test_window));
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_UP).active);
        CHECK(SDL_TextInputActive(test_window));

        rohr_ui_field_focus_clear();
        CHECK(SDL_StopTextInput(test_window));
        ui_field_focus_set("selection", f.binding, &f.text, true);
        CHECK(SDL_TextInputActive(test_window));
        text_input("rename");
        CHECK(frame(&f, p, MOUSE_BUTTON_STATE_UP).changed);
        CHECK(strcmp(f.value, "rename") == 0);

        key(SDLK_ESCAPE, SDL_KMOD_NONE);
        (void)frame(&f, p, MOUSE_BUTTON_STATE_UP);
        CHECK(SDL_StopTextInput(test_window));
        CHECK(!frame(&f, p, MOUSE_BUTTON_STATE_UP).active);
        CHECK(!SDL_TextInputActive(test_window));
        fixture_stop(&f);
    }
    return true;
}

static bool keyboard_and_field_transfer_check(void) {
    FieldFixture f;
    CHECK(fixture_start(&f, "12", false));
    f.number = 12;
    f.binding = (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &f.number};
    (void)frame(&f, (Position){0}, MOUSE_BUTTON_STATE_UP);
    CHECK(SDL_StopTextInput(test_window));
    (void)rohr_ui_navigation_move(UI_NAVIGATION_RIGHT);
    CHECK(rohr_ui_navigation_activate());
    CHECK(frame(&f, (Position){0}, MOUSE_BUTTON_STATE_UP).active);
    CHECK(SDL_TextInputActive(test_window));
    key('9', SDL_KMOD_NONE);
    CHECK(frame(&f, (Position){0}, MOUSE_BUTTON_STATE_UP).changed);
    CHECK(f.number == 129);

    char other[64] = "other";
    UIFieldBinding binding = {.kind = UI_FIELD_STRING,
        .string = other, .string_capacity = sizeof(other)};
    for(int step = 0; step < 3; step += 1) {
        if(step == 2) {
            CHECK(SDL_TextInputActive(test_window));
            key(SDLK_A, SDL_KMOD_CTRL);
            text_input("replacement");
        }
        rohr_ui_frame_begin((UIInput){.pointer = {250, 140},
            .primary_button = step == 0 ? MOUSE_BUTTON_STATE_PRESSED :
                step == 1 ? MOUSE_BUTTON_STATE_RELEASED : MOUSE_BUTTON_STATE_UP});
        CHECK(rohr_ui_field("other", binding, &f.text, f.bounds, NULL).active);
        rohr_ui_frame_end();
    }
    CHECK(strcmp(other, "replacement") == 0 && f.number == 129);
    fixture_stop(&f);
    return true;
}

int main(void) {
    EngineResult result=rohr_engine_start();
    if(rohr_error_check(result)) return 1;
    result=rohr_graphics_start();
    if(rohr_error_check(result)) { rohr_engine_stop(); return 1; }
    int window_count = 0;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    test_window = window_count > 0 ? windows[0] : NULL;
    SDL_free(windows);
    if(test_window == NULL || !SDL_SetKeyboardFocus(test_window)) return 1;
    FontAssetResult font = rohr_graphics_font_load((FontDescriptor){ROHR_TEST_FONT_PATH, 18});
    if(rohr_error_check(font)) { rohr_graphics_stop(); rohr_engine_stop(); return 1; }
    test_font = font.result.value;
    bool passed=editing_check() && clicks_check() && rejection_check() &&
        capture_check() && multiline_check() && overflow_and_translation_check() &&
        explicit_focus_and_empty_check() && text_input_focus_check() &&
        keyboard_and_field_transfer_check();
    rohr_ui_field_focus_clear();
    (void)rohr_graphics_font_release(&test_font);
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
