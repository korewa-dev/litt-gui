# Litt Engine GUI - C++ ImGui Frontend

A native C++ GUI frontend for the Litt Engine Vulkan path tracer, using Dear ImGui.

## Prerequisites

- **CMake** 3.20+
- **GCC 11+** or **Clang 14+** (with C++17 support)
- **Vulkan SDK** 1.3+
- **GLFW3** (with Vulkan support)
- **Dear ImGui** (submodule)

## Build

```bash
cd litt-gui-cpp

# Clone submodules (Dear ImGui)
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
    ├── engine_bridge.c # Engine communication (TCP)
    └── engine_bridge.h # Bridge API
```

## Controls

| Key | Action |
|-----|--------|
| 1-6 | Quality presets (Ultra Low to Ultra Max) |
| Space | Start/Stop rendering |
| R | Reset render |
| Escape | Exit |

## Engine Connection

The GUI connects to the Litt Engine REST API at `http://localhost:8080` by default.
The engine must be running before launching the GUI.

## License

MIT
