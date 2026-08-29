# Litt Engine GUI (litt-gui)

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
- Latest commit: 81ed610
- All source code present

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
    ├── gui.c          # ImGui UI panels (Dashboard, Entities, Scene, etc.)
    ├── gui.h          # GUI API
    ├── engine_bridge.c # Engine communication via TCP
    └── engine_bridge.h # Bridge API
```

## Components

### GUI Panels
1. **Dashboard** - Engine state, GPU info, render stats
2. **Entities** - List and manage game entities
3. **Scene** - Scene hierarchy and properties
4. **Properties** - Component inspector
5. **Render** - Quality settings, exposure, FOV
6. **Display** - Camera settings, resolution

### Engine Bridge
- TCP connection to Litt Engine (localhost:8080)
- State management (Disconnected, Connecting, Connected, Running, Paused, Error)
- Quality presets (Ultra Low to Ultra Max)
- Camera control
- Frame buffer streaming
- Logging

## Controls

| Key | Action |
|-----|--------|
| 1-6 | Quality presets (Ultra Low to Ultra Max) |
| Space | Start/Stop rendering |
| R | Reset render |
| Escape | Exit |

## Engine Connection

The GUI connects to Litt Engine at `http://localhost:8080` by default.

The engine must be running before launching the GUI:

```bash
# Start Litt Engine with API
littcli server --port 8080

# Then launch GUI
./litt-gui
```

## License

MIT
