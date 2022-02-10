#pragma once
#include <nanovg.h>
#include <memory>
#include <unordered_map>

class TextureManager;
class GLNVGshader;
class Renderer {
  std::shared_ptr<TextureManager> _texture;
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
  const std::shared_ptr<TextureManager> &textureManager() const {
    return _texture;
  }

  int nvglCreateImageFromHandleGL3(unsigned int textureId, int w, int h,
                                   int flags);
  unsigned int nvglImageHandleGL3(int image);

private:
  void glnvg__fill(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__convexFill(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__stroke(const GLNVGcall *call, const GLNVGpath *paths);
  void glnvg__triangles(const GLNVGcall *call);
  void glnvg__blendFuncSeparate(const struct GLNVGblend *blend);
  void glnvg__setUniforms(int uniformOffset);
};
