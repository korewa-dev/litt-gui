cmake_minimum_required(VERSION 3.16)
project(litt-gui VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Litt Engine is consumed as a packaged SDK. Point LITT_SDK_ROOT at the
# directory produced by: make -C alt/src/native sdk
set(LITT_SDK_ROOT "" CACHE PATH "Litt Engine SDK root (contains include/ and lib/)")
if(NOT LITT_SDK_ROOT)
    message(FATAL_ERROR "Set -DLITT_SDK_ROOT=/path/to/litt-engine/alt/src/native/dist")
endif()

find_path(LITT_INCLUDE_DIR litt/litt_c.h PATHS "${LITT_SDK_ROOT}/include" NO_DEFAULT_PATH REQUIRED)
find_library(LITT_LIBRARY NAMES litt PATHS "${LITT_SDK_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)

# Bridge contract target. This compiles without Vulkan/GLFW/ImGui and is the
# integration gate used by CI.
add_library(litt-gui-bridge STATIC src/gui_bridge.cpp)
target_include_directories(litt-gui-bridge PUBLIC src "${LITT_INCLUDE_DIR}")
target_link_libraries(litt-gui-bridge PUBLIC "${LITT_LIBRARY}")
if(MSVC)
    target_compile_options(litt-gui-bridge PRIVATE /W4)
else()
    target_compile_options(litt-gui-bridge PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(LITT_GUI_BUILD_APP "Build the experimental Vulkan/ImGui desktop shell" OFF)
if(LITT_GUI_BUILD_APP)
    find_package(Vulkan REQUIRED)
    find_package(Threads REQUIRED)
    find_package(glfw3 REQUIRED)

    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
    if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
        message(FATAL_ERROR "ImGui sources missing. Initialize the imgui submodule before building the desktop shell.")
    endif()

    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )

    # The desktop shell is intentionally opt-in until its Vulkan frame/swapchain
    # path has its own release contract.
    add_executable(litt-gui src/main.cpp src/gui.cpp ${IMGUI_SOURCES})
    target_include_directories(litt-gui PRIVATE src ${IMGUI_DIR} ${IMGUI_DIR}/backends "${LITT_INCLUDE_DIR}")
    target_link_libraries(litt-gui PRIVATE litt-gui-bridge glfw Vulkan::Vulkan Threads::Threads ${CMAKE_DL_LIBS})
    if(MSVC)
        target_compile_options(litt-gui PRIVATE /W4)
    else()
        target_compile_options(litt-gui PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    install(TARGETS litt-gui DESTINATION bin)
endif()
