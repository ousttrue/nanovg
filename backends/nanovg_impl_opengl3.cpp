#include "nanovg_impl_opengl3.h"
#include "renderer.h"
#include "texture_manager.h"
#include <nanovg.h>

std::shared_ptr<Renderer> g_renderer;

static int glnvg__renderCreateTexture(struct NVGparams *params, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data)
{
  if (!g_renderer)
  {
    return 0;
  }
  auto tex = GLNVGtexture::load(w, h, type, data, imageFlags);
  if (!tex)
  {
    return 0;
  }
  g_renderer->textureManager()->registerTexture(tex);
  return tex->id();
}

static int glnvg__renderDeleteTexture(struct NVGparams *params, int image)
{
  if (!g_renderer)
  {
    return 0;
  }
  return g_renderer->textureManager()->deleteTexture(image);
}

static int glnvg__renderUpdateTexture(struct NVGparams *params, int image, int x, int y,
                                      int w, int h, const unsigned char *data)
{
  if (!g_renderer)
  {
    return 0;
  }
  auto tex = g_renderer->textureManager()->findTexture(image);
  if (!tex)
    return 0;
  tex->update(x, y, w, h, data);
  return 1;
}

static NVGtextureInfo *glnvg__renderGetTexture(struct NVGparams *params, int image)
{
  if (!g_renderer)
  {
    return 0;
  }
  auto tex = g_renderer->textureManager()->findTexture(image);
  if (!tex)
    return 0;
  return (NVGtextureInfo *)tex.get();
}

bool nvg_ImplOpenGL3_Init(struct NVGcontext *vg)
{
  auto params = nvgParams(vg);
  g_renderer = Renderer::create(params->_flags & NVG_ANTIALIAS);
  params->renderCreateTexture = glnvg__renderCreateTexture;
  params->renderDeleteTexture = glnvg__renderDeleteTexture;
  params->renderUpdateTexture = glnvg__renderUpdateTexture;
  params->renderGetTexture = glnvg__renderGetTexture;
  return true;
}

void nvg_ImplOpenGL3_Shutdown()
{
  g_renderer = nullptr;
}

void nvg_ImplOpenGL3_RenderDrawData(struct NVGdrawData *draw_data)
{
  g_renderer->render(draw_data);
}
