// native/src/bypass.cpp
// C++17 | arm64-v8a
// Bypass helpers: click delay randomization, smooth position, velocity jitter.
// Goal: break fixed-interval and constant-speed patterns that server ACs flag.

#include <random>
#include <cmath>
#include "bypass.h"
#include "types.h"

static std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

// cps=8, variance=0.2 -> delay in [104ms, 156ms] vs constant 125ms
float BypassClickDelay(float cps, float variance) {
    if (cps <= 0.f) cps = 1.f;
    float base = 1000.f / cps;
    float lo   = base * (1.f - variance);
    float hi   = base * (1.f + variance);
    return std::uniform_real_distribution<float>(lo, hi)(rng());
}

// alpha=0.25 reaches ~98% of target in ~16 ticks (~260ms at 62Hz)
// Prevents sudden Y jumps that trip server-side teleport flags.
Vec3 BypassSmooth(Vec3 cur, Vec3 tgt, float alpha) {
    return {
        cur.x + (tgt.x - cur.x) * alpha,
        cur.y + (tgt.y - cur.y) * alpha,
        cur.z + (tgt.z - cur.z) * alpha,
    };
}

// Tiny XZ noise — breaks constant-speed signature used by movement ACs.
// amount ~0.005-0.015: invisible to player, noticeable pattern break.
Vec3 BypassJitter(Vec3 vel, float amount) {
    std::uniform_real_distribution<float> d(-amount, amount);
    return { vel.x + d(rng()), vel.y, vel.z + d(rng()) };
}
