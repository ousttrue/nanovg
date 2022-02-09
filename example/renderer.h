#pragma once
#include "nanovg_drawdata.h"
#include <memory>
#include <unordered_map>

class TextureManager;
class GLNVGshader;
class Renderer
{
  std::shared_ptr<GLNVGshader> _shader;
  unsigned int _vertBuf = {};
  unsigned int _vertArr = {};
  unsigned int _fragBuf = {};
  int _fragSize = {};

  Renderer(const std::shared_ptr<GLNVGshader> &shader);

public:
  ~Renderer();
  static std::shared_ptr<Renderer> create(bool useAntiAlias);

  void render(const NVGdrawData *data);
  int fragSize() const { return _fragSize; }

private:
  void glnvg__fill(const GLNVGcall *call, const GLNVGpath *paths, const std::shared_ptr<TextureManager> &textureManager);
  void glnvg__convexFill(const GLNVGcall *call, const GLNVGpath *paths, const std::shared_ptr<TextureManager> &textureManager);
  void glnvg__stroke(const GLNVGcall *call, const GLNVGpath *paths, const std::shared_ptr<TextureManager> &textureManager);
  void glnvg__triangles(const GLNVGcall *call, const std::shared_ptr<TextureManager> &textureManager);
  void glnvg__blendFuncSeparate(const GLNVGblend *blend);
  void glnvg__setUniforms(int uniformOffset);
};
