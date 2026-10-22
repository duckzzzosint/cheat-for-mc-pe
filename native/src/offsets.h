// native/src/offsets.h
// Target: libminecraftpe.so arm64-v8a
// ALL = 0x0 — fill via IDA Pro for your Bedrock version
// RVA = relative to GetLibBase("libminecraftpe.so")
#pragma once
#include <cstdint>

namespace Offsets {

    // ── Singleton RVA ──────────────────────────────────────────────────────
    inline constexpr uintptr_t CLIENT_INSTANCE_STATIC   = 0x0; // FILL: ClientInstance* global

    // ── ClientInstance ─────────────────────────────────────────────────────
    inline constexpr uintptr_t CLINST_LEVEL             = 0x0; // FILL: -> Level*

    // ── Level ──────────────────────────────────────────────────────────────
    inline constexpr uintptr_t LEVEL_LOCAL_PLAYER       = 0x0; // FILL: -> LocalPlayer*
    inline constexpr uintptr_t LEVEL_ENTITY_LIST        = 0x0; // FILL: -> vector<Actor*>
    inline constexpr uintptr_t LEVEL_ENTITY_COUNT       = 0x0; // FILL: int32_t count

    // ── Actor ──────────────────────────────────────────────────────────────
    inline constexpr uintptr_t ACTOR_POS                = 0x0; // FILL: Vec3 (StateVectorComponent)
    inline constexpr uintptr_t ACTOR_VELOCITY           = 0x0; // FILL: Vec3
    inline constexpr uintptr_t ACTOR_HEALTH             = 0x0; // FILL: float (AttributeInstance)
    inline constexpr uintptr_t ACTOR_MAX_HEALTH         = 0x0; // FILL: float
    inline constexpr uintptr_t ACTOR_AABB_MIN           = 0x0; // FILL: Vec3
    inline constexpr uintptr_t ACTOR_AABB_MAX           = 0x0; // FILL: Vec3
    inline constexpr uintptr_t ACTOR_TYPE_ID            = 0x0; // FILL: ActorType int32
    inline constexpr uintptr_t ACTOR_ON_GROUND          = 0x0; // FILL: bool
    inline constexpr uintptr_t ACTOR_FALL_DIST          = 0x0; // FILL: float
    inline constexpr uintptr_t ACTOR_NAME_TAG           = 0x0; // FILL: std::string nameTag
    inline constexpr uintptr_t ACTOR_UNIQUE_ID          = 0x0; // FILL: int64_t ActorUniqueID

    // ── LocalPlayer ────────────────────────────────────────────────────────
    inline constexpr uintptr_t PLAYER_PITCH             = 0x0; // FILL: float degrees
    inline constexpr uintptr_t PLAYER_YAW               = 0x0; // FILL: float degrees
    inline constexpr uintptr_t PLAYER_SWING             = 0x0; // FILL: float swing progress

    // ── Camera ─────────────────────────────────────────────────────────────
    inline constexpr uintptr_t CAMERA_VP_MATRIX         = 0x0; // FILL: float[16] col-major ViewProj

    // ── Vtable indices (0-based, arm64: *8 bytes) ──────────────────────────
    inline constexpr uintptr_t VTBL_ATTACK              = 0;   // FILL: LocalPlayer::attack(Entity*)
    inline constexpr uintptr_t VTBL_SWING_ARM           = 0;   // FILL: Actor::swing()

} // namespace Offsets
