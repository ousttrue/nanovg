#pragma once
#include <memory>

enum GLNVGuniformLoc {
  GLNVG_LOC_VIEWSIZE,
  GLNVG_LOC_TEX,
  GLNVG_LOC_FRAG,
  GLNVG_MAX_LOCS
};


enum GLNVGuniformBindings {
  GLNVG_FRAG_BINDING = 0,
};

class GLNVGshader {
  unsigned int prog = 0;
  unsigned int frag = 0;
  unsigned int vert = 0;
  int loc[GLNVG_MAX_LOCS];

  GLNVGshader();
public:
  ~GLNVGshader();
  static std::shared_ptr<GLNVGshader> create(bool useAntiAlias);
  void use();
  void getUniforms();
  void blockBind();
  void set_texture_and_view(int texture, const float view[2]);
};
