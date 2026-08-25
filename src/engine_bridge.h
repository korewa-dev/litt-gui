#ifndef LITT_ENGINE_BRIDGE_H
#define LITT_ENGINE_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Engine state ─────────────────────────────────────────────────────────── */
typedef enum {
    LITT_ENGINE_DISCONNECTED,
    LITT_ENGINE_CONNECTING,
    LITT_ENGINE_CONNECTED,
    LITT_ENGINE_RUNNING,
    LITT_ENGINE_PAUSED,
    LITT_ENGINE_ERROR
} LittEngineState;

/* ── Quality presets ──────────────────────────────────────────────────────── */
typedef enum {
    LITT_ULT_LOW = 0,   /* 360p, 1 bounce    */
    LITT_LOW,           /* 480p, 2 bounces   */
    LITT_MEDIUM,        /* 720p, 4 bounces   */
    LITT_HIGH,          /* 1080p, 8 bounces  */
    LITT_ULTRA,         /* 1440p, 16 bounces */
    LITT_ULT_MAX        /* 4K, 32 bounces    */
} LittQualityPreset;

/* ── GPU info ─────────────────────────────────────────────────────────────── */
typedef struct {
    char    name[128];
    char    vendor[64];
    uint64_t memory_total;   /* bytes */
    uint64_t memory_free;    /* bytes */
    uint32_t max_bounces;
    uint32_t max_texture_size;
} LittGpuInfo;

/* ── Camera ───────────────────────────────────────────────────────────────── */
typedef struct {
    float pos_x, pos_y, pos_z;
    float yaw, pitch;
    float fov;
    float exposure;
    float aspect_ratio;
} LittCamera;

/* ── Render stats ─────────────────────────────────────────────────────────── */
typedef struct {
    float   fps;
    float   frame_time_ms;
    float   path_time_ms;
    uint32_t spp;       /* samples per pixel */
    uint32_t bounces;
    uint32_t width;
    uint32_t height;
} LittRenderStats;

/* ── Opaque handle ────────────────────────────────────────────────────────── */
typedef struct LittEngine LittEngine;

/* ── Public API ───────────────────────────────────────────────────────────── */

/* Connect to engine at host:port (default localhost:8080). Returns true on success. */
bool  litt_connect(LittEngine *eng, const char *host, uint16_t port);
void  litt_disconnect(LittEngine *eng);
LittEngineState litt_get_state(const LittEngine *eng);

/* GPU */
LittGpuInfo litt_get_gpu_info(const LittEngine *eng);

/* Camera */
LittCamera  litt_get_camera(const LittEngine *eng);
bool        litt_set_camera(LittEngine *eng, const LittCamera *cam);

/* Rendering */
bool        litt_start_render(LittEngine *eng);
bool        litt_stop_render(LittEngine *eng);
bool        litt_pause_render(LittEngine *eng);
bool        litt_resume_render(LittEngine *eng);
bool        litt_reset_render(LittEngine *eng);

/* Quality */
bool        litt_set_quality(LittEngine *eng, LittQualityPreset q);
LittQualityPreset litt_get_quality(const LittEngine *eng);
bool        litt_set_resolution(LittEngine *eng, uint32_t w, uint32_t h);
bool        litt_set_bounces(LittEngine *eng, uint32_t n);
bool        litt_set_aa_samples(LittEngine *eng, uint32_t n);

/* Stats */
LittRenderStats litt_get_stats(const LittEngine *eng);

/* Frame capture – fills out_rgba with RGBA pixels; returns true if new frame. */
bool        litt_capture_frame(LittEngine *eng,
                               uint8_t *out_rgba, size_t rgba_cap,
                               uint32_t *out_w, uint32_t *out_h);

/* Logging callback type */
typedef void (*LittLogCb)(const char *msg, void *user);
void        litt_set_log_cb(LittEngine *eng, LittLogCb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* LITT_ENGINE_BRIDGE_H */
