#pragma once
#include "Animation.h"
#include <vector>

struct RecoilShot { float pitch, yaw; }; // degrees per shot, deterministic

struct RecoilProfile {
  std::vector<RecoilShot> pattern;
  float recoverDelay = 0.14f;  // s after last shot before recovery starts
  float recoverSpeed = 7.f;    // 1/s decay of accumulated offset
  float resetTime    = 0.42f;  // s of no fire -> pattern index resets
  float adsScale     = 0.55f;  // recoil multiplier while fully aimed
  float camKick      = 7.f;    // spring impulse (deg/s) — visual snap
  float wpnBack      = 1.1f;   // weapon kickback impulse (m/s)
  float wpnPitch     = 1.2f;   // weapon muzzle-up impulse (rad/s)
  float wpnRoll      = 0.5f;   // alternating roll impulse (rad/s)
};

RecoilProfile makeRifleProfile();
RecoilProfile makePistolProfile();

class RecoilSystem {
public:
  RecoilProfile profile;
  void init(const RecoilProfile& p) { profile = p; reset(); }
  void reset();
  void onShot(float adsBlend, float scale);
  void update(float dt, float scale);

  float camPitchDeg() const { return accum_.x; }
  float camYawDeg()   const { return accum_.y; }
  float kickDeg()     const { return camKick_.x; }
  float wpnBack()     const { return back_.x; }
  float wpnPitch()    const { return pitch_.x; }
  float wpnRoll()     const { return roll_.x; }
  int   patternIndex() const { return index_; }

private:
  int index_ = 0;
  int rollSide_ = 1;
  float sinceShot_ = 99.f;
  glm::vec2 accum_{0}, target_{0};
  anim::Spring camKick_, back_, pitch_, roll_;
};
