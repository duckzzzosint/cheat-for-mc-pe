cmake_minimum_required(VERSION 3.18)
project(mccheat CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# ── ImGui ─────────────────────────────────────────────────────────────────────
# IMGUI_DIR is passed from CI via -DIMGUI_DIR=<pre-cloned path>.
# Fallback: FetchContent download (local builds only).
if(DEFINED IMGUI_DIR AND EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "ImGui: using pre-cloned dir ${IMGUI_DIR}")
    set(IMGUI_ROOT "${IMGUI_DIR}")
else()
    message(STATUS "ImGui: fetching from git...")
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_Populate(imgui)
    set(IMGUI_ROOT "${imgui_SOURCE_DIR}")
endif()

if(NOT EXISTS "${IMGUI_ROOT}/imgui.cpp")
    message(FATAL_ERROR "ImGui not found at ${IMGUI_ROOT}")
endif()
message(STATUS "ImGui root: ${IMGUI_ROOT}")

set(IMGUI_SRC
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp
)

# ── Dobby ─────────────────────────────────────────────────────────────────────
set(DOBBY_BUILD_EXAMPLE         OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_SHARED_LIBRARY  OFF CACHE BOOL "" FORCE)
set(DOBBY_GENERATE_SHARED       OFF CACHE BOOL "" FORCE)
set(DOBBY_DEBUG                 OFF CACHE BOOL "" FORCE)
FetchContent_Declare(dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(dobby)

# ── libcheat.so ───────────────────────────────────────────────────────────────
add_library(cheat SHARED
    native/src/main.cpp
    native/src/hooks.cpp
    native/src/imgui_menu.cpp
    native/src/esp.cpp
    native/src/killaura.cpp
    native/src/movement.cpp
    native/src/bypass.cpp
    ${IMGUI_SRC}
)

target_include_directories(cheat PRIVATE
    native/src
    ${IMGUI_ROOT}
    ${IMGUI_ROOT}/backends
    ${dobby_SOURCE_DIR}/include
)

target_compile_definitions(cheat PRIVATE
    IMGUI_IMPL_OPENGL_ES3
    IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(cheat android log dl EGL GLESv3 dobby)

# ── mc_injector ───────────────────────────────────────────────────────────────
add_executable(mc_injector native/src/injector.cpp)
target_include_directories(mc_injector PRIVATE native/src)
target_compile_options(mc_injector PRIVATE -O2 -fPIE)
target_link_options(mc_injector PRIVATE -fPIE -pie)
target_link_libraries(mc_injector log)

# ── Copy to assets ────────────────────────────────────────────────────────────
set(ASSETS_DIR ${CMAKE_SOURCE_DIR}/app/src/main/assets)
file(MAKE_DIRECTORY ${ASSETS_DIR})

add_custom_command(TARGET mc_injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mc_injector> ${ASSETS_DIR}/mc_injector)

add_custom_command(TARGET cheat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cheat> ${ASSETS_DIR}/libcheat.so)
