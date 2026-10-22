// native/src/killaura.cpp
// C++17 | arm64-v8a | injected into com.mojang.minecraftpe
// KillAura: attack nearest entities each tick via vtable dispatch.

#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
#include "imgui_menu.h"
#include "offsets.h"
#include "types.h"
#include "bypass.h"

using stclock = std::chrono::steady_clock;
static auto   s_last_attack = stclock::now();
static float  s_next_delay  = 0.f;

typedef void (*fn_attack)(uintptr_t player, uintptr_t entity);
typedef void (*fn_swing) (uintptr_t player);

static void AttackEntity(uintptr_t local_player, uintptr_t entity) {
    if (!local_player || !entity) return;
    uintptr_t vtable = SafeMem<uintptr_t>(local_player, 0);
    if (!vtable) return;

    // FILL: VTBL_ATTACK / VTBL_SWING_ARM in offsets.h
    uintptr_t attack_fn = SafeMem<uintptr_t>(vtable, Offsets::VTBL_ATTACK    * 8);
    uintptr_t swing_fn  = SafeMem<uintptr_t>(vtable, Offsets::VTBL_SWING_ARM * 8);
    if (attack_fn) ((fn_attack)attack_fn)(local_player, entity);
    if (swing_fn)  ((fn_swing) swing_fn) (local_player);
}

void KillAuraTick(uintptr_t level, uintptr_t local_player) {
    if (!g_menu.killaura_enabled || !level || !local_player) return;

    auto  now     = stclock::now();
    float elapsed = (float)std::chrono::duration_cast<std::chrono::milliseconds>
                        (now - s_last_attack).count();
    if (elapsed < s_next_delay) return;

    Vec3 my_pos = SafeMem<Vec3>(local_player, Offsets::ACTOR_POS);

    uintptr_t ent_list  = SafeMem<uintptr_t>(level, Offsets::LEVEL_ENTITY_LIST);
    int32_t   ent_count = SafeMem<int32_t>(level,   Offsets::LEVEL_ENTITY_COUNT);
    if (!ent_list || ent_count <= 0) return;

    struct Target { uintptr_t ptr; float dist; };
    std::vector<Target> targets;
    targets.reserve(16);

    for (int i = 0; i < ent_count && i < 512; i++) {
        uintptr_t ent = SafeMem<uintptr_t>(ent_list + (uintptr_t)i * sizeof(uintptr_t), 0);
        if (!ent || ent == local_player) continue;
        Vec3  pos  = SafeMem<Vec3>(ent, Offsets::ACTOR_POS);
        float dist = Dist(my_pos, pos);
        if (dist <= g_menu.killaura_radius)
            targets.push_back({ent, dist});
    }
    if (targets.empty()) return;

    std::sort(targets.begin(), targets.end(),
        [](const Target& a, const Target& b){ return a.dist < b.dist; });

    int n = g_menu.killaura_multitarget
        ? (int)std::min(targets.size(), (size_t)3) : 1;
    for (int i = 0; i < n; i++)
        AttackEntity(local_player, targets[i].ptr);

    s_last_attack = now;
    s_next_delay  = BypassClickDelay(
        g_menu.killaura_cps,
        g_menu.bypass_enabled ? g_menu.cps_variance : 0.f);
}
