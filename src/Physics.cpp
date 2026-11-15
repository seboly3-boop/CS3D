#include "Physics.h"
#include <cmath>
#include <algorithm>

static const float EPS = 0.001f;

bool aabbOverlap(const AABB& a, const AABB& b) {
  return a.mn.x < b.mx.x - 1e-5f && a.mx.x > b.mn.x + 1e-5f &&
         a.mn.y < b.mx.y - 1e-5f && a.mx.y > b.mn.y + 1e-5f &&
         a.mn.z < b.mx.z - 1e-5f && a.mx.z > b.mn.z + 1e-5f;
}

bool rayAABB(const glm::vec3& ro, const glm::vec3& rd, const AABB& box,
             float maxT, float& tOut, glm::vec3& nOut) {
  float tmin = 0.f, tmax = maxT;
  glm::vec3 n(0);
  for (int a = 0; a < 3; ++a) {
    if (std::fabs(rd[a]) < 1e-8f) {
      if (ro[a] < box.mn[a] || ro[a] > box.mx[a]) return false;
    } else {
      float inv = 1.f / rd[a];
      float t1 = (box.mn[a] - ro[a]) * inv;
      float t2 = (box.mx[a] - ro[a]) * inv;
      float sign = -1.f;
      if (t1 > t2) { std::swap(t1, t2); sign = 1.f; }
      if (t1 > tmin) { tmin = t1; n = glm::vec3(0); n[a] = sign; }
      tmax = std::min(tmax, t2);
      if (tmin > tmax) return false;
    }
  }
  if (tmin <= 1e-5f) return false; // origin inside
  tOut = tmin; nOut = n;
  return true;
}

// Moves box along one axis and clamps against every overlapping solid.
static bool resolveAxis(AABB& box, int axis, float d, const std::vector<AABB>& solids) {
  if (std::fabs(d) < 1e-9f) return false;
  box.mn[axis] += d; box.mx[axis] += d;
  bool hit = false;
  float size = box.mx[axis] - box.mn[axis];
  for (int pass = 0; pass < 2; ++pass) {
    for (const auto& s : solids) {
      if (!aabbOverlap(box, s)) continue;
      hit = true;
      if (d > 0) { box.mx[axis] = s.mn[axis] - EPS; box.mn[axis] = box.mx[axis] - size; }
      else       { box.mn[axis] = s.mx[axis] + EPS; box.mx[axis] = box.mn[axis] + size; }
    }
  }
  return hit;
}

static bool anyOverlap(const AABB& box, const std::vector<AABB>& solids) {
  for (const auto& s : solids) if (aabbOverlap(box, s)) return true;
  return false;
}

MoveResult moveAABB(AABB& box, const glm::vec3& delta,
                    const std::vector<AABB>& solids,
                    float stepHeight, bool wasGrounded) {
  MoveResult r;

  // --- Horizontal axes with step-up attempt ---
  for (int axis : {0, 2}) {
    float d = delta[axis];
    if (std::fabs(d) < 1e-9f) continue;
    AABB before = box;
    bool hit = resolveAxis(box, axis, d, solids);
    if (axis == 0) r.hitWallX = hit; else r.hitWallZ = hit;
    if (hit && wasGrounded) {
      AABB lifted = before;
      lifted.mn.y += stepHeight; lifted.mx.y += stepHeight;
      if (!anyOverlap(lifted, solids)) {
        resolveAxis(lifted, axis, d, solids);
        AABB snapped = lifted;
        bool down = resolveAxis(snapped, 1, -(stepHeight + 0.02f), solids);
        if (down && snapped.mn.y > before.mn.y + 1e-4f &&
            snapped.mn.y <= before.mn.y + stepHeight + 1e-3f) {
          box = snapped;
          r.steppedUp = true;
          r.grounded = true;
          if (axis == 0) r.hitWallX = false; else r.hitWallZ = false;
        }
      }
    }
  }

  // --- Vertical axis (with ground snap so steps-down don't launch us) ---
  float dy = delta.y;
  if (wasGrounded && !r.steppedUp && dy <= 0.f) dy -= 0.05f;
  bool vhit = resolveAxis(box, 1, dy, solids);
  if (vhit) {
    if (dy < 0.f) r.grounded = true;
    else r.hitCeiling = true;
  }
  return r;
}

bool fitsAt(const glm::vec3& feet, float halfWidth, float height,
            const std::vector<AABB>& solids) {
  AABB b{ feet + glm::vec3(-halfWidth, 0.002f, -halfWidth),
          feet + glm::vec3( halfWidth, height,    halfWidth) };
  return !anyOverlap(b, solids);
}
