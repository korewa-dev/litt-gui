// Litt GUI Bridge - C++ bridge for the Litt Engine GUI
// Connects ImGui GUI to the C API

#pragma once
#include "litt_c.h"
#include <string>
#include <vector>
#include <functional>

class LittGuiBridge {
public:
    LittGuiBridge();
    ~LittGuiBridge();

    // Engine management
    bool connect(const char* host, int port);
    void disconnect();
    bool isConnected() const;
    LittEngineState getState() const;

    // World management
    bool loadWorld(const char* scene_path, const char* assets_base = nullptr);
    bool saveWorld(const char* scene_path);
    void destroyWorld();

    // Entity management
    litt_entity_t createEntity(const LittEntityDesc* desc);
    bool deleteEntity(litt_entity_t entity_id);
    bool getEntity(litt_entity_t entity_id, LittEntityDesc* out);
    int listEntities(litt_entity_t* ids, int max_count);

    // Component management
    bool addComponent(litt_entity_t entity_id, LittComponentType type, const char* config_json);
    bool removeComponent(litt_entity_t entity_id, LittComponentType type);
    bool hasComponent(litt_entity_t entity_id, LittComponentType type);

    // Transform
    bool setPosition(litt_entity_t entity_id, const litt_vec3_t* pos);
    bool getPosition(litt_entity_t entity_id, litt_vec3_t* out);
    bool setRotation(litt_entity_t entity_id, const litt_vec3_t* rot);
    bool getRotation(litt_entity_t entity_id, litt_vec3_t* out);
    bool setScale(litt_entity_t entity_id, const litt_vec3_t* scale);
    bool getScale(litt_entity_t entity_id, litt_vec3_t* out);

    // Simulation
    bool startSimulation();
    bool stopSimulation();
    bool isRunning() const;
    void step(float dt);

    // Rendering
    void setQuality(LittQualityPreset quality);
    LittQualityPreset getQuality() const;
    void getGpuInfo(LittGpuInfo* info);
    void getRenderStats(LittRenderStats* stats);
    void getCamera(LittCamera* cam);
    void setCamera(const LittCamera* cam);
    bool getFramebuffer(uint8_t* buf, int* width, int* height);

    // Logging
    void setLogCallback(LittLogCb cb, void* user);
    void log(const char* fmt, ...);

    // Version
    static const char* getVersion();

private:
    LittEngine* engine_;
    LittWorld* world_;
};
