#pragma once
#include "call.h"
#include "nanovg.h"
#include <memory>
#include <unordered_map>
#include <vector>

class Renderer;
class GLNVGcontext {
  std::shared_ptr<Renderer> _renderer;
  float _view[2] = {};
  int _flags = {};

  // Per frame buffers
  std::vector<GLNVGcall> _calls;
  std::vector<GLNVGpath> _paths;
  NVGvertex *_verts = {};
  int _nverts = {};
  int _cverts = {};
  unsigned char *_uniforms = {};
  int _cuniforms = {};
  int _nuniforms = {};

public:
  GLNVGcontext(int flags) : _flags(flags){};
  ~GLNVGcontext();
  const std::shared_ptr<Renderer> &getRenderer() const { return _renderer; }
  void clear();
  void setViewSize(int width, int height) {
    _view[0] = width;
    _view[1] = height;
  }
  bool initialize();
  void render();
  void callFill(NVGpaint *paint, NVGcompositeOperationState compositeOperation,
                NVGscissor *scissor, float fringe, const float *bounds,
                const NVGpath *paths, int npaths);
  void callStroke(NVGpaint *paint,
                  NVGcompositeOperationState compositeOperation,
                  NVGscissor *scissor, float fringe, float strokeWidth,
                  const NVGpath *paths, int npaths);
  void callTriangles(NVGpaint *paint,
                     NVGcompositeOperationState compositeOperation,
                     NVGscissor *scissor, const NVGvertex *verts, int nverts,
                     float fringe);
private:
  int flags() const { return _flags; }
  GLNVGcall *glnvg__allocCall();
  int glnvg__allocPaths(int n);
  int glnvg__allocVerts(int n);
  int glnvg__allocFragUniforms(int n);
  GLNVGpath &get_path(size_t index) { return _paths[index]; }
  NVGvertex &get_vertex(size_t index) { return _verts[index]; }
  struct GLNVGfragUniforms *nvg__fragUniformPtr(int i) {
    return (GLNVGfragUniforms *)&_uniforms[i];
  }
  int glnvg__convertPaint(GLNVGfragUniforms *frag, NVGpaint *paint,
                          NVGscissor *scissor, float width, float fringe,
                          float strokeThr);
};
