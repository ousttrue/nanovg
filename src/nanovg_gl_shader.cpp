#include "nanovg_gl_shader.h"
#include <stdio.h>

static void glnvg__dumpShaderError(GLuint shader, const char *name,
                                   const char *type) {
  GLchar str[512 + 1];
  GLsizei len = 0;
  glGetShaderInfoLog(shader, 512, &len, str);
  if (len > 512)
    len = 512;
  str[len] = '\0';
  printf("Shader %s/%s error:\n%s\n", name, type, str);
}

static void glnvg__dumpProgramError(GLuint prog, const char *name) {
  GLchar str[512 + 1];
  GLsizei len = 0;
  glGetProgramInfoLog(prog, 512, &len, str);
  if (len > 512)
    len = 512;
  str[len] = '\0';
  printf("Program %s error:\n%s\n", name, str);
}

GLNVGshader::~GLNVGshader() {
  if (prog != 0)
    glDeleteProgram(prog);
  if (vert != 0)
    glDeleteShader(vert);
  if (frag != 0)
    glDeleteShader(frag);
}

void GLNVGshader::use() { glUseProgram(prog); }

bool GLNVGshader::createShader(const char *name, const char *header,
                               const char *opts, const char *vshader,
                               const char *fshader) {
  GLint status;
  const char *str[3];
  str[0] = header;
  str[1] = opts != NULL ? opts : "";

  prog = glCreateProgram();
  vert = glCreateShader(GL_VERTEX_SHADER);
  frag = glCreateShader(GL_FRAGMENT_SHADER);
  str[2] = vshader;
  glShaderSource(vert, 3, str, 0);
  str[2] = fshader;
  glShaderSource(frag, 3, str, 0);

  glCompileShader(vert);
  glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpShaderError(vert, name, "vert");
    return false;
  }

  glCompileShader(frag);
  glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpShaderError(frag, name, "frag");
    return false;
  }

  glAttachShader(prog, vert);
  glAttachShader(prog, frag);

  glBindAttribLocation(prog, 0, "vertex");
  glBindAttribLocation(prog, 1, "tcoord");

  glLinkProgram(prog);
  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpProgramError(prog, name);
    return false;
  }

  return true;
}

void GLNVGshader::getUniforms() {
  loc[GLNVG_LOC_VIEWSIZE] = glGetUniformLocation(prog, "viewSize");
  loc[GLNVG_LOC_TEX] = glGetUniformLocation(prog, "tex");
  loc[GLNVG_LOC_FRAG] = glGetUniformBlockIndex(prog, "frag");
}

void GLNVGshader::blockBind() {
  glUniformBlockBinding(prog, loc[GLNVG_LOC_FRAG], GLNVG_FRAG_BINDING);
}

void GLNVGshader::set_texture_and_view(int texture, const float view[2]) {
  glUniform1i(loc[GLNVG_LOC_TEX], texture);
  glUniform2fv(loc[GLNVG_LOC_VIEWSIZE], 1, view);
}
