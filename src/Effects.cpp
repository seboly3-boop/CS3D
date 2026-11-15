#include "Effects.h"
#include "Animation.h"
#include <cstdlib>
using namespace anim;

float Effects::rnd() { return (float)std::rand() / RAND_MAX; }
static glm::vec3 randDir() {
  float a = (float)std::rand() / RAND_MAX * 6.2831853f;
  float z = (float)std::rand() / RAND_MAX * 2.f - 1.f;
  float r = std::sqrt(std::max(0.f, 1 - z * z));
  return { std::cos(a) * r, z, std::sin(a) * r };
}

void Effects::clear() { parts_.clear(); tracers_.clear(); decalCount_ = decalIdx_ = 0; flashI = 0; }

void Effects::muzzle(const glm::vec3& pos) {
  flashI = 1.f; flashPos = pos;
  Particle f; f.p = pos; f.size = 0.10f + rnd() * 0.05f;
  f.life = f.maxLife = 0.045f; f.col = { 1.f, 0.88f, 0.55f, 0.9f };
  f.type = 3; f.ang = rnd() * 6.28f;
  parts_.push_back(f);
}

void Effects::tracer(const glm::vec3& from, const glm::vec3& to) {
  tracers_.push_back({ from, to, 0.07f, 0.07f });
}

void Effects::impact(const glm::vec3& p, const glm::vec3& n, bool target) {
  int sparks = target ? 9 : 6;
  for (int i = 0; i < sparks; ++i) {
    Particle s; s.p = p + n * 0.01f;
    s.v = (randDir() * 0.8f + n * 1.6f) * (1.5f + rnd() * 2.5f);
    s.size = 0.014f + rnd() * 0.012f;
    s.life = s.maxLife = 0.22f + rnd() * 0.22f;
    s.col = target ? glm::vec4(1.f, 0.42f, 0.20f, 0.95f) : glm::vec4(1.f, 0.80f, 0.40f, 0.9f);
    s.grav = 9.f; s.drag = 2.f; s.type = 0;
    parts_.push_back(s);
  }
  for (int i = 0; i < 2; ++i) {
    Particle s; s.p = p + n * 0.03f;
    s.v = n * 0.6f + randDir() * 0.3f;
    s.size = 0.07f + rnd() * 0.05f;
    s.life = s.maxLife = 0.45f + rnd() * 0.2f;
    s.col = glm::vec4(0.55f, 0.55f, 0.60f, 0.30f);
    s.grav = -0.3f; s.drag = 3.f; s.type = 1;
    parts_.push_back(s);
  }
  if (!target) {
    Decal d; d.p = p; d.n = n; d.size = 0.045f + rnd() * 0.02f; d.ang = rnd() * 6.28f;
    decals_[decalIdx_ % 128] = d;
    decalIdx_++; decalCount_ = std::min(decalCount_ + 1, 128);
  }
}

void Effects::casing(const glm::vec3& p, const glm::vec3& right, const glm::vec3& up, const glm::vec3& fwd) {
  Particle c; c.p = p;
  c.v = right * (1.4f + rnd() * 0.8f) + up * (1.1f + rnd() * 0.6f) + fwd * 0.3f;
  c.size = 0.018f; c.life = c.maxLife = 1.4f;
  c.col = glm::vec4(0.85f, 0.62f, 0.25f, 1.f);
  c.grav = 12.f; c.drag = 0.4f; c.ang = rnd() * 6.28f; c.spin = 10.f + rnd() * 8.f; c.type = 2;
  parts_.push_back(c);
}

void Effects::magDrop(const glm::vec3& p) {
  Particle m; m.p = p;
  m.v = glm::vec3((rnd() - 0.5f) * 0.4f, -0.4f, (rnd() - 0.5f) * 0.4f);
  m.size = 0.055f; m.life = m.maxLife = 1.2f;
  m.col = glm::vec4(0.20f, 0.21f, 0.24f, 1.f);
  m.grav = 12.f; m.ang = rnd() * 6.28f; m.spin = 4.f; m.type = 4;
  parts_.push_back(m);
}

void Effects::landDust(const glm::vec3& feet, float impact) {
  int n = (int)clampf(impact * 0.6f, 2.f, 8.f);
  for (int i = 0; i < n; ++i) {
    Particle s; s.p = feet + glm::vec3(0, 0.03f, 0);
    glm::vec3 d = randDir(); d.y = std::fabs(d.y) * 0.25f;
    s.v = d * (0.8f + rnd() * 0.8f) * clampf(impact * 0.12f, 0.5f, 1.6f);
    s.size = 0.10f + rnd() * 0.06f;
    s.life = s.maxLife = 0.4f + rnd() * 0.25f;
    s.col = glm::vec4(0.5f, 0.48f, 0.46f, 0.22f);
    s.grav = -0.2f; s.drag = 3.5f; s.type = 1;
    parts_.push_back(s);
  }
}

void Effects::update(float dt) {
  flashI *= std::exp(-26.f * dt);
  for (auto& p : parts_) {
    p.v.y -= p.grav * dt;
    p.v *= std::exp(-p.drag * dt);
    p.p += p.v * dt;
    p.ang += p.spin * dt;
    if ((p.type == 2 || p.type == 4) && p.p.y < p.size * 0.5f && p.v.y < 0) {
      p.p.y = p.size * 0.5f;
      p.v.y *= -0.35f; p.v.x *= 0.6f; p.v.z *= 0.6f; p.spin *= 0.6f;
    }
    p.life -= dt;
  }
  parts_.erase(std::remove_if(parts_.begin(), parts_.end(),
               [](const Particle& p) { return p.life <= 0; }), parts_.end());
  for (auto& t : tracers_) t.life -= dt;
  tracers_.erase(std::remove_if(tracers_.begin(), tracers_.end(),
                 [](const Tracer& t) { return t.life <= 0; }), tracers_.end());
}

void Effects::drawDecals(FlatBatch& b) {
  for (int i = 0; i < decalCount_; ++i) {
    const Decal& d = decals_[i];
    glm::vec3 ref = std::fabs(d.n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 t = glm::normalize(glm::cross(d.n, ref));
    glm::vec3 u = glm::cross(d.n, t);
    float ca = std::cos(d.ang), sa = std::sin(d.ang);
    glm::vec3 t2 = (t * ca + u * sa) * d.size, u2 = (u * ca - t * sa) * d.size;
    glm::vec3 c = d.p + d.n * 0.006f;
    glm::vec4 col(0.04f, 0.04f, 0.05f, 0.85f);
    b.quad(c - t2 - u2, c + t2 - u2, c + t2 + u2, c - t2 + u2, col);
  }
}

void Effects::draw(FlatBatch& solid, FlatBatch& add,
                   const glm::vec3& camRight, const glm::vec3& camUp, const glm::vec3& camPos) {
  for (const auto& p : parts_) {
    float lf = clampf(p.life / p.maxLife, 0.f, 1.f);
    glm::vec3 r = camRight, u = camUp;
    if (p.spin != 0 || p.type == 2 || p.type == 4) {
      float ca = std::cos(p.ang), sa = std::sin(p.ang);
      r = camRight * ca + camUp * sa;
      u = camUp * ca - camRight * sa;
    }
    r *= p.size; u *= p.size;
    glm::vec4 c = p.col;
    if (p.type == 1) c.a *= lf; else c.a *= (p.type == 3 ? lf : std::min(1.f, lf * 3.f));
    auto& b = (p.type == 0 || p.type == 3) ? add : solid;
    b.quad(p.p - r - u, p.p + r - u, p.p + r + u, p.p - r + u, c);
    if (p.type == 3) { // cross flare
      glm::vec3 r2 = camRight * p.size * 1.8f, u2 = camUp * p.size * 0.35f;
      add.quad(p.p - r2 - u2, p.p + r2 - u2, p.p + r2 + u2, p.p - r2 + u2, c * 0.7f);
      glm::vec3 r3 = camRight * p.size * 0.35f, u3 = camUp * p.size * 1.8f;
      add.quad(p.p - r3 - u3, p.p + r3 - u3, p.p + r3 + u3, p.p - r3 + u3, c * 0.7f);
    }
  }
  for (const auto& t : tracers_) {
    glm::vec3 dir = t.b - t.a;
    float len = glm::length(dir);
    if (len < 1e-4f) continue;
    glm::vec3 mid = (t.a + t.b) * 0.5f;
    glm::vec3 side = glm::normalize(glm::cross(dir, glm::normalize(camPos - mid))) * 0.011f;
    float a = clampf(t.life / t.maxLife, 0.f, 1.f) * 0.8f;
    add.quad(t.a - side, t.b - side, t.b + side, t.a + side, glm::vec4(1.f, 0.75f, 0.35f, a));
  }
}
