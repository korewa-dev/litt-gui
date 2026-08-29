// Litt GUI Bridge Implementation

#include "gui_bridge.h"
#include <cstdarg>
#include <cstdio>

LittGuiBridge::LittGuiBridge() 
    : engine_(litt_engine_create()), world_(nullptr) {
}

LittGuiBridge::~LittGuiBridge() {
    if (world_) {
        litt_world_destroy(world_);
        world_ = nullptr;
    }
    if (engine_) {
        litt_engine_destroy(engine_);
        engine_ = nullptr;
    }
}

bool LittGuiBridge::connect(const char* host, int port) {
    if (!engine_) return false;
    return litt_connect(engine_, host, port);
}

void LittGuiBridge::disconnect() {
    if (!engine_) return;
    litt_disconnect(engine_);
}

bool LittGuiBridge::isConnected() const {
    if (!engine_) return false;
    return litt_is_connected(engine_);
}

LittEngineState LittGuiBridge::getState() const {
    if (!engine_) return LITT_ENGINE_STATE_DISCONNECTED;
    return litt_get_state(engine_);
}

bool LittGuiBridge::loadWorld(const char* scene_path, const char* assets_base) {
    if (world_) {
        litt_world_destroy(world_);
        world_ = nullptr;
    }
    
    if (!scene_path || scene_path[0] == '\0') {
        world_ = litt_world_create(nullptr, nullptr);
    } else {
        world_ = litt_world_create(scene_path, assets_base);
        if (world_) {
            litt_world_load(world_, scene_path);
        }
    }
    
    return world_ != nullptr;
}

bool LittGuiBridge::saveWorld(const char* scene_path) {
    if (!world_ || !scene_path) return false;
    return litt_world_save(world_, scene_path);
}

void LittGuiBridge::destroyWorld() {
    if (world_) {
        litt_world_destroy(world_);
        world_ = nullptr;
    }
}

litt_entity_t LittGuiBridge::createEntity(const LittEntityDesc* desc) {
    if (!world_) return 0xFFFFFFFF;
    return litt_world_create_entity(world_, desc);
}

bool LittGuiBridge::deleteEntity(litt_entity_t entity_id) {
    if (!world_) return false;
    return litt_world_delete_entity(world_, entity_id);
}

bool LittGuiBridge::getEntity(litt_entity_t entity_id, LittEntityDesc* out) {
    if (!world_ || !out) return false;
    return litt_world_get_entity(world_, entity_id, out);
}

int LittGuiBridge::listEntities(litt_entity_t* ids, int max_count) {
    if (!world_ || !ids) return 0;
    return litt_world_list_entities(world_, ids, max_count);
}

bool LittGuiBridge::addComponent(litt_entity_t entity_id, LittComponentType type, const char* config_json) {
    if (!world_) return false;
    return litt_world_add_component(world_, entity_id, type, config_json);
}

bool LittGuiBridge::removeComponent(litt_entity_t entity_id, LittComponentType type) {
    if (!world_) return false;
    return litt_world_remove_component(world_, entity_id, type);
}

bool LittGuiBridge::hasComponent(litt_entity_t entity_id, LittComponentType type) {
    if (!world_) return false;
    return litt_world_has_component(world_, entity_id, type);
}

bool LittGuiBridge::setPosition(litt_entity_t entity_id, const litt_vec3_t* pos) {
    if (!world_ || !pos) return false;
    return litt_world_set_position(world_, entity_id, pos);
}

bool LittGuiBridge::getPosition(litt_entity_t entity_id, litt_vec3_t* out) {
    if (!world_ || !out) return false;
    return litt_world_get_position(world_, entity_id, out);
}

bool LittGuiBridge::setRotation(litt_entity_t entity_id, const litt_vec3_t* rot) {
    if (!world_ || !rot) return false;
    return litt_world_set_rotation(world_, entity_id, rot);
}

bool LittGuiBridge::getRotation(litt_entity_t entity_id, litt_vec3_t* out) {
    if (!world_ || !out) return false;
    return litt_world_get_rotation(world_, entity_id, out);
}

bool LittGuiBridge::setScale(litt_entity_t entity_id, const litt_vec3_t* scale) {
    if (!world_ || !scale) return false;
    return litt_world_set_scale(world_, entity_id, scale);
}

bool LittGuiBridge::getScale(litt_entity_t entity_id, litt_vec3_t* out) {
    if (!world_ || !out) return false;
    return litt_world_get_scale(world_, entity_id, out);
}

bool LittGuiBridge::startSimulation() {
    if (!world_) return false;
    return litt_world_start(world_);
}

bool LittGuiBridge::stopSimulation() {
    if (!world_) return false;
    return litt_world_stop(world_);
}

bool LittGuiBridge::isRunning() const {
    if (!world_) return false;
    return litt_world_is_running(world_);
}

void LittGuiBridge::step(float dt) {
    if (!world_) return;
    litt_world_step(world_, dt);
}

void LittGuiBridge::setQuality(LittQualityPreset quality) {
    if (!engine_) return;
    litt_engine_set_quality(engine_, quality);
}

LittQualityPreset LittGuiBridge::getQuality() const {
    if (!engine_) return LITT_QUALITY_MEDIUM;
    return litt_engine_get_quality(engine_);
}

void LittGuiBridge::getGpuInfo(LittGpuInfo* info) {
    if (!engine_) return;
    litt_engine_get_gpu_info(engine_, info);
}

void LittGuiBridge::getRenderStats(LittRenderStats* stats) {
    if (!engine_) return;
    litt_engine_get_render_stats(engine_, stats);
}

void LittGuiBridge::getCamera(LittCamera* cam) {
    if (!engine_) return;
    litt_engine_get_camera(engine_, cam);
}

void LittGuiBridge::setCamera(const LittCamera* cam) {
    if (!engine_) return;
    litt_engine_set_camera(engine_, cam);
}

bool LittGuiBridge::getFramebuffer(uint8_t* buf, int* width, int* height) {
    if (!engine_ || !buf) return false;
    return litt_engine_get_framebuffer(engine_, buf, width, height);
}

void LittGuiBridge::setLogCallback(LittLogCb cb, void* user) {
    if (!engine_) return;
    litt_engine_set_log_callback(engine_, cb, user);
}

void LittGuiBridge::log(const char* fmt, ...) {
    if (!engine_) return;
    
    va_list args;
    va_start(args, fmt);
    
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    litt_engine_log(engine_, "%s", buf);
}

const char* LittGuiBridge::getVersion() {
    return litt_version();
}
