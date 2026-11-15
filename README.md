# CS3D — Phase 1: Advanced FPS Movement & Gunplay

Pure C++17 + OpenGL 3.3 core. GLFW (window/input), GLM (math). No engine,
no prebuilt FPS controller, no asset files — geometry, font, and audio
are all generated in code.

## Dependencies (Windows 10/11 x64)

1. **Visual Studio 2022** (Community or Build Tools) with the
   *Desktop development with C++* workload.
2. **CMake ≥ 3.20** in PATH (VS installer can include it).
3. **Internet on first build only** — CMake FetchContent downloads
   GLFW 3.4 and GLM 1.0.1 source zips from GitHub automatically.
   Nothing else to install: OpenGL comes with Windows, audio uses
   the built-in WinMM API, the GL loader is part of src/.

## Build & Run

    BUILD.bat      (configure + Release x64 build)
    RUN.bat        (launch)

Or manually:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --parallel
    build\Release\CS3D.exe

Status: source-complete prototype. It has not been pre-compiled or
executed as part of this delivery — build it with BUILD.bat.

## Controls

| Key | Action |
|---|---|
| WASD | Move (accel/friction, momentum preserving) |
| Mouse | Look (raw input, smoothing configurable) |
| LMB / RMB | Fire / **ADS** (per-weapon speed + FOV) |
| Shift | Sprint (FOV kick, stronger bob, sprint pose) |
| Ctrl (hold) / C (toggle) | Crouch (height + camera transition) |
| Space | Jump (buffered, coyote time, hold to bhop) |
| R | Reload (staged procedural animation) |
| F | Weapon inspect |
| 1 / 2 / Wheel | Switch weapon (lower/raise animation) |
| Tab | **Live gameplay settings panel** (arrows adjust) |
| T | Reset drill (targets, ammo, stats, spawn) |
| M | Mute all audio |
| H / F1 | Toggle help / debug readouts |
| F11 | Fullscreen toggle |
| Esc | Pause (click/Esc resume, X quits) |

## Test environment

Dusk-lit blockout arena: 30 m firing range with 8 reactive dummies
(head/body/leg hitboxes, 4x headshots, two strafing movers), stair
climb (step-up), 1.3 m crouch tunnel, jump crates and bhop pads.
HUD: FPS, speed, movement state, ADS %, ammo, recoil pattern/offset
debug, live spread, score/accuracy. Settings panel exposes ~25 live
parameters including **time scale** (slow motion for animation review).

## Architecture

- `Player`     Quake/CS-style accelerate/friction, air control, crouch,
               step-up collision, jump buffer + coyote, footstep events.
- `Camera`     smoothed look, speed-driven head bob, land/jump springs,
               strafe lean, breathing sway, sprint/ADS FOV blending.
- `Weapon`     two weapons (auto rifle / semi pistol), ADS state machine,
               reload/inspect/switch actions, procedural viewmodel with
               aligned iron sights, animated mag/bolt/slide/hands.
- `Animation`  delta-time springs, easing, keyframe tracks, layered
               additive pose blending (no snapping anywhere).
- `Recoil`     deterministic per-shot patterns, accumulation, spring
               camera kick + weapon kickback, delayed recovery, ADS scale.
- `Physics`    AABB slab raycasts, axis-resolved sweep, step-up, probes.
- `Effects`    tracers, sparks, smoke, bullet decals, shell casings,
               mag drops, muzzle flash + dynamic light.
- `Audio`      synthesized one-shots mixed on a WinMM playback thread.
