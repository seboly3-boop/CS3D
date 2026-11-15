#pragma once
// Minimal OpenGL 3.3 core loader built on glfwGetProcAddress.
// GL 1.1 entry points come from the system <GL/gl.h> via GLFW.
#include <GLFW/glfw3.h>
#include <cstddef>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#ifndef APIENTRY
#define APIENTRY
#endif

#define GL_FRAGMENT_SHADER        0x8B30
#define GL_VERTEX_SHADER          0x8B31
#define GL_COMPILE_STATUS         0x8B81
#define GL_LINK_STATUS            0x8B82
#define GL_ARRAY_BUFFER           0x8892
#define GL_ELEMENT_ARRAY_BUFFER   0x8893
#define GL_STATIC_DRAW            0x88E4
#define GL_DYNAMIC_DRAW           0x88E8
#define GL_STREAM_DRAW            0x88E0
#define GL_MULTISAMPLE            0x809D
#define GL_CLAMP_TO_EDGE          0x812F

typedef void (APIENTRY *PFN_glGenVertexArrays)(GLsizei, GLuint*);
typedef void (APIENTRY *PFN_glDeleteVertexArrays)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFN_glBindVertexArray)(GLuint);
typedef void (APIENTRY *PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void (APIENTRY *PFN_glDeleteBuffers)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFN_glBindBuffer)(GLenum, GLuint);
typedef void (APIENTRY *PFN_glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFN_glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const void*);
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFN_glUseProgram)(GLuint);
typedef void (APIENTRY *PFN_glDeleteShader)(GLuint);
typedef void (APIENTRY *PFN_glDeleteProgram)(GLuint);
typedef GLint (APIENTRY *PFN_glGetUniformLocation)(GLuint, const GLchar*);
typedef void (APIENTRY *PFN_glUniform1i)(GLint, GLint);
typedef void (APIENTRY *PFN_glUniform1f)(GLint, GLfloat);
typedef void (APIENTRY *PFN_glUniform2f)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);
typedef void (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

extern PFN_glGenVertexArrays        glGenVertexArrays;
extern PFN_glDeleteVertexArrays     glDeleteVertexArrays;
extern PFN_glBindVertexArray        glBindVertexArray;
extern PFN_glGenBuffers             glGenBuffers;
extern PFN_glDeleteBuffers          glDeleteBuffers;
extern PFN_glBindBuffer             glBindBuffer;
extern PFN_glBufferData             glBufferData;
extern PFN_glBufferSubData          glBufferSubData;
extern PFN_glCreateShader           glCreateShader;
extern PFN_glShaderSource           glShaderSource;
extern PFN_glCompileShader          glCompileShader;
extern PFN_glGetShaderiv            glGetShaderiv;
extern PFN_glGetShaderInfoLog       glGetShaderInfoLog;
extern PFN_glCreateProgram          glCreateProgram;
extern PFN_glAttachShader           glAttachShader;
extern PFN_glLinkProgram            glLinkProgram;
extern PFN_glGetProgramiv           glGetProgramiv;
extern PFN_glGetProgramInfoLog      glGetProgramInfoLog;
extern PFN_glUseProgram             glUseProgram;
extern PFN_glDeleteShader           glDeleteShader;
extern PFN_glDeleteProgram          glDeleteProgram;
extern PFN_glGetUniformLocation     glGetUniformLocation;
extern PFN_glUniform1i              glUniform1i;
extern PFN_glUniform1f              glUniform1f;
extern PFN_glUniform2f              glUniform2f;
extern PFN_glUniform3f              glUniform3f;
extern PFN_glUniform4f              glUniform4f;
extern PFN_glUniformMatrix4fv       glUniformMatrix4fv;
extern PFN_glEnableVertexAttribArray glEnableVertexAttribArray;
extern PFN_glVertexAttribPointer    glVertexAttribPointer;

bool loadGLFunctions();
