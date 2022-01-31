#include "nanovg_gl.h"
#include "nanovg.h"
#include "nanovg_gl_shader.h"
#include <glad/glad.h>
#include <math.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

static int glnvg__maxi(int a, int b) { return a > b ? a : b; }

class GLNVGtexture {
  int _id = {};
  GLuint _handle = {};
  int _width = {};
  int _height = {};
  int _type = {};
  int _flags = {};

  GLNVGtexture() {
    static int s_textureId = 0;
    _id = ++s_textureId;
  }

public:
  ~GLNVGtexture() {
    if (_handle != 0 && (_flags & NVG_IMAGE_NODELETE) == 0) {
      glDeleteTextures(1, &_handle);
    }
  }

  static std::shared_ptr<GLNVGtexture> load(int w, int h, int type,
                                            const void *data, int imageFlags) {
    auto tex = std::shared_ptr<GLNVGtexture>(new GLNVGtexture);
    glGenTextures(1, &tex->_handle);
    tex->bind();
    tex->_width = w;
    tex->_height = h;
    tex->_type = type;
    tex->_flags = imageFlags;
    glBindTexture(GL_TEXTURE_2D, tex->_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->_width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    if (type == NVG_TEXTURE_RGBA)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, data);
    else
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE,
                   data);

    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS) {
      if (imageFlags & NVG_IMAGE_NEAREST) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST_MIPMAP_NEAREST);
      } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
      }
    } else {
      if (imageFlags & NVG_IMAGE_NEAREST) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      }
    }

    if (imageFlags & NVG_IMAGE_NEAREST) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    if (imageFlags & NVG_IMAGE_REPEATX)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    else
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    if (imageFlags & NVG_IMAGE_REPEATY)
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    else
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    // The new way to build mipmaps on GLES and GL3
    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS) {
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    tex->unbind();
    return tex;
  }

  static std::shared_ptr<GLNVGtexture> fromHandle(GLuint textureId, int w,
                                                  int h, int imageFlags) {
    auto tex = std::shared_ptr<GLNVGtexture>(new GLNVGtexture);
    tex->_type = NVG_TEXTURE_RGBA;
    tex->_handle = textureId;
    tex->_flags = imageFlags;
    tex->_width = w;
    tex->_height = h;
    return tex;
  }

  void update(int x, int y, int w, int h, const void *data) {
    bind();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, _width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, y);

    if (_type == NVG_TEXTURE_RGBA)
      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                      data);
    else
      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE,
                      data);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    unbind();
  }
  GLuint handle() const { return _handle; }
  void bind() { glBindTexture(GL_TEXTURE_2D, _handle); }
  void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }
  int id() const { return _id; }
  int width() const { return _width; }
  int height() const { return _height; }
  int flags() const { return _flags; }
  int type() const { return _type; }
};

struct GLNVGblend {
  GLenum srcRGB;
  GLenum dstRGB;
  GLenum srcAlpha;
  GLenum dstAlpha;
};

enum GLNVGcallType {
  GLNVG_NONE = 0,
  GLNVG_FILL,
  GLNVG_CONVEXFILL,
  GLNVG_STROKE,
  GLNVG_TRIANGLES,
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

struct GLNVGfragUniforms {
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

struct GLNVGcontext {
  GLNVGshader shader = {};
  std::unordered_map<int, std::shared_ptr<GLNVGtexture>> textures;
  float view[2] = {};
  GLuint vertBuf = {};
  GLuint vertArr = {};
  GLuint fragBuf = {};
  int fragSize = {};
  int flags = {};

  // Per frame buffers
  GLNVGcall *calls = {};
  int ccalls = {};
  int ncalls = {};
  GLNVGpath *paths = {};
  int cpaths = {};
  int npaths = {};
  struct NVGvertex *verts = {};
  int cverts = {};
  int nverts = {};
  unsigned char *uniforms = {};
  int cuniforms = {};
  int nuniforms = {};

public:
  // cached state
  GLuint boundTexture;
  GLuint stencilMask;
  GLenum stencilFunc;
  GLint stencilFuncRef;
  GLuint stencilFuncMask;
  GLNVGblend blendFunc;

  int dummyTex;

public:
  std::shared_ptr<GLNVGtexture> glnvg__findTexture(int id) {
    auto found = textures.find(id);
    if (found != textures.end()) {
      return found->second;
    }
    return {};
  }

  bool glnvg__deleteTexture(int id) {
    auto found = textures.find(id);
    if (found != textures.end()) {
      textures.erase(found);
      return true;
    }
    return false;
  }
};

static void glnvg__bindTexture(GLNVGcontext *gl, GLuint tex) {
  if (gl->boundTexture != tex) {
    gl->boundTexture = tex;
    glBindTexture(GL_TEXTURE_2D, tex);
  }
}

static void glnvg__stencilMask(GLNVGcontext *gl, GLuint mask) {
  if (gl->stencilMask != mask) {
    gl->stencilMask = mask;
    glStencilMask(mask);
  }
}

static void glnvg__stencilFunc(GLNVGcontext *gl, GLenum func, GLint ref,
                               GLuint mask) {
  if ((gl->stencilFunc != func) || (gl->stencilFuncRef != ref) ||
      (gl->stencilFuncMask != mask)) {

    gl->stencilFunc = func;
    gl->stencilFuncRef = ref;
    gl->stencilFuncMask = mask;
    glStencilFunc(func, ref, mask);
  }
}
static void glnvg__blendFuncSeparate(GLNVGcontext *gl,
                                     const GLNVGblend *blend) {
  if ((gl->blendFunc.srcRGB != blend->srcRGB) ||
      (gl->blendFunc.dstRGB != blend->dstRGB) ||
      (gl->blendFunc.srcAlpha != blend->srcAlpha) ||
      (gl->blendFunc.dstAlpha != blend->dstAlpha)) {

    gl->blendFunc = *blend;
    glBlendFuncSeparate(blend->srcRGB, blend->dstRGB, blend->srcAlpha,
                        blend->dstAlpha);
  }
}

static void glnvg__checkError(GLNVGcontext *gl, const char *str) {
  GLenum err;
  if ((gl->flags & NVG_DEBUG) == 0)
    return;
  err = glGetError();
  if (err != GL_NO_ERROR) {
    printf("Error %08x after %s\n", err, str);
    return;
  }
}

static int glnvg__renderCreateTexture(void *uptr, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data);

static int glnvg__renderCreate(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  int align = 4;

  glnvg__checkError(gl, "init");

  if (!gl->shader.createShader(gl->flags & NVG_ANTIALIAS)) {
    return 0;
  }

  glnvg__checkError(gl, "uniform locations");
  gl->shader.getUniforms();

  // Create dynamic vertex array
  glGenVertexArrays(1, &gl->vertArr);
  glGenBuffers(1, &gl->vertBuf);

  // Create UBOs
  gl->shader.blockBind();
  glGenBuffers(1, &gl->fragBuf);
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
  gl->fragSize =
      sizeof(GLNVGfragUniforms) + align - sizeof(GLNVGfragUniforms) % align;

  // Some platforms does not allow to have samples to unset textures.
  // Create empty one which is bound when there's no texture specified.
  gl->dummyTex =
      glnvg__renderCreateTexture(gl, NVG_TEXTURE_ALPHA, 1, 1, 0, NULL);

  glnvg__checkError(gl, "create done");

  glFinish();

  return 1;
}

static int glnvg__renderCreateTexture(void *uptr, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data) {
  auto gl = (GLNVGcontext *)uptr;
  auto tex = GLNVGtexture::load(w, h, type, data, imageFlags);
  if (!tex) {
    return 0;
  }
  gl->textures.insert(std::make_pair(tex->id(), tex));
  return tex->id();
}

static int glnvg__renderDeleteTexture(void *uptr, int image) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  return gl->glnvg__deleteTexture(image);
}

static int glnvg__renderUpdateTexture(void *uptr, int image, int x, int y,
                                      int w, int h, const unsigned char *data) {
  auto gl = (GLNVGcontext *)uptr;
  auto tex = gl->glnvg__findTexture(image);
  if (!tex)
    return 0;

  tex->update(x, y, w, h, data);
  return 1;
}

static int glnvg__renderGetTextureSize(void *uptr, int image, int *w, int *h) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  auto tex = gl->glnvg__findTexture(image);
  if (tex == NULL)
    return 0;
  *w = tex->width();
  *h = tex->height();
  return 1;
}

static void glnvg__xformToMat3x4(float *m3, float *t) {
  m3[0] = t[0];
  m3[1] = t[1];
  m3[2] = 0.0f;
  m3[3] = 0.0f;
  m3[4] = t[2];
  m3[5] = t[3];
  m3[6] = 0.0f;
  m3[7] = 0.0f;
  m3[8] = t[4];
  m3[9] = t[5];
  m3[10] = 1.0f;
  m3[11] = 0.0f;
}

static NVGcolor glnvg__premulColor(NVGcolor c) {
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return c;
}

static int glnvg__convertPaint(GLNVGcontext *gl, GLNVGfragUniforms *frag,
                               NVGpaint *paint, NVGscissor *scissor,
                               float width, float fringe, float strokeThr) {
  std::shared_ptr<GLNVGtexture> tex = NULL;
  float invxform[6];

  memset(frag, 0, sizeof(*frag));

  frag->innerCol = glnvg__premulColor(paint->innerColor);
  frag->outerCol = glnvg__premulColor(paint->outerColor);

  if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
    memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
    frag->scissorExt[0] = 1.0f;
    frag->scissorExt[1] = 1.0f;
    frag->scissorScale[0] = 1.0f;
    frag->scissorScale[1] = 1.0f;
  } else {
    nvgTransformInverse(invxform, scissor->xform);
    glnvg__xformToMat3x4(frag->scissorMat, invxform);
    frag->scissorExt[0] = scissor->extent[0];
    frag->scissorExt[1] = scissor->extent[1];
    frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] +
                                  scissor->xform[2] * scissor->xform[2]) /
                            fringe;
    frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] +
                                  scissor->xform[3] * scissor->xform[3]) /
                            fringe;
  }

  memcpy(frag->extent, paint->extent, sizeof(frag->extent));
  frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
  frag->strokeThr = strokeThr;

  if (paint->image != 0) {
    tex = gl->glnvg__findTexture(paint->image);
    if (tex == NULL)
      return 0;
    if ((tex->flags() & NVG_IMAGE_FLIPY) != 0) {
      float m1[6], m2[6];
      nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
      nvgTransformMultiply(m1, paint->xform);
      nvgTransformScale(m2, 1.0f, -1.0f);
      nvgTransformMultiply(m2, m1);
      nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
      nvgTransformMultiply(m1, m2);
      nvgTransformInverse(invxform, m1);
    } else {
      nvgTransformInverse(invxform, paint->xform);
    }
    frag->type = NSVG_SHADER_FILLIMG;

    if (tex->type() == NVG_TEXTURE_RGBA)
      frag->texType = (tex->flags() & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1;
    else
      frag->texType = 2;
    //		printf("frag->texType = %d\n", frag->texType);
  } else {
    frag->type = NSVG_SHADER_FILLGRAD;
    frag->radius = paint->radius;
    frag->feather = paint->feather;
    nvgTransformInverse(invxform, paint->xform);
  }

  glnvg__xformToMat3x4(frag->paintMat, invxform);

  return 1;
}

static GLNVGfragUniforms *nvg__fragUniformPtr(GLNVGcontext *gl, int i);

static void glnvg__setUniforms(GLNVGcontext *gl, int uniformOffset, int image) {
  std::shared_ptr<GLNVGtexture> tex = NULL;
  glBindBufferRange(GL_UNIFORM_BUFFER, GLNVG_FRAG_BINDING, gl->fragBuf,
                    uniformOffset, sizeof(GLNVGfragUniforms));

  if (image != 0) {
    tex = gl->glnvg__findTexture(image);
  }
  // If no image is set, use empty texture
  if (tex == NULL) {
    tex = gl->glnvg__findTexture(gl->dummyTex);
  }
  glnvg__bindTexture(gl, tex ? tex->handle() : 0);
  glnvg__checkError(gl, "tex paint tex");
}

static void glnvg__renderViewport(void *uptr, float width, float height,
                                  float devicePixelRatio) {
  NVG_NOTUSED(devicePixelRatio);
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->view[0] = width;
  gl->view[1] = height;
}

static void glnvg__fill(GLNVGcontext *gl, GLNVGcall *call) {
  GLNVGpath *paths = &gl->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  // Draw shapes
  glEnable(GL_STENCIL_TEST);
  glnvg__stencilMask(gl, 0xff);
  glnvg__stencilFunc(gl, GL_ALWAYS, 0, 0xff);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // set bindpoint for solid loc
  glnvg__setUniforms(gl, call->uniformOffset, 0);
  glnvg__checkError(gl, "fill simple");

  glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
  glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
  glDisable(GL_CULL_FACE);
  for (i = 0; i < npaths; i++)
    glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
  glEnable(GL_CULL_FACE);

  // Draw anti-aliased pixels
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
  glnvg__checkError(gl, "fill fill");

  if (gl->flags & NVG_ANTIALIAS) {
    glnvg__stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    // Draw fringes
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
  }

  // Draw fill
  glnvg__stencilFunc(gl, GL_NOTEQUAL, 0x0, 0xff);
  glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
  glDrawArrays(GL_TRIANGLE_STRIP, call->triangleOffset, call->triangleCount);

  glDisable(GL_STENCIL_TEST);
}

static void glnvg__convexFill(GLNVGcontext *gl, GLNVGcall *call) {
  GLNVGpath *paths = &gl->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  glnvg__setUniforms(gl, call->uniformOffset, call->image);
  glnvg__checkError(gl, "convex fill");

  for (i = 0; i < npaths; i++) {
    glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
    // Draw fringes
    if (paths[i].strokeCount > 0) {
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
    }
  }
}

static void glnvg__stroke(GLNVGcontext *gl, GLNVGcall *call) {
  GLNVGpath *paths = &gl->paths[call->pathOffset];
  int npaths = call->pathCount, i;

  if (gl->flags & NVG_STENCIL_STROKES) {

    glEnable(GL_STENCIL_TEST);
    glnvg__stencilMask(gl, 0xff);

    // Fill the stroke base without overlap
    glnvg__stencilFunc(gl, GL_EQUAL, 0x0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
    glnvg__checkError(gl, "stroke fill 0");
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);

    // Draw anti-aliased pixels.
    glnvg__setUniforms(gl, call->uniformOffset, call->image);
    glnvg__stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);

    // Clear stencil buffer.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glnvg__stencilFunc(gl, GL_ALWAYS, 0x0, 0xff);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glnvg__checkError(gl, "stroke fill 1");
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);

    //		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl,
    // call->uniformOffset
    //+ gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);

  } else {
    glnvg__setUniforms(gl, call->uniformOffset, call->image);
    glnvg__checkError(gl, "stroke fill");
    // Draw Strokes
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
  }
}

static void glnvg__triangles(GLNVGcontext *gl, GLNVGcall *call) {
  glnvg__setUniforms(gl, call->uniformOffset, call->image);
  glnvg__checkError(gl, "triangles fill");

  glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void glnvg__renderCancel(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  gl->nverts = 0;
  gl->npaths = 0;
  gl->ncalls = 0;
  gl->nuniforms = 0;
}

static GLenum glnvg_convertBlendFuncFactor(int factor) {
  if (factor == NVG_ZERO)
    return GL_ZERO;
  if (factor == NVG_ONE)
    return GL_ONE;
  if (factor == NVG_SRC_COLOR)
    return GL_SRC_COLOR;
  if (factor == NVG_ONE_MINUS_SRC_COLOR)
    return GL_ONE_MINUS_SRC_COLOR;
  if (factor == NVG_DST_COLOR)
    return GL_DST_COLOR;
  if (factor == NVG_ONE_MINUS_DST_COLOR)
    return GL_ONE_MINUS_DST_COLOR;
  if (factor == NVG_SRC_ALPHA)
    return GL_SRC_ALPHA;
  if (factor == NVG_ONE_MINUS_SRC_ALPHA)
    return GL_ONE_MINUS_SRC_ALPHA;
  if (factor == NVG_DST_ALPHA)
    return GL_DST_ALPHA;
  if (factor == NVG_ONE_MINUS_DST_ALPHA)
    return GL_ONE_MINUS_DST_ALPHA;
  if (factor == NVG_SRC_ALPHA_SATURATE)
    return GL_SRC_ALPHA_SATURATE;
  return GL_INVALID_ENUM;
}

static GLNVGblend
glnvg__blendCompositeOperation(NVGcompositeOperationState op) {
  GLNVGblend blend;
  blend.srcRGB = glnvg_convertBlendFuncFactor(op.srcRGB);
  blend.dstRGB = glnvg_convertBlendFuncFactor(op.dstRGB);
  blend.srcAlpha = glnvg_convertBlendFuncFactor(op.srcAlpha);
  blend.dstAlpha = glnvg_convertBlendFuncFactor(op.dstAlpha);
  if (blend.srcRGB == GL_INVALID_ENUM || blend.dstRGB == GL_INVALID_ENUM ||
      blend.srcAlpha == GL_INVALID_ENUM || blend.dstAlpha == GL_INVALID_ENUM) {
    blend.srcRGB = GL_ONE;
    blend.dstRGB = GL_ONE_MINUS_SRC_ALPHA;
    blend.srcAlpha = GL_ONE;
    blend.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
  }
  return blend;
}

static void glnvg__renderFlush(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  int i;

  if (gl->ncalls > 0) {

    // Setup require GL state.
    gl->shader.use();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xffffffff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    gl->boundTexture = 0;
    gl->stencilMask = 0xffffffff;
    gl->stencilFunc = GL_ALWAYS;
    gl->stencilFuncRef = 0;
    gl->stencilFuncMask = 0xffffffff;
    gl->blendFunc.srcRGB = GL_INVALID_ENUM;
    gl->blendFunc.srcAlpha = GL_INVALID_ENUM;
    gl->blendFunc.dstRGB = GL_INVALID_ENUM;
    gl->blendFunc.dstAlpha = GL_INVALID_ENUM;

    // Upload ubo for frag shaders
    glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
    glBufferData(GL_UNIFORM_BUFFER, gl->nuniforms * gl->fragSize, gl->uniforms,
                 GL_STREAM_DRAW);

    // Upload vertex data
    glBindVertexArray(gl->vertArr);
    glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
    glBufferData(GL_ARRAY_BUFFER, gl->nverts * sizeof(NVGvertex), gl->verts,
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(NVGvertex),
                          (const GLvoid *)(size_t)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(NVGvertex),
                          (const GLvoid *)(0 + 2 * sizeof(float)));

    // Set view and texture just once per frame.
    gl->shader.set_texture_and_view(0, gl->view);

    glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);

    for (i = 0; i < gl->ncalls; i++) {
      GLNVGcall *call = &gl->calls[i];
      glnvg__blendFuncSeparate(gl, &call->blendFunc);
      if (call->type == GLNVG_FILL)
        glnvg__fill(gl, call);
      else if (call->type == GLNVG_CONVEXFILL)
        glnvg__convexFill(gl, call);
      else if (call->type == GLNVG_STROKE)
        glnvg__stroke(gl, call);
      else if (call->type == GLNVG_TRIANGLES)
        glnvg__triangles(gl, call);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glnvg__bindTexture(gl, 0);
  }

  // Reset calls
  gl->nverts = 0;
  gl->npaths = 0;
  gl->ncalls = 0;
  gl->nuniforms = 0;
}

static int glnvg__maxVertCount(const NVGpath *paths, int npaths) {
  int i, count = 0;
  for (i = 0; i < npaths; i++) {
    count += paths[i].nfill;
    count += paths[i].nstroke;
  }
  return count;
}

static GLNVGcall *glnvg__allocCall(GLNVGcontext *gl) {
  GLNVGcall *ret = NULL;
  if (gl->ncalls + 1 > gl->ccalls) {
    GLNVGcall *calls;
    int ccalls =
        glnvg__maxi(gl->ncalls + 1, 128) + gl->ccalls / 2; // 1.5x Overallocate
    calls = (GLNVGcall *)realloc(gl->calls, sizeof(GLNVGcall) * ccalls);
    if (calls == NULL)
      return NULL;
    gl->calls = calls;
    gl->ccalls = ccalls;
  }
  ret = &gl->calls[gl->ncalls++];
  memset(ret, 0, sizeof(GLNVGcall));
  return ret;
}

static int glnvg__allocPaths(GLNVGcontext *gl, int n) {
  int ret = 0;
  if (gl->npaths + n > gl->cpaths) {
    GLNVGpath *paths;
    int cpaths =
        glnvg__maxi(gl->npaths + n, 128) + gl->cpaths / 2; // 1.5x Overallocate
    paths = (GLNVGpath *)realloc(gl->paths, sizeof(GLNVGpath) * cpaths);
    if (paths == NULL)
      return -1;
    gl->paths = paths;
    gl->cpaths = cpaths;
  }
  ret = gl->npaths;
  gl->npaths += n;
  return ret;
}

static int glnvg__allocVerts(GLNVGcontext *gl, int n) {
  int ret = 0;
  if (gl->nverts + n > gl->cverts) {
    NVGvertex *verts;
    int cverts =
        glnvg__maxi(gl->nverts + n, 4096) + gl->cverts / 2; // 1.5x Overallocate
    verts = (NVGvertex *)realloc(gl->verts, sizeof(NVGvertex) * cverts);
    if (verts == NULL)
      return -1;
    gl->verts = verts;
    gl->cverts = cverts;
  }
  ret = gl->nverts;
  gl->nverts += n;
  return ret;
}

static int glnvg__allocFragUniforms(GLNVGcontext *gl, int n) {
  int ret = 0, structSize = gl->fragSize;
  if (gl->nuniforms + n > gl->cuniforms) {
    unsigned char *uniforms;
    int cuniforms = glnvg__maxi(gl->nuniforms + n, 128) +
                    gl->cuniforms / 2; // 1.5x Overallocate
    uniforms = (unsigned char *)realloc(gl->uniforms, structSize * cuniforms);
    if (uniforms == NULL)
      return -1;
    gl->uniforms = uniforms;
    gl->cuniforms = cuniforms;
  }
  ret = gl->nuniforms * structSize;
  gl->nuniforms += n;
  return ret;
}

static GLNVGfragUniforms *nvg__fragUniformPtr(GLNVGcontext *gl, int i) {
  return (GLNVGfragUniforms *)&gl->uniforms[i];
}

static void glnvg__vset(NVGvertex *vtx, float x, float y, float u, float v) {
  vtx->x = x;
  vtx->y = y;
  vtx->u = u;
  vtx->v = v;
}

static void glnvg__renderFill(void *uptr, NVGpaint *paint,
                              NVGcompositeOperationState compositeOperation,
                              NVGscissor *scissor, float fringe,
                              const float *bounds, const NVGpath *paths,
                              int npaths) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  GLNVGcall *call = glnvg__allocCall(gl);
  NVGvertex *quad;
  GLNVGfragUniforms *frag;
  int i, maxverts, offset;

  if (call == NULL)
    return;

  call->type = GLNVG_FILL;
  call->triangleCount = 4;
  call->pathOffset = glnvg__allocPaths(gl, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

  if (npaths == 1 && paths[0].convex) {
    call->type = GLNVG_CONVEXFILL;
    call->triangleCount =
        0; // Bounding box fill quad not needed for convex fill
  }

  // Allocate vertices for all the paths.
  maxverts = glnvg__maxVertCount(paths, npaths) + call->triangleCount;
  offset = glnvg__allocVerts(gl, maxverts);
  if (offset == -1)
    goto error;

  for (i = 0; i < npaths; i++) {
    GLNVGpath *copy = &gl->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(GLNVGpath));
    if (path->nfill > 0) {
      copy->fillOffset = offset;
      copy->fillCount = path->nfill;
      memcpy(&gl->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
      offset += path->nfill;
    }
    if (path->nstroke > 0) {
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&gl->verts[offset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  // Setup uniforms for draw calls
  if (call->type == GLNVG_FILL) {
    // Quad
    call->triangleOffset = offset;
    quad = &gl->verts[call->triangleOffset];
    glnvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
    glnvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
    glnvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
    glnvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

    call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
    if (call->uniformOffset == -1)
      goto error;
    // Simple shader for stencil
    frag = nvg__fragUniformPtr(gl, call->uniformOffset);
    memset(frag, 0, sizeof(*frag));
    frag->strokeThr = -1.0f;
    frag->type = NSVG_SHADER_SIMPLE;
    // Fill shader
    glnvg__convertPaint(
        gl, nvg__fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint,
        scissor, fringe, fringe, -1.0f);
  } else {
    call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
    if (call->uniformOffset == -1)
      goto error;
    // Fill shader
    glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint,
                        scissor, fringe, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (gl->ncalls > 0)
    gl->ncalls--;
}

static void glnvg__renderStroke(void *uptr, NVGpaint *paint,
                                NVGcompositeOperationState compositeOperation,
                                NVGscissor *scissor, float fringe,
                                float strokeWidth, const NVGpath *paths,
                                int npaths) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  GLNVGcall *call = glnvg__allocCall(gl);
  int i, maxverts, offset;

  if (call == NULL)
    return;

  call->type = GLNVG_STROKE;
  call->pathOffset = glnvg__allocPaths(gl, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

  // Allocate vertices for all the paths.
  maxverts = glnvg__maxVertCount(paths, npaths);
  offset = glnvg__allocVerts(gl, maxverts);
  if (offset == -1)
    goto error;

  for (i = 0; i < npaths; i++) {
    GLNVGpath *copy = &gl->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(GLNVGpath));
    if (path->nstroke) {
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&gl->verts[offset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  if (gl->flags & NVG_STENCIL_STROKES) {
    // Fill shader
    call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
    if (call->uniformOffset == -1)
      goto error;

    glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint,
                        scissor, strokeWidth, fringe, -1.0f);
    glnvg__convertPaint(
        gl, nvg__fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint,
        scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);

  } else {
    // Fill shader
    call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
    if (call->uniformOffset == -1)
      goto error;
    glnvg__convertPaint(gl, nvg__fragUniformPtr(gl, call->uniformOffset), paint,
                        scissor, strokeWidth, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (gl->ncalls > 0)
    gl->ncalls--;
}

static void glnvg__renderTriangles(
    void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation,
    NVGscissor *scissor, const NVGvertex *verts, int nverts, float fringe) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  GLNVGcall *call = glnvg__allocCall(gl);
  GLNVGfragUniforms *frag;

  if (call == NULL)
    return;

  call->type = GLNVG_TRIANGLES;
  call->image = paint->image;
  call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

  // Allocate vertices for all the paths.
  call->triangleOffset = glnvg__allocVerts(gl, nverts);
  if (call->triangleOffset == -1)
    goto error;
  call->triangleCount = nverts;

  memcpy(&gl->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

  // Fill shader
  call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
  if (call->uniformOffset == -1)
    goto error;
  frag = nvg__fragUniformPtr(gl, call->uniformOffset);
  glnvg__convertPaint(gl, frag, paint, scissor, 1.0f, fringe, -1.0f);
  frag->type = NSVG_SHADER_IMG;

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (gl->ncalls > 0)
    gl->ncalls--;
}

static void glnvg__renderDelete(void *uptr) {
  GLNVGcontext *gl = (GLNVGcontext *)uptr;
  int i;
  if (gl == NULL)
    return;

  if (gl->fragBuf != 0)
    glDeleteBuffers(1, &gl->fragBuf);
  if (gl->vertArr != 0)
    glDeleteVertexArrays(1, &gl->vertArr);
  if (gl->vertBuf != 0)
    glDeleteBuffers(1, &gl->vertBuf);

  free(gl->paths);
  free(gl->verts);
  free(gl->uniforms);
  free(gl->calls);

  free(gl);
}

NVGcontext *nvgCreateGL3(int flags) {
  auto gl = new GLNVGcontext;
  if (gl == NULL)
    goto error;

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
  params.userPtr = gl;
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

  gl->flags = flags;

  NVGcontext *ctx = NULL;
  ctx = nvgCreateInternal(&params);
  if (ctx == NULL)
    goto error;

  return ctx;

error:
  // 'gl' is freed by nvgDeleteInternal.
  if (ctx != NULL)
    nvgDeleteInternal(ctx);
  return NULL;
}

void nvgDeleteGL3(NVGcontext *ctx) { nvgDeleteInternal(ctx); }

int nvglCreateImageFromHandleGL3(NVGcontext *ctx, GLuint textureId, int w,
                                 int h, int imageFlags) {
  auto gl = (GLNVGcontext *)nvgInternalParams(ctx)->userPtr;
  auto tex = GLNVGtexture::fromHandle(textureId, w, h, imageFlags);
  if (!tex)
    return 0;
  gl->textures.insert(std::make_pair(tex->id(), tex));
  return tex->id();
}

GLuint nvglImageHandleGL3(NVGcontext *ctx, int image) {
  GLNVGcontext *gl = (GLNVGcontext *)nvgInternalParams(ctx)->userPtr;
  auto tex = gl->glnvg__findTexture(image);
  return tex->handle();
}
