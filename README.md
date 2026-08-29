# Litt Engine GUI

A native C++ GUI frontend for the Litt Engine using Dear ImGui and Vulkan.

## Overview

This is the native desktop GUI for Litt Engine, providing:
- Real-time scene editing
- Entity and component management
- Visual rendering preview
- Integration with Litt Engine via TCP/REST API

## Current Status

✅ **Repository exists and is functional**
- GitHub: https://github.com/korewa-dev/litt-gui.git
- Branch: master
- Latest commit: Updated with proper C bindings to Litt Engine

## Prerequisites

- **CMake** 3.14+
- **GCC 11+** or **Clang 14+** (C++17)
- **Vulkan SDK** 1.3+
- **GLFW3** (with Vulkan support)
- **Dear ImGui** (submodule)

## Build Instructions

```bash
cd litt-gui-cpp

# Initialize submodules (Dear ImGui)
git submodule update --init --recursive

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel

# Run
./litt-gui
```

## Architecture

```
litt-gui-cpp/
├── CMakeLists.txt      # Build configuration
├── imgui/             # Dear ImGui (submodule)
└── src/
    ├── main.c         # Entry point, Vulkan init
    ├── gui.c          # ImGui UI panels
    ├── gui.h          # GUI API
    ├── gui_bridge.h   # C++ bridge header
    ├── gui_bridge.cpp # C++ bridge implementation
    ├── engine_bridge.c # Engine communication via TCP
    └── engine_bridge.h # Bridge API
```

## Components

### GUI Panels
1. **Dashboard** - Engine state, GPU info, render stats
2. **Entities** - List and manage game entities
3. **Properties** - Camera settings, transform
4. **Render** - Quality settings, exposure, FOV
5. **Display** - Camera settings, resolution

### Engine Bridge
- TCP connection to Litt Engine (localhost:8080)
- State management (Disconnected, Connecting, Connected, Running, Paused, Error)
- Quality presets (Ultra Low to Ultra Max)
- Camera control
- Frame buffer streaming

## Integration with Litt Engine

The GUI connects to the Litt Engine via:
1. **TCP/Socket** - Connect to running engine instance
2. **Direct C API** - Use `litt_c.h` bindings from `../litt engine/native/`
3. **JSON Protocol** - Send commands via JSON-RPC

## API Reference

### C API (litt_c.h)
```c
// Create engine
LittEngine* eng = litt_engine_create();

// Connect to engine
litt_connect(eng, "127.0.0.1", 8080);

// Create world
LittWorld* world = litt_world_create("scene.json", "assets/");

// Create entity
litt_entity_t id = litt_world_create_entity(world, &desc);

// Cleanup
litt_world_destroy(world);
litt_engine_destroy(eng);
```

### C++ Bridge (gui_bridge.h)
```cpp
LittGuiBridge bridge;
bridge.connect("127.0.0.1", 8080);
bridge.loadWorld("scene.json");
bridge.createEntity(&desc);
```

## License

Same as Litt Engine (MIT).
