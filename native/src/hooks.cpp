// native/src/hooks.cpp
// C++17 | arm64-v8a | injected into com.mojang.minecraftpe
// Hooks: eglSwapBuffers (ImGui render), AInputQueue_preDispatchEvent (touch steal)

#include <dlfcn.h>
#include <EGL/egl.h>
#include <android/input.h>
#include <android/log.h>
#include <cstring>
#include <algorithm>
#include "dobby.h"
#include "hooks.h"
#include "imgui_menu.h"

#define TAG  "MCCheat/Hooks"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

uintptr_t g_level_ptr        = 0;
uintptr_t g_local_player     = 0;
bool      g_imgui_wants_mouse = false;

// ── eglSwapBuffers ────────────────────────────────────────────────────────────
typedef EGLBoolean (*fn_eglSwap)(EGLDisplay, EGLSurface);
static fn_eglSwap orig_swap = nullptr;
static bool       swap_init = false;

static EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH,  &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);

    if (w > 0 && h > 0) {
        if (!swap_init) { ImGuiInitialize(w, h); swap_init = true; }
        ImGuiFrame((float)w, (float)h, g_level_ptr, g_local_player);
    }
    return orig_swap(dpy, surf);
}

// ── AInputQueue_preDispatchEvent ──────────────────────────────────────────────
typedef int32_t (*fn_preDispatch)(AInputQueue*, AInputEvent*);
static fn_preDispatch orig_predispatch = nullptr;

static int32_t hook_preDispatch(AInputQueue* queue, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        bool down = (action == AMOTION_EVENT_ACTION_DOWN ||
                     action == AMOTION_EVENT_ACTION_MOVE);
        ImGuiFeedTouch(AMotionEvent_getX(event, 0),
                       AMotionEvent_getY(event, 0), down);
        if (g_imgui_wants_mouse) return 1; // consume — don't send to game
    }
    return orig_predispatch ? orig_predispatch(queue, event) : 0;
}

// ── Install / uninstall ───────────────────────────────────────────────────────
int InstallHooks() {
    void* libEGL = dlopen("libEGL.so", RTLD_NOW | RTLD_NOLOAD);
    if (!libEGL) { LOGE("libEGL not loaded"); return -1; }

    void* swap_sym = dlsym(libEGL, "eglSwapBuffers");
    if (!swap_sym) { LOGE("eglSwapBuffers not found"); return -1; }

    if (DobbyHook(swap_sym, (void*)hook_eglSwapBuffers, (void**)&orig_swap) != 0) {
        LOGE("DobbyHook(eglSwapBuffers) failed"); return -1;
    }
    LOGI("Hooked eglSwapBuffers @ %p", swap_sym);

    void* libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_NOLOAD);
    if (libandroid) {
        void* pd = dlsym(libandroid, "AInputQueue_preDispatchEvent");
        if (pd) {
            if (DobbyHook(pd, (void*)hook_preDispatch, (void**)&orig_predispatch) == 0)
                LOGI("Hooked AInputQueue_preDispatchEvent @ %p", pd);
            else
                LOGE("DobbyHook(preDispatch) failed — touch limited");
        }
    }

    return 0;
}

void UninstallHooks() {
    if (orig_swap)        DobbyDestroy((void*)hook_eglSwapBuffers);
    if (orig_predispatch) DobbyDestroy((void*)hook_preDispatch);
    orig_swap = nullptr;
    orig_predispatch = nullptr;
}
