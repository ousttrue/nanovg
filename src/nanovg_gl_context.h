#pragma once
#include "nanovg.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

struct GLNVGblend {
  unsigned int srcRGB;
  unsigned int dstRGB;
  unsigned int srcAlpha;
  unsigned int dstAlpha;
};

struct GLNVGcall {
  int type;
  int image;
  int pathOffset;
  int pathCount;
  int triangleOffset;
  int triangleCount;
  int uniformOffset;
  GLNVGblend blendFunc;
};

struct GLNVGpath {
  int fillOffset;
  int fillCount;
  int strokeOffset;
  int strokeCount;
};

class GLNVGcontext {
  std::shared_ptr<class GLNVGshader> _shader;
  std::unordered_map<int, std::shared_ptr<class GLNVGtexture>> _textures;

  float _view[2] = {};
  int _fragSize = {};

  unsigned int _vertBuf = {};
  unsigned int _vertArr = {};
  unsigned int _fragBuf = {};

  int _flags = {};
  int _dummyTex = {};

  // Per frame buffers
  std::list<GLNVGcall> _calls;
  std::vector<GLNVGpath> _paths;
  NVGvertex *_verts = {};
  int _nverts = {};
  int _cverts = {};
  unsigned char *_uniforms = {};
  int _cuniforms = {};
  int _nuniforms = {};

  // cached state
  unsigned int _boundTexture = {};
  unsigned int _stencilMask = {};
  unsigned int _stencilFunc = {};
  int _stencilFuncRef = {};
  unsigned int _stencilFuncMask = {};
  GLNVGblend _blendFunc = {};

public:
  GLNVGcontext(int flags) : _flags(flags){};
  ~GLNVGcontext();
  int flags() const { return _flags; }
  void clear();
  void glnvg__checkError(const char *str);
  void set_viewsize(int width, int height) {
    _view[0] = width;
    _view[1] = height;
  }
  bool initialize();
  void register_texture(const std::shared_ptr<GLNVGtexture> &tex);
  std::shared_ptr<GLNVGtexture> glnvg__findTexture(int id);
  GLNVGcall *glnvg__allocCall();
  int glnvg__allocPaths(int n);
  int glnvg__allocVerts(int n);
  int glnvg__allocFragUniforms(int n);
  GLNVGpath &get_path(size_t index) { return _paths[index]; }
  NVGvertex &get_vertex(size_t index) { return _verts[index]; }
  struct GLNVGfragUniforms *nvg__fragUniformPtr(int i) {
    return (GLNVGfragUniforms *)&_uniforms[i];
  }
  bool glnvg__deleteTexture(int id);
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
  int glnvg__convertPaint(GLNVGfragUniforms *frag, NVGpaint *paint,
                          NVGscissor *scissor, float width, float fringe,
                          float strokeThr);
  void glnvg__blendFuncSeparate(const GLNVGblend *blend);
  void glnvg__fill(GLNVGcall *call);
  void glnvg__convexFill(GLNVGcall *call);
  void glnvg__stroke(GLNVGcall *call);
  void glnvg__triangles(GLNVGcall *call);
  void glnvg__setUniforms(int uniformOffset, int image);
  void glnvg__bindTexture(unsigned int tex);
  void glnvg__stencilMask(unsigned int mask);
  void glnvg__stencilFunc(unsigned int func, int ref, unsigned int mask);
};
