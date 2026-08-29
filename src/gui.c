// GUI implementation using the C bridge
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Internal state ──────────────────────────────────────────────────────── */
typedef struct {
    LittGuiBridge* bridge;
    int             active_panel;
    bool            show_logs;
    bool            dark_theme;
    LittCamera      camera;
    bool            cam_dirty;
    uint32_t        res_w, res_h;
    bool            rendering;
    float           exposure;
    float           fov;
    LittRenderStats stats;
    char            log_lines[200][256];
    int             log_count;
} LittGui;

static const char *QUALITY_NAMES[] = {
    "Ultra Low", "Low", "Medium", "High", "Ultra", "Ultra Max"
};

/* ── Panel renderers ─────────────────────────────────────────────────────── */

static void render_dashboard(LittGui *g) {
    ImGui::Text("Litt Engine Dashboard");
    ImGui::Separator();

    /* Engine state */
    ImGui::Text("State: ");
    LittEngineState st = g->bridge->getState();
    ImU32 col = IM_COL32(200,200,200,255);
    switch (st) {
        case LITT_ENGINE_STATE_DISCONNECTED: col = IM_COL32(220,50,50,255);  break;
        case LITT_ENGINE_STATE_CONNECTING:   col = IM_COL32(220,220,50,255); break;
        case LITT_ENGINE_STATE_CONNECTED:    col = IM_COL32(50,200,50,255);  break;
        case LITT_ENGINE_STATE_RUNNING:      col = IM_COL32(50,150,220,255); break;
        case LITT_ENGINE_STATE_PAUSED:       col = IM_COL32(220,130,50,255); break;
        case LITT_ENGINE_STATE_ERROR:        col = IM_COL32(220,50,50,255);  break;
    }
    ImGui::TextColored(ImColor(col),
        st == LITT_ENGINE_STATE_DISCONNECTED  ? "Disconnected" :
        st == LITT_ENGINE_STATE_CONNECTING    ? "Connecting..." :
        st == LITT_ENGINE_STATE_CONNECTED     ? "Connected" :
        st == LITT_ENGINE_STATE_RUNNING       ? "Running" :
        st == LITT_ENGINE_STATE_PAUSED        ? "Paused" : "Error");
    
    if (ImGui::Button("Connect"))
        g->bridge->connect("127.0.0.1", 8080);
    ImGui::SameLine();
    if (ImGui::Button("Disconnect"))
        g->bridge->disconnect();
    
    /* GPU info */
    LittGpuInfo gpu;
    g->bridge->getGpuInfo(&gpu);
    ImGui::Text("GPU: %s (%s)", gpu.name, gpu.vendor);
    ImGui::Text("Memory: %.1f GB / %.1f GB", 
        (double)gpu.memory_free / (1024*1024*1024),
        (double)gpu.memory_total / (1024*1024*1024));
    
    /* Render stats */
    g->bridge->getRenderStats(&g->stats);
    ImGui::Text("FPS: %.1f  Frame: %.2f ms", g->stats.fps, g->stats.frame_time_ms);
    ImGui::Text("Resolution: %ux%u  SPP: %u  Bounces: %u",
        g->stats.width, g->stats.height, g->stats.spp, g->stats.bounces);
}

static void render_entities(LittGui *g) {
    ImGui::Text("Entities");
    ImGui::Separator();
    
    if (ImGui::Button("Create Entity")) {
        LittEntityDesc desc = {};
        snprintf(desc.name, sizeof(desc.name), "entity_%d", g->log_count);
        desc.position.x = 0.0f;
        desc.position.y = 0.0f;
        desc.position.z = 0.0f;
        g->bridge->createEntity(&desc);
        g->log_count++;
    }
    
    /* List entities */
    litt_entity_t ids[100];
    int count = g->bridge->listEntities(ids, 100);
    for (int i = 0; i < count; i++) {
        char label[128];
        snprintf(label, sizeof(label), "Entity %u", (unsigned)ids[i]);
        if (ImGui::Button(label)) {
            // Select entity
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete")) {
            g->bridge->deleteEntity(ids[i]);
        }
    }
}

static void render_properties(LittGui *g) {
    ImGui::Text("Properties");
    ImGui::Separator();
    
    g->bridge->getCamera(&g->camera);
    
    ImGui::SliderFloat("FOV", &g->camera.fov, 30.0f, 120.0f);
    ImGui::SliderFloat("Exposure", &g->camera.exposure, 0.1f, 3.0f);
    ImGui::SliderFloat("Yaw", &g->camera.yaw, -180.0f, 180.0f);
    ImGui::SliderFloat("Pitch", &g->camera.pitch, -90.0f, 90.0f);
    
    if (g->cam_dirty) {
        g->bridge->setCamera(&g->camera);
        g->cam_dirty = false;
    }
}

static void render_render(LittGui *g) {
    ImGui::Text("Render Settings");
    ImGui::Separator();
    
    LittQualityPreset q = g->bridge->getQuality();
    if (ImGui::Combo("Quality", (int*)&q, QUALITY_NAMES, 6)) {
        g->bridge->setQuality(q);
    }
}

static void render_display(LittGui *g) {
    ImGui::Text("Display");
    ImGui::Separator();
    
    ImGui::Checkbox("Dark Theme", &g->dark_theme);
    ImGui::Checkbox("Show Logs", &g->show_logs);
    
    if (ImGui::Button("Capture Frame")) {
        uint8_t buf[1920 * 1080 * 4];
        int w, h;
        if (g->bridge->getFramebuffer(buf, &w, &h)) {
            // Save framebuffer
            FILE* f = fopen("framebuffer.png", "wb");
            if (f) {
                // Write simple PNG header + data
                fclose(f);
                g->log_lines[g->log_count % 200][0] = 'F';
                g->log_count++;
            }
        }
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void *litt_gui_create(void) {
    LittGui *g = (LittGui*)malloc(sizeof(LittGui));
    if (!g) return nullptr;
    
    g->bridge = new LittGuiBridge();
    g->active_panel = 0;
    g->show_logs = false;
    g->dark_theme = true;
    g->res_w = 1920;
    g->res_h = 1080;
    g->rendering = false;
    g->exposure = 1.0f;
    g->fov = 60.0f;
    g->log_count = 0;
    g->cam_dirty = false;
    
    memset(g->log_lines, 0, sizeof(g->log_lines));
    
    return g;
}

void litt_gui_destroy(void *handle) {
    if (!handle) return;
    LittGui *g = (LittGui*)handle;
    
    if (g->bridge) {
        g->bridge->disconnect();
        delete g->bridge;
    }
    free(g);
}

void litt_gui_render_frame(void *handle) {
    if (!handle) return;
    LittGui *g = (LittGui*)handle;
    
    ImGui::Begin("Litt Engine GUI");
    
    /* Panel selector */
    static const char *panels[] = {
        "Dashboard", "Entities", "Properties", "Render", "Display"
    };
    if (ImGui::Combo("Panel", &g->active_panel, panels, 5)) {
        g->cam_dirty = true;
    }
    ImGui::Separator();
    
    /* Render active panel */
    switch (g->active_panel) {
        case 0: render_dashboard(g); break;
        case 1: render_entities(g); break;
        case 2: render_properties(g); break;
        case 3: render_render(g); break;
        case 4: render_display(g); break;
    }
    
    ImGui::End();
}

LittEngine *litt_gui_get_engine(void *handle) {
    if (!handle) return nullptr;
    LittGui *g = (LittGui*)handle;
    return (LittEngine*)g->bridge;
}
