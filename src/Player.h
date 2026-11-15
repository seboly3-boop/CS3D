#pragma once
#include "Physics.h"
#include "Input.h"
#include "Animation.h"
#include <glm/glm.hpp>
#include <vector>

struct MoveParams {
  float walkSpeed   = 5.2f;
  float sprintSpeed = 7.4f;
  float crouchSpeed = 2.5f;
  float groundAccel = 10.f;   // 1/s (Quake-style accelerate)
  float airAccel    = 12.f;
  float airCap      = 0.8f;   // m/s gain cap per frame while airborne
  float friction    = 6.5f;
  float stopSpeed   = 1.5f;
  float gravity     = 20.f;
  float jumpSpeed   = 6.6f;
  float stepHeight  = 0.45f;
  float standHeight = 1.8f;
  float crouchHeight= 1.25f;
  float eyeStand    = 1.64f;
  float eyeCrouch   = 1.06f;
  float crouchTime  = 0.18f;
  float halfWidth   = 0.30f;
};

struct PlayerEvents {
  bool jumped = false;
  bool landed = false;
  float landImpact = 0;
  bool footstep = false;
  float stepIntensity = 0;
};

class Player {
public:
  MoveParams p;
  PlayerEvents ev;

  glm::vec3 pos{0, 0, 8.5f};   // feet center
  glm::vec3 vel{0};
  bool grounded = true;
  bool sprinting = false;
  float sprintBlend = 0, crouchT = 0;
  float maxSpeedCur = 5.2f;

  void update(Input& in, float dt, float camYaw, const std::vector<AABB>& solids,
              bool wantAds, float time);
  void teleport(const glm::vec3& feet);

  float horizSpeed() const { return std::sqrt(vel.x * vel.x + vel.z * vel.z); }
  float eyeHeight() const;
  glm::vec3 eyeBase() const { return pos + glm::vec3(0, eyeHeight(), 0); }
  const char* stateName() const;
  AABB box() const;

private:
  void accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt, float cap);
  bool crouchToggle_ = false;
  float lastGroundT_ = -99.f, lastJumpT_ = -99.f, jumpBufferT_ = 0;
  float stepDist_ = 0;
  float vyBefore_ = 0;
  bool wasGrounded_ = true;
};
