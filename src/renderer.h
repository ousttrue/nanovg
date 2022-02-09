#pragma once
#include "nanovg_drawdata.h"
#include <memory>
#include <unordered_map>

class GLNVGtexture;
class GLNVGshader;
class Renderer {
  std::shared_ptr<GLNVGshader> _shader;
  std::unordered_map<int, std::shared_ptr<GLNVGtexture>> _textures;
  unsigned int _vertBuf = {};
  unsigned int _vertArr = {};
  unsigned int _fragBuf = {};
  int _fragSize = {};
  int _dummyTex = {};

  Renderer(const std::shared_ptr<GLNVGshader> &shader);

public:
  ~Renderer();
  static std::shared_ptr<Renderer> create(bool useAntiAlias);
  void registerTexture(const std::shared_ptr<GLNVGtexture> &tex);
  std::shared_ptr<GLNVGtexture> findTexture(int id);
  bool deleteTexture(int id);
  void render(const float view[2], const GLNVGcall *pCall, int callCount,
              const void *pUniform, int uniformByteSize,
              const NVGvertex *pVertex, int vertexCount,
              const GLNVGpath *pPath);
  int fragSize() const { return _fragSize; }

private:
  void glnvg__fill(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__convexFill(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__stroke(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__triangles(const GLNVGcall *call);
  void glnvg__blendFuncSeparate(const GLNVGblend *blend);
  void glnvg__setUniforms(int uniformOffset, int image);
};

unsigned int glnvg_convertBlendFuncFactor(int factor);
GLNVGblend glnvg__blendCompositeOperation(NVGcompositeOperationState op);

void RenderDrawData(struct NVGcontext *ctx, NVGdrawData *data);
