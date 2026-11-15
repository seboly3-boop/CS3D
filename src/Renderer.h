#pragma once
#include "GL.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct Shader {
  GLuint id = 0;
  bool create(const char* vsSrc, const char* fsSrc);
  void use() const { glUseProgram(id); }
  GLint loc(const char* name);
  void set(const char* n, int v)        { glUniform1i(loc(n), v); }
  void set(const char* n, float v)      { glUniform1f(loc(n), v); }
  void set(const char* n, const glm::vec2& v) { glUniform2f(loc(n), v.x, v.y); }
  void set(const char* n, const glm::vec3& v) { glUniform3f(loc(n), v.x, v.y, v.z); }
  void set(const char* n, const glm::vec4& v) { glUniform4f(loc(n), v.x, v.y, v.z, v.w); }
  void set(const char* n, const glm::mat4& m);
  void destroy() { if (id) { glDeleteProgram(id); id = 0; } }
private:
  std::unordered_map<std::string, GLint> cache_;
};

struct V3C { glm::vec3 p, n, c; };

struct Mesh {
  GLuint vao = 0, vbo = 0, ebo = 0;
  GLsizei count = 0;
  void draw() const;
  void destroy();
};

struct MeshBuilder {
  std::vector<V3C> v;
  std::vector<unsigned> i;
  void box(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& col,
           const glm::mat4& xf = glm::mat4(1));
  void quad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
            const glm::vec3& col, const glm::mat4& xf = glm::mat4(1));
  void cylinder(float r, float h, int seg, const glm::vec3& col,
                const glm::mat4& xf = glm::mat4(1)); // along Y, centered
  void clear() { v.clear(); i.clear(); }
  Mesh build();
};

struct FlatVert { glm::vec3 p; glm::vec4 c; };

// Unlit colored triangles — particles, tracers, HUD, sky-adjacent overlays.
struct FlatBatch {
  std::vector<FlatVert> v;
  GLuint vao = 0, vbo = 0;
  void init();
  void destroy();
  void clear() { v.clear(); }
  void vert(const glm::vec3& p, const glm::vec4& c) { v.push_back({ p, c }); }
  void quad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
            const glm::vec4& col);
  void draw(const Shader& s, const glm::mat4& proj, const glm::mat4& view,
            const glm::mat4& model, bool additive, bool depthWrite);
};

// 5x7 bitmap font + immediate-mode 2D helpers (pixel coordinates, y down).
namespace UI {
  void rect(FlatBatch& b, float x, float y, float w, float h, const glm::vec4& c);
  void line(FlatBatch& b, float x0, float y0, float x1, float y1, float w, const glm::vec4& c);
  void text(FlatBatch& b, float x, float y, float scale, const std::string& s, const glm::vec4& c);
  float textW(float scale, const std::string& s);
  void panel(FlatBatch& b, float x, float y, float w, float h, const glm::vec4& accent);
}
