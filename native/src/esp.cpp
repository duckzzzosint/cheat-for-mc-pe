// native/src/esp.cpp
// C++17 | arm64-v8a | injected into com.mojang.minecraftpe
// Reads entity list from memory, projects AABBs to screen, draws via ImGui.

#include <cstring>
#include <algorithm>
#include "esp.h"
#include "imgui.h"
#include "imgui_menu.h"
#include "offsets.h"
#include "types.h"

// Column-major ViewProj matrix -> clip -> NDC -> screen
static bool W2S(Vec3 world, Vec2& screen, const float vp[16], float sw, float sh) {
    float cx = vp[0]*world.x + vp[4]*world.y + vp[8]*world.z  + vp[12];
    float cy = vp[1]*world.x + vp[5]*world.y + vp[9]*world.z  + vp[13];
    float cw = vp[3]*world.x + vp[7]*world.y + vp[11]*world.z + vp[15];
    if (cw < 0.01f) return false;
    screen.x = ( cx/cw + 1.f) * 0.5f * sw;
    screen.y = (-cy/cw + 1.f) * 0.5f * sh;
    return true;
}

void ESPRender(uintptr_t level_ptr, float sw, float sh) {
    if (!level_ptr) return;

    // CAMERA_VP_MATRIX must be set to the actual live pointer before ESP works
    if (!Offsets::CAMERA_VP_MATRIX) return;
    float vp[16];
    memcpy(vp, reinterpret_cast<const void*>(Offsets::CAMERA_VP_MATRIX), sizeof(vp));

    uintptr_t ent_list  = SafeMem<uintptr_t>(level_ptr, Offsets::LEVEL_ENTITY_LIST);
    int32_t   ent_count = SafeMem<int32_t>(level_ptr,   Offsets::LEVEL_ENTITY_COUNT);
    if (!ent_list || ent_count <= 0) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    for (int i = 0; i < ent_count && i < 512; i++) {
        uintptr_t ent = SafeMem<uintptr_t>(ent_list + (uintptr_t)i * sizeof(uintptr_t), 0);
        if (!ent) continue;

        int32_t type     = SafeMem<int32_t>(ent, Offsets::ACTOR_TYPE_ID);
        bool is_player   = (type & 0x001) != 0; // FILL: adjust bits per version
        bool is_mob      = (type & 0x040) != 0; // FILL: adjust bits per version
        if (!is_player && !is_mob && !g_menu.esp_all_entities) continue;

        Vec3  aabb_min = SafeMem<Vec3>(ent, Offsets::ACTOR_AABB_MIN);
        Vec3  aabb_max = SafeMem<Vec3>(ent, Offsets::ACTOR_AABB_MAX);
        float hp       = SafeMem<float>(ent, Offsets::ACTOR_HEALTH);
        float max_hp   = SafeMem<float>(ent, Offsets::ACTOR_MAX_HEALTH);

        // Project all 8 AABB corners
        Vec3 corners[8] = {
            {aabb_min.x, aabb_min.y, aabb_min.z}, {aabb_max.x, aabb_min.y, aabb_min.z},
            {aabb_min.x, aabb_max.y, aabb_min.z}, {aabb_max.x, aabb_max.y, aabb_min.z},
            {aabb_min.x, aabb_min.y, aabb_max.z}, {aabb_max.x, aabb_min.y, aabb_max.z},
            {aabb_min.x, aabb_max.y, aabb_max.z}, {aabb_max.x, aabb_max.y, aabb_max.z},
        };
        Vec2 pts[8]; bool behind = false;
        for (int j = 0; j < 8; j++)
            if (!W2S(corners[j], pts[j], vp, sw, sh)) { behind = true; break; }
        if (behind) continue;

        float x0=pts[0].x, x1=pts[0].x, y0=pts[0].y, y1=pts[0].y;
        for (int j = 1; j < 8; j++) {
            x0 = std::min(x0, pts[j].x); x1 = std::max(x1, pts[j].x);
            y0 = std::min(y0, pts[j].y); y1 = std::max(y1, pts[j].y);
        }

        ImU32 col = is_player ? IM_COL32(255,60,60,220) : IM_COL32(60,220,60,220);

        if (g_menu.esp_boxes)
            dl->AddRect(ImVec2(x0,y0), ImVec2(x1,y1), col, 0, 0, 1.5f);

        if (g_menu.esp_lines)
            dl->AddLine(ImVec2(sw*.5f, sh), ImVec2((x0+x1)*.5f, y1), col, 1.f);

        if (g_menu.esp_health && max_hp > 0.f) {
            float ratio = std::clamp(hp/max_hp, 0.f, 1.f);
            float bx    = x0 - 5.f;
            float bh    = y1 - y0;
            dl->AddRectFilled(ImVec2(bx-3,y0), ImVec2(bx,y1), IM_COL32(30,30,30,180));
            ImU32 hc = ratio>.5f ? IM_COL32(0,210,0,220) : IM_COL32(220,0,0,220);
            dl->AddRectFilled(ImVec2(bx-3, y0+bh*(1.f-ratio)), ImVec2(bx,y1), hc);
        }

        if (g_menu.esp_names) {
            char name[64] = {};
            ReadStdStr(ent + Offsets::ACTOR_NAME_TAG, name, sizeof(name));
            if (name[0]) dl->AddText(ImVec2(x0, y0-14.f), col, name);
        }
    }
}
