#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
public:
  void init(GLFWwindow* w);
  void endFrame();                 // latch previous state, clear deltas
  bool down(int key) const;
  bool pressed(int key) const;
  bool released(int key) const;
  bool mouseDown(int btn) const;
  bool mousePressed(int btn) const;
  glm::vec2 mouseDelta() const { return delta_; }
  float wheel() const { return wheel_; }
  void setLocked(bool locked);
  bool locked() const { return locked_; }
  bool lostFocus = false;          // set by focus callback, consumed by game

  static Input* inst;
private:
  static void keyCB(GLFWwindow*, int, int, int, int);
  static void mouseCB(GLFWwindow*, int, int, int);
  static void cursorCB(GLFWwindow*, double, double);
  static void scrollCB(GLFWwindow*, double, double);
  static void focusCB(GLFWwindow*, int);

  GLFWwindow* win_ = nullptr;
  bool cur_[GLFW_KEY_LAST + 1] = {}, prev_[GLFW_KEY_LAST + 1] = {};
  bool mb_[8] = {}, mbPrev_[8] = {};
  glm::vec2 delta_{0};
  float wheel_ = 0;
  double lastX_ = 0, lastY_ = 0;
  bool haveLast_ = false;
  bool locked_ = false;
};
