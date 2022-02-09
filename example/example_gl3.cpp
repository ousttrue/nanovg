#include "GlfwApp.h"
#include "demo.h"
#include "nanovg.h"
#include "nanovg_gl.h"
#include "perf.h"
#include <glad/glad.h>
#include <stdio.h>

int main() {
  DemoData data;
  NVGcontext *vg = nullptr;
  GPUtimer gpuTimer;
  PerfGraph fps, cpuGraph, gpuGraph;

  initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");
  initGraph(&cpuGraph, GRAPH_RENDER_MS, "CPU Time");
  initGraph(&gpuGraph, GRAPH_RENDER_MS, "GPU Time");

  GlfwApp app;
  if (!app.createWindow()) {
    return 1;
  }

#ifdef DEMO_MSAA
  vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
#else
  vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
  if (vg == nullptr) {
    // printf("Could not init nanovg.\n");
    return -1;
  }

  if (loadDemoData(vg, &data) == -1)
    return -1;

  initGPUTimer(&gpuTimer);

  auto prevt = app.now();
  double mx, my;
  int winWidth, winHeight;
  int fbWidth, fbHeight;
  while (app.beginFrame(&mx, &my, &winWidth, &winHeight, &fbWidth, &fbHeight)) {

    auto t = app.now();
    auto dt = t - prevt;
    prevt = t;

    float gpuTimes[3];
    startGPUTimer(&gpuTimer);

    // clear
    glViewport(0, 0, fbWidth, fbHeight);
    if (premult)
      glClearColor(0, 0, 0, 0);
    else
      glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    auto pxRatio = (float)fbWidth / (float)winWidth;
    nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

    renderDemo(vg, mx, my, winWidth, winHeight, t, blowup, &data);

    renderGraph(vg, 5, 5, &fps);
    renderGraph(vg, 5 + 200 + 5, 5, &cpuGraph);
    if (gpuTimer.supported)
      renderGraph(vg, 5 + 200 + 5 + 200 + 5, 5, &gpuGraph);

    nvgEndFrame(vg);

    // Measure the CPU time taken excluding swap buffers (as the swap may wait
    // for GPU)
    auto cpuTime = app.now() - t;

    updateGraph(&fps, dt);
    updateGraph(&cpuGraph, cpuTime);

    // We may get multiple results.
    int n = stopGPUTimer(&gpuTimer, gpuTimes, 3);
    for (int i = 0; i < n; i++)
      updateGraph(&gpuGraph, gpuTimes[i]);

    if (screenshot) {
      screenshot = 0;
      saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
    }

    app.endFrame();
  }

  freeDemoData(vg, &data);

  nvgDeleteGL3(vg);

  printf("Average Frame Time: %.2f ms\n", getGraphAverage(&fps) * 1000.0f);
  printf("          CPU Time: %.2f ms\n", getGraphAverage(&cpuGraph) * 1000.0f);
  printf("          GPU Time: %.2f ms\n", getGraphAverage(&gpuGraph) * 1000.0f);

  return 0;
}
