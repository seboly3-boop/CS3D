#pragma once
// Delta-time animation toolkit: exponential damping, semi-implicit springs,
// easing, pose blending and keyframe tracks. Everything is frame-rate
// independent; springs substep internally so large dt never explodes.
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

namespace anim {

constexpr float PI = 3.14159265358979f;
inline float rad(float deg) { return deg * PI / 180.0f; }
inline float deg(float r)   { return r * 180.0f / PI; }
inline float clampf(float x, float a, float b) { return std::min(std::max(x, a), b); }
inline float damp(float a, float b, float lambda, float dt) {
  return b + (a - b) * std::exp(-lambda * dt);
}
inline glm::vec2 damp(const glm::vec2& a, const glm::vec2& b, float l, float dt) {
  return b + (a - b) * std::exp(-l * dt);
}
inline glm::vec3 damp(const glm::vec3& a, const glm::vec3& b, float l, float dt) {
  return b + (a - b) * std::exp(-l * dt);
}
inline float moveToward(float a, float b, float maxDelta) {
  if (a < b) return std::min(a + maxDelta, b);
  return std::max(a - maxDelta, b);
}
inline float smooth01(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }
inline float smoother01(float t) { t = clampf(t, 0, 1); return t * t * t * (t * (t * 6 - 15) + 10); }
inline float easeInCubic(float t)  { t = clampf(t, 0, 1); return t * t * t; }
inline float easeOutCubic(float t) { t = clampf(t, 0, 1); float u = 1 - t; return 1 - u * u * u; }
inline float easeOutBack(float t) {
  t = clampf(t, 0, 1); const float c = 1.70158f; float u = t - 1;
  return 1 + (c + 1) * u * u * u + c * u * u;
}

// Critically/under-damped spring. k = stiffness, d = damping.
struct Spring {
  float x = 0, v = 0;
  void update(float target, float k, float d, float dt) {
    int n = std::max(1, (int)std::ceil(dt / 0.008f));
    float h = dt / (float)n;
    for (int i = 0; i < n; ++i) {
      v += (-k * (x - target) - d * v) * h;
      x += v * h;
    }
  }
  void impulse(float dv) { v += dv; }
  void reset() { x = v = 0; }
};

// Pose: position (meters, view space) + rotation (radians: x=pitch, y=yaw, z=roll)
struct Pose {
  glm::vec3 pos{0}, rot{0};
};
inline Pose blend(const Pose& a, const Pose& b, float t) {
  return { a.pos + (b.pos - a.pos) * t, a.rot + (b.rot - a.rot) * t };
}
inline Pose addScaled(const Pose& base, const Pose& off, float w) {
  return { base.pos + off.pos * w, base.rot + off.rot * w };
}
inline Pose damp(const Pose& a, const Pose& b, float l, float dt) {
  return { damp(a.pos, b.pos, l, dt), damp(a.rot, b.rot, l, dt) };
}

struct PoseKey { float t; glm::vec3 pos, rot; };
Pose trackEval(const std::vector<PoseKey>& keys, float t);

} // namespace anim

// ---------------------------------------------------------------------------
// Layered procedural weapon animator.
// ---------------------------------------------------------------------------
enum class WAction { None, Reload, Inspect, SwitchDown, SwitchUp };

struct WeaponVisual {
  anim::Pose hip, ads, sprint;
  float sightY = 0.056f;     // iron-sight line height above bore (local)
  float scale = 1.0f;        // offsets scale (pistol smaller than rifle)
  float bobScale = 1.0f;
  glm::vec3 muzzle{0, 0.01f, -0.5f};
};

struct AnimInput {
  float dt = 0, time = 0;
  float speedNorm = 0;             // horizontal speed / sprint speed
  float sprintBlend = 0, crouchBlend = 0, adsBlend = 0;
  bool  grounded = true;
  float vertVel = 0;
  glm::vec2 moveLocal{0};          // x=strafe, y=forward, normalized-ish
  glm::vec2 mouseDelta{0};         // raw counts this frame
  float landImpact = 0;            // m/s, consumed impulse (0 if none)
  bool  jumped = false;
  float fireT = 99.f;              // seconds since last shot
  bool  emptyMag = false;
  WAction action = WAction::None;
  float actionT = 0, actionDur = 1;
  float swayScale = 1, bobScale = 1;
};

struct AnimOutput {
  anim::Pose pose;
  glm::vec3 magPos{0};
  float magPitch = 0;
  float boltZ = 0;      // rifle charging handle / pistol slide travel (+z = back)
  glm::vec3 leftHandPos{0};
};

class WeaponAnimator {
public:
  void compute(const WeaponVisual& vis, const AnimInput& in, AnimOutput& out);
  void reset();
private:
  anim::Spring swayX_, swayY_, swayPitch_, swayYaw_;
  anim::Spring landSpr_, jumpSpr_, boltLock_;
  float tiltRoll_ = 0, tiltPitch_ = 0, airW_ = 0;
  float bobPhase_ = 0, breath_ = 0;
  anim::Pose cur_{};
  bool curInit_ = false;
  float magY_ = 0, magP_ = 0;
  glm::vec3 handCur_{0};
};
