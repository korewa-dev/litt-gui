cmake_minimum_required(VERSION 3.14)
project(litt-gui-cpp VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Dependencies — set paths via -D flags if not in standard locations
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# Dear ImGui (submodule — clone with: git submodule update --init --recursive)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

find_package(glfw3 REQUIRED)

# Source files
set(SOURCES src/main.c src/gui.c src/engine_bridge.c)

add_executable(litt-gui ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(litt-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
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

# ── Convenience targets ───────────────────────────────────────────────────────

# Download and extract ImGui + GLFW as submodules if missing
add_custom_target(fetch-deps ALL
    COMMAND ${CMAKE_COMMAND} -E echo
            "Info: please run 'git submodule update --init --recursive' first"
    COMMAND ${CMAKE_COMMAND} -E echo
            "Or manually place imgui/ and src/glfw/ in the project root"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS litt-gui DESTINATION bin)
