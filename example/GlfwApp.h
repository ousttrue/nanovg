#pragma once

extern int blowup;
extern int screenshot;
extern int premult;

class GlfwApp {
  struct GLFWwindow *window;

  static void errorcb(int error, const char *desc);

public:
  GlfwApp();
  ~GlfwApp();
  bool createWindow();
  bool beginFrame(double *mx, double *my, int *winWidth, int *winHeight,
                  int *fbWidth, int *fbHeight);
  void endFrame();
  double now();
};
