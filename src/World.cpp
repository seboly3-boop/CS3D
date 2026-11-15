#include "World.h"
#include "Animation.h"
#include <glm/gtc/matrix_transform.hpp>
using namespace anim;

void World::floorQuad(MeshBuilder& b, float x0, float x1, float z0, float z1, float y, const glm::vec3& col) {
  b.quad({ x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { x0, y, z0 }, col);
}

void World::build() {
  solids.clear(); targets.clear(); time = 0;
  MeshBuilder sb, mb, gb, fb;

  auto solid = [&](const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& col) {
    solids.push_back({ mn, mx });
    sb.box(mn, mx, col);
    sb.quad({ mn.x, mx.y + 0.001f, mx.z }, { mx.x, mx.y + 0.001f, mx.z },
            { mx.x, mx.y + 0.001f, mn.z }, { mn.x, mx.y + 0.001f, mn.z }, col * 1.28f);
  };

  const glm::vec3 asphalt(0.20f, 0.20f, 0.225f);
  const glm::vec3 wall(0.30f, 0.32f, 0.38f);
  const glm::vec3 olive(0.38f, 0.40f, 0.26f);
  const glm::vec3 rust(0.55f, 0.30f, 0.18f);
  const glm::vec3 concrete(0.42f, 0.42f, 0.45f);
  const glm::vec3 amber(0.95f, 0.62f, 0.16f);
  const glm::vec3 white(0.85f, 0.85f, 0.88f);

  // Floor (also a collision solid + raycast target).
  solids.push_back({ {-16, -1, -38}, {16, 0, 12} });
  fb.quad({ -16, 0, 12 }, { 16, 0, 12 }, { 16, 0, -38 }, { -16, 0, -38 }, asphalt);

  // Perimeter walls.
  solid({ -16, 0, -38 }, { 16, 4, -37 }, wall);
  solid({ -16, 0, 11 },  { 16, 4, 12 },  wall);
  solid({ -16, 0, -38 }, { -15, 4, 12 }, wall);
  solid({ 15, 0, -38 },  { 16, 4, 12 },  wall);

  // Stair climb to a 2 m platform (step-up test), back-right.
  for (int i = 0; i < 5; ++i)
    solid({ 6, 0, 4 + i * 0.6f }, { 9, 0.4f * (i + 1), 4.6f + i * 0.6f }, concrete);
  solid({ 6, 0, 7.6f }, { 12, 2.0f, 11 }, concrete);

  // Crouch tunnel (1.3 m clearance), back-left.
  solid({ -9, 0, 5 },   { -8, 2.3f, 6.5f }, wall);
  solid({ -5, 0, 5 },   { -4, 2.3f, 6.5f }, wall);
  solid({ -9, 1.3f, 5 },{ -4, 2.3f, 6.5f }, wall);

  // Jump crates + bhop pads, center-back.
  solid({ 0.0f, 0, 6.0f },  { 1.0f, 0.5f, 7.0f },  olive);
  solid({ 1.2f, 0, 6.4f },  { 2.2f, 1.0f, 7.4f },  rust);
  solid({ -1.5f, 0, 7.0f }, { -0.5f, 1.0f, 8.0f }, olive);
  solid({ 2.8f, 0, 7.5f },  { 3.8f, 0.5f, 8.5f },  rust);
  solid({ -3.4f, 0, 9.2f }, { -2.4f, 1.0f, 10.2f },olive);
  for (int i = 0; i < 5; ++i)
    solid({ -3.f + i * 1.5f, 0, 2.2f }, { -2.1f + i * 1.5f, 0.3f, 3.1f }, concrete);

  // Range cover.
  solid({ -7, 0, -14 },   { -5.5f, 1.2f, -12.5f }, olive);
  solid({ 5.5f, 0, -18 }, { 7, 1.2f, -16.5f },     rust);
  solid({ -6, 0, -24 },   { -4.5f, 1.0f, -22.5f }, olive);
  solid({ 6, 0, -27 },    { 7.5f, 1.4f, -25.5f },  rust);

  // Pillars + floodlight poles.
  for (float z : { -8.f, -22.f }) {
    solid({ -13.4f, 0, z - 0.4f }, { -12.6f, 4, z + 0.4f }, wall);
    solid({ 12.6f, 0, z - 0.4f },  { 13.4f, 4, z + 0.4f },  wall);
  }
  for (float x : { -12.f, 12.f }) {
    solid({ x - 0.15f, 0, -33.15f }, { x + 0.15f, 5, -32.85f }, wall);
    gb.box({ x - 0.35f, 5, -33.35f }, { x + 0.35f, 5.18f, -32.65f }, glm::vec3(1.0f, 0.8f, 0.45f));
  }

  // Emissive trim strips along wall tops.
  gb.box({ -15, 3.85f, -37.12f }, { 15, 3.95f, -37.0f }, amber);
  gb.box({ -15.12f, 3.85f, -37 }, { -15.0f, 3.95f, 11 }, amber * 0.8f);
  gb.box({ 15.0f, 3.85f, -37 },   { 15.12f, 3.95f, 11 },  amber * 0.8f);

  // Painted markings: firing line, lane lines, spawn pad, distance posts.
  floorQuad(mb, -8, 8, -0.06f, 0.06f, 0.012f, amber);
  for (float z : { -10.f, -20.f, -30.f })
    floorQuad(mb, -10, 10, z - 0.04f, z + 0.04f, 0.012f, white * 0.7f);
  floorQuad(mb, -1, 1, 7.5f, 7.56f, 0.012f, white);
  floorQuad(mb, -1, 1, 9.44f, 9.5f, 0.012f, white);
  floorQuad(mb, -1, -0.94f, 7.5f, 9.5f, 0.012f, white);
  floorQuad(mb, 0.94f, 1, 7.5f, 9.5f, 0.012f, white);
  for (int d = 1; d <= 3; ++d)
    for (int k = 0; k < d; ++k)
      gb.box({ 10.4f, 0.02f + k * 0.22f, -10.f * d - 0.09f },
             { 10.6f, 0.18f + k * 0.22f, -10.f * d + 0.09f }, amber);

  // Targets: static ranks + two strafing movers.
  auto addTarget = [&](float x, float z, float amp, float spd, float phase) {
    Target t; t.base = { x, 0, z };
    t.moveAmp = amp; t.moveSpeed = spd; t.movePhase = phase;
    targets.push_back(t);
  };
  addTarget(-4, -10, 0, 0, 0); addTarget(0, -10, 0, 0, 0); addTarget(4, -10, 0, 0, 0);
  addTarget(0, -15, 3, 0.7f, 0);
  addTarget(-2, -20, 0, 0, 0); addTarget(2, -20, 0, 0, 0);
  addTarget(0, -25, 4, 0.5f, 2.f);
  addTarget(0, -30, 0, 0, 0);

  // Target dummy mesh (base at y=0 so the death fall pivots at the feet).
  MeshBuilder tb;
  const glm::vec3 orange(0.90f, 0.36f, 0.12f);
  tb.cylinder(0.30f, 0.05f, 14, glm::vec3(0.16f, 0.16f, 0.18f),
              glm::translate(glm::mat4(1), glm::vec3(0, 0.025f, 0)));
  tb.box({ -0.16f, 0.05f, -0.10f }, { 0.16f, 0.62f, 0.10f }, glm::vec3(0.25f, 0.25f, 0.28f)); // legs
  tb.box({ -0.24f, 0.62f, -0.13f }, { 0.24f, 1.42f, 0.13f }, orange);                          // body
  tb.box({ -0.11f, 1.46f, -0.11f }, { 0.11f, 1.70f, 0.11f }, orange * 0.8f);                  // head
  // Bullseye (faces +z, toward the firing line).
  tb.quad({ -0.10f, 0.90f, 0.131f }, { 0.10f, 0.90f, 0.131f },
          { 0.10f, 1.10f, 0.131f },  { -0.10f, 1.10f, 0.131f }, white);
  tb.quad({ -0.06f, 0.94f, 0.132f }, { 0.06f, 0.94f, 0.132f },
          { 0.06f, 1.06f, 0.132f },  { -0.06f, 1.06f, 0.132f }, orange);
  tb.quad({ -0.02f, 0.98f, 0.133f }, { 0.02f, 0.98f, 0.133f },
          { 0.02f, 1.02f, 0.133f },  { -0.02f, 1.02f, 0.133f }, white);
  tb.quad({ -0.055f, 1.52f, 0.111f }, { 0.055f, 1.52f, 0.111f },
          { 0.055f, 1.64f, 0.111f },  { -0.055f, 1.64f, 0.111f }, white * 0.9f);

  floor_ = fb.build(); solids_ = sb.build(); marks_ = mb.build();
  glow_ = gb.build(); target_ = tb.build();
}

void World::destroy() {
  floor_.destroy(); solids_.destroy(); marks_.destroy(); glow_.destroy(); target_.destroy();
}

void World::update(float dt) {
  time += dt;
  for (auto& t : targets) {
    t.flash *= std::exp(-10.f * dt);
    if (t.moveAmp > 0 && t.state == 0)
      t.xOff = std::sin(time * t.moveSpeed + t.movePhase) * t.moveAmp;
    switch (t.state) {
      case 1: // dying: tip over, then sink
        t.t += dt;
        t.fall = easeOutCubic(std::min(t.t / 0.55f, 1.f)) * 1.50f;
        if (t.t > 1.4f) { t.state = 2; t.t = 0; }
        break;
      case 2:
        t.t += dt;
        if (t.t > 2.0f) { t.state = 3; t.t = 0; t.hp = 100; t.fall = 0; t.rise = 0.01f; }
        break;
      case 3:
        t.t += dt;
        t.rise = std::max(0.01f, easeOutBack(std::min(t.t / 0.4f, 1.f)));
        if (t.t > 0.4f) { t.state = 0; t.rise = 1; }
        break;
      default: break;
    }
  }
}

void World::resetTargets() {
  for (auto& t : targets) { t.hp = 100; t.state = 0; t.t = 0; t.fall = 0; t.rise = 1; t.flash = 0; }
}
void World::resetAll() { time = 0; resetTargets(); }

void World::draw(Shader& s, const glm::mat4& proj, const glm::mat4& view) {
  s.use();
  s.set("uProj", proj); s.set("uView", view);
  glm::mat4 I(1);
  s.set("uModel", I); s.set("uTint", glm::vec4(1)); s.set("uEmissive", 0.f);

  s.set("uGrid", 1.f); floor_.draw();
  s.set("uGrid", 0.f); solids_.draw();

  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(-2.f, -2.f);
  s.set("uEmissive", 0.55f); marks_.draw();
  glDisable(GL_POLYGON_OFFSET_FILL);

  s.set("uEmissive", 1.f); glow_.draw();
  s.set("uEmissive", 0.f);

  for (const auto& t : targets) {
    if (t.state == 2) continue;
    glm::mat4 m = glm::translate(glm::mat4(1), t.pos());
    m = glm::rotate(m, -t.fall, glm::vec3(1, 0, 0));
    m = glm::scale(m, glm::vec3(t.rise));
    s.set("uModel", m);
    s.set("uTint", glm::vec4(1 + t.flash * 1.6f, 1 + t.flash * 0.4f, 1 + t.flash * 0.4f, 1));
    target_.draw();
  }
  s.set("uModel", I); s.set("uTint", glm::vec4(1));
}

void World::appendHealthBars(FlatBatch& b, const glm::vec3& camRight, const glm::vec3& camUp) {
  for (const auto& t : targets) {
    if (t.state != 0 || t.hp >= 100) continue;
    glm::vec3 c = t.pos() + glm::vec3(0, 1.92f, 0);
    float w = 0.55f, h = 0.065f;
    glm::vec3 r = camRight * (w * 0.5f), u = camUp * (h * 0.5f);
    b.quad(c - r - u, c + r - u, c + r + u, c - r + u, glm::vec4(0.03f, 0.03f, 0.05f, 0.8f));
    float f = t.hp / 100.f;
    glm::vec3 r2 = camRight * (w * 0.5f * f);
    glm::vec3 off = camRight * (w * 0.5f * (1.f - f));
    glm::vec4 col(f > 0.5f ? glm::vec4(0.45f, 1.f, 0.55f, 0.95f)
                           : glm::vec4(1.f, 0.45f, 0.25f, 0.95f));
    b.quad(c - r2 - u * 0.7f - off, c + r2 - u * 0.7f - off,
           c + r2 + u * 0.7f - off, c - r2 + u * 0.7f - off, col);
  }
}

bool World::raycast(const glm::vec3& ro, const glm::vec3& rd, float maxT, RayHit& out) const {
  bool hit = false; float best = maxT;
  for (size_t i = 0; i < solids.size(); ++i) {
    float t; glm::vec3 n;
    if (rayAABB(ro, rd, solids[i], best, t, n) && t < best) {
      best = t; hit = true;
      out = { t, ro + rd * t, n, (int)i };
    }
  }
  return hit;
}

static AABB partBox(const glm::vec3& p, int part) {
  switch (part) {
    case 0: return { p + glm::vec3(-0.16f, 0.05f, -0.10f), p + glm::vec3(0.16f, 0.62f, 0.10f) };
    case 1: return { p + glm::vec3(-0.24f, 0.62f, -0.13f), p + glm::vec3(0.24f, 1.42f, 0.13f) };
    default:return { p + glm::vec3(-0.11f, 1.46f, -0.11f), p + glm::vec3(0.11f, 1.70f, 0.11f) };
  }
}

bool World::raycastTargets(const glm::vec3& ro, const glm::vec3& rd, float maxT, TargetHit& out) {
  bool hit = false; float best = maxT;
  for (auto& t : targets) {
    if (t.state != 0) continue;
    glm::vec3 p = t.pos();
    for (int part = 0; part < 3; ++part) {
      float tt; glm::vec3 n;
      if (rayAABB(ro, rd, partBox(p, part), best, tt, n) && tt < best) {
        best = tt; hit = true;
        out = { &t, part, tt, ro + rd * tt, n };
      }
    }
  }
  return hit;
}

bool World::damage(Target* t, int part, float dmg) {
  if (!t || t->state != 0) return false;
  float mult = part == 2 ? 4.f : (part == 0 ? 0.75f : 1.f);
  t->hp -= (int)std::lround(dmg * mult);
  t->flash = 1.f;
  if (t->hp <= 0) { t->state = 1; t->t = 0; return true; }
  return false;
}

int World::aliveCount() const {
  int n = 0; for (const auto& t : targets) if (t.state == 0) ++n;
  return n;
}
