#pragma once
#include "Renderer.h"
#include "Physics.h"
#include <vector>

struct Target {
  glm::vec3 base{0};
  float moveAmp = 0, moveSpeed = 0, movePhase = 0, xOff = 0;
  int hp = 100;
  float flash = 0;
  int state = 0;              // 0 alive, 1 dying, 2 waiting, 3 rising
  float t = 0, fall = 0, rise = 1;
  glm::vec3 pos() const { return base + glm::vec3(xOff, 0, 0); }
};

struct TargetHit {
  Target* target = nullptr;
  int part = 0;               // 0 legs, 1 body, 2 head
  float t = 0;
  glm::vec3 point{0}, normal{0};
};

class World {
public:
  std::vector<AABB> solids;
  std::vector<Target> targets;
  float time = 0;

  void build();
  void destroy();
  void update(float dt);
  void resetTargets();
  void resetAll();

  // Common lighting/fog uniforms must already be set on the shader.
  void draw(Shader& s, const glm::mat4& proj, const glm::mat4& view);
  void appendHealthBars(FlatBatch& b, const glm::vec3& camRight, const glm::vec3& camUp);

  bool raycast(const glm::vec3& ro, const glm::vec3& rd, float maxT, RayHit& out) const;
  bool raycastTargets(const glm::vec3& ro, const glm::vec3& rd, float maxT, TargetHit& out);
  // Returns true if this damage killed the target.
  bool damage(Target* t, int part, float dmg);
  int aliveCount() const;

private:
  Mesh floor_, solids_, marks_, glow_, target_;
  void floorQuad(MeshBuilder& b, float x0, float x1, float z0, float z1, float y, const glm::vec3& col);
};
