#include "Animation.h"

namespace anim {

Pose trackEval(const std::vector<PoseKey>& keys, float t) {
  if (keys.empty()) return {};
  if (t <= keys.front().t) return { keys.front().pos, keys.front().rot };
  for (size_t i = 1; i < keys.size(); ++i) {
    if (t <= keys[i].t) {
      float u = smoother01((t - keys[i - 1].t) / std::max(1e-5f, keys[i].t - keys[i - 1].t));
      return { keys[i - 1].pos + (keys[i].pos - keys[i - 1].pos) * u,
               keys[i - 1].rot + (keys[i].rot - keys[i - 1].rot) * u };
    }
  }
  return { keys.back().pos, keys.back().rot };
}

} // namespace anim

using namespace anim;

// Offset tracks (added on top of the blended base pose).
static const std::vector<PoseKey> kReload = {
  {0.00f, {0,0,0},        {0,0,0}},
  {0.15f, {-0.02f,-0.055f,0.03f}, {-0.28f,0.18f,0.12f}},
  {0.62f, {-0.02f,-0.055f,0.03f}, {-0.28f,0.18f,0.12f}},
  {0.80f, {-0.01f,-0.03f,0.02f},  {-0.15f,0.10f,0.06f}},
  {1.00f, {0,0,0},        {0,0,0}},
};
static const std::vector<PoseKey> kInspect = {
  {0.00f, {0,0,0},        {0,0,0}},
  {0.18f, {-0.16f,0.03f,0.06f},  {0.05f,0.45f,0.30f}},
  {0.38f, {-0.18f,0.05f,0.08f},  {0.10f,0.25f,0.85f}},
  {0.55f, {-0.16f,0.04f,0.06f},  {-0.05f,-0.20f,0.55f}},
  {0.75f, {-0.14f,0.02f,0.05f},  {0.08f,-0.45f,-0.25f}},
  {1.00f, {0,0,0},        {0,0,0}},
};

void WeaponAnimator::reset() {
  swayX_.reset(); swayY_.reset(); swayPitch_.reset(); swayYaw_.reset();
  landSpr_.reset(); jumpSpr_.reset(); boltLock_.reset();
  tiltRoll_ = tiltPitch_ = airW_ = bobPhase_ = breath_ = magY_ = magP_ = 0;
  handCur_ = glm::vec3(0);
  curInit_ = false;
}

void WeaponAnimator::compute(const WeaponVisual& vis, const AnimInput& in, AnimOutput& out) {
  const float dt = in.dt;
  const float ads = in.adsBlend;
  breath_ += dt;

  // ---- Layer 0: base pose (hip <-> ADS) -----------------------------------
  Pose p = blend(vis.hip, vis.ads, ads);

  // ---- Layer 1: sprint cant (suppressed right after firing) ---------------
  float fireSup = smooth01(in.fireT / 0.22f);
  float sprintW = in.sprintBlend * (1.f - ads) * fireSup;
  if (in.action != WAction::None) sprintW *= 0.15f;
  p = blend(p, vis.sprint, sprintW * 0.9f);

  // ---- Layer 2: movement bob (figure-8) ------------------------------------
  float moveW = clampf(in.speedNorm * 1.4f, 0.f, 1.f) * (in.grounded ? 1.f : 0.15f)
              * (1.f - 0.85f * ads) * in.bobScale * vis.bobScale;
  bobPhase_ += dt * (3.0f + 7.0f * in.speedNorm) * (in.grounded ? 1.f : 0.3f);
  float bobA = moveW * (0.006f + 0.011f * sprintW);
  p.pos.x += std::sin(bobPhase_) * bobA;
  p.pos.y += std::sin(bobPhase_ * 2.f) * bobA * 0.8f;
  p.rot.x += std::sin(bobPhase_ * 2.f) * 0.010f * moveW;
  p.rot.y += std::sin(bobPhase_) * 0.008f * moveW;
  p.rot.z += std::sin(bobPhase_) * 0.022f * moveW * (1.f + sprintW);

  // ---- Layer 3: idle breathing ---------------------------------------------
  float idleW = (1.f - ads * 0.85f) * (1.f - moveW * 0.8f) * (1.f - sprintW);
  p.pos.y += std::sin(breath_ * 1.4f) * 0.0016f * idleW;
  p.pos.x += std::sin(breath_ * 0.9f) * 0.0008f * idleW;
  p.rot.x += std::sin(breath_ * 1.1f) * 0.0025f * idleW;
  p.rot.z += std::cos(breath_ * 0.8f) * 0.0020f * idleW;

  // ---- Layer 4: airborne lag + jump/land springs ---------------------------
  airW_ = damp(airW_, in.grounded ? 0.f : 1.f, 10.f, dt);
  p.pos.y -= clampf(in.vertVel * 0.004f, -0.02f, 0.02f) * airW_;
  p.rot.x += clampf(-in.vertVel * 0.010f, -0.06f, 0.06f) * airW_;
  if (in.jumped) jumpSpr_.impulse(-1.5f);
  if (in.landImpact > 0.01f) landSpr_.impulse(-in.landImpact * 0.22f);
  jumpSpr_.update(0, 120, 12, dt);
  landSpr_.update(0, 120, 12, dt);
  p.pos.y += (jumpSpr_.x + landSpr_.x) * 0.012f;
  p.rot.x += (jumpSpr_.x + landSpr_.x) * 0.020f;

  // ---- Layer 5: mouse sway (lag-and-return springs, reduced in ADS) --------
  float swayK = in.swayScale * (1.f - 0.78f * ads);
  float dx = clampf(in.mouseDelta.x, -40.f, 40.f);
  float dy = clampf(in.mouseDelta.y, -40.f, 40.f);
  swayX_.impulse(-dx * 0.00045f * swayK);
  swayY_.impulse( dy * 0.00045f * swayK);
  swayYaw_.impulse(-dx * 0.0030f * swayK);
  swayPitch_.impulse(dy * 0.0024f * swayK);
  swayX_.update(0, 90, 14, dt);  swayY_.update(0, 90, 14, dt);
  swayYaw_.update(0, 90, 14, dt); swayPitch_.update(0, 90, 14, dt);
  p.pos.x += clampf(swayX_.x, -0.02f, 0.02f);
  p.pos.y += clampf(swayY_.x, -0.02f, 0.02f);
  p.rot.y += clampf(swayYaw_.x, -0.06f, 0.06f);
  p.rot.x += clampf(swayPitch_.x, -0.06f, 0.06f);

  // ---- Layer 6: velocity tilt ----------------------------------------------
  tiltRoll_  = damp(tiltRoll_,  -in.moveLocal.x * 0.028f * (1.f - ads), 8.f, dt);
  tiltPitch_ = damp(tiltPitch_, -in.moveLocal.y * 0.012f * (1.f - ads), 8.f, dt);
  p.rot.z += tiltRoll_;
  p.rot.x += tiltPitch_;
  p.pos.y -= in.crouchBlend * 0.006f;

  // ---- Layer 7: action tracks ----------------------------------------------
  float ap = clampf(in.actionT / std::max(0.01f, in.actionDur), 0.f, 1.f);
  float magY = 0, magP = 0;
  glm::vec3 handOff(0);
  if (in.action == WAction::Reload) {
    Pose off = trackEval(kReload, ap);
    p = addScaled(p, off, vis.scale);
    if (ap < 0.20f)       magY = 0;
    else if (ap < 0.42f)  magY = -0.22f * easeInCubic((ap - 0.20f) / 0.22f);
    else if (ap < 0.45f)  magY = -0.22f;
    else if (ap < 0.62f)  magY = -0.22f * (1.f - easeOutCubic((ap - 0.45f) / 0.17f));
    else                  magY = 0;
    magP = 0.30f * std::sin(PI * clampf((ap - 0.20f) / 0.42f, 0.f, 1.f));
    float follow = smooth01((ap - 0.15f) / 0.10f) * (1.f - smooth01((ap - 0.62f) / 0.12f));
    handOff = glm::vec3(0, magY * 0.85f * follow, 0.04f * follow);
  } else if (in.action == WAction::Inspect) {
    p = addScaled(p, trackEval(kInspect, ap), vis.scale);
  } else if (in.action == WAction::SwitchDown) {
    float e = easeInCubic(ap);
    p.pos.y -= 0.30f * e; p.rot.x -= 0.75f * e;
  } else if (in.action == WAction::SwitchUp) {
    float e = 1.f - easeOutCubic(ap);
    p.pos.y -= 0.30f * e; p.rot.x -= 0.75f * e;
  }

  // ---- Parts: bolt / slide cycle + empty lock (spring, never snaps) --------
  float cycle = 0;
  float travel = 0.032f * vis.scale + 0.012f; // pistol slide travels a bit more
  if (in.fireT < 0.055f) cycle = std::sin(PI * in.fireT / 0.055f) * travel;
  if (in.action == WAction::Reload && ap > 0.78f && ap < 0.92f)
    cycle = std::sin(PI * (ap - 0.78f) / 0.14f) * travel;
  bool locked = in.emptyMag && !(in.action == WAction::Reload && ap > 0.75f);
  boltLock_.update(locked ? travel * 1.05f : 0.f, 420, 26, dt);

  // ---- Temporal smoothing of everything (kills any residual snap) ---------
  if (!curInit_) { cur_ = p; curInit_ = true; }
  cur_ = damp(cur_, p, 25.f, dt);
  magY_ = damp(magY_, magY, 30.f, dt);
  magP_ = damp(magP_, magP, 30.f, dt);
  handCur_ = damp(handCur_, handOff, 25.f, dt);

  out.pose = cur_;
  out.magPos = glm::vec3(0, magY_ * vis.scale, -magY_ * 0.15f);
  out.magPitch = magP_;
  out.boltZ = cycle + boltLock_.x;
  out.leftHandPos = handCur_;
}
