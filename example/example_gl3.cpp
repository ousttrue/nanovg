#include "GlfwApp.h"
#include "demo.h"
#include "nanovg.h"
#include "nanovg_gl.h"
#include "perf.h"
#include "renderer.h"
#include "texture_manager.h"
#include <glad/glad.h>
#include <memory>

std::shared_ptr<Renderer> g_renderer;

static int glnvg__renderCreateTexture(struct NVGparams *params, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data) {
  // auto gl = (GLNVGcontext *)uptr;
  auto tex = GLNVGtexture::load(w, h, type, data, imageFlags);
  if (!tex) {
    return 0;
  }
  g_renderer->textureManager()->registerTexture(tex);
  return tex->id();
}

static int glnvg__renderDeleteTexture(struct NVGparams *params, int image) {
  // GLNVGcontext *gl = (GLNVGcontext *)uptr;
  return g_renderer->textureManager()->deleteTexture(image);
}

static int glnvg__renderUpdateTexture(struct NVGparams *params, int image, int x, int y,
                                      int w, int h, const unsigned char *data) {
  // auto gl = (GLNVGcontext *)uptr;
  auto tex = g_renderer->textureManager()->findTexture(image);
  if (!tex)
    return 0;
  tex->update(x, y, w, h, data);
  return 1;
}

static NVGtextureInfo *glnvg__renderGetTexture(struct NVGparams *params, int image) {
  // GLNVGcontext *gl = (GLNVGcontext *)uptr;
  auto tex = g_renderer->textureManager()->findTexture(image);
  if (!tex)
    return 0;
  return (NVGtextureInfo*)tex.get();
}

int main()
{
  GlfwApp app;
  if (!app.createWindow())
  {
    return 1;
  }

  auto vg = nvgCreate();
  if (!vg)
  {
    return 2;
  }

#ifdef DEMO_MSAA
  auto flags = NVG_STENCIL_STROKES | NVG_DEBUG;
#else
  auto flags = NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG;
#endif
  nvgInitGL3(vg, flags);
  auto params = nvgParams(vg);
  params->renderCreateTexture = glnvg__renderCreateTexture;
  params->renderDeleteTexture = glnvg__renderDeleteTexture;
  params->renderUpdateTexture = glnvg__renderUpdateTexture;
  params->renderGetTexture = glnvg__renderGetTexture;

  {
    g_renderer = Renderer::create(flags & NVG_ANTIALIAS);

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

      g_renderer->render(nvgGetDrawData(vg));

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

  nvgDelete(vg);

  return 0;
}
