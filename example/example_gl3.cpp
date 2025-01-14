#include "GlfwApp.h"
#include "demo.h"
#include "nanovg.h"
#include "perf.h"
#include <glad/glad.h>
#include <nanovg_impl_opengl3.h>
#include <memory>

int main()
{
  GlfwApp app;
  if (!app.createWindow())
  {
    return 1;
  }

#ifdef DEMO_MSAA
  auto flags = NVG_STENCIL_STROKES | NVG_DEBUG;
#else
  auto flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG;
#endif
  auto vg = nvgCreate(flags);
  if (!vg)
  {
    return 2;
  }

  nvg_ImplOpenGL3_Init(vg);

  {
    DemoData data(vg);
    if (!data.load())
    {
      return -1;
    }

    PerfGraph fps(GRAPH_RENDER_FPS, "Frame Time");
    PerfGraph cpuGraph(GRAPH_RENDER_MS, "CPU Time");
    PerfGraph gpuGraph(GRAPH_RENDER_MS, "GPU Time");
    GPUtimer gpuTimer;

    auto prevt = app.now();
    double mx, my;
    int winWidth, winHeight;
    int fbWidth, fbHeight;
    while (
        app.beginFrame(&mx, &my, &winWidth, &winHeight, &fbWidth, &fbHeight))
    {

      auto t = app.now();
      auto dt = t - prevt;
      prevt = t;

      float gpuTimes[3];
      gpuTimer.startGPUTimer();

      // clear
      glViewport(0, 0, fbWidth, fbHeight);
      if (premult)
        glClearColor(0, 0, 0, 0);
      else
        glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
              GL_STENCIL_BUFFER_BIT);

      auto pxRatio = (float)fbWidth / (float)winWidth;
      nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

      data.render(mx, my, winWidth, winHeight, t, blowup);

      fps.renderGraph(vg, 5, 5);
      cpuGraph.renderGraph(vg, 5 + 200 + 5, 5);
      if (gpuTimer.supported)
      {
        gpuGraph.renderGraph(vg, 5 + 200 + 5 + 200 + 5, 5);
      }

      nvg_ImplOpenGL3_RenderDrawData(nvgGetDrawData(vg));

      // Measure the CPU time taken excluding swap buffers (as the swap may wait
      // for GPU)
      auto cpuTime = app.now() - t;

      fps.updateGraph(dt);
      cpuGraph.updateGraph(cpuTime);

      // We may get multiple results.
      int n = gpuTimer.stopGPUTimer(gpuTimes, 3);
      for (int i = 0; i < n; i++)
        gpuGraph.updateGraph(gpuTimes[i]);

      if (screenshot)
      {
        screenshot = 0;
        data.saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
      }

      app.endFrame();
    }
  }
  nvg_ImplOpenGL3_Shutdown();
  nvgDelete(vg);

  return 0;
}
