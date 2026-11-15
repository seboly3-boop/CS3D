#include "Camera.h"
using namespace anim;

void Camera::resetAngles(float y, float p) {
  yaw = tYaw_ = y; pitch = tPitch_ = p;
  landSpr_.reset(); jumpSpr_.reset();
  bobPhase_ = bobW_ = rollCur_ = 0;
}

void Camera::look(const glm::vec2& d, float adsBlend, float adsFov) {
  // ADS sensitivity scales with the FOV ratio so tracking speed feels constant.
  float ratio = std::tan(rad(adsFov) * 0.5f) / std::tan(rad(p.baseFov) * 0.5f);
  float scale = (1.f - adsBlend) + ratio * adsBlend;
  float perCount = 0.0022f * p.sensitivity * scale;
  tYaw_   -= d.x * perCount;
  tPitch_ -= d.y * perCount;
  tPitch_ = clampf(tPitch_, rad(-89.f), rad(89.f));
}

void Camera::onLand(float impact) {
  landSpr_.impulse(-clampf(impact, 0.f, 16.f) * 0.048f);
}
void Camera::onJump() { jumpSpr_.impulse(-0.55f); }

void Camera::update(float dt, const glm::vec3& baseEye, float hSpeed, float speedNorm,
                    bool grounded, float crouchBlend, float sprintBlend,
                    float adsBlend, float adsFov, float strafe,
                    float recoilPitchDeg, float recoilYawDeg, float kickDeg, float time) {
  // --- rotation smoothing (natural inertia, still razor-fast at 0) ---
  float lambda = 60.f - 46.f * clampf(p.smoothing, 0.f, 1.f);
  yaw   = damp(yaw,   tYaw_,   lambda, dt);
  pitch = damp(pitch, tPitch_, lambda, dt);

  // --- head bob: phase driven by actual distance travelled ---
  float targetW = (grounded && hSpeed > 0.4f)
      ? ((crouchBlend > 0.5f ? 0.5f : 1.f) + 0.8f * sprintBlend) : 0.f;
  targetW *= (1.f - 0.75f * adsBlend);
  bobW_ = damp(bobW_, targetW, 8.f, dt);
  bobPhase_ += dt * hSpeed * 1.55f;
  float amp = p.bobAmount * bobW_ * clampf(speedNorm * 1.3f, 0.2f, 1.2f);
  float bx = std::sin(bobPhase_) * 0.011f * amp;
  float by = std::sin(bobPhase_ * 2.f) * 0.009f * amp;
  float bobRoll = std::sin(bobPhase_) * 0.0075f * amp;

  // --- landing / jumping vertical springs ---
  landSpr_.update(0, 130, 13, dt);
  jumpSpr_.update(0, 130, 13, dt);
  float springY = clampf(landSpr_.x + jumpSpr_.x, -0.10f, 0.10f);

  // --- subtle breathing sway + strafe lean ---
  by += std::sin(time * 1.2f) * 0.0012f * (1.f - 0.7f * adsBlend);
  bx += std::sin(time * 0.8f) * 0.0008f * (1.f - 0.7f * adsBlend);
  rollCur_ = damp(rollCur_, -strafe * 0.010f * (1.f - 0.6f * adsBlend), 8.f, dt);

  // --- FOV: base + sprint expansion, blended into per-weapon ADS FOV ---
  float fovTarget = (p.baseFov + sprintBlend * p.sprintFovAdd * (1.f - adsBlend)) * (1.f - adsBlend)
                  + adsFov * adsBlend;
  fov = damp(fov, fovTarget, 12.f, dt);

  // --- final orientation includes recoil offsets (bullets follow the view) ---
  float fy = yaw   + rad(recoilYawDeg);
  float fp = clampf(pitch + rad(recoilPitchDeg + kickDeg), rad(-89.5f), rad(89.5f));
  fwd = glm::vec3(std::sin(fy) * std::cos(fp), std::sin(fp), -std::cos(fy) * std::cos(fp));
  // NOTE: yaw sign — mouse right turns right because look() subtracts dx; with
  // this basis increasing yaw turns left, which matches. fwd at yaw 0 = -Z.
  fwd = glm::normalize(glm::vec3(-std::sin(fy) * std::cos(fp), std::sin(fp), -std::cos(fy) * std::cos(fp)));
  right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
  up    = glm::cross(right, fwd);

  eye = baseEye + right * bx + up * (by + springY);

  float roll = bobRoll + rollCur_ + rad(kickDeg) * 0.15f;
  glm::vec3 upRolled = glm::normalize(up * std::cos(roll) + right * std::sin(roll));
  up = upRolled;
}

glm::mat4 Camera::view() const { return glm::lookAt(eye, eye + fwd, up); }
glm::mat4 Camera::proj(float aspect) const {
  return glm::perspective(rad(fov), aspect, 0.05f, 220.f);
}
