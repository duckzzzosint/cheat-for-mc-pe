// native/src/types.h
// C++17 | arm64-v8a
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

struct Vec2 { float x = 0, y = 0; };

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(Vec3 o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    float len()    const { return sqrtf(x*x + y*y + z*z); }
    float len_xz() const { return sqrtf(x*x + z*z); }
};

inline float Dist(Vec3 a, Vec3 b) { return (a - b).len(); }

template<typename T>
inline T Mem(uintptr_t p, uintptr_t off = 0) {
    return *reinterpret_cast<T*>(p + off);
}
template<typename T>
inline T& MemRef(uintptr_t p, uintptr_t off = 0) {
    return *reinterpret_cast<T*>(p + off);
}
template<typename T>
inline void MemW(uintptr_t p, uintptr_t off, T val) {
    *reinterpret_cast<T*>(p + off) = val;
}
template<typename T>
inline T SafeMem(uintptr_t p, uintptr_t off, T fallback = {}) {
    if (!p) return fallback;
    return Mem<T>(p, off);
}

// libc++ std::string on arm64 — SSO threshold 22 bytes
// byte[23] MSB=0 -> inline buf, =1 -> heap ptr at [0..7]
inline void ReadStdStr(uintptr_t str_ptr, char* out, size_t max) {
    if (!str_ptr || !out || !max) { if (out) out[0] = '\0'; return; }
    uint8_t last = Mem<uint8_t>(str_ptr, 23);
    if (!(last & 0x80)) {
        strncpy(out, reinterpret_cast<const char*>(str_ptr), std::min(max, (size_t)23));
    } else {
        uintptr_t heap = Mem<uintptr_t>(str_ptr, 0);
        if (!heap) { out[0] = '\0'; return; }
        strncpy(out, reinterpret_cast<const char*>(heap), max - 1);
    }
    out[max - 1] = '\0';
}
