/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
typedef SOCKET NativeSocket;
#define SOCKET_ERROR_CODE() WSAGetLastError()
#define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int NativeSocket;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR_CODE() errno
#define SOCKET_WOULD_BLOCK EWOULDBLOCK
#endif
#include "scene_transport.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void scene_u32_write(void *out, uint32_t v) {
    unsigned char *p = out;
    for(unsigned i = 0; i < 4; ++i) p[i] = (unsigned char)(v >> (i * 8));
}
uint32_t scene_u32_read(const void *in) {
    const unsigned char *p = in; uint32_t v = 0;
    for(unsigned i = 0; i < 4; ++i) v |= (uint32_t)p[i] << (i * 8);
    return v;
}
void scene_u64_write(void *out, uint64_t v) {
    unsigned char *p = out;
    for(unsigned i = 0; i < 8; ++i) p[i] = (unsigned char)(v >> (i * 8));
}
uint64_t scene_u64_read(const void *in) {
    const unsigned char *p = in; uint64_t v = 0;
    for(unsigned i = 0; i < 8; ++i) v |= (uint64_t)p[i] << (i * 8);
    return v;
}
void scene_double_write(void *out, double v) { uint64_t bits; memcpy(&bits, &v, 8); scene_u64_write(out, bits); }
double scene_double_read(const void *in) { uint64_t bits = scene_u64_read(in); double v; memcpy(&v, &bits, 8); return v; }

bool scene_transport_start(void) {
#ifdef _WIN32
    WSADATA data; return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return true;
#endif
}
void scene_transport_stop(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}
bool scene_token_create(char token[SCENE_TOKEN_TEXT_SIZE]) {
    unsigned char random[SCENE_TOKEN_BYTES];
#ifdef _WIN32
    if(BCryptGenRandom(NULL, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return false;
#else
    FILE *file = fopen("/dev/urandom", "rb");
    if(!file) return false;
    bool read = fread(random, 1, sizeof(random), file) == sizeof(random);
    fclose(file);
    if(!read) return false;
#endif
    static const char hex[] = "0123456789abcdef";
    for(size_t i = 0; i < sizeof(random); ++i) {
        token[2*i] = hex[random[i] >> 4]; token[2*i+1] = hex[random[i] & 15];
    }
    token[SCENE_TOKEN_TEXT_SIZE - 1] = 0; return true;
}
bool scene_token_check(const char *token) {
    if(!token || strlen(token) != SCENE_TOKEN_TEXT_SIZE - 1) return false;
    for(size_t i = 0; i < SCENE_TOKEN_TEXT_SIZE - 1; ++i)
        if(!((token[i] >= '0' && token[i] <= '9') || (token[i] >= 'a' && token[i] <= 'f'))) return false;
    return true;
}
static bool nonblocking(NativeSocket s) {
#ifdef _WIN32
    u_long enabled = 1;
    return SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, 0) &&
        ioctlsocket(s, FIONBIO, &enabled) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0 &&
        fcntl(s, F_SETFD, FD_CLOEXEC) == 0;
#endif
}
void scene_socket_close(SceneSocket socket) {
    if(socket == SCENE_SOCKET_INVALID) return;
#ifdef _WIN32
    closesocket((NativeSocket)socket);
#else
    close((NativeSocket)socket);
#endif
}
SceneSocket scene_socket_listen(uint16_t *port) {
    NativeSocket s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s == INVALID_SOCKET) return SCENE_SOCKET_INVALID;
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#ifdef _WIN32
    int length = sizeof(addr);
#else
    socklen_t length = sizeof(addr);
#endif
    if(!nonblocking(s) || bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
            listen(s, 1) != 0 || getsockname(s, (struct sockaddr *)&addr, &length) != 0) {
        scene_socket_close((SceneSocket)s); return SCENE_SOCKET_INVALID;
    }
    *port = ntohs(addr.sin_port); return (SceneSocket)s;
}
SceneSocket scene_socket_accept(SceneSocket listener) {
    NativeSocket s = accept((NativeSocket)listener, NULL, NULL);
    if(s == INVALID_SOCKET) return SCENE_SOCKET_INVALID;
    if(!nonblocking(s)) { scene_socket_close((SceneSocket)s); return SCENE_SOCKET_INVALID; }
    return (SceneSocket)s;
}
SceneSocket scene_socket_connect(uint16_t port) {
    NativeSocket s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s == INVALID_SOCKET) return SCENE_SOCKET_INVALID;
    if(!nonblocking(s)) { scene_socket_close((SceneSocket)s); return SCENE_SOCKET_INVALID; }
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons(port);
    if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        int error = SOCKET_ERROR_CODE();
        if(error != SOCKET_WOULD_BLOCK
#ifndef _WIN32
                && error != EINPROGRESS
#endif
                ) { scene_socket_close((SceneSocket)s); return SCENE_SOCKET_INVALID; }
    }
    return (SceneSocket)s;
}
int scene_socket_connected_get(SceneSocket socket) {
    NativeSocket s = (NativeSocket)socket;
#ifdef _WIN32
    fd_set writes; FD_ZERO(&writes); FD_SET(s, &writes); struct timeval timeout = {0};
    int selected = select((int)s + 1, NULL, &writes, NULL, &timeout);
#else
    struct pollfd descriptor = {.fd = s, .events = POLLOUT};
    int selected = poll(&descriptor, 1, 0);
#endif
    if(selected <= 0) return selected;
    int error = 0;
#ifdef _WIN32
    int length = sizeof(error);
#else
    socklen_t length = sizeof(error);
#endif
    if(getsockopt(s, SOL_SOCKET, SO_ERROR, (void *)&error, &length) != 0 || error) return -1;
    return 1;
}
void scene_wire_init(SceneWire *wire, SceneSocket socket) { *wire = (SceneWire){.socket = socket}; }
void scene_wire_destroy(SceneWire *wire) {
    scene_socket_close(wire->socket); SDL_free(wire->rx); SDL_free(wire->tx);
    scene_wire_init(wire, SCENE_SOCKET_INVALID);
}
static bool fail(SceneWire *wire, const char *detail) {
    wire->failed = true; SDL_strlcpy(wire->error, detail, sizeof(wire->error)); return false;
}
static bool length_check(SceneMessage kind, size_t size) {
    switch(kind) {
        case SCENE_HELLO: case SCENE_READY: return size == SCENE_TOKEN_TEXT_SIZE - 1;
        case SCENE_FRAME: return size >= SCENE_FRAME_HEADER_SIZE && size <= SCENE_PACKET_BYTES_MAX;
        case SCENE_STATE: return size == 48;
        case SCENE_PAUSE: case SCENE_RESUME: case SCENE_STOP: return size == 0;
        case SCENE_INPUT: return size == SCENE_INPUT_BYTES;
        case SCENE_FOCUS: return size == 4;
        case SCENE_FAILURE: return size > 0 && size <= 256;
        default: return false;
    }
}
bool scene_wire_busy_check(const SceneWire *wire) { return wire->tx_used < wire->tx_size; }
bool scene_wire_send_parts(SceneWire *wire, SceneMessage kind, const void *prefix,
        size_t prefix_size, const void *data, size_t size) {
    if(wire->failed || wire->closed || scene_wire_busy_check(wire)) return false;
    if(prefix_size > SCENE_PACKET_BYTES_MAX || size > SCENE_PACKET_BYTES_MAX - prefix_size ||
            !length_check(kind, size + prefix_size) || (prefix_size && !prefix) || (size && !data))
        return fail(wire, "Invalid outgoing scene packet");
    size_t total = 16 + prefix_size + size;
    if(total > wire->tx_capacity) {
        void *allocated = SDL_realloc(wire->tx, total);
        if(!allocated) return fail(wire, "Scene send allocation failed");
        wire->tx = allocated; wire->tx_capacity = total;
    }
    scene_u32_write(wire->tx, UINT32_C(0x52484353));
    scene_u32_write(wire->tx + 4, SCENE_PROTOCOL_VERSION);
    scene_u32_write(wire->tx + 8, kind); scene_u32_write(wire->tx + 12, (uint32_t)(prefix_size + size));
    if(prefix_size) memcpy(wire->tx + 16, prefix, prefix_size);
    if(size) memcpy(wire->tx + 16 + prefix_size, data, size);
    wire->tx_used = 0; wire->tx_size = total; return true;
}
bool scene_wire_send(SceneWire *wire, SceneMessage kind, const void *data, size_t size) {
    return scene_wire_send_parts(wire, kind, NULL, 0, data, size);
}
bool scene_wire_flush(SceneWire *wire, size_t *budget) {
    while(scene_wire_busy_check(wire) && *budget && !wire->failed) {
        size_t size = SDL_min(*budget, wire->tx_size - wire->tx_used);
        int n = send((NativeSocket)wire->socket, (const char *)wire->tx + wire->tx_used, (int)size,
#ifdef _WIN32
            0
#else
            MSG_NOSIGNAL
#endif
        );
        if(n < 0) {
            int error = SOCKET_ERROR_CODE();
            if(error == SOCKET_WOULD_BLOCK) return true;
#ifndef _WIN32
            if(error == EINTR) continue;
#endif
            return fail(wire, "Scene connection send failed");
        }
        if(n == 0) return fail(wire, "Scene connection closed during send");
        wire->tx_used += (size_t)n; *budget -= (size_t)n;
    }
    return !wire->failed;
}
int scene_wire_receive(SceneWire *wire, size_t *budget) {
    if(wire->closed || wire->failed) return -1;
    /* A completed packet is consumed by the next receive invocation. */
    if(wire->header_used == 16 && wire->received == wire->size) {
        wire->header_used = 0; wire->received = 0;
    }
    while(*budget) {
        bool header = wire->header_used < 16;
        size_t remaining = header ? 16 - wire->header_used : wire->size - wire->received;
        unsigned char *target = header ? wire->header + wire->header_used : wire->rx + wire->received;
        int n = recv((NativeSocket)wire->socket, (char *)target, (int)SDL_min(*budget, remaining), 0);
        if(n < 0) {
            int error = SOCKET_ERROR_CODE();
            if(error == SOCKET_WOULD_BLOCK) return 0;
#ifndef _WIN32
            if(error == EINTR) continue;
#endif
            fail(wire, "Scene connection receive failed"); return -1;
        }
        if(n == 0) { wire->closed = true; return -1; }
        *budget -= (size_t)n;
        if(header) {
            wire->header_used += (size_t)n;
            if(wire->header_used != 16) continue;
            wire->kind = (SceneMessage)scene_u32_read(wire->header + 8);
            wire->size = scene_u32_read(wire->header + 12);
            if(scene_u32_read(wire->header) != UINT32_C(0x52484353) ||
                    scene_u32_read(wire->header + 4) != SCENE_PROTOCOL_VERSION ||
                    !length_check(wire->kind, wire->size) ||
                    (!wire->authenticated && wire->kind != SCENE_HELLO && wire->kind != SCENE_READY)) {
                fail(wire, "Incompatible or malformed scene protocol"); return -1;
            }
            if(wire->size > wire->rx_capacity) {
                void *allocated = SDL_realloc(wire->rx, wire->size);
                if(!allocated) { fail(wire, "Scene receive allocation failed"); return -1; }
                wire->rx = allocated; wire->rx_capacity = wire->size;
            }
        } else wire->received += (size_t)n;
        if(wire->header_used == 16 && wire->received == wire->size) return 1;
    }
    return 0;
}
