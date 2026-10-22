// native/src/hooks.h
#pragma once
int  InstallHooks();
void UninstallHooks();

extern uintptr_t g_level_ptr;
extern uintptr_t g_local_player;
extern bool      g_imgui_wants_mouse;
