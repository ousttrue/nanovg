#include "nanovg_gl.h"
#include "nanovg.h"
#include <assert.h>

///
/// NVGparams
///
static int glnvg__renderCreate(struct NVGparams *params) {
  return params->initialize();
}

static void glnvg__renderViewport(struct NVGparams *params, float width, float height,
                                  float devicePixelRatio) {
  params->setViewSize(width, height);
}

static void glnvg__renderCancel(struct NVGparams *params) {
  params->clear();
}

static void glnvg__renderFlush(struct NVGparams *params) {
  // GLNVGcontext *gl = (GLNVGcontext *)uptr;
  // gl->render();
}

static void glnvg__renderFill(struct NVGparams *params, NVGpaint *paint,
                              NVGcompositeOperationState compositeOperation,
                              NVGscissor *scissor, float fringe,
                              const float *bounds, const NVGpath *paths,
                              int npaths) {
  params->callFill(paint, compositeOperation, scissor, fringe, bounds, paths,
               npaths);
}

static void glnvg__renderStroke(struct NVGparams *params, NVGpaint *paint,
                                NVGcompositeOperationState compositeOperation,
                                NVGscissor *scissor, float fringe,
                                float strokeWidth, const NVGpath *paths,
                                int npaths) {
  params->callStroke(paint, compositeOperation, scissor, fringe, strokeWidth, paths,
                 npaths);
}

static void glnvg__renderTriangles(
    struct NVGparams *params, NVGpaint *paint, NVGcompositeOperationState compositeOperation,
    NVGscissor *scissor, const NVGvertex *verts, int nverts, float fringe) {
  params->callTriangles(paint, compositeOperation, scissor, verts, nverts, fringe);
}

static void glnvg__renderDelete(struct NVGparams *params) {
  // auto gl = (GLNVGcontext *)uptr;
  // if (!gl)
  //   return;

  // delete gl;
}

///
/// public interafce
///
void nvgInitGL3(NVGcontext *ctx, int flags) {
  // NVGparams params;
  // memset(&params, 0, sizeof(params));
  auto &params = *nvgParams(ctx);
  params.renderCreate = glnvg__renderCreate;
  params.renderViewport = glnvg__renderViewport;
  params.renderCancel = glnvg__renderCancel;
  params.renderFlush = glnvg__renderFlush;
  params.renderFill = glnvg__renderFill;
  params.renderStroke = glnvg__renderStroke;
  params.renderTriangles = glnvg__renderTriangles;
  params.renderDelete = glnvg__renderDelete;
  params._flags = flags;
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;
}
