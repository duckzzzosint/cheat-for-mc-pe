# MC Cheat — Bedrock Android

ptrace injector + ImGui overlay cheat client for Minecraft Bedrock (Android).  
Targets: arm64-v8a, Android 8.0+ (API 26+), root required.

---

## Build (GitHub Actions — recommended)

1. Create a new GitHub repository
2. Push this folder as the root
3. Go to **Actions** → workflow runs automatically on push
4. Download the APK from the **Artifacts** section (~10–15 min build)

No local toolchain needed.

---

## Build locally

Requirements: JDK 17, Android SDK (API 34), NDK 26.3.11579264, CMake 3.22.1

```sh
# 1. Download gradle wrapper (one time)
chmod +x setup.sh && ./setup.sh

# 2. Set SDK path
echo "sdk.dir=$ANDROID_HOME" > local.properties

# 3. Build
./gradlew assembleDebug

# APK -> app/build/outputs/apk/debug/app-debug.apk
```

---

## Usage

### Step 1 — Install APK
Install the APK on a rooted device. Grant overlay permission when prompted.

### Step 2 — Extract Assets
Tap **Extract Assets**. This copies from the APK to `/data/local/tmp/`:
- `mc_injector` — ptrace injector binary
- `libcheat.so`  — cheat payload

### Step 3 — Launch Minecraft
Tap **Launch Minecraft**. Wait for the title screen to fully load.

### Step 4 — Inject
Tap **Inject**. PID is auto-detected from `/proc/*/cmdline`.  
On success: `[+] Injection OK` appears in the log.

### Step 5 — Use the menu
A green **MC** button appears in the corner of the game screen.  
- **Tap** — toggle menu open/close  
- **Drag** — move the button anywhere on screen  
- Menu is resizable by dragging the corner

---

## Cheat features

| Tab     | Feature         | Notes |
|---------|-----------------|-------|
| Combat  | Kill Aura       | Auto-attack nearest, configurable radius + CPS |
| Combat  | Hitbox Expander | Inflates enemy AABB for easier hits |
| Visual  | ESP             | Boxes, lines to feet, name tags, health bar |
| Move    | Fly             | Free flight with configurable speed |
| Move    | Speed           | Horizontal speed multiplier |
| Move    | NoFall          | Zero fall damage accumulation |
| Bypass  | CPS Variance    | Randomize click delay ±N% to break fixed-interval detection |
| Bypass  | Smooth Fly      | Interpolate Y position to avoid teleport flags |

---

## Filling offsets (IDA Pro)

All offsets in `native/src/offsets.h` are `0x0` placeholders.  
The cheat loads but features do nothing until you fill them.

**Workflow:**

```
1. Pull libminecraftpe.so from device:
   adb pull /data/app/com.mojang.minecraftpe-*/lib/arm64/libminecraftpe.so

2. Load in IDA Pro (ARM64, ELF)
   Wait for auto-analysis to finish.

3. Fill each offset:
```

### CLIENT_INSTANCE_STATIC
Search strings for `"ClientInstance"` or xref to `ClientInstance::getInstance()`.  
The function ends with a `adrp/ldr` pair loading the global pointer → that's your RVA.

### CLINST_LEVEL
In `ClientInstance`, find the method that returns a `Level*` (often named `getLevel` in dumped symbols).  
The offset from `this` to the pointer field.

### LEVEL_LOCAL_PLAYER
In `Level`, find `getLocalPlayer()`. Usually `*(Level* + offset)` → `LocalPlayer*`.

### LEVEL_ENTITY_LIST / LEVEL_ENTITY_COUNT
Find the entity registry or the vector of `Actor*` pointers inside `Level`.  
Two consecutive offsets: pointer to array start, then `int32` count.

### ACTOR_* fields
Dump the `Actor` struct. Key fields:
- `POS` — inside `StateVectorComponent`, typically `Actor + 0x?? + 0x??`
- `VELOCITY` — adjacent to POS
- `HEALTH` — inside `AttributeInstance`, follow the attribute map
- `AABB_MIN/MAX` — `AABB` struct, usually near POS
- `NAME_TAG` — `std::string` (24 bytes on arm64 ABI)
- `ON_GROUND` — `bool` near velocity
- `FALL_DIST` — `float` near ON_GROUND

### CAMERA_VP_MATRIX
Find `LevelRenderer` or `CameraViewBobbing`.  
Look for a 64-byte block (float[16]) written every frame — that's the ViewProj matrix.  
`CAMERA_VP_MATRIX` is the **absolute runtime address**, not an RVA —  
set it as: `mc_base + RVA_of_matrix_store`.

### VTBL_ATTACK / VTBL_SWING_ARM
Open the vtable for `LocalPlayer` in IDA.  
Count from index 0 (each entry = 8 bytes on arm64).  
Find `attack(Entity*)` and `swing()`.

---

## Project structure

```
mc-cheat/
├── .github/workflows/build.yml   # CI: NDK build + APK upload
├── CMakeLists.txt                # Fetches ImGui + Dobby, builds cheat + injector
├── app/
│   ├── build.gradle
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── java/com/toker/mccheat/
│       │   ├── MainActivity.java     # Launcher UI
│       │   └── InjectorService.java  # Foreground service
│       └── res/...
└── native/src/
    ├── offsets.h      # ← fill these from IDA
    ├── types.h        # Vec2/Vec3, Mem/MemW/SafeMem helpers
    ├── main.cpp       # .so entry, CheatThread
    ├── injector.cpp   # ptrace injector (standalone binary)
    ├── hooks.cpp      # eglSwapBuffers + input hooks via Dobby
    ├── imgui_menu.cpp # toggle button + tabbed menu
    ├── esp.cpp        # AABB projection + draw
    ├── killaura.cpp   # vtable attack dispatch
    ├── movement.cpp   # fly/speed/nofall/hitbox
    └── bypass.cpp     # click delay variance, smooth, jitter
```

---

## Dependencies (auto-fetched by CMake)

| Library | Version | Use |
|---------|---------|-----|
| [ImGui](https://github.com/ocornut/imgui) | v1.90.4 | Menu + ESP rendering |
| [Dobby](https://github.com/jmpews/Dobby) | master | Inline hooks (eglSwapBuffers, input) |

Both are pulled at CMake configure time via `FetchContent` — no manual download needed.

---

## Notes

- Requires **root** (ptrace across processes needs `CAP_SYS_PTRACE`)
- SELinux must be **permissive** or you need a policy that allows `ptrace` + `execmem`
- The injector detaches immediately after `dlopen` — Minecraft continues normally
- All features are no-ops until offsets are filled; the menu still renders
- Tested architecture: arm64-v8a only (Minecraft dropped 32-bit)
