#include "Player.h"
using namespace anim;

AABB Player::box() const {
  float h = p.standHeight + (p.crouchHeight - p.standHeight) * smoother01(crouchT);
  return { pos + glm::vec3(-p.halfWidth, 0, -p.halfWidth),
           pos + glm::vec3( p.halfWidth, h,  p.halfWidth) };
}

float Player::eyeHeight() const {
  return p.eyeStand + (p.eyeCrouch - p.eyeStand) * smoother01(crouchT);
}

void Player::teleport(const glm::vec3& feet) {
  pos = feet; vel = glm::vec3(0);
  grounded = true; sprintBlend = 0; crouchT = 0; crouchToggle_ = false;
  stepDist_ = 0; lastGroundT_ = lastJumpT_ = -99.f; jumpBufferT_ = 0;
}

void Player::accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt, float cap) {
  float current = glm::dot(vel, wishDir);
  float target = cap > 0 ? std::min(wishSpeed, cap) : wishSpeed;
  float add = target - current;
  if (add <= 0) return;
  float accelSpeed = std::min(accel * wishSpeed * dt, add);
  vel += wishDir * accelSpeed;
}

void Player::update(Input& in, float dt, float camYaw, const std::vector<AABB>& solids,
                    bool wantAds, float time) {
  ev = PlayerEvents{};
  if (dt <= 0) return;

  // ---------------- wish direction (normalized diagonals) ----------------
  float f = (in.down(GLFW_KEY_W) ? 1.f : 0.f) - (in.down(GLFW_KEY_S) ? 1.f : 0.f);
  float s = (in.down(GLFW_KEY_D) ? 1.f : 0.f) - (in.down(GLFW_KEY_A) ? 1.f : 0.f);
  glm::vec3 fwdH(-std::sin(camYaw), 0, -std::cos(camYaw));
  glm::vec3 rightH(std::cos(camYaw), 0, -std::sin(camYaw));
  glm::vec3 wish = fwdH * f + rightH * s;
  float wishLen = glm::length(wish);
  if (wishLen > 1e-4f) wish /= wishLen;
  bool hasInput = wishLen > 1e-4f;

  // ---------------- crouch (hold Ctrl or toggle C, headroom check) --------
  if (in.pressed(GLFW_KEY_C)) crouchToggle_ = !crouchToggle_;
  bool wantCrouch = in.down(GLFW_KEY_LEFT_CONTROL) || crouchToggle_;
  if (!wantCrouch && crouchT > 0.01f) {
    if (!fitsAt(pos, p.halfWidth, p.standHeight, solids)) wantCrouch = true; // blocked ceiling
  }
  crouchT = moveToward(crouchT, wantCrouch ? 1.f : 0.f, dt / std::max(0.03f, p.crouchTime));
  bool crouching = crouchT > 0.5f;

  // ---------------- sprint (forward-only, cancels for ADS/crouch) ---------
  sprinting = in.down(GLFW_KEY_LEFT_SHIFT) && f > 0.1f && !wantCrouch && !wantAds && hasInput;
  sprintBlend = damp(sprintBlend, sprinting ? 1.f : 0.f, 7.f, dt);
  float targetMax = crouching ? p.crouchSpeed
                  : p.walkSpeed + (p.sprintSpeed - p.walkSpeed) * sprintBlend;
  maxSpeedCur = damp(maxSpeedCur, targetMax, 12.f, dt);

  // ---------------- jump: buffer + coyote + hold-to-bhop -------------------
  if (in.pressed(GLFW_KEY_SPACE)) jumpBufferT_ = 0.12f;
  jumpBufferT_ = std::max(0.f, jumpBufferT_ - dt);
  bool coyote = (time - lastGroundT_) < 0.08f;
  bool wantJump = jumpBufferT_ > 0 || in.down(GLFW_KEY_SPACE);
  bool doJump = wantJump && (grounded || coyote) && (time - lastJumpT_) > 0.20f;
  if (doJump) {
    vel.y = p.jumpSpeed;
    grounded = false;
    lastJumpT_ = time; lastGroundT_ = -99.f; jumpBufferT_ = 0;
    ev.jumped = true;
  }

  // ---------------- friction (grounded only; preserves excess momentum) ---
  float speed = horizSpeed();
  if (grounded && !doJump) {
    float control = hasInput ? std::max(speed - maxSpeedCur * 1.02f, 0.f) : speed;
    if (control > 0.001f) {
      float base = hasInput ? control : std::max(speed, p.stopSpeed);
      float drop = base * p.friction * dt;
      float newSpeed = std::max(0.f, speed - drop);
      if (speed > 1e-5f) { vel.x *= newSpeed / speed; vel.z *= newSpeed / speed; }
    }
  }

  // ---------------- acceleration -------------------------------------------
  if (hasInput) {
    if (grounded && !doJump) accelerate(wish, maxSpeedCur, p.groundAccel, dt, 0);
    else                     accelerate(wish, maxSpeedCur, p.airAccel, dt, p.airCap);
  }

  // ---------------- gravity -------------------------------------------------
  if (!grounded || doJump) vel.y -= p.gravity * dt;
  vel.y = std::max(vel.y, -32.f);

  // ---------------- integrate + collide (substepped sweep) ------------------
  wasGrounded_ = grounded;
  vyBefore_ = vel.y;
  AABB b = box();
  int steps = std::max(1, (int)std::ceil(dt / 0.008f));
  float h = dt / (float)steps;
  bool anyGround = false, ceilHit = false, wallX = false, wallZ = false;
  for (int i = 0; i < steps; ++i) {
    MoveResult r = moveAABB(b, vel * h, solids, p.stepHeight, grounded);
    grounded = r.grounded;
    anyGround |= r.grounded;
    ceilHit |= r.hitCeiling;
    wallX |= r.hitWallX; wallZ |= r.hitWallZ;
    if (r.grounded && vel.y < 0) vel.y = 0;
    if (r.hitCeiling && vel.y > 0) vel.y = 0;
  }
  if (wallX) vel.x = 0;
  if (wallZ) vel.z = 0;
  pos = glm::vec3((b.mn.x + b.mx.x) * 0.5f, b.mn.y, (b.mn.z + b.mx.z) * 0.5f);
  if (grounded) lastGroundT_ = time;

  // ---------------- events ---------------------------------------------------
  if (!wasGrounded_ && grounded && vyBefore_ < -2.f) {
    ev.landed = true;
    ev.landImpact = -vyBefore_;
  }
  float hs = horizSpeed();
  if (grounded && hs > 0.6f) {
    stepDist_ += hs * dt;
    float stride = crouching ? 1.5f : (sprintBlend > 0.5f ? 2.7f : 2.1f);
    if (stepDist_ >= stride) {
      stepDist_ = 0;
      ev.footstep = true;
      ev.stepIntensity = clampf(hs / p.sprintSpeed, 0.25f, 1.f) * (crouching ? 0.45f : 1.f);
    }
  } else if (!grounded) stepDist_ = 0;
}

const char* Player::stateName() const {
  if (!grounded) return vel.y > 0.5f ? "JUMP" : "FALL";
  if (crouchT > 0.5f) return horizSpeed() > 0.4f ? "CROUCH WALK" : "CROUCH";
  if (sprintBlend > 0.5f && horizSpeed() > p.walkSpeed * 1.05f) return "SPRINT";
  return horizSpeed() > 0.4f ? "WALK" : "IDLE";
}
