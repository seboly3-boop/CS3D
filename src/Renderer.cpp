#include "Renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cctype>

// ---------------------------------------------------------------- Shader
static GLuint compileStage(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[4096]; GLsizei n = 0;
    glGetShaderInfoLog(s, sizeof(log), &n, log);
    fprintf(stderr, "[Shader] compile error:\n%s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

bool Shader::create(const char* vsSrc, const char* fsSrc) {
  GLuint vs = compileStage(GL_VERTEX_SHADER, vsSrc);
  GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsSrc);
  if (!vs || !fs) return false;
  id = glCreateProgram();
  glAttachShader(id, vs); glAttachShader(id, fs);
  glLinkProgram(id);
  glDeleteShader(vs); glDeleteShader(fs);
  GLint ok = 0; glGetProgramiv(id, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[4096]; GLsizei n = 0;
    glGetProgramInfoLog(id, sizeof(log), &n, log);
    fprintf(stderr, "[Shader] link error:\n%s\n", log);
    glDeleteProgram(id); id = 0;
    return false;
  }
  return true;
}

GLint Shader::loc(const char* name) {
  auto it = cache_.find(name);
  if (it != cache_.end()) return it->second;
  GLint l = glGetUniformLocation(id, name);
  cache_[name] = l;
  return l;
}
void Shader::set(const char* n, const glm::mat4& m) {
  glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m));
}

// ---------------------------------------------------------------- Mesh
void Mesh::draw() const {
  if (!count) return;
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}
void Mesh::destroy() {
  if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
  if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
  if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
  count = 0;
}

void MeshBuilder::box(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& col, const glm::mat4& xf) {
  const glm::vec3 n[6] = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
  const glm::vec3 c[6][4] = {
    {{mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mx.x,mn.y,mx.z}},
    {{mn.x,mn.y,mn.z},{mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z}},
    {{mn.x,mx.y,mn.z},{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z}},
    {{mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z}},
    {{mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z}},
    {{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mn.y,mn.z}},
  };
  glm::mat3 nm(xf);
  for (int f = 0; f < 6; ++f) {
    unsigned base = (unsigned)v.size();
    glm::vec3 nn = glm::normalize(nm * n[f]);
    for (int k = 0; k < 4; ++k)
      v.push_back({ glm::vec3(xf * glm::vec4(c[f][k], 1)), nn, col });
    i.insert(i.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
  }
}

void MeshBuilder::quad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
                       const glm::vec3& col, const glm::mat4& xf) {
  glm::vec3 n = glm::normalize(glm::cross(b - a, d - a));
  glm::mat3 nm(xf);
  unsigned base = (unsigned)v.size();
  glm::vec3 nn = glm::normalize(nm * n);
  for (const auto& p : { a, b, c, d })
    v.push_back({ glm::vec3(xf * glm::vec4(p, 1)), nn, col });
  i.insert(i.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
}

void MeshBuilder::cylinder(float r, float h, int seg, const glm::vec3& col, const glm::mat4& xf) {
  glm::mat3 nm(xf);
  float y0 = -h * 0.5f, y1 = h * 0.5f;
  for (int s = 0; s < seg; ++s) {
    float a0 = (float)s / seg * 6.2831853f, a1 = (float)(s + 1) / seg * 6.2831853f;
    glm::vec3 d0(std::cos(a0), 0, std::sin(a0)), d1(std::cos(a1), 0, std::sin(a1));
    unsigned base = (unsigned)v.size();
    v.push_back({ glm::vec3(xf * glm::vec4(d0 * r + glm::vec3(0, y0, 0), 1)), glm::normalize(nm * d0), col });
    v.push_back({ glm::vec3(xf * glm::vec4(d1 * r + glm::vec3(0, y0, 0), 1)), glm::normalize(nm * d1), col });
    v.push_back({ glm::vec3(xf * glm::vec4(d1 * r + glm::vec3(0, y1, 0), 1)), glm::normalize(nm * d1), col });
    v.push_back({ glm::vec3(xf * glm::vec4(d0 * r + glm::vec3(0, y1, 0), 1)), glm::normalize(nm * d0), col });
    i.insert(i.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
  }
  for (int cap = 0; cap < 2; ++cap) {
    float y = cap ? y1 : y0;
    glm::vec3 n(0, cap ? 1.f : -1.f, 0);
    unsigned center = (unsigned)v.size();
    v.push_back({ glm::vec3(xf * glm::vec4(0, y, 0, 1)), glm::normalize(nm * n), col });
    for (int s = 0; s <= seg; ++s) {
      float a = (float)s / seg * 6.2831853f;
      v.push_back({ glm::vec3(xf * glm::vec4(std::cos(a) * r, y, std::sin(a) * r, 1)), glm::normalize(nm * n), col });
    }
    for (int s = 0; s < seg; ++s) {
      if (cap) i.insert(i.end(), { center, center + 1 + s, center + 2 + s });
      else     i.insert(i.end(), { center, center + 2 + s, center + 1 + s });
    }
  }
}

Mesh MeshBuilder::build() {
  Mesh m;
  glGenVertexArrays(1, &m.vao);
  glGenBuffers(1, &m.vbo);
  glGenBuffers(1, &m.ebo);
  glBindVertexArray(m.vao);
  glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(V3C)), v.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(i.size() * sizeof(unsigned)), i.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V3C), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V3C), (void*)12);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(V3C), (void*)24);
  glBindVertexArray(0);
  m.count = (GLsizei)i.size();
  return m;
}

// ---------------------------------------------------------------- FlatBatch
void FlatBatch::init() {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FlatVert), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FlatVert), (void*)12);
  glBindVertexArray(0);
}
void FlatBatch::destroy() {
  if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
  if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
}
void FlatBatch::quad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
                     const glm::vec4& col) {
  vert(a, col); vert(b, col); vert(c, col);
  vert(a, col); vert(c, col); vert(d, col);
}
void FlatBatch::draw(const Shader& s, const glm::mat4& proj, const glm::mat4& view,
                     const glm::mat4& model, bool additive, bool depthWrite) {
  if (v.empty()) return;
  s.use();
  s.set("uProj", proj); s.set("uView", view); s.set("uModel", model);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(FlatVert)), v.data(), GL_DYNAMIC_DRAW);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(depthWrite ? GL_TRUE : GL_FALSE);
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)v.size());
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(0);
}

// ---------------------------------------------------------------- Font (5x7)
static const unsigned char FONT[64][7] = {
  {0,0,0,0,0,0,0},        // space
  {4,4,4,4,4,0,4},        // !
  {10,10,10,0,0,0,0},     // "
  {10,10,31,10,31,10,10}, // #
  {4,15,20,14,5,30,4},    // $
  {24,25,2,4,8,19,3},     // %
  {12,18,20,8,21,18,13},  // &
  {4,4,8,0,0,0,0},        // '
  {2,4,8,8,8,4,2},        // (
  {8,4,2,2,2,4,8},        // )
  {0,4,21,14,21,4,0},     // *
  {0,4,4,31,4,4,0},       // +
  {0,0,0,0,0,4,8},        // ,
  {0,0,0,31,0,0,0},       // -
  {0,0,0,0,0,12,12},      // .
  {1,1,2,4,8,16,16},      // /
  {14,17,19,21,25,17,14}, // 0
  {4,12,4,4,4,4,14},      // 1
  {14,17,1,2,4,8,31},     // 2
  {31,2,4,2,1,17,14},     // 3
  {2,6,10,18,31,2,2},     // 4
  {31,16,30,1,1,17,14},   // 5
  {6,8,16,30,17,17,14},   // 6
  {31,1,2,4,8,8,8},       // 7
  {14,17,17,14,17,17,14}, // 8
  {14,17,17,15,1,2,12},   // 9
  {0,12,12,0,12,12,0},    // :
  {0,12,12,0,12,4,8},     // ;
  {2,4,8,16,8,4,2},       // <
  {0,0,31,0,31,0,0},      // =
  {8,4,2,1,2,4,8},        // >
  {14,17,1,2,4,0,4},      // ?
  {14,17,23,21,23,16,14}, // @
  {14,17,17,31,17,17,17}, // A
  {30,17,17,30,17,17,30}, // B
  {14,17,16,16,16,17,14}, // C
  {28,18,17,17,17,18,28}, // D
  {31,16,16,30,16,16,31}, // E
  {31,16,16,30,16,16,16}, // F
  {14,17,16,23,17,17,14}, // G
  {17,17,17,31,17,17,17}, // H
  {14,4,4,4,4,4,14},      // I
  {7,2,2,2,2,18,12},      // J
  {17,18,20,24,20,18,17}, // K
  {16,16,16,16,16,16,31}, // L
  {17,27,21,21,17,17,17}, // M
  {17,25,21,19,17,17,17}, // N
  {14,17,17,17,17,17,14}, // O
  {30,17,17,30,16,16,16}, // P
  {14,17,17,17,21,18,13}, // Q
  {30,17,17,30,20,18,17}, // R
  {15,16,16,14,1,1,30},   // S
  {31,4,4,4,4,4,4},       // T
  {17,17,17,17,17,17,14}, // U
  {17,17,17,17,17,10,4},  // V
  {17,17,17,21,21,27,17}, // W
  {17,17,10,4,10,17,17},  // X
  {17,17,10,4,4,4,4},     // Y
  {31,1,2,4,8,16,31},     // Z
  {14,8,8,8,8,8,14},      // [
  {16,16,8,4,2,1,1},      /* \ */
  {14,2,2,2,2,2,14},      // ]
  {4,10,17,0,0,0,0},      // ^
  {0,0,0,0,0,0,31},       // _
};

namespace UI {

void rect(FlatBatch& b, float x, float y, float w, float h, const glm::vec4& c) {
  b.quad({ x, y, 0 }, { x + w, y, 0 }, { x + w, y + h, 0 }, { x, y + h, 0 }, c);
}

void line(FlatBatch& b, float x0, float y0, float x1, float y1, float w, const glm::vec4& c) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 1e-4f) return;
  float px = -dy / len * w * 0.5f, py = dx / len * w * 0.5f;
  b.quad({ x0 + px, y0 + py, 0 }, { x1 + px, y1 + py, 0 },
         { x1 - px, y1 - py, 0 }, { x0 - px, y0 - py, 0 }, c);
}

void text(FlatBatch& b, float x, float y, float s, const std::string& str, const glm::vec4& c) {
  float cx = x;
  for (char ch : str) {
    unsigned char u = (unsigned char)std::toupper((unsigned char)ch);
    if (u == '\n') { cx = x; y += 9 * s; continue; }
    int idx = (u >= 32 && u <= 95) ? u - 32 : ('?' - 32);
    const unsigned char* g = FONT[idx];
    for (int r = 0; r < 7; ++r)
      for (int col = 0; col < 5; ++col)
        if (g[r] & (1 << (4 - col)))
          rect(b, cx + col * s, y + r * s, s, s, c);
    cx += 6 * s;
  }
}

float textW(float s, const std::string& str) { return (float)str.size() * 6.f * s; }

void panel(FlatBatch& b, float x, float y, float w, float h, const glm::vec4& accent) {
  rect(b, x, y, w, h, glm::vec4(0.04f, 0.05f, 0.09f, 0.74f));
  glm::vec4 e = accent; e.a = 0.55f;
  rect(b, x, y, w, 2, e); rect(b, x, y + h - 2, w, 2, e);
  rect(b, x, y, 2, h, e); rect(b, x + w - 2, y, 2, h, e);
}

} // namespace UI
