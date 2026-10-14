/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef AUDIO_FIXTURE_H
#define AUDIO_FIXTURE_H
#include "audio_ogg_fixture.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#ifndef AUDIO_TEST_PATH
#define AUDIO_TEST_PATH "rohr_audio_test.wav"
#endif
#ifndef AUDIO_TEST_MUSIC_PATH
#define AUDIO_TEST_MUSIC_PATH "rohr_audio_test.ogg"
#endif
#define AUDIO_TEST_SAMPLE_RATE 48000
#define AUDIO_TEST_SAMPLE_COUNT 4800

static void write_u16_le(FILE *file, uint16_t value) {
    fputc((int)(value & UINT16_C(0xff)), file);
    fputc((int)((value >> 8) & UINT16_C(0xff)), file);
}

static void write_u32_le(FILE *file, uint32_t value) {
    fputc((int)(value & UINT32_C(0xff)), file);
    fputc((int)((value >> 8) & UINT32_C(0xff)), file);
    fputc((int)((value >> 16) & UINT32_C(0xff)), file);
    fputc((int)((value >> 24) & UINT32_C(0xff)), file);
}

static bool audio_test_wav_write(void) {
    const uint32_t data_size = AUDIO_TEST_SAMPLE_COUNT * sizeof(int16_t);
    FILE *file = fopen(AUDIO_TEST_PATH, "wb");
    if(file == NULL) return false;
    fwrite("RIFF", 1, 4, file);
    write_u32_le(file, 36 + data_size);
    fwrite("WAVEfmt ", 1, 8, file);
    write_u32_le(file, 16);
    write_u16_le(file, 1);
    write_u16_le(file, 1);
    write_u32_le(file, AUDIO_TEST_SAMPLE_RATE);
    write_u32_le(file, AUDIO_TEST_SAMPLE_RATE * sizeof(int16_t));
    write_u16_le(file, sizeof(int16_t));
    write_u16_le(file, 16);
    fwrite("data", 1, 4, file);
    write_u32_le(file, data_size);
    for(size_t sample = 0; sample < AUDIO_TEST_SAMPLE_COUNT; sample += 1) {
        double phase = (double)sample * 440.0 * 6.283185307179586 /
            AUDIO_TEST_SAMPLE_RATE;
        int16_t value = (int16_t)(sin(phase) * 4096.0);
        write_u16_le(file, (uint16_t)value);
    }
    if(fclose(file) != 0) return false;
    return true;
}

static int audio_test_base64_value(char value) {
    if(value >= 'A' && value <= 'Z') return value - 'A';
    if(value >= 'a' && value <= 'z') return value - 'a' + 26;
    if(value >= '0' && value <= '9') return value - '0' + 52;
    if(value == '+') return 62;
    if(value == '/') return 63;
    return -1;
}

static bool audio_test_ogg_write(void) {
    FILE *file = fopen(AUDIO_TEST_MUSIC_PATH, "wb");
    uint32_t pending = 0;
    int pending_bits = 0;
    if(file == NULL) return false;
    for(size_t index = 0; audio_test_ogg_base64[index] != '\0'; index += 1) {
        int value;
        if(audio_test_ogg_base64[index] == '=') break;
        value = audio_test_base64_value(audio_test_ogg_base64[index]);
        if(value < 0) {
            fclose(file);
            (void)remove(AUDIO_TEST_MUSIC_PATH);
            return false;
        }
        pending = (pending << 6) | (uint32_t)value;
        pending_bits += 6;
        if(pending_bits >= 8) {
            pending_bits -= 8;
            if(fputc((int)((pending >> pending_bits) & UINT32_C(0xff)), file) ==
                    EOF) {
                fclose(file);
                (void)remove(AUDIO_TEST_MUSIC_PATH);
                return false;
            }
            pending &= pending_bits == 0 ? 0 :
                (UINT32_C(1) << pending_bits) - 1;
        }
    }
    if(fclose(file) != 0) return false;
    return true;
}


#endif
