#pragma once
// AABB world physics: slab raycasts, axis-resolved movement with step-up.
#include <glm/glm.hpp>
#include <vector>

struct AABB {
  glm::vec3 mn{0}, mx{0};
};

struct RayHit {
  float t = 0;
  glm::vec3 point{0}, normal{0};
  int index = -1;
};

struct MoveResult {
  bool grounded = false;
  bool hitWallX = false, hitWallZ = false, hitCeiling = false;
  bool steppedUp = false;
};

bool rayAABB(const glm::vec3& ro, const glm::vec3& rd, const AABB& box,
             float maxT, float& tOut, glm::vec3& nOut);

bool aabbOverlap(const AABB& a, const AABB& b);

// Moves `box` by delta against solids. Resolves per axis; attempts a step-up
// (<= stepHeight) when a grounded horizontal move hits a ledge.
MoveResult moveAABB(AABB& box, const glm::vec3& delta,
                    const std::vector<AABB>& solids,
                    float stepHeight, bool wasGrounded);

// Headroom test for standing up.
bool fitsAt(const glm::vec3& feet, float halfWidth, float height,
            const std::vector<AABB>& solids);
