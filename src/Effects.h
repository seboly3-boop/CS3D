#pragma once
#include "Renderer.h"
#include <vector>
#include <array>

struct Particle {
  glm::vec3 p{0}, v{0};
  float size = 0.02f, life = 0, maxLife = 1;
  glm::vec4 col{1};
  float grav = 0, drag = 0, ang = 0, spin = 0;
  int type = 0; // 0 spark(add) 1 smoke 2 casing 3 flash(add) 4 mag drop
};

struct Tracer { glm::vec3 a{0}, b{0}; float life = 0, maxLife = 1; };
struct Decal  { glm::vec3 p{0}, n{0,1,0}; float size = 0.05f, ang = 0; };

class Effects {
public:
  float flashI = 0;
  glm::vec3 flashPos{0};

  void update(float dt);
  void clear();
  void muzzle(const glm::vec3& pos);
  void tracer(const glm::vec3& from, const glm::vec3& to);
  void impact(const glm::vec3& p, const glm::vec3& n, bool target);
  void casing(const glm::vec3& p, const glm::vec3& right, const glm::vec3& up, const glm::vec3& fwd);
  void magDrop(const glm::vec3& p);
  void landDust(const glm::vec3& feet, float impact);

  void drawDecals(FlatBatch& b);
  void draw(FlatBatch& solid, FlatBatch& add,
            const glm::vec3& camRight, const glm::vec3& camUp, const glm::vec3& camPos);

private:
  std::vector<Particle> parts_;
  std::vector<Tracer> tracers_;
  std::array<Decal, 128> decals_{};
  int decalCount_ = 0, decalIdx_ = 0;
  float rnd();
};
