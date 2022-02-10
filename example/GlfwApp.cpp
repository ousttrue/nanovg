#include "GlfwApp.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdexcept>

int blowup = 0;
int screenshot = 0;
int premult = 0;

static void key(GLFWwindow *window, int key, int scancode, int action,
                int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    blowup = !blowup;
  if (key == GLFW_KEY_S && action == GLFW_PRESS)
    screenshot = 1;
  if (key == GLFW_KEY_P && action == GLFW_PRESS)
    premult = !premult;
}

void GlfwApp::errorcb(int error, const char *desc) {
  printf("GLFW error %d: %s\n", error, desc);
}

GlfwApp::GlfwApp() {
  if (!glfwInit()) {
    throw std::runtime_error("glfwInit");
  }
}
GlfwApp::~GlfwApp() { glfwTerminate(); }

bool GlfwApp::createWindow() {
  glfwSetErrorCallback(errorcb);
#ifndef _WIN32 // don't require this on win32, and works with more cards
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

#ifdef DEMO_MSAA
  glfwWindowHint(GLFW_SAMPLES, 4);
#endif
  window = glfwCreateWindow(1000, 600, "NanoVG", NULL, NULL);
  //	window = glfwCreateWindow(1000, 600, "NanoVG", glfwGetPrimaryMonitor(),
  // NULL);
  if (!window) {
    return false;
  }

  glfwSetKeyCallback(window, key);

  glfwMakeContextCurrent(window);
  gladLoadGL();
#ifdef NANOVG_GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    printf("Could not init glew.\n");
    return -1;
  }
  // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS),
  // we'll consume it here.
  glGetError();
#endif

  glfwSwapInterval(0);
  glfwSetTime(0);

  return true;
}

bool GlfwApp::beginFrame(double *mx, double *my, int *winWidth, int *winHeight,
                         int *fbWidth, int *fbHeight) {
  if (glfwWindowShouldClose(window)) {
    return false;
  }

  glfwGetCursorPos(window, mx, my);
  glfwGetWindowSize(window, winWidth, winHeight);
  glfwGetFramebufferSize(window, fbWidth, fbHeight);
  return true;
}

void GlfwApp::endFrame() {
  glfwSwapBuffers(window);
  glfwPollEvents();
}

double GlfwApp::now() { return glfwGetTime(); }