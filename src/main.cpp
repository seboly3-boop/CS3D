// CS3D — Phase 1: movement & gunplay prototype.
// Game loop, hitscan resolution, HUD, live settings, audio/effect wiring.
#include "GL.h"
#include "Renderer.h"
#include "Input.h"
#include "Player.h"
#include "Camera.h"
#include "Weapon.h"
#include "World.h"
#include "Effects.h"
#include "Audio.h"
#include "Animation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

using namespace anim;

// ------------------------------------------------------------------ shaders
static const char* WORLD_VS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
uniform mat4 uProj, uView, uModel;
out vec3 vWorld; out vec3 vNormal; out vec3 vColor;
void main() {
  vec4 w = uModel * vec4(aPos, 1.0);
  vWorld = w.xyz;
  vNormal = mat3(uModel) * aNormal;
  vColor = aColor;
  gl_Position = uProj * uView * w;
}
)";

static const char* WORLD_FS = R"(
#version 330 core
in vec3 vWorld; in vec3 vNormal; in vec3 vColor;
uniform vec3 uLightDir, uKeyColor, uAmbUp, uAmbDown, uCamPos;
uniform vec3 uFogColor, uFlashPos, uFlashColor;
uniform float uFogNear, uFogFar, uFlashIntensity, uGrid, uEmissive;
uniform vec4 uTint;
out vec4 FragColor;
void main() {
  vec3 n = normalize(vNormal);
  vec3 base = vColor;
  if (uGrid > 0.5) {
    vec2 g1 = abs(fract(vWorld.xz) - 0.5);
    float minor = smoothstep(0.47, 0.5, max(g1.x, g1.y));
    vec2 g5 = abs(fract(vWorld.xz / 5.0) - 0.5);
    float major = smoothstep(0.485, 0.5, max(g5.x, g5.y));
    base = mix(base, vec3(0.32, 0.32, 0.36), minor * 0.55);
    base = mix(base, vec3(0.55, 0.42, 0.22), major * 0.7);
  }
  float hemi = n.y * 0.5 + 0.5;
  vec3 amb = mix(uAmbDown, uAmbUp, hemi);
  float dif = max(dot(n, uLightDir), 0.0);
  vec3 lit = base * (amb + uKeyColor * dif);
  vec3 v = normalize(uCamPos - vWorld);
  vec3 h = normalize(v + uLightDir);
  lit += uKeyColor * pow(max(dot(n, h), 0.0), 32.0) * 0.12;
  vec3 L = uFlashPos - vWorld;
  float att = uFlashIntensity / (1.0 + dot(L, L) * 2.0);
  lit += uFlashColor * att * max(dot(n, normalize(L)), 0.0) * base * 2.0;
  lit = mix(lit, base * 1.35, uEmissive);
  float dist = length(uCamPos - vWorld);
  float fog = clamp((dist - uFogNear) / (uFogFar - uFogNear), 0.0, 1.0);
  lit = mix(lit, uFogColor, fog);
  FragColor = vec4(lit * uTint.rgb, uTint.a);
}
)";

static const char* FLAT_VS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
uniform mat4 uProj, uView, uModel;
out vec4 vColor;
void main() { vColor = aColor; gl_Position = uProj * uView * uModel * vec4(aPos, 1.0); }
)";

static const char* FLAT_FS = R"(
#version 330 core
in vec4 vColor; out vec4 FragColor;
void main() { FragColor = vColor; }
)";

static const char* SKY_VS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
out vec2 vNdc;
void main() { vNdc = aPos.xy; gl_Position = vec4(aPos, 1.0); }
)";

static const char* SKY_FS = R"(
#version 330 core
in vec2 vNdc;
uniform mat4 uInvProj, uInvView;
uniform vec3 uSunDir;
out vec4 FragColor;
void main() {
  vec4 vp = uInvProj * vec4(vNdc, 1.0, 1.0);
  vec3 vd = normalize(vp.xyz / vp.w);
  vec3 dir = normalize(mat3(uInvView) * vd);
  vec3 zenith  = vec3(0.065, 0.085, 0.175);
  vec3 mid     = vec3(0.16, 0.19, 0.33);
  vec3 horizon = vec3(0.85, 0.46, 0.22);
  vec3 below   = vec3(0.10, 0.10, 0.13);
  vec3 col;
  if (dir.y >= 0.0) {
    col = mix(horizon, mid, smoothstep(0.0, 0.18, dir.y));
    col = mix(col, zenith, smoothstep(0.12, 0.65, dir.y));
  } else {
    col = mix(horizon * 0.55, below, smoothstep(0.0, 0.25, -dir.y));
  }
  float sun = max(dot(dir, uSunDir), 0.0);
  col += vec3(1.0, 0.75, 0.45) * pow(sun, 700.0) * 3.0;
  col += vec3(0.9, 0.5, 0.25) * pow(sun, 10.0) * 0.22;
  FragColor = vec4(col, 1.0);
}
)";

// ------------------------------------------------------------------ settings
struct SettingRow {
  const char* label;
  int kind;            // 0 float, 1 bool, 2 action
  float* f = nullptr;
  bool* b = nullptr;
  float mn = 0, mx = 1, step = 0.1f;
  int action = 0;      // 1 reset settings, 2 reset drill
};

struct Hitmarker { float t = 9; int kind = 0; }; // 1 body 2 head 3 kill
struct KillPopup { float t = 9; bool head = false; };

static glm::vec4 hudAmber(1.0f, 0.78f, 0.30f, 1.f);
static glm::vec4 hudWhite(0.92f, 0.94f, 0.97f, 1.f);
static glm::vec4 hudDim(0.55f, 0.60f, 0.70f, 1.f);
static glm::vec4 hudCyan(0.35f, 0.90f, 1.0f, 1.f);
static glm::vec4 hudRed(1.0f, 0.32f, 0.25f, 1.f);
static glm::vec4 hudGreen(0.45f, 1.0f, 0.55f, 1.f);

static std::string fmt(const char* f, float v) {
  char buf[128]; snprintf(buf, sizeof(buf), f, v); return buf;
}

int main() {
  if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  GLFWwindow* win = glfwCreateWindow(1600, 900, "CS3D - Phase 1 | Movement & Gunplay", nullptr, nullptr);
  if (!win) { // MSAA fallback
    glfwWindowHint(GLFW_SAMPLES, 0);
    win = glfwCreateWindow(1600, 900, "CS3D - Phase 1 | Movement & Gunplay", nullptr, nullptr);
  }
  if (!win) { fprintf(stderr, "Window/context creation failed (need OpenGL 3.3)\n"); glfwTerminate(); return 1; }
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);
  if (!loadGLFunctions()) { glfwTerminate(); return 1; }
  printf("[GL] %s\n", (const char*)glGetString(GL_VERSION));
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_MULTISAMPLE);
  glDisable(GL_CULL_FACE);

  Shader worldS, flatS, skyS;
  if (!worldS.create(WORLD_VS, WORLD_FS) || !flatS.create(FLAT_VS, FLAT_FS) || !skyS.create(SKY_VS, SKY_FS)) {
    glfwTerminate(); return 1;
  }

  Input input; input.init(win); input.setLocked(true);
  Player player; Camera camera; WeaponSystem ws; World world; Effects fx; Audio audio;
  ws.init(); world.build();
  bool audioOk = audio.init();
  printf("[Audio] %s\n", audioOk ? "WinMM mixer running" : "disabled");

  FlatBatch worldFlat, worldAdd, hud, vmFx, skyBatch;
  worldFlat.init(); worldAdd.init(); hud.init(); vmFx.init(); skyBatch.init();
  skyBatch.vert({ -1, -1, 0 }, { 1, 1, 1, 1 });
  skyBatch.vert({  3, -1, 0 }, { 1, 1, 1, 1 });
  skyBatch.vert({ -1,  3, 0 }, { 1, 1, 1, 1 });

  // ---- game state ----
  float timescale = 1.f, audioVol = 0.6f;
  bool muted = false, showDebug = true, showHelp = true, wireframe = false;
  bool paused = false, settingsOpen = false, fullscreen = false;
  bool justResumed = false, stepAlt = false;
  int winPos[2] = {100, 100}, winSize[2] = {1600, 900};
  Hitmarker hm; KillPopup popup;
  std::string banner; float bannerT = 9;
  float fps = 0, frameMs = 0; int fpsFrames = 0; float fpsTimer = 0;
  double lastT = glfwGetTime();
  float simTime = 0;

  MoveParams defMove = player.p;
  CameraParams defCam = camera.p;

  std::vector<SettingRow> rows = {
    { "MOUSE SENSITIVITY", 0, &camera.p.sensitivity, nullptr, 0.1f, 3.f, 0.05f },
    { "LOOK SMOOTHING",    0, &camera.p.smoothing,   nullptr, 0.f, 1.f, 0.05f },
    { "BASE FOV",          0, &camera.p.baseFov,     nullptr, 60.f, 100.f, 1.f },
    { "SPRINT FOV ADD",    0, &camera.p.sprintFovAdd,nullptr, 0.f, 15.f, 0.5f },
    { "HEAD BOB AMOUNT",   0, &camera.p.bobAmount,   nullptr, 0.f, 2.f, 0.1f },
    { "WEAPON SWAY",       0, &ws.swayScale,         nullptr, 0.f, 2.f, 0.1f },
    { "RECOIL SCALE",      0, &ws.recoilScale,       nullptr, 0.f, 2.f, 0.1f },
    { "SPREAD SCALE",      0, &ws.spreadScale,       nullptr, 0.f, 2.f, 0.1f },
    { "ADS TIME SCALE",    0, &ws.adsTimeScale,      nullptr, 0.5f, 2.f, 0.1f },
    { "WALK SPEED",        0, &player.p.walkSpeed,   nullptr, 2.f, 8.f, 0.2f },
    { "SPRINT SPEED",      0, &player.p.sprintSpeed, nullptr, 5.f, 12.f, 0.2f },
    { "CROUCH SPEED",      0, &player.p.crouchSpeed, nullptr, 1.f, 5.f, 0.2f },
    { "GROUND ACCEL",      0, &player.p.groundAccel, nullptr, 2.f, 30.f, 1.f },
    { "AIR ACCEL",         0, &player.p.airAccel,    nullptr, 2.f, 60.f, 2.f },
    { "FRICTION",          0, &player.p.friction,    nullptr, 1.f, 15.f, 0.5f },
    { "GRAVITY",           0, &player.p.gravity,     nullptr, 8.f, 30.f, 1.f },
    { "JUMP SPEED",        0, &player.p.jumpSpeed,   nullptr, 4.f, 9.f, 0.2f },
    { "STEP HEIGHT",       0, &player.p.stepHeight,  nullptr, 0.2f, 0.8f, 0.05f },
    { "CROUCH TIME",       0, &player.p.crouchTime,  nullptr, 0.05f, 0.4f, 0.02f },
    { "TIME SCALE",        0, &timescale,            nullptr, 0.1f, 2.f, 0.1f },
    { "AUDIO VOLUME",      0, &audioVol,             nullptr, 0.f, 1.f, 0.05f },
    { "INFINITE AMMO",     1, nullptr, &ws.infiniteAmmo },
    { "MUTE AUDIO",        1, nullptr, &muted },
    { "WIREFRAME",         1, nullptr, &wireframe },
    { "SHOW DEBUG",        1, nullptr, &showDebug },
    { "SHOW HELP",         1, nullptr, &showHelp },
    { "RESET SETTINGS",    2, nullptr, nullptr, 0, 0, 0, 1 },
    { "RESET DRILL",       2, nullptr, nullptr, 0, 0, 0, 2 },
  };
  int sel = 0, scroll = 0;
  float repeatT = 0;

  auto resetDrill = [&]() {
    world.resetAll(); fx.clear();
    ws.refillAll(); ws.resetStats();
    player.teleport({ 0, 0, 8.5f });
    camera.resetAngles(0, 0);
    hm = {}; popup = {};
    banner = "DRILL RESET"; bannerT = 0;
  };
  auto resetSettings = [&]() {
    player.p = defMove; camera.p = defCam;
    ws.swayScale = ws.recoilScale = ws.spreadScale = ws.adsTimeScale = 1;
    ws.infiniteAmmo = false; timescale = 1; audioVol = 0.6f;
    muted = false; wireframe = false; showDebug = true; showHelp = true;
    banner = "SETTINGS RESTORED"; bannerT = 0;
  };
  auto applyRow = [&](int i, int dir) {
    SettingRow& r = rows[i];
    if (r.kind == 0) *r.f = clampf(*r.f + dir * r.step, r.mn, r.mx);
    else if (r.kind == 1 && dir != 0) *r.b = !*r.b;
    else if (r.kind == 2 && dir == 0) {
      if (r.action == 1) resetSettings();
      if (r.action == 2) resetDrill();
    }
  };
  auto setPaused = [&](bool p) {
    paused = p;
    input.setLocked(!p);
    if (!p) justResumed = true;
  };

  // ============================================================ main loop
  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents();
    double now = glfwGetTime();
    float dt = clampf((float)(now - lastT), 0.f, 0.05f);
    lastT = now;
    frameMs = frameMs * 0.92f + dt * 1000.f * 0.08f;
    fpsFrames++; fpsTimer += dt;
    if (fpsTimer >= 0.5f) { fps = fpsFrames / fpsTimer; fpsFrames = 0; fpsTimer = 0; }

    // ---- global keys ----
    if (input.lostFocus) { input.lostFocus = false; if (!paused) setPaused(true); }
    if (input.pressed(GLFW_KEY_ESCAPE)) setPaused(!paused);
    if (paused && input.down(GLFW_KEY_X)) glfwSetWindowShouldClose(win, 1);
    if (paused && input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) setPaused(false);
    if (!paused && input.pressed(GLFW_KEY_TAB)) settingsOpen = !settingsOpen;
    if (input.pressed(GLFW_KEY_M)) muted = !muted;
    if (input.pressed(GLFW_KEY_H)) showHelp = !showHelp;
    if (input.pressed(GLFW_KEY_F1)) showDebug = !showDebug;
    if (input.pressed(GLFW_KEY_T)) resetDrill();
    if (input.pressed(GLFW_KEY_F11)) {
      fullscreen = !fullscreen;
      if (fullscreen) {
        glfwGetWindowPos(win, &winPos[0], &winPos[1]);
        glfwGetWindowSize(win, &winSize[0], &winSize[1]);
        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* vm = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(win, mon, 0, 0, vm->width, vm->height, vm->refreshRate);
      } else {
        glfwSetWindowMonitor(win, nullptr, winPos[0], winPos[1], winSize[0], winSize[1], 0);
      }
      input.setLocked(!paused);
    }

    // ---- settings navigation (gameplay keeps running for live tuning) ----
    if (settingsOpen && !paused) {
      auto nav = [&](int key, int dir, bool isAction) {
        if (input.pressed(key)) { applyRow(sel, dir); repeatT = 0.35f; }
        else if (input.down(key)) {
          repeatT -= dt;
          if (repeatT <= 0) { applyRow(sel, dir); repeatT = 0.07f; }
        }
        (void)isAction;
      };
      if (input.pressed(GLFW_KEY_UP))   { sel = (sel + (int)rows.size() - 1) % (int)rows.size(); repeatT = 0.35f; }
      else if (input.down(GLFW_KEY_UP)) { repeatT -= dt; if (repeatT <= 0) { sel = (sel + (int)rows.size() - 1) % (int)rows.size(); repeatT = 0.09f; } }
      if (input.pressed(GLFW_KEY_DOWN))   { sel = (sel + 1) % (int)rows.size(); repeatT = 0.35f; }
      else if (input.down(GLFW_KEY_DOWN)) { repeatT -= dt; if (repeatT <= 0) { sel = (sel + 1) % (int)rows.size(); repeatT = 0.09f; } }
      nav(GLFW_KEY_LEFT, -1, false);
      nav(GLFW_KEY_RIGHT, +1, false);
      if (input.pressed(GLFW_KEY_ENTER)) applyRow(sel, 0);
      const int VIS = 16;
      if (sel < scroll) scroll = sel;
      if (sel >= scroll + VIS) scroll = sel - VIS + 1;
    }

    audio.setVolume(audioVol);
    audio.setMuted(muted);

    float simDt = paused ? 0.f : dt * timescale;
    simTime += simDt;
    bool allowFire = !paused && !settingsOpen && !justResumed;

    // ---- simulate ----
    if (simDt > 0) {
      bool wantAds = input.mouseDown(GLFW_MOUSE_BUTTON_RIGHT) && allowFire;
      if (!paused && input.locked())
        camera.look(input.mouseDelta(), ws.adsBlend, ws.def().adsFov);
      player.update(input, simDt, camera.yaw, world.solids, wantAds, simTime);
      WSEvents e = ws.update(simDt, input, player, camera, allowFire, simTime);

      glm::vec3 kickExtra(0);
      camera.update(simDt, player.eyeBase(), player.horizSpeed(),
                    player.horizSpeed() / std::max(1.f, player.p.sprintSpeed),
                    player.grounded, player.crouchT, player.sprintBlend,
                    ws.adsBlend, ws.def().adsFov,
                    glm::dot(player.vel, camera.right) / std::max(1.f, player.p.sprintSpeed),
                    ws.cur().recoil.camPitchDeg(), ws.cur().recoil.camYawDeg(),
                    ws.cur().recoil.kickDeg(), simTime);
      if (player.ev.jumped) { camera.onJump(); audio.play("jump", 0.30f); }
      if (player.ev.landed) {
        camera.onLand(player.ev.landImpact);
        audio.play("land", clampf(player.ev.landImpact / 11.f, 0.2f, 1.f));
        if (player.ev.landImpact > 6.f) fx.landDust(player.pos, player.ev.landImpact);
      }
      if (player.ev.footstep) {
        stepAlt = !stepAlt;
        audio.play("step", 0.22f + 0.30f * player.ev.stepIntensity, stepAlt ? 1.f : 0.92f);
      }

      // ---- shots: hitscan from the true camera aim + spread cone ----
      glm::vec3 muzzle = ws.muzzleWorld(camera);
      for (int s = 0; s < e.shots; ++s) {
        const ShotInfo& si = e.shotInfo[s];
        float spreadRad = rad(ws.currentSpreadDeg(player));
        float a = (float)std::rand() / RAND_MAX * 6.2831853f;
        float r = std::sqrt((float)std::rand() / RAND_MAX) * std::tan(spreadRad);
        glm::vec3 dir = glm::normalize(camera.fwd + camera.right * (std::cos(a) * r)
                                     + camera.up * (std::sin(a) * r));
        RayHit wh; TargetHit th;
        bool hitW = world.raycast(camera.eye, dir, 200.f, wh);
        bool hitT = world.raycastTargets(camera.eye, dir, hitW ? wh.t : 200.f, th);
        glm::vec3 end = camera.eye + dir * 120.f;
        bool killed = false, head = false;
        if (hitT) {
          end = th.point;
          head = th.part == 2;
          killed = world.damage(th.target, th.part, si.damage);
          fx.impact(th.point, th.normal, true);
          ws.reportHit(true, head, killed);
          hm = { 0.f, killed ? 3 : (head ? 2 : 1) };
          audio.play(head ? "head" : "hit", head ? 0.6f : 0.5f, 1.f + ((float)std::rand() / RAND_MAX - 0.5f) * 0.06f);
          if (killed) { popup = { 0.f, head }; audio.play("kill", 0.55f); }
        } else if (hitW) {
          end = wh.point;
          fx.impact(wh.point, wh.normal, false);
          ws.reportHit(false, false, false);
        } else {
          ws.reportHit(false, false, false);
        }
        fx.tracer(muzzle, end);
        fx.muzzle(camera.eye + camera.fwd * 0.8f);
        fx.casing(muzzle, camera.right, camera.up, camera.fwd);
        audio.play(ws.def().fireSound, 0.85f, 1.f + ((float)std::rand() / RAND_MAX - 0.5f) * 0.08f);
      }
      if (e.dry) audio.play("dry", 0.4f);
      if (e.reloadStart) audio.play("switch", 0.4f);
      if (e.magOut) {
        audio.play("magout", 0.5f);
        fx.magDrop(muzzle + camera.up * -0.15f + camera.right * 0.05f);
      }
      if (e.magIn) audio.play("magin", 0.55f);
      if (e.bolt) audio.play("bolt", 0.5f);
      if (e.inspectStart) audio.play("switch", 0.35f, 0.9f);
      if (e.switchStart) audio.play("switch", 0.45f);
      if (e.raiseDone) audio.play("bolt", 0.35f, 1.1f);

      world.update(simDt);
      fx.update(simDt);
      hm.t += simDt; popup.t += simDt; bannerT += simDt;
    }
    justResumed = false;

    // ============================================================ render
    int fbW, fbH; glfwGetFramebufferSize(win, &fbW, &fbH);
    if (fbW < 1 || fbH < 1) { glfwSwapBuffers(win); input.endFrame(); continue; }
    glViewport(0, 0, fbW, fbH);
    float aspect = (float)fbW / fbH;
    glm::mat4 proj = camera.proj(aspect);
    glm::mat4 view = camera.view();
    const glm::vec3 sunDir = glm::normalize(glm::vec3(-0.4f, 0.16f, -0.9f));
    const glm::vec3 fogCol(0.30f, 0.26f, 0.30f);

    glClearColor(fogCol.r, fogCol.g, fogCol.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // sky
    glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
    skyS.use();
    skyS.set("uInvProj", glm::inverse(proj));
    skyS.set("uInvView", glm::transpose(glm::mat4(glm::mat3(view))));
    skyS.set("uSunDir", sunDir);
    glBindVertexArray(skyBatch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, skyBatch.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(skyBatch.v.size() * sizeof(FlatVert)), skyBatch.v.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE); glEnable(GL_DEPTH_TEST);

    // world
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    worldS.use();
    worldS.set("uLightDir", sunDir);
    worldS.set("uKeyColor", glm::vec3(1.05f, 0.78f, 0.52f));
    worldS.set("uAmbUp", glm::vec3(0.30f, 0.34f, 0.45f));
    worldS.set("uAmbDown", glm::vec3(0.16f, 0.14f, 0.12f));
    worldS.set("uCamPos", camera.eye);
    worldS.set("uFogColor", fogCol);
    worldS.set("uFogNear", 25.f); worldS.set("uFogFar", 95.f);
    worldS.set("uFlashPos", fx.flashPos);
    worldS.set("uFlashColor", glm::vec3(1.f, 0.75f, 0.4f));
    worldS.set("uFlashIntensity", fx.flashI * 3.f);
    world.draw(worldS, proj, view);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // decals + health bars + particles + tracers
    worldFlat.clear(); worldAdd.clear();
    fx.drawDecals(worldFlat);
    world.appendHealthBars(worldFlat, camera.right, camera.up);
    fx.draw(worldFlat, worldAdd, camera.right, camera.up, camera.eye);
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-2.f, -2.f);
    worldFlat.draw(flatS, proj, view, glm::mat4(1), false, false);
    glDisable(GL_POLYGON_OFFSET_FILL);
    worldAdd.draw(flatS, proj, view, glm::mat4(1), true, false);

    // viewmodel (own projection, cleared depth so it never clips walls)
    glClear(GL_DEPTH_BUFFER_BIT);
    glm::mat4 vmProj = glm::perspective(rad(54.f), aspect, 0.01f, 6.f);
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    ws.draw(vmProj, worldS, flatS, vmFx, simTime);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ============================================================ HUD
    float sc = fbH / 1080.f;
    float W = (float)fbW, H = (float)fbH;
    hud.clear();
    glm::mat4 ortho = glm::ortho(0.f, W, H, 0.f, -1.f, 1.f);
    float cx = W * 0.5f, cy = H * 0.5f;
    const WeaponDef& wd = ws.def();
    const Weapon& cw = ws.cur();

    // crosshair: dynamic gap from live spread; fades to a dot in ADS
    float spreadPx = std::tan(rad(ws.currentSpreadDeg(player))) * (H * 0.5f) / std::tan(rad(camera.fov) * 0.5f);
    float gap = (7.f + spreadPx * 0.55f) * sc;
    float len = 9.f * sc, th = 2.f * sc;
    float chA = 1.f - ws.adsBlend;
    if (chA > 0.02f && !paused) {
      glm::vec4 dark(0, 0, 0, 0.55f * chA), bright(0.95f, 0.97f, 1.f, 0.92f * chA);
      for (int pass = 0; pass < 2; ++pass) {
        float t2 = pass ? th : th + 2.f * sc;
        glm::vec4 c = pass ? bright : dark;
        UI::line(hud, cx - gap - len, cy, cx - gap, cy, t2, c);
        UI::line(hud, cx + gap, cy, cx + gap + len, cy, t2, c);
        UI::line(hud, cx, cy - gap - len, cx, cy - gap, t2, c);
        UI::line(hud, cx, cy + gap, cx, cy + gap + len, t2, c);
      }
    }
    if (!paused) {
      glm::vec4 dot = ws.adsBlend > 0.5f ? hudCyan : glm::vec4(1, 1, 1, 0.85f);
      float ds = (ws.adsBlend > 0.5f ? 2.f : 1.5f) * sc;
      UI::rect(hud, cx - ds, cy - ds, ds * 2, ds * 2, dot);
    }
    // hitmarker
    if (hm.t < 0.16f && hm.kind > 0) {
      float a = 1.f - hm.t / 0.16f;
      float g2 = 8.f * sc, l2 = (hm.kind == 3 ? 13.f : 9.f) * sc;
      glm::vec4 c = hm.kind == 3 ? glm::vec4(1.f, 0.75f, 0.25f, a)
                  : hm.kind == 2 ? glm::vec4(1.f, 0.3f, 0.25f, a)
                                 : glm::vec4(1, 1, 1, a);
      for (int dx : {-1, 1}) for (int dy : {-1, 1})
        UI::line(hud, cx + dx * g2, cy + dy * g2, cx + dx * (g2 + l2), cy + dy * (g2 + l2), 2.5f * sc, c);
    }
    // kill popup
    if (popup.t < 0.8f) {
      float a = 1.f - popup.t / 0.8f;
      std::string s = popup.head ? "HEADSHOT  +100" : "ELIMINATED  +100";
      glm::vec4 c = popup.head ? hudRed : hudAmber; c.a = a;
      UI::text(hud, cx - UI::textW(2.5f * sc, s) * 0.5f, cy - 60.f * sc - popup.t * 30.f * sc, 2.5f * sc, s, c);
    }
    if (bannerT < 1.2f) {
      glm::vec4 c = hudCyan; c.a = 1.f - bannerT / 1.2f;
      UI::text(hud, cx - UI::textW(3.f * sc, banner) * 0.5f, cy - 120.f * sc, 3.f * sc, banner, c);
    }

    // top-center: audio state (single master mute)
    {
      std::string s = audioOk ? (muted ? "[M] SOUND: OFF" : "[M] SOUND: ON") : "[M] AUDIO UNAVAILABLE";
      glm::vec4 c = muted ? hudDim : hudGreen;
      UI::text(hud, cx - UI::textW(2.f * sc, s) * 0.5f, 14.f * sc, 2.f * sc, s, c);
    }

    // top-left debug block
    if (showDebug) {
      float x = 16.f * sc, y = 14.f * sc, s = 2.f * sc, lh = 24.f * sc;
      UI::panel(hud, x - 6 * sc, y - 6 * sc, 320.f * sc, 13.f * lh + 12.f * sc, hudAmber);
      UI::text(hud, x, y, s, fmt("FPS %.0f  (%.2f MS)", fps), hudWhite); y += lh;
      UI::text(hud, x, y, s, fmt("FRAME %.2f MS", frameMs), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("SPEED %.2f M/S", player.horizSpeed()), hudAmber); y += lh;
      UI::text(hud, x, y, s, std::string("STATE ") + player.stateName(), hudWhite); y += lh;
      UI::text(hud, x, y, s, std::string("GROUND ") + (player.grounded ? "YES" : "NO"), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("ADS %.0f%%", ws.adsBlend * 100.f), ws.adsBlend > 0.5f ? hudCyan : hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("SPREAD %.2F DEG", ws.currentSpreadDeg(player)), hudWhite); y += lh;
      UI::text(hud, x, y, s, fmt("RECOIL P %.2f Y %.2f", ws.cur().recoil.camPitchDeg(), ws.cur().recoil.camYawDeg()), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("PATTERN #%d  KICK %.2f", (float)ws.cur().recoil.patternIndex(), ws.cur().recoil.kickDeg()), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("POS %.1f %.1f %.1f", 0.f).substr(0,0) + fmt("X %.1f", player.pos.x) + fmt("  Y %.1f", player.pos.y) + fmt("  Z %.1f", player.pos.z), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("FOV %.1f", camera.fov), hudDim); y += lh;
      UI::text(hud, x, y, s, fmt("TIME SCALE %.1f", timescale), timescale < 0.95f ? hudCyan : hudDim); y += lh;
      UI::text(hud, x, y, s, "[F1] DEBUG  [TAB] SETTINGS", hudDim);
    }

    // top-right score
    {
      float s = 2.f * sc, lh = 24.f * sc;
      float acc = ws.shotsFired > 0 ? 100.f * ws.shotsHit / ws.shotsFired : 0.f;
      std::string lines[4] = {
        fmt("KILLS %d", (float)ws.kills),
        fmt("HITS %d / %d", (float)ws.shotsHit) + fmt("  (%d SHOTS)", (float)ws.shotsFired),
        fmt("ACCURACY %.0f%%", acc),
        fmt("HEADSHOTS %d   TARGETS %d/8", (float)ws.headshots),
      };
      lines[3] = fmt("HS %d   ALIVE %d/8", (float)ws.headshots, (float)world.aliveCount());
      float w = 300.f * sc, x = W - w - 16.f * sc, y = 14.f * sc;
      UI::panel(hud, x - 6 * sc, y - 6 * sc, w + 12.f * sc, 4.f * lh + 12.f * sc, hudAmber);
      for (auto& l : lines) { UI::text(hud, x, y, s, l, hudWhite); y += lh; }
    }

    // bottom-right ammo
    {
      float s = 2.f * sc;
      std::string ammo = std::to_string(cw.mag);
      std::string res = ws.infiniteAmmo ? "INF" : std::to_string(cw.reserve);
      float big = 7.f * sc;
      std::string bigS = ammo + " / " + res;
      float w = UI::textW(big, bigS);
      float x = W - w - 24.f * sc, y = H - 118.f * sc;
      glm::vec4 magC = cw.mag == 0 ? hudRed : (cw.mag <= wd.magSize * 0.3f ? hudAmber : hudWhite);
      UI::text(hud, x, y, big, ammo, magC);
      UI::text(hud, x + UI::textW(big, ammo + " "), y + 3.f * sc, 4.f * sc, "/ " + res, hudDim);
      UI::text(hud, x, y + 56.f * sc, s, wd.name, hudAmber);
      UI::text(hud, x, y + 78.f * sc, s, std::string(wd.autoFire ? "AUTO" : "SEMI") + (ws.adsBlend > 0.5f ? "  AIMING" : "  HIP"), ws.adsBlend > 0.5f ? hudCyan : hudDim);
      if (ws.action == WAction::Reload) {
        float p = clampf(ws.actionT / ws.actionDur, 0.f, 1.f);
        UI::rect(hud, x, y - 16.f * sc, w, 6.f * sc, glm::vec4(0.1f, 0.1f, 0.14f, 0.8f));
        UI::rect(hud, x, y - 16.f * sc, w * p, 6.f * sc, hudAmber);
        UI::text(hud, x, y - 36.f * sc, s, "RELOADING", hudAmber);
      } else if (cw.mag == 0) {
        bool blink = std::fmod(simTime * 2.f, 1.f) < 0.6f;
        if (blink) UI::text(hud, x, y - 30.f * sc, s, cw.reserve > 0 || ws.infiniteAmmo ? "[R] RELOAD" : "NO AMMO - [T] RESET", hudRed);
      } else if (cw.mag <= wd.magSize * 0.25f) {
        UI::text(hud, x, y - 30.f * sc, s, "LOW AMMO - [R]", hudAmber);
      }
      if (ws.action == WAction::Inspect)
        UI::text(hud, x, y - 30.f * sc, s, "INSPECTING", hudCyan);
    }

    // bottom-left help
    if (showHelp) {
      float s = 1.8f * sc, lh = 20.f * sc;
      const char* lines[] = {
        "WASD MOVE  SHIFT SPRINT  CTRL/C CROUCH  SPACE JUMP",
        "LMB FIRE  RMB ADS  R RELOAD  F INSPECT  1/2/WHEEL SWAP",
        "TAB SETTINGS  T RESET DRILL  M MUTE  H HELP  ESC PAUSE",
      };
      float h = 3.f * lh + 14.f * sc;
      float y = H - h - 14.f * sc;
      UI::panel(hud, 16.f * sc, y, 620.f * sc, h, hudAmber);
      float ty = y + 8.f * sc;
      for (auto* l : lines) { UI::text(hud, 26.f * sc, ty, s, l, hudDim); ty += lh; }
    }

    // settings panel
    if (settingsOpen && !paused) {
      float s = 2.f * sc, rowH = 28.f * sc;
      float pw = 560.f * sc, px = 16.f * sc, py = 340.f * sc;
      const int VIS = 16;
      float ph = (VIS + 2.6f) * rowH;
      UI::panel(hud, px, py, pw, ph, hudCyan);
      UI::text(hud, px + 14.f * sc, py + 10.f * sc, s, "GAMEPLAY SETTINGS - LIVE", hudCyan);
      float ry = py + 2.0f * rowH;
      for (int vi = 0; vi < VIS; ++vi) {
        int i = scroll + vi;
        if (i >= (int)rows.size()) break;
        const SettingRow& r = rows[i];
        bool selected = i == sel;
        if (selected) UI::rect(hud, px + 6.f * sc, ry - 4.f * sc, pw - 12.f * sc, rowH - 2.f * sc, glm::vec4(1.f, 0.78f, 0.3f, 0.13f));
        glm::vec4 lc = selected ? hudWhite : hudDim;
        UI::text(hud, px + 14.f * sc, ry, s, (selected ? "> " : "  ") + std::string(r.label), lc);
        std::string val;
        glm::vec4 vc = hudAmber;
        if (r.kind == 0) val = fmt("%.2f", *r.f);
        else if (r.kind == 1) { val = *r.b ? "ON" : "OFF"; vc = *r.b ? hudGreen : hudDim; }
        else { val = "[ENTER]"; vc = hudCyan; }
        UI::text(hud, px + pw - UI::textW(s, val) - 16.f * sc, ry, s, val, vc);
        ry += rowH;
      }
      UI::text(hud, px + 14.f * sc, py + ph - rowH + 4.f * sc, 1.8f * sc,
               "UP/DN SELECT   LEFT/RIGHT ADJUST   TAB CLOSE", hudDim);
    }

    // pause overlay
    if (paused) {
      UI::rect(hud, 0, 0, W, H, glm::vec4(0.02f, 0.03f, 0.06f, 0.62f));
      std::string t = "PAUSED";
      UI::text(hud, cx - UI::textW(6.f * sc, t) * 0.5f, cy - 60.f * sc, 6.f * sc, t, hudAmber);
      std::string l = "CLICK OR ESC TO RESUME   -   X TO QUIT";
      UI::text(hud, cx - UI::textW(2.5f * sc, l) * 0.5f, cy + 20.f * sc, 2.5f * sc, l, hudWhite);
    }

    glDisable(GL_DEPTH_TEST);
    hud.draw(flatS, ortho, glm::mat4(1), glm::mat4(1), false, false);
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(win);
    input.endFrame();
  }

  audio.shutdown();
  ws.destroy(); world.destroy();
  worldFlat.destroy(); worldAdd.destroy(); hud.destroy(); vmFx.destroy(); skyBatch.destroy();
  worldS.destroy(); flatS.destroy(); skyS.destroy();
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
