#include "Weapon.h"
#include "Animation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
using namespace anim;

// ---------------------------------------------------------------------------
void WeaponSystem::buildDefs() {
  WeaponDef& r = defs_[0];
  r.name = "AR-17 VANGUARD"; r.autoFire = true; r.rpm = 660;
  r.magSize = 30; r.reserveMax = 90;
  r.damage = 33; r.headMult = 4; r.legMult = 0.75f;
  r.adsTime = 0.24f; r.adsFov = 52;
  r.hipSpread = 1.15f; r.adsSpread = 0.06f; r.moveSpread = 1.7f; r.airSpread = 2.6f;
  r.reloadTime = 2.2f; r.inspectTime = 2.8f;
  r.recoil = makeRifleProfile();
  r.vis.hip    = { {0.24f,-0.22f,-0.46f}, {0.0f, 0.05f, 0.0f} };
  r.vis.ads    = { {0.0f, -0.056f, -0.30f}, {0,0,0} };
  r.vis.sprint = { {0.06f,-0.24f,-0.38f}, {0.25f, 0.50f, -0.30f} };
  r.vis.sightY = 0.056f; r.vis.scale = 1.0f; r.vis.bobScale = 1.0f;
  r.vis.muzzle = { 0, 0.012f, -0.55f };
  r.fireSound = "rifle";

  WeaponDef& p = defs_[1];
  p.name = "P-9 SIDEARM"; p.autoFire = false; p.rpm = 420;
  p.magSize = 12; p.reserveMax = 48;
  p.damage = 26; p.headMult = 4; p.legMult = 0.75f;
  p.adsTime = 0.16f; p.adsFov = 60;
  p.hipSpread = 0.8f; p.adsSpread = 0.05f; p.moveSpread = 1.1f; p.airSpread = 2.0f;
  p.reloadTime = 1.65f; p.inspectTime = 2.2f;
  p.recoil = makePistolProfile();
  p.vis.hip    = { {0.21f,-0.20f,-0.42f}, {0.0f, 0.04f, 0.0f} };
  p.vis.ads    = { {0.0f, -0.055f, -0.26f}, {0,0,0} };
  p.vis.sprint = { {0.08f,-0.22f,-0.36f}, {0.30f, 0.40f, -0.25f} };
  p.vis.sightY = 0.055f; p.vis.scale = 0.9f; p.vis.bobScale = 0.8f;
  p.vis.muzzle = { 0, 0.02f, -0.16f };
  p.fireSound = "pistol";
}

void WeaponSystem::init() {
  buildDefs();
  for (int i = 0; i < 2; ++i) {
    weapons[i].def = &defs_[i];
    weapons[i].mag = defs_[i].magSize;
    weapons[i].reserve = defs_[i].reserveMax;
    weapons[i].recoil.init(defs_[i].recoil);
  }
  current = 0; action = WAction::None; adsT = adsBlend = 0;
  anim_.reset();
  buildMeshes();
}

void WeaponSystem::destroy() {
  for (int i = 0; i < 2; ++i) {
    body_[i].destroy(); mag_[i].destroy(); bolt_[i].destroy();
    sights_[i].destroy(); handL_[i].destroy();
  }
}

void WeaponSystem::refillAll() {
  for (int i = 0; i < 2; ++i) {
    weapons[i].mag = weapons[i].def->magSize;
    weapons[i].reserve = weapons[i].def->reserveMax;
    weapons[i].recoil.reset();
  }
  action = WAction::None; adsT = 0;
}
void WeaponSystem::resetStats() { shotsFired = shotsHit = kills = headshots = 0; }

// ---------------------------------------------------------------- viewmodel
void WeaponSystem::buildMeshes() {
  const glm::vec3 receiver(0.16f, 0.17f, 0.19f), barrel(0.11f, 0.11f, 0.13f);
  const glm::vec3 guard(0.20f, 0.21f, 0.23f), stock(0.18f, 0.19f, 0.21f);
  const glm::vec3 grip(0.14f, 0.15f, 0.16f), magC(0.22f, 0.23f, 0.25f);
  const glm::vec3 glove(0.22f, 0.24f, 0.20f), cuff(0.15f, 0.16f, 0.14f);
  const glm::vec3 amber(1.0f, 0.72f, 0.25f), white(0.95f, 0.95f, 1.0f);
  auto T = [](glm::vec3 v) { return glm::translate(glm::mat4(1), v); };
  auto rotXf = [](glm::vec3 v, float a) { return glm::translate(glm::mat4(1), v) * glm::rotate(glm::mat4(1), a, glm::vec3(1, 0, 0)); };

  { // ---- Rifle ----
    MeshBuilder b;
    b.box({ -0.030f, -0.045f, -0.21f }, { 0.030f, 0.045f, 0.21f }, receiver);
    b.cylinder(0.014f, 0.34f, 12, barrel,
               T({ 0, 0.012f, -0.38f }) * glm::rotate(glm::mat4(1), rad(90.f), glm::vec3(1, 0, 0)));
    b.box({ -0.028f, -0.030f, -0.43f }, { 0.028f, 0.030f, -0.21f }, guard);
    for (int k = 0; k < 3; ++k)
      b.box({ -0.030f, 0.030f, -0.40f + k * 0.06f }, { 0.030f, 0.036f, -0.37f + k * 0.06f }, barrel);
    b.box({ -0.025f, -0.040f, 0.21f }, { 0.025f, 0.040f, 0.39f }, stock);
    b.box({ -0.020f, -0.135f, 0.065f }, { 0.020f, -0.045f, 0.115f }, grip, rotXf({0,0,0}, 0.28f));
    b.box({ -0.010f, -0.062f, -0.045f }, { 0.010f, -0.045f, 0.02f }, barrel); // trigger guard
    // sight posts (dark) live on the body; glowing dots are a separate mesh
    b.box({ -0.0155f, 0.042f, 0.098f }, { -0.0105f, 0.056f, 0.103f }, barrel);
    b.box({  0.0105f, 0.042f, 0.098f }, {  0.0155f, 0.056f, 0.103f }, barrel);
    b.box({ -0.0025f, 0.032f, -0.462f }, { 0.0025f, 0.056f, -0.457f }, barrel);
    // right hand on grip + cuff
    b.box({ -0.028f, -0.130f, 0.062f }, { 0.032f, -0.058f, 0.128f }, glove, rotXf({0,0,0}, 0.28f));
    b.box({ -0.030f, -0.190f, 0.095f }, { 0.034f, -0.130f, 0.160f }, cuff, rotXf({0,0,0}, 0.28f));
    body_[0] = b.build();

    b.clear(); // magazine: origin at insertion point, extends downward
    b.box({ -0.021f, -0.135f, -0.040f }, { 0.021f, 0.0f, 0.040f }, magC, rotXf({ 0, -0.045f, -0.02f }, 0.12f));
    b.box({ -0.023f, -0.145f, -0.043f }, { 0.023f, -0.128f, 0.043f }, amber * 0.6f, rotXf({ 0, -0.045f, -0.02f }, 0.12f));
    mag_[0] = b.build();

    b.clear(); // charging handle
    b.box({ 0.030f, 0.020f, 0.02f }, { 0.046f, 0.036f, 0.08f }, magC);
    bolt_[0] = b.build();

    b.clear(); // emissive sight dots (aligned to y = 0.056 sight line)
    b.box({ -0.0145f, 0.052f, 0.0975f }, { -0.0115f, 0.056f, 0.1005f }, white);
    b.box({  0.0115f, 0.052f, 0.0975f }, {  0.0145f, 0.056f, 0.1005f }, white);
    b.box({ -0.002f, 0.052f, -0.4625f }, { 0.002f, 0.056f, -0.4585f }, amber);
    sights_[0] = b.build();

    b.clear(); // support hand on handguard
    b.box({ -0.033f, -0.075f, -0.345f }, { 0.027f, -0.018f, -0.255f }, glove);
    b.box({ -0.035f, -0.125f, -0.320f }, { 0.029f, -0.075f, -0.262f }, cuff);
    handL_[0] = b.build();
  }
  { // ---- Pistol ----
    MeshBuilder b;
    b.box({ -0.021f, -0.045f, -0.095f }, { 0.021f, -0.005f, 0.085f }, receiver); // frame
    b.box({ -0.020f, -0.140f, 0.028f }, { 0.020f, -0.045f, 0.082f }, grip, rotXf({0,0,0}, 0.22f));
    b.box({ -0.008f, -0.062f, -0.055f }, { 0.008f, -0.045f, -0.010f }, barrel); // trigger guard
    b.cylinder(0.009f, 0.035f, 10, barrel,
               T({ 0, 0.020f, -0.115f }) * glm::rotate(glm::mat4(1), rad(90.f), glm::vec3(1, 0, 0)));
    b.box({ -0.0135f, 0.043f, 0.072f }, { -0.0085f, 0.055f, 0.078f }, barrel); // rear posts
    b.box({  0.0085f, 0.043f, 0.072f }, {  0.0135f, 0.055f, 0.078f }, barrel);
    b.box({ -0.0025f, 0.043f, -0.118f }, { 0.0025f, 0.055f, -0.112f }, barrel); // front post
    b.box({ -0.028f, -0.135f, 0.020f }, { 0.030f, -0.055f, 0.090f }, glove, rotXf({0,0,0}, 0.22f));
    b.box({ -0.030f, -0.195f, 0.055f }, { 0.032f, -0.135f, 0.120f }, cuff, rotXf({0,0,0}, 0.22f));
    body_[1] = b.build();

    b.clear(); // magazine inside grip (baseplate visible)
    b.box({ -0.016f, -0.030f, -0.022f }, { 0.016f, 0.0f, 0.022f }, magC, T({ 0, -0.140f, 0.055f }));
    b.box({ -0.019f, -0.038f, -0.026f }, { 0.019f, -0.028f, 0.026f }, amber * 0.6f, T({ 0, -0.140f, 0.055f }));
    mag_[1] = b.build();

    b.clear(); // slide (animated: cycles on fire, locks back empty)
    b.box({ -0.0225f, -0.005f, -0.105f }, { 0.0225f, 0.045f, 0.090f }, glm::vec3(0.19f, 0.20f, 0.23f));
    for (int k = 0; k < 4; ++k)
      b.box({ -0.0235f, 0.005f, 0.045f + k * 0.011f }, { 0.0235f, 0.040f, 0.050f + k * 0.011f }, barrel);
    bolt_[1] = b.build();

    b.clear();
    b.box({ -0.0125f, 0.051f, 0.0715f }, { -0.0095f, 0.055f, 0.0745f }, white);
    b.box({  0.0095f, 0.051f, 0.0715f }, {  0.0125f, 0.055f, 0.0745f }, white);
    b.box({ -0.002f, 0.051f, -0.1185f }, { 0.002f, 0.055f, -0.1145f }, amber);
    sights_[1] = b.build();
    handL_[1] = Mesh{}; // pistol: one-handed
  }
}

// ---------------------------------------------------------------- update
void WeaponSystem::startSwitch(int want, WSEvents& e) {
  pending_ = want;
  action = WAction::SwitchDown; actionT = 0; actionDur = 0.20f;
  e.switchStart = true;
}

WSEvents WeaponSystem::update(float dt, Input& in, const Player& pl, const Camera& cam,
                              bool allowFire, float time) {
  WSEvents e;
  Weapon& w = cur();
  const WeaponDef& d = *w.def;

  bool fireHeld = allowFire && in.mouseDown(GLFW_MOUSE_BUTTON_LEFT);
  bool firePressed = allowFire && in.mousePressed(GLFW_MOUSE_BUTTON_LEFT);
  adsHeld = allowFire && in.mouseDown(GLFW_MOUSE_BUTTON_RIGHT);

  // --- weapon switch input ---
  int want = -1;
  if (in.pressed(GLFW_KEY_1)) want = 0;
  if (in.pressed(GLFW_KEY_2)) want = 1;
  if (in.wheel() > 0.5f) want = (current + 1) % 2;
  if (in.wheel() < -0.5f) want = (current + 1) % 2;
  if (want != -1 && want != current && action != WAction::SwitchDown && action != WAction::SwitchUp)
    startSwitch(want, e);

  // --- advance action state machine ---
  actionT += dt;
  float ap = actionT / std::max(0.01f, actionDur);
  switch (action) {
    case WAction::SwitchDown:
      if (ap >= 1.f) {
        current = pending_; action = WAction::SwitchUp; actionT = 0; actionDur = 0.28f;
        cur().cooldown = std::max(cur().cooldown, 0.05f);
      }
      break;
    case WAction::SwitchUp:
      if (ap >= 1.f) { action = WAction::None; e.raiseDone = true; }
      break;
    case WAction::Reload:
      if (!magOutDone_ && ap >= 0.42f) { magOutDone_ = true; e.magOut = true; }
      if (!ammoApplied_ && ap >= 0.62f) {
        ammoApplied_ = true; e.magIn = true;
        Weapon& rw = cur();
        int need = rw.def->magSize - rw.mag;
        int take = infiniteAmmo ? need : std::min(need, rw.reserve);
        rw.mag += take;
        if (!infiniteAmmo) rw.reserve -= take;
      }
      if (!boltDone_ && ap >= 0.80f) { boltDone_ = true; e.bolt = true; }
      if (ap >= 1.f) { action = WAction::None; e.reloadDone = true; }
      break;
    case WAction::Inspect:
      if (firePressed || adsHeld || pl.sprintBlend > 0.5f || !pl.grounded) action = WAction::None;
      else if (ap >= 1.f) action = WAction::None;
      break;
    default: break;
  }

  // --- start actions ---
  if (action == WAction::None) {
    if (in.pressed(GLFW_KEY_R) && w.mag < d.magSize && (w.reserve > 0 || infiniteAmmo)) {
      action = WAction::Reload; actionT = 0; actionDur = d.reloadTime;
      magOutDone_ = ammoApplied_ = boltDone_ = false;
      e.reloadStart = true;
    } else if (in.pressed(GLFW_KEY_F)) {
      action = WAction::Inspect; actionT = 0; actionDur = d.inspectTime;
      e.inspectStart = true;
    }
  }

  // --- ADS: linear timer + smootherstep easing, per-weapon speed ---
  bool adsAllowed = adsHeld && action == WAction::None;
  float rate = dt / std::max(0.05f, d.adsTime * adsTimeScale);
  adsT = moveToward(adsT, adsAllowed ? 1.f : 0.f, rate);
  adsBlend = smoother01(adsT);

  // --- timers ---
  for (auto& wp : weapons) { wp.cooldown -= dt; wp.recoil.update(dt, recoilScale); }
  dryCd_ -= dt;
  lastShotT_ += dt;
  fireFlash_ *= std::exp(-28.f * dt);

  // --- fire decision (recoil applied now so the camera sees it this frame) ---
  if (action == WAction::None) {
    bool trigger = d.autoFire ? fireHeld : firePressed;
    while (trigger && e.shots < 2 && w.cooldown <= 0.f) {
      if (w.mag <= 0) {
        if (dryCd_ <= 0.f) { e.dry = true; dryCd_ = 0.30f; }
        break;
      }
      w.mag--;
      w.cooldown += 60.f / d.rpm;
      w.recoil.onShot(adsBlend, recoilScale);
      lastShotT_ = 0; fireFlash_ = 1;
      flashSeed_ = (float)std::rand() / RAND_MAX;
      shotsFired++;
      e.shotInfo[e.shots++] = { d.damage, d.headMult, d.legMult };
      if (!d.autoFire) break;
    }
  }

  // --- animator ---
  AnimInput ai;
  ai.dt = dt; ai.time = time;
  float hs = pl.horizSpeed();
  ai.speedNorm = hs / std::max(1.f, pl.p.sprintSpeed);
  ai.sprintBlend = pl.sprintBlend;
  ai.crouchBlend = pl.crouchT;
  ai.adsBlend = adsBlend;
  ai.grounded = pl.grounded;
  ai.vertVel = pl.vel.y;
  glm::vec3 fwdH(-std::sin(cam.yaw), 0, -std::cos(cam.yaw));
  glm::vec3 rightH(std::cos(cam.yaw), 0, -std::sin(cam.yaw));
  ai.moveLocal = glm::vec2(glm::dot(pl.vel, rightH), glm::dot(pl.vel, fwdH))
               / std::max(1.f, pl.p.sprintSpeed);
  ai.mouseDelta = in.mouseDelta();
  ai.landImpact = pl.ev.landed ? pl.ev.landImpact : 0.f;
  ai.jumped = pl.ev.jumped;
  ai.fireT = lastShotT_;
  ai.emptyMag = w.mag == 0;
  ai.action = action; ai.actionT = actionT; ai.actionDur = actionDur;
  ai.swayScale = swayScale; ai.bobScale = 1.f;
  anim_.compute(d.vis, ai, out);
  return e;
}

void WeaponSystem::reportHit(bool target, bool head, bool kill) {
  if (target) { shotsHit++; if (head) headshots++; }
  if (kill) kills++;
}

float WeaponSystem::currentSpreadDeg(const Player& pl) const {
  const WeaponDef& d = def();
  float s = d.hipSpread * (1.f - adsBlend) + d.adsSpread * adsBlend;
  float speedNorm = pl.horizSpeed() / std::max(1.f, pl.p.sprintSpeed);
  s += d.moveSpread * speedNorm * (1.f - 0.6f * adsBlend);
  if (!pl.grounded) s += d.airSpread * (1.f - 0.4f * adsBlend);
  if (pl.crouchT > 0.5f) s *= 0.75f;
  s += std::min(cur().recoil.camPitchDeg() * 0.12f, 0.6f) * (1.f - 0.8f * adsBlend);
  return s * spreadScale;
}

glm::vec3 WeaponSystem::muzzleWorld(const Camera& cam) const {
  return cam.eye + cam.fwd * 0.55f
       + cam.right * (out.pose.pos.x * 0.6f)
       + cam.up * (out.pose.pos.y * 0.6f + 0.02f);
}

// ---------------------------------------------------------------- draw
void WeaponSystem::draw(const glm::mat4& vmProj, Shader& world, Shader& flat, FlatBatch& fx, float time) {
  (void)time;
  const WeaponDef& d = def();
  const RecoilSystem& rc = cur().recoil;

  // Final pose = smoothed animator output + recoil kick springs.
  glm::vec3 pos = out.pose.pos;
  glm::vec3 rot = out.pose.rot;
  pos.z += rc.wpnBack() * 0.04f;
  pos.y += rc.wpnBack() * 0.006f;
  rot.x += rc.wpnPitch() * 0.04f;
  rot.z += rc.wpnRoll() * 0.04f;

  glm::mat4 base = glm::translate(glm::mat4(1), pos);
  base = base * glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
  base = base * glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
  base = base * glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));

  world.use();
  world.set("uProj", vmProj);
  world.set("uView", glm::mat4(1));
  world.set("uCamPos", glm::vec3(0));
  world.set("uLightDir", glm::normalize(glm::vec3(-0.4f, 0.8f, 0.35f)));
  world.set("uFogNear", 1e5f); world.set("uFogFar", 2e5f);
  world.set("uGrid", 0.f);
  glm::vec3 flashLocal = glm::vec3(base * glm::vec4(d.vis.muzzle, 1));
  world.set("uFlashPos", flashLocal);
  world.set("uFlashIntensity", fireFlash_ * 2.2f);

  auto part = [&](const Mesh& m, const glm::mat4& xf, float emissive) {
    world.set("uModel", xf);
    world.set("uTint", glm::vec4(1));
    world.set("uEmissive", emissive);
    m.draw();
  };
  part(body_[current], base, 0.f);
  glm::mat4 magM = base * glm::translate(glm::mat4(1), out.magPos)
                 * glm::rotate(glm::mat4(1), out.magPitch, glm::vec3(1, 0, 0));
  part(mag_[current], magM, 0.f);
  part(bolt_[current], base * glm::translate(glm::mat4(1), glm::vec3(0, 0, out.boltZ)), 0.f);
  // Sights ride the slide on the pistol so the lock-back reads correctly.
  glm::mat4 sightM = current == 1
      ? base * glm::translate(glm::mat4(1), glm::vec3(0, 0, out.boltZ)) : base;
  part(sights_[current], sightM, 1.f);
  if (hasLeft_[current])
    part(handL_[current], base * glm::translate(glm::mat4(1), out.leftHandPos), 0.f);

  // Viewmodel muzzle flash (additive crossed quads at the muzzle).
  if (fireFlash_ > 0.05f) {
    fx.clear();
    float s = (0.07f + flashSeed_ * 0.05f) * (current == 0 ? 1.f : 0.75f) * fireFlash_;
    float a = fireFlash_;
    glm::vec3 m = d.vis.muzzle;
    float ang = flashSeed_ * 6.28f;
    for (int k = 0; k < 2; ++k) {
      float ca = std::cos(ang + k * 1.5708f), sa = std::sin(ang + k * 1.5708f);
      glm::vec3 r(ca * s, sa * s, 0), u(-sa * s * 0.45f, ca * s * 0.45f, 0);
      fx.quad(m - r - u, m + r - u, m + r + u, m - r + u, glm::vec4(1.f, 0.85f, 0.45f, a));
    }
    glm::vec3 r2(s * 0.35f, 0, 0), u2(0, s * 0.35f, 0);
    fx.quad(m - r2 - u2 + glm::vec3(0,0,0.01f), m + r2 - u2 + glm::vec3(0,0,0.01f),
            m + r2 + u2 + glm::vec3(0,0,0.01f), m - r2 + u2 + glm::vec3(0,0,0.01f),
            glm::vec4(1.f, 0.97f, 0.85f, a));
    fx.draw(flat, vmProj, glm::mat4(1), base, true, false);
  }
}
