#include "engine_bridge.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define RX_BUF_SIZE  4096

struct LittEngine {
    int                sock;
    LittEngineState    state;
    LittGpuInfo        gpu;
    LittCamera         cam;
    LittRenderStats    stats;
    LittQualityPreset  quality;
    bool               running;

    /* frame buffer (ring) */
    uint8_t   frame_buf[1920 * 1080 * 4];
    uint32_t  frame_w, frame_h;
    bool      frame_valid;

    /* logging */
    LittLogCb  log_cb;
    void     *log_user;
    char      log_buf[512];

    /* thread */
    pthread_t rx_thread;
    bool      rx_stop;
};

static void litt_log(LittEngine *eng, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(eng->log_buf, sizeof(eng->log_buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[litt] %s\n", eng->log_buf);
    if (eng->log_cb)
        eng->log_cb(eng->log_buf, eng->log_user);
}

/* ── receive loop thread ──────────────────────────────────────────────────── */
static void *rx_loop(void *arg) {
    LittEngine *eng = (LittEngine *)arg;
    char buf[RX_BUF_SIZE];

    while (!eng->rx_stop) {
        ssize_t n = recv(eng->sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (eng->rx_stop) break;
            litt_log(eng, "recv error or connection lost (errno=%d)", errno);
            eng->state = LITT_ENGINE_DISCONNECTED;
            break;
        }
        buf[n] = '\0';

        /* Simple text protocol: first line is state, second line JSON-ish payload */
        char *line = strtok(buf, "\n");
        if (line) {
            if (strcmp(line, "STATE:RUNNING") == 0)
                eng->state = LITT_ENGINE_RUNNING;
            else if (strcmp(line, "STATE:PAUSED") == 0)
                eng->state = LITT_ENGINE_PAUSED;
            else if (strcmp(line, "STATE:ERROR") == 0)
                eng->state = LITT_ENGINE_ERROR;
            else
                litt_log(eng, "received: %s", line);
        }

        /* If frame data detected (binary prefix 0x89 0x50 = PNG, or "FRAME:" tag) */
        if (strncmp(buf, "FRAME:", 6) == 0) {
            /* Parse "FRAME:WxH<raw-pixels>" – simplified; real impl would
               use a binary protobuf or msgpack over the socket. */
            eng->frame_valid = false;  /* placeholder */
        }
    }
    return NULL;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

bool litt_connect(LittEngine *eng, const char *host, uint16_t port) {
    if (!eng) return false;

    eng->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (eng->sock < 0) {
        litt_log(eng, "socket() failed: %d", errno);
        return false;
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
    };
    if (inet_pton(AF_INET, host ? host : DEFAULT_HOST, &addr.sin_addr) != 1) {
        litt_log(eng, "inet_pton failed");
        close(eng->sock);
        return false;
    }

    if (connect(eng->sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        litt_log(eng, "connect failed: %d", errno);
        close(eng->sock);
        return false;
    }

    eng->state   = LITT_ENGINE_CONNECTED;
    eng->running = true;
    eng->frame_valid = false;
    eng->rx_stop   = false;

    pthread_create(&eng->rx_thread, NULL, rx_loop, eng);
    litt_log(eng, "connected to %s:%d", host ? host : DEFAULT_HOST, port);
    return true;
}

void litt_disconnect(LittEngine *eng) {
    if (!eng) return;
    eng->rx_stop = true;
    if (eng->sock >= 0) {
        shutdown(eng->sock, SHUT_RDWR);
        close(eng->sock);
        eng->sock = -1;
    }
    if (eng->rx_thread != (pthread_t)0) {
        pthread_join(eng->rx_thread, NULL);
        eng->rx_thread = (pthread_t)0;
    }
    eng->state = LITT_ENGINE_DISCONNECTED;
    eng->running = false;
}

LittEngineState litt_get_state(const LittEngine *eng) {
    return eng ? eng->state : LITT_ENGINE_DISCONNECTED;
}

LittGpuInfo litt_get_gpu_info(const LittEngine *eng) {
    return eng ? eng->gpu : (LittGpuInfo){0};
}

LittCamera  litt_get_camera(const LittEngine *eng) {
    return eng ? eng->cam : (LittCamera){0};
}

bool litt_set_camera(LittEngine *eng, const LittCamera *cam) {
    if (!eng || !cam) return false;
    eng->cam = *cam;
    /* Send command to engine – in production this would be a binary protocol frame */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "CAM:%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f\n",
             cam->pos_x, cam->pos_y, cam->pos_z,
             cam->yaw, cam->pitch, cam->fov, cam->exposure);
    send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL);
    return true;
}

bool litt_start_render(LittEngine *eng) {
    if (!eng) return false;
    char *cmd = "RENDER:START\n";
    ssize_t n = send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL);
    eng->state = LITT_ENGINE_RUNNING;
    return n > 0;
}

bool litt_stop_render(LittEngine *eng) {
    if (!eng) return false;
    char *cmd = "RENDER:STOP\n";
    ssize_t n = send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL);
    eng->state = LITT_ENGINE_PAUSED;
    return n > 0;
}

bool litt_pause_render(LittEngine *eng) {
    return litt_stop_render(eng);
}

bool litt_resume_render(LittEngine *eng) {
    return litt_start_render(eng);
}

bool litt_reset_render(LittEngine *eng) {
    if (!eng) return false;
    char *cmd = "RENDER:RESET\n";
    ssize_t n = send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL);
    return n > 0;
}

bool litt_set_quality(LittEngine *eng, LittQualityPreset q) {
    if (!eng) return false;
    eng->quality = q;
    static const char *names[] = {"ULTRA_LOW","LOW","MEDIUM","HIGH","ULTRA","ULTRA_MAX"};
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "QUALITY:%s\n", names[q]);
    ssize_t n = send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL);
    return n > 0;
}

LittQualityPreset litt_get_quality(const LittEngine *eng) {
    return eng ? eng->quality : LITT_HIGH;
}

bool litt_set_resolution(LittEngine *eng, uint32_t w, uint32_t h) {
    if (!eng) return false;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "RES:%ux%u\n", w, h);
    return send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL) > 0;
}

bool litt_set_bounces(LittEngine *eng, uint32_t n) {
    if (!eng) return false;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "BOUNCES:%u\n", n);
    return send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL) > 0;
}

bool litt_set_aa_samples(LittEngine *eng, uint32_t n) {
    if (!eng) return false;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AA:%u\n", n);
    return send(eng->sock, cmd, strlen(cmd), MSG_NOSIGNAL) > 0;
}

LittRenderStats litt_get_stats(const LittEngine *eng) {
    return eng ? eng->stats : (LittRenderStats){0};
}

bool litt_capture_frame(LittEngine *eng,
                        uint8_t *out_rgba, size_t rgba_cap,
                        uint32_t *out_w, uint32_t *out_h) {
    if (!eng) return false;
    if (!eng->frame_valid) return false;
    size_t need = (size_t)eng->frame_w * eng->frame_h * 4;
    if (rgba_cap < need) return false;
    memcpy(out_rgba, eng->frame_buf, need);
    if (out_w) *out_w = eng->frame_w;
    if (out_h) *out_h = eng->frame_h;
    return true;
}

void litt_set_log_cb(LittEngine *eng, LittLogCb cb, void *user) {
    if (eng) { eng->log_cb = cb; eng->log_user = user; }
}
