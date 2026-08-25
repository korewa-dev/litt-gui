# Build script for Litt GUI C++ (Linux)

#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== Litt Engine GUI Build Script ==="
echo ""

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Install with: sudo apt install cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Install with: sudo apt install g++"
    exit 1
fi

if ! pkg-config --exists vulkan; then
    echo "ERROR: Vulkan SDK not found. Install from https://vulkan.lunarg.com/"
    exit 1
fi

if ! pkg-config --exists glfw3; then
    echo "WARNING: GLFW3 not found. Installing..."
    sudo apt install -y libglfw3-dev libglfw3-x11-dev
fi

echo "Prerequisites OK"
echo ""

# Clone submodules if needed
if [ ! -d "$SCRIPT_DIR/imgui" ]; then
    echo "Cloning Dear ImGui submodule..."
    git submodule update --init --recursive
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . --parallel

echo ""
echo "=== Build Complete ==="
echo "Executable: $BUILD_DIR/litt-gui"
echo ""
echo "To run:"
echo "  $BUILD_DIR/litt-gui"
