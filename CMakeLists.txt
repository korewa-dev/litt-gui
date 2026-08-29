// Updated CMakeLists.txt for Litt GUI with proper dependencies

cmake_minimum_required(VERSION 3.14)
project(litt-gui VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED)

# Dear ImGui (submodule)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Litt Engine headers
set(LITT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../litt engine/native)
set(LITT_CORE_DIR ${LITT_INCLUDE_DIR}/littcore)

# Source files
set(SOURCES
    src/main.c
    src/gui.c
    src/engine_bridge.c
    src/gui_bridge.cpp
    ${IMGUI_SOURCES}
)

# Litt Engine core sources
set(LITT_CORE_SOURCES
    ${LITT_CORE_DIR}/litt_json.c
    ${LITT_CORE_DIR}/litt_obj.c
    ${LITT_CORE_DIR}/litt_math.cpp
    ${LITT_CORE_DIR}/litt_dither.cpp
    ${LITT_CORE_DIR}/litt_dither_renderer.cpp
    ${LITT_CORE_DIR}/litt_dither_vulkan.cpp
)

# Build executable
add_executable(litt-gui ${SOURCES} ${LITT_CORE_SOURCES})

target_include_directories(litt-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
    ${LITT_INCLUDE_DIR}
    ${LITT_CORE_DIR}
)

target_link_libraries(litt-gui PRIVATE
    glfw
    ${Vulkan_LIBRARIES}
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

target_compile_options(litt-gui PRIVATE -Wall -Wextra)

if(WIN32)
    target_compile_definitions(litt-gui PRIVATE
        IMGUI_DISABLE_WIN32_DEFAULT_IME_HANDLERS
        VK_USE_PLATFORM_WIN32_KHR
    )
endif()

# ── Submodule check ───────────────────────────────────────────────────────────
add_custom_target(fetch-deps
    COMMAND ${CMAKE_COMMAND} -E echo "Info: Run 'git submodule update --init --recursive' first"
    COMMAND ${CMAKE_COMMAND} -E echo "  Or manually place imgui/ in the project root"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS litt-gui DESTINATION bin)
