#pragma once
#include "nanovg.h"
#include <memory>

struct GLNVGfragUniforms
{
  float scissorMat[12]; // matrices are actually 3 vec4s
  float paintMat[12];
  struct NVGcolor innerCol;
  struct NVGcolor outerCol;
  float scissorExt[2];
  float scissorScale[2];
  float extent[2];
  float radius;
  float feather;
  float strokeMult;
  float strokeThr;
  int texType;
  int type;
};

enum GLNVGcallType
{
  GLNVG_NONE = 0,
  GLNVG_FILL,
  GLNVG_CONVEXFILL,
  GLNVG_STROKE,
  GLNVG_TRIANGLES,
};

struct GLNVGblend
{
  unsigned int srcRGB;
  unsigned int dstRGB;
  unsigned int srcAlpha;
  unsigned int dstAlpha;
};

struct GLNVGcall
{
  int type;
  int image;
  int pathOffset;
  int pathCount;
  int triangleOffset;
  int triangleCount;
  int uniformOffset;
  struct GLNVGblend blendFunc;
};

struct GLNVGpath
{
  int fillOffset;
  int fillCount;
  int strokeOffset;
  int strokeCount;
};

struct NVGdrawData
{
  float view[2];
  GLNVGcall *drawData;
  size_t drawCount;
  void *pUniform;
  int uniformByteSize;
  NVGvertex *pVertex;
  int vertexCount;
  GLNVGpath *pPath;
  std::shared_ptr<class TextureManager> textureManager;
};
