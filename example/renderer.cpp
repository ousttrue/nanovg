#include "renderer.h"
#include "nanovg_gl.h"
#include "nanovg_gl_shader.h"
#include "texture_manager.h"
#include <assert.h>
#include <glad/glad.h>
#include <memory>

// cached state
unsigned int _stencilMask = {};
unsigned int _stencilFunc = {};
int _stencilFuncRef = {};
unsigned int _stencilFuncMask = {};
GLNVGblend _blendFunc = {};

static void glnvg__stencilMask(GLuint mask) {
  if (_stencilMask != mask) {
    _stencilMask = mask;
    glStencilMask(mask);
  }
}

static void glnvg__stencilFunc(GLenum func, GLint ref, GLuint mask) {
  if ((_stencilFunc != func) || (_stencilFuncRef != ref) ||
      (_stencilFuncMask != mask)) {

    _stencilFunc = func;
    _stencilFuncRef = ref;
    _stencilFuncMask = mask;
    glStencilFunc(func, ref, mask);
  }
}

// TODO:
int _flags = 0;
static void glnvg__checkError(const char *str) {
  GLenum err;
  // if ((_flags & NVG_DEBUG) == 0)
  //   return;
  err = glGetError();
  if (err != GL_NO_ERROR) {
    printf("Error %08x after %s\n", err, str);
    return;
  }
}

Renderer::Renderer(const std::shared_ptr<GLNVGshader> &shader)
    : _shader(shader) {
  _texture = std::make_shared<TextureManager>();
  glnvg__checkError("init");
  _shader->getUniforms();
  glnvg__checkError("uniform locations");

  // Create dynamic vertex array
  glGenVertexArrays(1, &_vertArr);
  glGenBuffers(1, &_vertBuf);

  // Create UBOs
  int align = 4;
  _shader->blockBind();
  glGenBuffers(1, &_fragBuf);
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
  _fragSize =
      sizeof(GLNVGfragUniforms) + align - sizeof(GLNVGfragUniforms) % align;

  glnvg__checkError("create done");

  glFinish();
}

Renderer::~Renderer() {
  if (_fragBuf != 0)
    glDeleteBuffers(1, &_fragBuf);
  if (_vertArr != 0)
    glDeleteVertexArrays(1, &_vertArr);
  if (_vertBuf != 0)
    glDeleteBuffers(1, &_vertBuf);
}

std::shared_ptr<Renderer> Renderer::create(bool useAntiAlias) {
  auto shader = GLNVGshader::create(useAntiAlias);
  if (!shader) {
    return nullptr;
  }

  return std::shared_ptr<Renderer>(new Renderer(shader));
}

void Renderer::glnvg__blendFuncSeparate(const GLNVGblend *blend) {
  if ((_blendFunc.srcRGB != blend->srcRGB) ||
      (_blendFunc.dstRGB != blend->dstRGB) ||
      (_blendFunc.srcAlpha != blend->srcAlpha) ||
      (_blendFunc.dstAlpha != blend->dstAlpha)) {

    _blendFunc = *blend;
    glBlendFuncSeparate(blend->srcRGB, blend->dstRGB, blend->srcAlpha,
                        blend->dstAlpha);
  }
}

void Renderer::glnvg__fill(const GLNVGcall *call, const GLNVGpath *pPath) {
  auto paths = &pPath[call->pathOffset];
  int i, npaths = call->pathCount;

  // Draw shapes
  glEnable(GL_STENCIL_TEST);
  glnvg__stencilMask(0xff);
  glnvg__stencilFunc(GL_ALWAYS, 0, 0xff);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  // set bindpoint for solid loc
  glnvg__setUniforms(call->uniformOffset);
  _texture->bind(0);
  glnvg__checkError("fill simple");

  glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
  glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
  glDisable(GL_CULL_FACE);
  for (i = 0; i < npaths; i++)
    glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
  glEnable(GL_CULL_FACE);

  // Draw anti-aliased pixels
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glnvg__setUniforms(call->uniformOffset + _fragSize);
  _texture->bind(call->image);
  glnvg__checkError("fill fill");

  if (_flags & NVG_ANTIALIAS) {
    glnvg__stencilFunc(GL_EQUAL, 0x00, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    // Draw fringes
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
  }

  // Draw fill
  glnvg__stencilFunc(GL_NOTEQUAL, 0x0, 0xff);
  glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
  glDrawArrays(GL_TRIANGLE_STRIP, call->triangleOffset, call->triangleCount);

  glDisable(GL_STENCIL_TEST);
}

void Renderer::glnvg__convexFill(const GLNVGcall *call,
                                 const GLNVGpath *pPaths) {
  auto paths = &pPaths[call->pathOffset];
  int i, npaths = call->pathCount;

  glnvg__setUniforms(call->uniformOffset);
  _texture->bind(call->image);
  glnvg__checkError("convex fill");

  for (i = 0; i < npaths; i++) {
    glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
    // Draw fringes
    if (paths[i].strokeCount > 0) {
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
    }
  }
}

void Renderer::glnvg__stroke(const GLNVGcall *call, const GLNVGpath *pPaths) {
  auto paths = &pPaths[call->pathOffset];
  int npaths = call->pathCount, i;

  if (_flags & NVG_STENCIL_STROKES) {
    glEnable(GL_STENCIL_TEST);
    glnvg__stencilMask(0xff);

    // Fill the stroke base without overlap
    glnvg__stencilFunc(GL_EQUAL, 0x0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glnvg__setUniforms(call->uniformOffset + _fragSize);
    _texture->bind(call->image);
    glnvg__checkError("stroke fill 0");
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);

    // Draw anti-aliased pixels.
    glnvg__setUniforms(call->uniformOffset);
    _texture->bind(call->image);
    glnvg__stencilFunc(GL_EQUAL, 0x00, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);

    // Clear stencil buffer.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glnvg__stencilFunc(GL_ALWAYS, 0x0, 0xff);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glnvg__checkError("stroke fill 1");
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);

    //		glnvg__convertPaint(gl, nvg__fragUniformPtr(gl,
    // call->uniformOffset
    //+ fragSize), paint, scissor, strokeWidth, fringe, 1.0f -
    // 0.5f/255.0f);
  } else {
    glnvg__setUniforms(call->uniformOffset);
    _texture->bind(call->image);
    glnvg__checkError("stroke fill");
    // Draw Strokes
    for (i = 0; i < npaths; i++)
      glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset,
                   paths[i].strokeCount);
  }
}

void Renderer::glnvg__triangles(const GLNVGcall *call) {
  glnvg__setUniforms(call->uniformOffset);
  _texture->bind(call->image);
  glnvg__checkError("triangles fill");

  glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

void Renderer::glnvg__setUniforms(int uniformOffset) {
  glBindBufferRange(GL_UNIFORM_BUFFER, GLNVG_FRAG_BINDING, _fragBuf,
                    uniformOffset, sizeof(GLNVGfragUniforms));
}

void Renderer::render(const NVGdrawData *data) {
  // Setup require GL state.
  _shader->use();

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
  _texture->bind(0);
  _stencilMask = 0xffffffff;
  _stencilFunc = GL_ALWAYS;
  _stencilFuncRef = 0;
  _stencilFuncMask = 0xffffffff;
  _blendFunc.srcRGB = GL_INVALID_ENUM;
  _blendFunc.srcAlpha = GL_INVALID_ENUM;
  _blendFunc.dstRGB = GL_INVALID_ENUM;
  _blendFunc.dstAlpha = GL_INVALID_ENUM;

  // Upload ubo for frag shaders
  glBindBuffer(GL_UNIFORM_BUFFER, _fragBuf);
  glBufferData(GL_UNIFORM_BUFFER, data->uniformByteSize,
               data->pUniform, //  _nuniforms * _fragSize
               GL_STREAM_DRAW);

  // Upload vertex data
  glBindVertexArray(_vertArr);
  glBindBuffer(GL_ARRAY_BUFFER, _vertBuf);
  glBufferData(GL_ARRAY_BUFFER, data->vertexCount * sizeof(NVGvertex),
               data->pVertex, GL_STREAM_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(NVGvertex),
                        (const GLvoid *)(size_t)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(NVGvertex),
                        (const GLvoid *)(0 + 2 * sizeof(float)));

  // Set view and texture just once per frame.
  _shader->set_texture_and_view(0, data->view);

  glBindBuffer(GL_UNIFORM_BUFFER, _fragBuf);

  for (int i = 0; i < data->drawCount; ++i) {
    auto &call = data->drawData[i];
    glnvg__blendFuncSeparate(&call.blendFunc);
    if (call.type == GLNVG_FILL)
      glnvg__fill(&call, data->pPath);
    else if (call.type == GLNVG_CONVEXFILL)
      glnvg__convexFill(&call, data->pPath);
    else if (call.type == GLNVG_STROKE)
      glnvg__stroke(&call, data->pPath);
    else if (call.type == GLNVG_TRIANGLES)
      glnvg__triangles(&call);
  }

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindVertexArray(0);
  glDisable(GL_CULL_FACE);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int Renderer::nvglCreateImageFromHandleGL3(unsigned int textureId, int w, int h,
                                           int imageFlags) {
  auto tex = GLNVGtexture::fromHandle(textureId, w, h, imageFlags);
  if (!tex)
    return 0;
  _texture->registerTexture(tex);
  return tex->id();
}

unsigned int Renderer::nvglImageHandleGL3(int image) {
  auto tex = _texture->findTexture(image);
  return tex->handle();
}

// void RenderDrawData(struct NVGcontext *ctx, NVGdrawData *data) {
//   nvgParams(ctx)->clear();
//   // gl->render();
//   // gl->clear();
// }
