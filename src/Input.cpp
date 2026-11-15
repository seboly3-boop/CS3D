#include "Input.h"

Input* Input::inst = nullptr;

void Input::init(GLFWwindow* w) {
  win_ = w; inst = this;
  glfwSetKeyCallback(w, keyCB);
  glfwSetMouseButtonCallback(w, mouseCB);
  glfwSetCursorPosCallback(w, cursorCB);
  glfwSetScrollCallback(w, scrollCB);
  glfwSetWindowFocusCallback(w, focusCB);
  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(w, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

void Input::endFrame() {
  for (int i = 0; i <= GLFW_KEY_LAST; ++i) prev_[i] = cur_[i];
  for (int i = 0; i < 8; ++i) mbPrev_[i] = mb_[i];
  delta_ = glm::vec2(0);
  wheel_ = 0;
}

bool Input::down(int k) const     { return k >= 0 && k <= GLFW_KEY_LAST && cur_[k]; }
bool Input::pressed(int k) const  { return k >= 0 && k <= GLFW_KEY_LAST && cur_[k] && !prev_[k]; }
bool Input::released(int k) const { return k >= 0 && k <= GLFW_KEY_LAST && !cur_[k] && prev_[k]; }
bool Input::mouseDown(int b) const    { return b >= 0 && b < 8 && mb_[b]; }
bool Input::mousePressed(int b) const { return b >= 0 && b < 8 && mb_[b] && !mbPrev_[b]; }

void Input::setLocked(bool locked) {
  locked_ = locked;
  glfwSetInputMode(win_, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  haveLast_ = false;
  delta_ = glm::vec2(0);
}

void Input::keyCB(GLFWwindow*, int key, int, int action, int) {
  if (key < 0 || key > GLFW_KEY_LAST) return;
  if (action == GLFW_PRESS) Input::inst->cur_[key] = true;
  else if (action == GLFW_RELEASE) Input::inst->cur_[key] = false;
}
void Input::mouseCB(GLFWwindow*, int btn, int action, int) {
  if (btn < 0 || btn >= 8) return;
  Input::inst->mb_[btn] = (action == GLFW_PRESS);
}
void Input::cursorCB(GLFWwindow*, double x, double y) {
  Input* in = Input::inst;
  if (!in->locked_) { in->haveLast_ = false; return; }
  if (in->haveLast_) in->delta_ += glm::vec2((float)(x - in->lastX_), (float)(y - in->lastY_));
  in->lastX_ = x; in->lastY_ = y; in->haveLast_ = true;
}
void Input::scrollCB(GLFWwindow*, double, double dy) { Input::inst->wheel_ += (float)dy; }
void Input::focusCB(GLFWwindow*, int focused) { if (!focused) Input::inst->lostFocus = true; }
