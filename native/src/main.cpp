// native/src/main.cpp
// C++17 | arm64-v8a | dlopen'd into com.mojang.minecraftpe
// Entry: __attribute__((constructor)) fires when dlopen completes.

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdint>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <android/log.h>
#include "offsets.h"
#include "types.h"
#include "imgui_menu.h"
#include "hooks.h"

#define TAG  "MCCheat"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static std::atomic<bool> g_running{true};

void KillAuraTick(uintptr_t level, uintptr_t local_player);
void MovementTick(uintptr_t level, uintptr_t local_player);

// ── Locate loaded .so base via /proc/self/maps ────────────────────────────────
uintptr_t GetLibBase(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, name) && strstr(line, "r-xp")) {
            sscanf(line, "%lx-", &base);
            break;
        }
    }
    fclose(fp);
    return base;
}

// ── Cheat tick ~62 Hz ─────────────────────────────────────────────────────────
static void* CheatThread(void*) {
    LOGI("CheatThread started");
    sleep(2);

    uintptr_t mc_base = 0;

    while (g_running) {
        if (!mc_base) {
            mc_base = GetLibBase("libminecraftpe.so");
            if (!mc_base) { usleep(500000); continue; }
            LOGI("libminecraftpe base: 0x%lx", mc_base);
        }

        // FILL: CLIENT_INSTANCE_STATIC must be set in offsets.h
        uintptr_t client = 0;
        if (Offsets::CLIENT_INSTANCE_STATIC)
            client = Mem<uintptr_t>(mc_base + Offsets::CLIENT_INSTANCE_STATIC);

        if (!client) { usleep(16000); continue; }

        uintptr_t level        = SafeMem<uintptr_t>(client, Offsets::CLINST_LEVEL);
        uintptr_t local_player = SafeMem<uintptr_t>(level,  Offsets::LEVEL_LOCAL_PLAYER);

        g_level_ptr    = level;
        g_local_player = local_player;

        if (local_player) {
            KillAuraTick(level, local_player);
            MovementTick(level, local_player);
        }

        usleep(16000);
    }

    LOGI("CheatThread exit");
    return nullptr;
}

// ── .so constructor / destructor ──────────────────────────────────────────────
__attribute__((constructor))
static void lib_init() {
    LOGI("libcheat loaded");

    if (InstallHooks() < 0) {
        LOGE("Hook install failed");
        return;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t tid;
    pthread_create(&tid, &attr, CheatThread, nullptr);
    pthread_attr_destroy(&attr);

    LOGI("libcheat ready");
}

__attribute__((destructor))
static void lib_deinit() {
    g_running = false;
    UninstallHooks();
    LOGI("libcheat unloaded");
}
