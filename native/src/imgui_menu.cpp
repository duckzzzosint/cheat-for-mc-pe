// native/src/imgui_menu.cpp
// C++17 | arm64-v8a | injected into com.mojang.minecraftpe
// Floating toggle button + tabbed cheat menu + ESP overlay via ImGui/GLES3

#include <GLES3/gl3.h>
#include <android/log.h>
#include <algorithm>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_menu.h"
#include "esp.h"
#include "hooks.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MCCheat/UI", __VA_ARGS__)

MenuState g_menu;

static float s_tx = 0, s_ty = 0;
static bool  s_td = false;

void ImGuiFeedTouch(float x, float y, bool down) {
    s_tx = x; s_ty = y; s_td = down;
}

// ── Init — called once on first frame ────────────────────────────────────────
void ImGuiInitialize(int w, int h) {
    if (ImGui::GetCurrentContext()) {
        ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 8.f;
    s.FrameRounding     = 4.f;
    s.GrabRounding      = 4.f;
    s.ScrollbarRounding = 4.f;
    s.ItemSpacing       = ImVec2(8, 6);
    s.WindowPadding     = ImVec2(10, 10);
    s.FramePadding      = ImVec2(6, 4);

    auto& c = s.Colors;
    c[ImGuiCol_WindowBg]      = ImVec4(0.07f, 0.07f, 0.07f, 0.93f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.50f, 0.85f, 0.00f, 0.90f);
    c[ImGuiCol_CheckMark]     = ImVec4(0.50f, 0.85f, 0.00f, 1.00f);
    c[ImGuiCol_SliderGrab]    = ImVec4(0.50f, 0.85f, 0.00f, 1.00f);
    c[ImGuiCol_FrameBg]       = ImVec4(0.14f, 0.14f, 0.14f, 0.90f);
    c[ImGuiCol_Button]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.85f, 0.00f, 0.60f);
    c[ImGuiCol_Tab]           = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_TabSelected]   = ImVec4(0.50f, 0.85f, 0.00f, 1.00f);
    c[ImGuiCol_Header]        = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.85f, 0.00f, 0.50f);
    c[ImGuiCol_Separator]     = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

    ImGui_ImplOpenGL3_Init("#version 300 es");
    LOGI("ImGui %dx%d", w, h);
}

// ── Per-frame render ──────────────────────────────────────────────────────────
void ImGuiFrame(float sw, float sh, uintptr_t level_ptr, uintptr_t local_player) {
    if (!ImGui::GetCurrentContext()) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(sw, sh);
    io.AddMousePosEvent(s_tx, s_ty);
    io.AddMouseButtonEvent(0, s_td);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // ── Toggle button (always visible, draggable) ─────────────────────────
    ImGui::SetNextWindowPos(ImVec2(g_menu.btn_x, g_menu.btn_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(56, 56), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));

    ImGui::Begin("##TB", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoSavedSettings);

    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 c(wp.x + 28, wp.y + 28);
    ImU32 col = g_menu.menu_visible
        ? IM_COL32(140,230,0,240) : IM_COL32(90,160,0,210);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(c, 26, col, 32);
    dl->AddCircle(c, 26, IM_COL32(255,255,255,70), 32, 1.5f);
    dl->AddText(ImVec2(wp.x+14, wp.y+20), IM_COL32(0,0,0,255), "MC");

    if (ImGui::InvisibleButton("##BTN", ImVec2(56,56)))
        g_menu.menu_visible = !g_menu.menu_visible;

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 6.f)) {
        ImVec2 d = io.MouseDelta;
        g_menu.btn_x = std::clamp(g_menu.btn_x + d.x, 0.f, sw - 56.f);
        g_menu.btn_y = std::clamp(g_menu.btn_y + d.y, 0.f, sh - 56.f);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    // ── Main menu ─────────────────────────────────────────────────────────
    if (g_menu.menu_visible) {
        ImGui::SetNextWindowPos(ImVec2(sw*.08f, sh*.08f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 440),         ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(220,300), ImVec2(500, sh*.9f));

        ImGui::Begin("MC Cheat  v1.0", &g_menu.menu_visible, ImGuiWindowFlags_NoCollapse);

        if (ImGui::BeginTabBar("##T")) {

            // ── Combat ────────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Combat")) {
                ImGui::Checkbox("Kill Aura", &g_menu.killaura_enabled);
                if (g_menu.killaura_enabled) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Radius##ka", &g_menu.killaura_radius, 2.f, 12.f, "%.1f");
                    ImGui::SliderFloat("CPS##ka",    &g_menu.killaura_cps,    1.f, 20.f, "%.1f");
                    ImGui::Checkbox("Multitarget",   &g_menu.killaura_multitarget);
                    ImGui::Unindent();
                }
                ImGui::Separator();
                ImGui::Checkbox("Hitbox Expander", &g_menu.hitbox_enabled);
                if (g_menu.hitbox_enabled) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Scale##hb", &g_menu.hitbox_scale, 1.f, 3.f, "%.2fx");
                    ImGui::Unindent();
                }
                ImGui::EndTabItem();
            }

            // ── Visual ────────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Visual")) {
                ImGui::Checkbox("ESP", &g_menu.esp_enabled);
                if (g_menu.esp_enabled) {
                    ImGui::Indent();
                    ImGui::Checkbox("Boxes##e",    &g_menu.esp_boxes);
                    ImGui::Checkbox("Lines##e",    &g_menu.esp_lines);
                    ImGui::Checkbox("Names##e",    &g_menu.esp_names);
                    ImGui::Checkbox("Health##e",   &g_menu.esp_health);
                    ImGui::Checkbox("All Mobs##e", &g_menu.esp_all_entities);
                    ImGui::Unindent();
                }
                ImGui::EndTabItem();
            }

            // ── Movement ──────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Move")) {
                ImGui::Checkbox("Fly", &g_menu.fly_enabled);
                if (g_menu.fly_enabled) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Speed##fy", &g_menu.fly_speed, 0.1f, 8.f, "%.1f");
                    ImGui::Text("Hold to ascend / descend:");
                    g_menu.fly_up   = ImGui::Button("  UP  "); ImGui::SameLine();
                    g_menu.fly_down = ImGui::Button("  DOWN  ");
                    ImGui::Unindent();
                }
                ImGui::Separator();
                ImGui::Checkbox("Speed", &g_menu.speed_enabled);
                if (g_menu.speed_enabled) {
                    ImGui::Indent();
                    ImGui::SliderFloat("Mult##sp", &g_menu.speed_multiplier, 1.f, 6.f, "%.1fx");
                    ImGui::Unindent();
                }
                ImGui::Separator();
                ImGui::Checkbox("NoFall", &g_menu.nofall_enabled);
                ImGui::EndTabItem();
            }

            // ── Bypass ────────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Bypass")) {
                ImGui::Checkbox("Bypass Mode", &g_menu.bypass_enabled);
                if (g_menu.bypass_enabled) {
                    ImGui::Indent();
                    ImGui::SliderFloat("CPS Variance", &g_menu.cps_variance, 0.f, 0.5f, "+-%.0f%%");
                    ImGui::Checkbox("Smooth Fly",      &g_menu.smooth_fly);
                    ImGui::Unindent();
                    ImGui::Spacing();
                    ImGui::TextDisabled(
                        "Randomizes click delay +- variance.\n"
                        "Smooth fly interpolates position\n"
                        "to avoid server teleport flags.");
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    // ── ESP overlay ───────────────────────────────────────────────────────
    if (g_menu.esp_enabled && level_ptr)
        ESPRender(level_ptr, sw, sh);

    g_imgui_wants_mouse = io.WantCaptureMouse;
    ImGui::Render();

    // Minimal GL state save/restore — don't corrupt Minecraft's pipeline
    GLint    vao=0, prog=0, blend_eq=0, blend_src=0, blend_dst=0;
    GLboolean blend_en=0, depth_en=0, cull_en=0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_CURRENT_PROGRAM,      &prog);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,   &blend_eq);
    glGetIntegerv(GL_BLEND_SRC_RGB,        &blend_src);
    glGetIntegerv(GL_BLEND_DST_RGB,        &blend_dst);
    glGetBooleanv(GL_BLEND,      &blend_en);
    glGetBooleanv(GL_DEPTH_TEST, &depth_en);
    glGetBooleanv(GL_CULL_FACE,  &cull_en);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glBindVertexArray(vao);
    glUseProgram(prog);
    glBlendEquation(blend_eq);
    glBlendFunc(blend_src, blend_dst);
    blend_en ? glEnable(GL_BLEND)      : glDisable(GL_BLEND);
    depth_en ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    cull_en  ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);
}
