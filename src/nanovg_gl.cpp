#include "nanovg_gl.h"
#include "nanovg_gl_context.h"
#include "nanovg_gl_texture.h"
#include "renderer.h"
#include <assert.h>

///
/// NVGparams
///
static int glnvg__renderCreate(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  return gl->initialize();
}

static int glnvg__renderCreateTexture(void *uptr, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data) {
  auto gl = (GLNVGcontext *)uptr;
  auto tex = GLNVGtexture::load(w, h, type, data, imageFlags);
  if (!tex) {
    return 0;
  }
  gl->getRenderer()->registerTexture(tex);
  return tex->id();
}

static int glnvg__renderDeleteTexture(void *uptr, int image) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  return gl->getRenderer()->deleteTexture(image);
}

static int glnvg__renderUpdateTexture(void *uptr, int image, int x, int y,
                                      int w, int h, const unsigned char *data) {
  auto gl = (GLNVGcontext *)uptr;
  auto tex = gl->getRenderer()->findTexture(image);
  if (!tex)
    return 0;
  tex->update(x, y, w, h, data);
  return 1;
}

static int glnvg__renderGetTextureSize(void *uptr, int image, int *w, int *h) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  auto tex = gl->getRenderer()->findTexture(image);
  if (!tex)
    return 0;
  *w = tex->width();
  *h = tex->height();
  return 1;
}

static void glnvg__renderViewport(void *uptr, float width, float height,
                                  float devicePixelRatio) {
  NVG_NOTUSED(devicePixelRatio);
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->setViewSize(width, height);
}

static void glnvg__renderCancel(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->clear();
}

static void glnvg__renderFlush(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->render();
}

static void glnvg__renderFill(void *uptr, NVGpaint *paint,
                              NVGcompositeOperationState compositeOperation,
                              NVGscissor *scissor, float fringe,
                              const float *bounds, const NVGpath *paths,
                              int npaths) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->callFill(paint, compositeOperation, scissor, fringe, bounds, paths,
               npaths);
}

static void glnvg__renderStroke(void *uptr, NVGpaint *paint,
                                NVGcompositeOperationState compositeOperation,
                                NVGscissor *scissor, float fringe,
                                float strokeWidth, const NVGpath *paths,
                                int npaths) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->callStroke(paint, compositeOperation, scissor, fringe, strokeWidth, paths,
                 npaths);
}

static void glnvg__renderTriangles(
    void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation,
    NVGscissor *scissor, const NVGvertex *verts, int nverts, float fringe) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->callTriangles(paint, compositeOperation, scissor, verts, nverts, fringe);
}

static void glnvg__renderDelete(void *uptr) {
  auto gl = (GLNVGcontext *)uptr;
  if (!gl)
    return;

  delete gl;
}

///
/// public interafce
///
NVGcontext *nvgCreateGL3(int flags) {
  NVGparams params;
  memset(&params, 0, sizeof(params));
  params.renderCreate = glnvg__renderCreate;
  params.renderCreateTexture = glnvg__renderCreateTexture;
  params.renderDeleteTexture = glnvg__renderDeleteTexture;
  params.renderUpdateTexture = glnvg__renderUpdateTexture;
  params.renderGetTextureSize = glnvg__renderGetTextureSize;
  params.renderViewport = glnvg__renderViewport;
  params.renderCancel = glnvg__renderCancel;
  params.renderFlush = glnvg__renderFlush;
  params.renderFill = glnvg__renderFill;
  params.renderStroke = glnvg__renderStroke;
  params.renderTriangles = glnvg__renderTriangles;
  params.renderDelete = glnvg__renderDelete;
  params.userPtr = new GLNVGcontext(flags);
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

  auto ctx = nvgCreateInternal(&params);
  assert(ctx);
  return ctx;
}

void nvgDeleteGL3(NVGcontext *ctx) { nvgDeleteInternal(ctx); }

int nvglCreateImageFromHandleGL3(NVGcontext *ctx, unsigned int textureId, int w,
                                 int h, int imageFlags) {
  auto gl = (GLNVGcontext *)nvgInternalParams(ctx)->userPtr;
  auto tex = GLNVGtexture::fromHandle(textureId, w, h, imageFlags);
  if (!tex)
    return 0;
  gl->getRenderer()->registerTexture(tex);
  return tex->id();
}

unsigned int nvglImageHandleGL3(NVGcontext *ctx, int image) {
  GLNVGcontext *gl = (GLNVGcontext *)nvgInternalParams(ctx)->userPtr;
  auto tex = gl->getRenderer()->findTexture(image);
  return tex->handle();
}
