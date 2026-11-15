#include "GL.h"
#include <cstdio>

#define DEF(fn) PFN_##fn fn = nullptr;
DEF(glGenVertexArrays) DEF(glDeleteVertexArrays) DEF(glBindVertexArray)
DEF(glGenBuffers) DEF(glDeleteBuffers) DEF(glBindBuffer) DEF(glBufferData) DEF(glBufferSubData)
DEF(glCreateShader) DEF(glShaderSource) DEF(glCompileShader) DEF(glGetShaderiv) DEF(glGetShaderInfoLog)
DEF(glCreateProgram) DEF(glAttachShader) DEF(glLinkProgram) DEF(glGetProgramiv) DEF(glGetProgramInfoLog)
DEF(glUseProgram) DEF(glDeleteShader) DEF(glDeleteProgram) DEF(glGetUniformLocation)
DEF(glUniform1i) DEF(glUniform1f) DEF(glUniform2f) DEF(glUniform3f) DEF(glUniform4f) DEF(glUniformMatrix4fv)
DEF(glEnableVertexAttribArray) DEF(glVertexAttribPointer)
#undef DEF

bool loadGLFunctions() {
  bool ok = true;
#define LOAD(fn) do { \
    fn = reinterpret_cast<PFN_##fn>(glfwGetProcAddress(#fn)); \
    if (!fn) { fprintf(stderr, "[GL] missing entry point: %s\n", #fn); ok = false; } \
  } while (0)
  LOAD(glGenVertexArrays); LOAD(glDeleteVertexArrays); LOAD(glBindVertexArray);
  LOAD(glGenBuffers); LOAD(glDeleteBuffers); LOAD(glBindBuffer); LOAD(glBufferData); LOAD(glBufferSubData);
  LOAD(glCreateShader); LOAD(glShaderSource); LOAD(glCompileShader); LOAD(glGetShaderiv); LOAD(glGetShaderInfoLog);
  LOAD(glCreateProgram); LOAD(glAttachShader); LOAD(glLinkProgram); LOAD(glGetProgramiv); LOAD(glGetProgramInfoLog);
  LOAD(glUseProgram); LOAD(glDeleteShader); LOAD(glDeleteProgram); LOAD(glGetUniformLocation);
  LOAD(glUniform1i); LOAD(glUniform1f); LOAD(glUniform2f); LOAD(glUniform3f); LOAD(glUniform4f); LOAD(glUniformMatrix4fv);
  LOAD(glEnableVertexAttribArray); LOAD(glVertexAttribPointer);
#undef LOAD
  return ok;
}
