#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Internal state ──────────────────────────────────────────────────────── */
typedef struct {
    LittEngine      *engine;
    int              active_panel;   /* 0=Dashboard … 7=Display */
    bool             show_logs;
    bool             dark_theme;
    LittCamera       camera;
    bool             cam_dirty;
    uint32_t         res_w, res_h;
    bool             rendering;
    float            exposure;
    float            fov;
    LittRenderStats  stats;
    char             log_lines[200][256];
    int              log_count;
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
    LittEngineState st = litt_get_state(g->engine);
    ImU32 col = IM_COL32(200,200,200,255);
    switch (st) {
        case LITT_ENGINE_DISCONNECTED: col = IM_COL32(220,50,50,255);  break;
        case LITT_ENGINE_CONNECTING:   col = IM_COL32(220,220,50,255); break;
        case LITT_ENGINE_CONNECTED:    col = IM_COL32(50,200,50,255);  break;
        case LITT_ENGINE_RUNNING:      col = IM_COL32(50,150,220,255); break;
        case LITT_ENGINE_PAUSED:       col = IM_COL32(220,130,50,255); break;
        case LITT_ENGINE_ERROR:        col = IM_COL32(220,50,50,255);  break;
    }
    ImGui::TextColored(ImColor(col),
        st == LITT_ENGINE_DISCONNECTED  ? "Disconnected" :
        st == LITT_ENGINE_CONNECTING    ? "Connecting..." :
        st == LITT_ENGINE_CONNECTED     ? "Connected" :
        st == LITT_ENGINE_RUNNING       ? "Running" :
        st == LITT_ENGINE_PAUSED        ? "Paused" : "Error");
    ImGui::SameLine();

    /* Quick quality buttons */
    ImGui::Text("Quick Presets:");
    for (int i = 0; i < 6; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d. %s", i+1, QUALITY_NAMES[i]);
        if (ImGui::Button(buf)) {
            litt_set_quality(g->engine, (LittQualityPreset)i);
        }
        if (i < 5) ImGui::SameLine();
    }
    ImGui::NewLine();

    /* Render controls */
    ImGui::Text("Render:");
    if (ImGui::Button(g->rendering ? "Stop  [Space]" : "Start  [Space]")) {
        if (g->rendering) litt_stop_render(g->engine);
        else              litt_start_render(g->engine);
        g->rendering = !g->rendering;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset  [R]")) {
        litt_reset_render(g->engine);
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        litt_pause_render(g->engine);
    }

    /* Stats */
    g->stats = litt_get_stats(g->engine);
    ImGui::Text("FPS: %.1f  |  Frame: %.2f ms  |  Trace: %.2f ms",
                g->stats.fps, g->stats.frame_time_ms, g->stats.path_time_ms);
    ImGui::Text("Bounces: %u  |  AA: %u  |  %ux%u",
                g->stats.bounces, g->stats.spp,
                g->stats.width, g->stats.height);
}

static void render_quality(LittGui *g) {
    ImGui::Text("Quality Settings");
    ImGui::Separator();

    int cur = (int)litt_get_quality(g->engine);
    if (ImGui::Combo("Preset", &cur, QUALITY_NAMES, 6))
        litt_set_quality(g->engine, (LittQualityPreset)cur);

    ImGui::InputInt2("Resolution", (int*)&g->res_w, (int*)&g->res_h);
    if (g->res_w > 0 && g->res_h > 0)
        litt_set_resolution(g->engine, g->res_w, g->res_h);

    int bounces = g->stats.bounces;
    if (ImGui::SliderInt("Max Bounces", &bounces, 1, 64))
        litt_set_bounces(g->engine, (uint32_t)bounces);

    int aa = g->stats.spp;
    if (ImGui::SliderInt("AA Samples", &aa, 1, 8))
        litt_set_aa_samples(g->engine, (uint32_t)aa);
}

static void render_camera(LittGui *g) {
    ImGui::Text("Camera");
    ImGui::Separator();

    g->camera = litt_get_camera(g->engine);

    if (ImGui::SliderFloat3("Position (X Y Z)",
                            &g->camera.pos_x, -100.0f, 100.0f))
        g->cam_dirty = true;
    if (ImGui::SliderFloat("Yaw", &g->camera.yaw, -3.14159f, 3.14159f))
        g->cam_dirty = true;
    if (ImGui::SliderFloat("Pitch", &g->camera.pitch, -1.57f, 1.57f))
        g->cam_dirty = true;
    if (ImGui::SliderFloat("FOV", &g->fov, 0.1f, 3.14f)) {
        g->camera.fov = g->fov;
        g->cam_dirty = true;
    }
    if (ImGui::SliderFloat("Exposure", &g->exposure, 0.0f, 5.0f)) {
        g->camera.exposure = g->exposure;
        g->cam_dirty = true;
    }

    if (g->cam_dirty) {
        litt_set_camera(g->engine, &g->camera);
        g->cam_dirty = false;
    }
}

static void render_scene(LittGui *g) {
    ImGui::Text("Scene");
    ImGui::Separator();
    ImGui::Text("Preset: TestRoom");
    ImGui::Text("Shadow quality: High");
    ImGui::Text("Lights: 3");
}

static void render_tracer(LittGui *g) {
    ImGui::Text("Path Tracer");
    ImGui::Separator();
    ImGui::Checkbox("Russian Roulette", nullptr);
    ImGui::Checkbox("Next Event Estimation", nullptr);
    ImGui::SliderInt("Max Depth", nullptr, 1, 64);
}

static void render_fxfx(LittGui *g) {
    ImGui::Text("FidelityFX");
    ImGui::Separator();
    ImGui::Checkbox("FSR 3 Frame Gen", nullptr);
    ImGui::Checkbox("FSR 4", nullptr);
    ImGui::Checkbox("CAS Sharpen", nullptr);
    ImGui::Checkbox("Ray Reconstruction", nullptr);
}

static void render_npu(LittGui *g) {
    ImGui::Text("NPU Acceleration");
    ImGui::Separator();
    ImGui::Checkbox("Enable NPU", nullptr);
    ImGui::Text("Backend: DirectML");
    ImGui::Text("Model: ONNX");
}

static void render_display(LittGui *g) {
    ImGui::Text("Display");
    ImGui::Separator();
    ImGui::Checkbox("Fullscreen", nullptr);
    ImGui::Checkbox("VSync", nullptr);
    ImGui::Text("Window: %ux%u", g->res_w, g->res_h);
}

static void render_logs_panel(LittGui *g) {
    if (!g->show_logs) return;
    ImGui::OpenPopup("Logs");
    if (!ImGui::BeginPopupModal("Logs", nullptr,
                                ImGuiWindowFlags_TopMost|ImGuiWindowFlags_AlwaysAutoResize))
        return;
    ImGui::Text("Engine Logs");
    ImGui::Separator();
    for (int i = 0; i < g->log_count; i++)
        ImGui::Text("%s", g->log_lines[i]);
    if (ImGui::Button("Close")) {
        g->show_logs = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

static void render_about(LittGui *g) {
    ImGui::OpenPopup("About");
    if (!ImGui::BeginPopupModal("About", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize))
        return;
    ImGui::Text("Litt Engine GUI v0.1.0");
    ImGui::Text("C++ / ImGui / Vulkan frontend");
    ImGui::Separator();
    ImGui::Text("Keys:");
    ImGui::BulletText("1-6  Quality presets");
    ImGui::BulletText("Sp   Start / Stop render");
    ImGui::BulletText("R    Reset render");
    ImGui::BulletText("Esc  Exit");
    if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void *litt_gui_create(void) {
    LittGui *g = (LittGui *)calloc(1, sizeof(LittGui));
    if (!g) return nullptr;

    g->engine      = (LittEngine *)calloc(1, sizeof(LittEngine));
    g->active_panel= 0;
    g->dark_theme  = true;
    g->res_w       = 1920;
    g->res_h       = 1080;
    g->exposure    = 1.0f;
    g->fov         = 1.047f;
    g->quality     = LITT_HIGH;

    /* Connect to engine */
    if (!litt_connect(g->engine, "127.0.0.1", 8080)) {
        fprintf(stderr, "[gui] Cannot connect to engine at 127.0.0.1:8080\n");
    }

    /* Register log callback */
    litt_set_log_cb(g->engine, [](const char *msg, void *user) {
        LittGui *g = (LittGui *)user;
        int idx = g->log_count % 200;
        strncpy(g->log_lines[idx], msg, 255);
        g->log_lines[idx][255] = '\0';
        if (g->log_count < 200) g->log_count++;
    }, g);

    return (void *)g;
}

void litt_gui_destroy(void *handle) {
    if (!handle) return;
    LittGui *g = (LittGui *)handle;
    litt_disconnect(g->engine);
    free(g->engine);
    free(g);
}

void litt_gui_render_frame(void *handle) {
    if (!handle) return;
    LittGui *g = (LittGui *)handle;

    ImGui::NewFrame();

    /* ── Menu bar ─────────────────────────────────────────────────────── */
    ImGui::BeginMainMenuBar();
    ImGui::Text("Litt Engine GUI v0.1.0");
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(
            (GLFWwindow *)ImGui::GetIO().PlatformGetPlatformInfo()->Window, true);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Logs", nullptr, g->show_logs)) g->show_logs = !g->show_logs;
        if (ImGui::MenuItem("Dark Theme", nullptr, g->dark_theme)) {
            g->dark_theme = !g->dark_theme;
            ImGui::StyleColorsDark();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) render_about(g);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    /* ── Sidebar ──────────────────────────────────────────────────────── */
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(180, ImGui::GetIO().DisplaySize.y));
    ImGui::Begin("Sidebar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    static const char *panels[] = {
        "Dashboard", "Quality", "Camera", "Scene",
        "Tracer", "FidelityFX", "NPU", "Display"
    };
    for (int i = 0; i < 8; i++) {
        if (ImGui::Button(panels[i], ImVec2(160, 32)))
            g->active_panel = i;
        ImGui::Separator();
    }
    ImGui::End();

    /* ── Main panel ───────────────────────────────────────────────────── */
    ImGui::SetNextWindowPos(ImVec2(180, 28));
    ImGui::SetNextWindowSize(ImVec2(
        ImGui::GetIO().DisplaySize.x - 180,
        ImGui::GetIO().DisplaySize.y - 28 - 28));
    ImGui::Begin("Content", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove);

    switch (g->active_panel) {
        case 0: render_dashboard(g);      break;
        case 1: render_quality(g);        break;
        case 2: render_camera(g);         break;
        case 3: render_scene(g);          break;
        case 4: render_tracer(g);         break;
        case 5: render_fxfx(g);           break;
        case 6: render_npu(g);            break;
        case 7: render_display(g);        break;
    }

    ImGui::End();

    /* ── Status bar ───────────────────────────────────────────────────── */
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 28));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 28));
    ImGui::Begin("Status", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    LittRenderStats s = litt_get_stats(g->engine);
    ImGui::Text("GPU: %s  |  FPS: %.1f  |  Frame: %.2f ms  |  %ux%u",
                g->engine->gpu.name[0] ? g->engine->gpu.name : "Unknown",
                s.fps, s.frame_time_ms, s.width, s.height);
    if (litt_get_state(g->engine) == LITT_ENGINE_RUNNING) {
        ImGui::SameLine();
        ImGui::TextColored(ImColor(IM_COL32(50,200,50,255)), " ● Rendering");
    }
    ImGui::End();

    /* ── Popups ───────────────────────────────────────────────────────── */
    render_logs_panel(g);

    ImGui::Render();
}

LittEngine *litt_gui_get_engine(void *handle) {
    if (!handle) return nullptr;
    return ((LittGui *)handle)->engine;
}
