cmake_minimum_required(VERSION 3.18)
project(mccheat CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
# If FETCHCONTENT_SOURCE_DIR_IMGUI / _DOBBY are set (via -D from Gradle),
# FetchContent uses those local dirs and skips all git operations.

# ── ImGui ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
    GIT_SHALLOW    TRUE
)
FetchContent_Populate(imgui)

set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# ── Dobby ─────────────────────────────────────────────────────────────────────
set(DOBBY_BUILD_EXAMPLE         OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_SHARED_LIBRARY  OFF CACHE BOOL "" FORCE)
set(DOBBY_GENERATE_SHARED       OFF CACHE BOOL "" FORCE)
set(DOBBY_DEBUG                 OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    dobby
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
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
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
