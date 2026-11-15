#include "Recoil.h"
using namespace anim;

RecoilProfile makeRifleProfile() {
  RecoilProfile r;
  // CS-style: strong vertical climb for ~8 shots, then a deterministic
  // left/right drift. Controllable, learnable, never random.
  const float pitch[30] = {
    0.30f,0.34f,0.38f,0.42f,0.45f,0.47f,0.48f,0.49f,
    0.42f,0.38f,0.35f,0.33f,0.32f,0.32f,0.31f,0.31f,
    0.30f,0.30f,0.30f,0.29f,0.29f,0.29f,0.28f,0.28f,
    0.28f,0.28f,0.28f,0.28f,0.28f,0.28f };
  const float yaw[30] = {
    0.00f,0.00f,0.02f,-0.03f,0.05f,0.08f,0.12f,0.16f,
    0.18f,0.15f,0.05f,-0.08f,-0.16f,-0.20f,-0.18f,-0.10f,
    0.02f,0.10f,0.16f,0.19f,0.15f,0.06f,-0.05f,-0.12f,
    -0.16f,-0.13f,-0.06f,0.03f,0.09f,0.12f };
  for (int i = 0; i < 30; ++i) r.pattern.push_back({ pitch[i], yaw[i] });
  r.recoverDelay = 0.14f; r.recoverSpeed = 7.f; r.resetTime = 0.42f;
  r.adsScale = 0.55f; r.camKick = 7.f; r.wpnBack = 1.1f; r.wpnPitch = 1.2f; r.wpnRoll = 0.5f;
  return r;
}

RecoilProfile makePistolProfile() {
  RecoilProfile r;
  const float pitch[12] = { 0.55f,0.52f,0.50f,0.48f,0.46f,0.45f,0.44f,0.44f,0.43f,0.43f,0.42f,0.42f };
  const float yaw[12]   = { 0.00f,0.06f,-0.07f,0.08f,-0.08f,0.09f,-0.09f,0.08f,-0.08f,0.07f,-0.07f,0.06f };
  for (int i = 0; i < 12; ++i) r.pattern.push_back({ pitch[i], yaw[i] });
  r.recoverDelay = 0.09f; r.recoverSpeed = 14.f; r.resetTime = 0.30f;
  r.adsScale = 0.50f; r.camKick = 9.f; r.wpnBack = 1.4f; r.wpnPitch = 1.8f; r.wpnRoll = 0.7f;
  return r;
}

void RecoilSystem::reset() {
  index_ = 0; rollSide_ = 1; sinceShot_ = 99.f;
  accum_ = target_ = glm::vec2(0);
  camKick_.reset(); back_.reset(); pitch_.reset(); roll_.reset();
}

void RecoilSystem::onShot(float adsBlend, float scale) {
  const auto& pat = profile.pattern;
  int i = std::min(index_, (int)pat.size() - 1);
  float s = ((1.f - adsBlend) + profile.adsScale * adsBlend) * scale;
  target_ += glm::vec2(pat[i].pitch, pat[i].yaw) * s;
  camKick_.impulse(profile.camKick * s);
  back_.impulse(profile.wpnBack * (0.7f + 0.3f * s));
  pitch_.impulse(profile.wpnPitch * (0.7f + 0.3f * s));
  roll_.impulse(profile.wpnRoll * rollSide_ * (0.7f + 0.3f * s));
  rollSide_ = -rollSide_;
  index_++;
  sinceShot_ = 0;
}

void RecoilSystem::update(float dt, float scale) {
  (void)scale;
  sinceShot_ += dt;
  if (sinceShot_ > profile.resetTime) index_ = 0;
  if (sinceShot_ > profile.recoverDelay)
    target_ = damp(target_, glm::vec2(0), profile.recoverSpeed, dt);
  accum_ = damp(accum_, target_, 22.f, dt);
  camKick_.update(0, 300, 24, dt);
  back_.update(0, 220, 20, dt);
  pitch_.update(0, 200, 18, dt);
  roll_.update(0, 200, 18, dt);
}
