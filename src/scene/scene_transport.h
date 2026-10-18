/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_SCENE_TRANSPORT_H
#define ROHR_SCENE_TRANSPORT_H
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCENE_PROTOCOL_VERSION 1u
#define SCENE_TOKEN_BYTES 16u
#define SCENE_TOKEN_TEXT_SIZE 33u
#define SCENE_FRAME_SIDE_MAX 4096u
#define SCENE_FRAME_HEADER_SIZE 32u
#define SCENE_FRAME_BYTES_MAX (SCENE_FRAME_SIDE_MAX * SCENE_FRAME_SIDE_MAX * 4u)
#define SCENE_PACKET_BYTES_MAX (SCENE_FRAME_BYTES_MAX + SCENE_FRAME_HEADER_SIZE)
#define SCENE_INPUT_BYTES 304u
#define SCENE_PUMP_BYTES (8u * 1024u * 1024u)
#define SCENE_START_TIMEOUT_MS 10000u
#define SCENE_STOP_TIMEOUT_MS 2000u

typedef enum SceneMessage {
    SCENE_HELLO = 1, SCENE_READY, SCENE_FRAME, SCENE_STATE,
    SCENE_PAUSE, SCENE_RESUME, SCENE_STOP, SCENE_INPUT, SCENE_FOCUS,
    SCENE_FAILURE
} SceneMessage;
/* Opaque native socket stored without pulling platform headers into callers. */
typedef uintptr_t SceneSocket;
#define SCENE_SOCKET_INVALID UINTPTR_MAX

typedef struct SceneWire {
    SceneSocket socket;
    unsigned char header[16];
    size_t header_used, received, size, rx_capacity;
    unsigned char *rx;
    SceneMessage kind;
    unsigned char *tx;
    size_t tx_size, tx_used, tx_capacity;
    bool failed, closed;
    /* Only HELLO/READY are accepted until the token exchange completes. */
    bool authenticated;
    char error[256];
} SceneWire;

bool scene_transport_start(void);
void scene_transport_stop(void);
bool scene_token_create(char token[SCENE_TOKEN_TEXT_SIZE]);
bool scene_token_check(const char *token);
SceneSocket scene_socket_listen(uint16_t *port);
SceneSocket scene_socket_accept(SceneSocket listener);
SceneSocket scene_socket_connect(uint16_t port);
/* 0 pending, 1 connected, -1 failed. */
int scene_socket_connected_get(SceneSocket socket);
void scene_socket_close(SceneSocket socket);
void scene_wire_init(SceneWire *wire, SceneSocket socket);
void scene_wire_destroy(SceneWire *wire);
/* Borrowed receive bytes valid until next read. 0 pending, 1 packet, -1 error/EOF. */
int scene_wire_receive(SceneWire *wire, size_t *budget);
bool scene_wire_send(SceneWire *wire, SceneMessage kind, const void *data, size_t size);
bool scene_wire_send_parts(SceneWire *wire, SceneMessage kind,
    const void *prefix, size_t prefix_size, const void *data, size_t size);
bool scene_wire_flush(SceneWire *wire, size_t *budget);
bool scene_wire_busy_check(const SceneWire *wire);
void scene_u32_write(void *out, uint32_t value);
uint32_t scene_u32_read(const void *in);
void scene_u64_write(void *out, uint64_t value);
uint64_t scene_u64_read(const void *in);
void scene_double_write(void *out, double value);
double scene_double_read(const void *in);
bool scene_input_encode(const SDL_Event *event, unsigned char data[SCENE_INPUT_BYTES]);
bool scene_input_decode(const unsigned char data[SCENE_INPUT_BYTES], SDL_Event *event);
#endif
