// native/src/movement.cpp
// C++17 | arm64-v8a | injected into com.mojang.minecraftpe
// Fly / Speed / NoFall / Hitbox: modify velocity and collision state each tick.

#include <cstdint>
#include <algorithm>
#include "imgui_menu.h"
#include "offsets.h"
#include "types.h"
#include "bypass.h"

static Vec3 s_smooth_vel = {};

void MovementTick(uintptr_t level, uintptr_t local_player) {
    if (!local_player) return;

    Vec3 vel = SafeMem<Vec3>(local_player, Offsets::ACTOR_VELOCITY);

    // ── NoFall ────────────────────────────────────────────────────────────
    if (g_menu.nofall_enabled) {
        MemW<float>(local_player, Offsets::ACTOR_FALL_DIST, 0.f);
        if (vel.y < -0.08f) vel.y = 0.f;
    }

    // ── Fly ───────────────────────────────────────────────────────────────
    if (g_menu.fly_enabled) {
        float target_vy = 0.f;
        if (g_menu.fly_up)   target_vy =  g_menu.fly_speed;
        if (g_menu.fly_down) target_vy = -g_menu.fly_speed;

        if (g_menu.smooth_fly && g_menu.bypass_enabled) {
            Vec3 tv = {vel.x, target_vy, vel.z};
            Vec3 sv = BypassSmooth({vel.x, s_smooth_vel.y, vel.z}, tv, 0.25f);
            vel.y = sv.y;
            s_smooth_vel = sv;
        } else {
            vel.y = target_vy;
        }

        MemW<bool> (local_player, Offsets::ACTOR_ON_GROUND, true);
        MemW<float>(local_player, Offsets::ACTOR_FALL_DIST, 0.f);
    }

    // ── Speed ─────────────────────────────────────────────────────────────
    if (g_menu.speed_enabled) {
        float horiz = vel.len_xz();
        if (horiz > 0.02f) {
            float mult      = g_menu.speed_multiplier;
            float max_speed = 0.4f * mult;
            vel.x *= mult;
            vel.z *= mult;
            float new_h = vel.len_xz();
            if (new_h > max_speed) {
                float cf = max_speed / new_h;
                vel.x *= cf; vel.z *= cf;
            }
            if (g_menu.bypass_enabled)
                vel = BypassJitter(vel, 0.008f);
        }
    }

    // ── Hitbox expander ───────────────────────────────────────────────────
    if (g_menu.hitbox_enabled) {
        uintptr_t ent_list  = SafeMem<uintptr_t>(level, Offsets::LEVEL_ENTITY_LIST);
        int32_t   ent_count = SafeMem<int32_t>(level,   Offsets::LEVEL_ENTITY_COUNT);

        for (int i = 0; i < ent_count && i < 512; i++) {
            uintptr_t ent = SafeMem<uintptr_t>(
                ent_list + (uintptr_t)i * sizeof(uintptr_t), 0);
            if (!ent || ent == local_player) continue;

            Vec3 mn = SafeMem<Vec3>(ent, Offsets::ACTOR_AABB_MIN);
            Vec3 mx = SafeMem<Vec3>(ent, Offsets::ACTOR_AABB_MAX);
            float cx = (mn.x + mx.x) * .5f;
            float cz = (mn.z + mx.z) * .5f;
            float hx = (mx.x - mn.x) * .5f * g_menu.hitbox_scale;
            float hy = (mx.y - mn.y)        * g_menu.hitbox_scale;
            float hz = (mx.z - mn.z) * .5f * g_menu.hitbox_scale;

            MemW<Vec3>(ent, Offsets::ACTOR_AABB_MIN, {cx-hx, mn.y,      cz-hz});
            MemW<Vec3>(ent, Offsets::ACTOR_AABB_MAX, {cx+hx, mn.y + hy, cz+hz});
        }
    }

    MemW<Vec3>(local_player, Offsets::ACTOR_VELOCITY, vel);
}
