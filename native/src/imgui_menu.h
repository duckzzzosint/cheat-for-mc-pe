// native/src/imgui_menu.h
#pragma once
#include <cstdint>

struct MenuState {
    float btn_x = 20.0f, btn_y = 120.0f;
    bool  menu_visible = false;

    // ESP
    bool esp_enabled      = false;
    bool esp_boxes        = true;
    bool esp_lines        = true;
    bool esp_names        = true;
    bool esp_health       = true;
    bool esp_all_entities = false;

    // Kill Aura
    bool  killaura_enabled     = false;
    float killaura_radius      = 5.0f;
    float killaura_cps         = 8.0f;
    bool  killaura_multitarget = false;

    // Fly
    bool  fly_enabled = false;
    float fly_speed   = 1.0f;
    bool  fly_up      = false;
    bool  fly_down    = false;

    // Speed
    bool  speed_enabled    = false;
    float speed_multiplier = 1.5f;

    // NoFall
    bool nofall_enabled = false;

    // Hitbox
    bool  hitbox_enabled = false;
    float hitbox_scale   = 1.5f;

    // Bypass
    bool  bypass_enabled = true;
    bool  smooth_fly     = true;
    float cps_variance   = 0.20f;
};

extern MenuState g_menu;

void ImGuiInitialize(int w, int h);
void ImGuiFrame(float sw, float sh, uintptr_t level_ptr, uintptr_t local_player);
void ImGuiFeedTouch(float x, float y, bool down);
