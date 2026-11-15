#pragma once
#include "Animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraParams {
  float sensitivity = 1.0f;    // multiplier
  float smoothing   = 0.30f;   // 0 = razor, 1 = floaty
  float baseFov     = 75.f;    // vertical degrees
  float sprintFovAdd = 7.f;
  float bobAmount   = 1.0f;
};

class Camera {
public:
  CameraParams p;

  // Consumed each frame by look(); smoothed in update().
  void look(const glm::vec2& mouseDelta, float adsBlend, float adsFov);
  void update(float dt, const glm::vec3& baseEye, float hSpeed, float speedNorm,
              bool grounded, float crouchBlend, float sprintBlend,
              float adsBlend, float adsFov, float strafe,
              float recoilPitchDeg, float recoilYawDeg, float kickDeg, float time);
  void onLand(float impact);
  void onJump();
  void resetAngles(float yaw, float pitch);

  glm::mat4 view() const;
  glm::mat4 proj(float aspect) const;
  glm::vec3 aimDir() const { return fwd; }

  float yaw = 0, pitch = 0;
  float fov = 75.f;
  glm::vec3 eye{0}, fwd{0,0,-1}, right{1,0,0}, up{0,1,0};

private:
  float tYaw_ = 0, tPitch_ = 0;
  float bobPhase_ = 0, bobW_ = 0, rollCur_ = 0;
  anim::Spring landSpr_, jumpSpr_;
};
