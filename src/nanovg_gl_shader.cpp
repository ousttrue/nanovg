#include "nanovg_gl_shader.h"
#include <assert.h>
#include <memory>
#include <stdio.h>

// TODO: mediump float may not be enough for GLES2 in iOS.
// see the following discussion: https://github.com/memononen/nanovg/issues/46
static const char *shaderHeader = "#version 150 core\n"
                                  "#define NANOVG_GL3 1\n"

                                  "#define USE_UNIFORMBUFFER 1\n"
                                  "\n";

static const char *fillVertShader =
    "#ifdef NANOVG_GL3\n"
    "	uniform vec2 viewSize;\n"
    "	in vec2 vertex;\n"
    "	in vec2 tcoord;\n"
    "	out vec2 ftcoord;\n"
    "	out vec2 fpos;\n"
    "#else\n"
    "	uniform vec2 viewSize;\n"
    "	attribute vec2 vertex;\n"
    "	attribute vec2 tcoord;\n"
    "	varying vec2 ftcoord;\n"
    "	varying vec2 fpos;\n"
    "#endif\n"
    "void main(void) {\n"
    "	ftcoord = tcoord;\n"
    "	fpos = vertex;\n"
    "	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - "
    "2.0*vertex.y/viewSize.y, 0, 1);\n"
    "}\n";

static const char *fillFragShader =
    "#ifdef GL_ES\n"
    "#if defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NANOVG_GL3)\n"
    " precision highp float;\n"
    "#else\n"
    " precision mediump float;\n"
    "#endif\n"
    "#endif\n"
    "#ifdef NANOVG_GL3\n"
    "#ifdef USE_UNIFORMBUFFER\n"
    "	layout(std140) uniform frag {\n"
    "		mat3 scissorMat;\n"
    "		mat3 paintMat;\n"
    "		vec4 innerCol;\n"
    "		vec4 outerCol;\n"
    "		vec2 scissorExt;\n"
    "		vec2 scissorScale;\n"
    "		vec2 extent;\n"
    "		float radius;\n"
    "		float feather;\n"
    "		float strokeMult;\n"
    "		float strokeThr;\n"
    "		int texType;\n"
    "		int type;\n"
    "	};\n"
    "#else\n" // NANOVG_GL3 && !USE_UNIFORMBUFFER
    "	uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
    "#endif\n"
    "	uniform sampler2D tex;\n"
    "	in vec2 ftcoord;\n"
    "	in vec2 fpos;\n"
    "	out vec4 outColor;\n"
    "#else\n" // !NANOVG_GL3
    "	uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
    "	uniform sampler2D tex;\n"
    "	varying vec2 ftcoord;\n"
    "	varying vec2 fpos;\n"
    "#endif\n"
    "#ifndef USE_UNIFORMBUFFER\n"
    "	#define scissorMat mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)\n"
    "	#define paintMat mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)\n"
    "	#define innerCol frag[6]\n"
    "	#define outerCol frag[7]\n"
    "	#define scissorExt frag[8].xy\n"
    "	#define scissorScale frag[8].zw\n"
    "	#define extent frag[9].xy\n"
    "	#define radius frag[9].z\n"
    "	#define feather frag[9].w\n"
    "	#define strokeMult frag[10].x\n"
    "	#define strokeThr frag[10].y\n"
    "	#define texType int(frag[10].z)\n"
    "	#define type int(frag[10].w)\n"
    "#endif\n"
    "\n"
    "float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
    "	vec2 ext2 = ext - vec2(rad,rad);\n"
    "	vec2 d = abs(pt) - ext2;\n"
    "	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
    "}\n"
    "\n"
    "// Scissoring\n"
    "float scissorMask(vec2 p) {\n"
    "	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);\n"
    "	sc = vec2(0.5,0.5) - sc * scissorScale;\n"
    "	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
    "}\n"
    "#ifdef EDGE_AA\n"
    "// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.\n"
    "float strokeMask() {\n"
    "	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, "
    "ftcoord.y);\n"
    "}\n"
    "#endif\n"
    "\n"
    "void main(void) {\n"
    "   vec4 result;\n"
    "	float scissor = scissorMask(fpos);\n"
    "#ifdef EDGE_AA\n"
    "	float strokeAlpha = strokeMask();\n"
    "	if (strokeAlpha < strokeThr) discard;\n"
    "#else\n"
    "	float strokeAlpha = 1.0;\n"
    "#endif\n"
    "	if (type == 0) {			// Gradient\n"
    "		// Calculate gradient color using box gradient\n"
    "		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
    "		float d = clamp((sdroundrect(pt, extent, radius) + "
    "feather*0.5) / feather, 0.0, 1.0);\n"
    "		vec4 color = mix(innerCol,outerCol,d);\n"
    "		// Combine alpha\n"
    "		color *= strokeAlpha * scissor;\n"
    "		result = color;\n"
    "	} else if (type == 1) {		// Image\n"
    "		// Calculate color fron texture\n"
    "		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
    "#ifdef NANOVG_GL3\n"
    "		vec4 color = texture(tex, pt);\n"
    "#else\n"
    "		vec4 color = texture2D(tex, pt);\n"
    "#endif\n"
    "		if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
    "		if (texType == 2) color = vec4(color.x);"
    "		// Apply color tint and alpha.\n"
    "		color *= innerCol;\n"
    "		// Combine alpha\n"
    "		color *= strokeAlpha * scissor;\n"
    "		result = color;\n"
    "	} else if (type == 2) {		// Stencil fill\n"
    "		result = vec4(1,1,1,1);\n"
    "	} else if (type == 3) {		// Textured tris\n"
    "#ifdef NANOVG_GL3\n"
    "		vec4 color = texture(tex, ftcoord);\n"
    "#else\n"
    "		vec4 color = texture2D(tex, ftcoord);\n"
    "#endif\n"
    "		if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
    "		if (texType == 2) color = vec4(color.x);"
    "		color *= scissor;\n"
    "		result = color * innerCol;\n"
    "	}\n"
    "#ifdef NANOVG_GL3\n"
    "	outColor = result;\n"
    "#else\n"
    "	gl_FragColor = result;\n"
    "#endif\n"
    "}\n";

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

GLNVGshader::GLNVGshader() {
  prog = glCreateProgram();
  assert(prog);
  vert = glCreateShader(GL_VERTEX_SHADER);
  assert(vert);
  frag = glCreateShader(GL_FRAGMENT_SHADER);
  assert(frag);
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

std::shared_ptr<GLNVGshader> GLNVGshader::create(bool useAntiAlias) {
  auto name = "shader";
  auto header = shaderHeader;
  auto vshader = fillVertShader;
  auto fshader = fillFragShader;
  const char *opts = nullptr;
  if (useAntiAlias) {
    opts = "#define EDGE_AA 1\n";
  }

  const char *str[3];
  str[0] = header;
  str[1] = opts != NULL ? opts : "";

  auto shader = std::shared_ptr<GLNVGshader>(new GLNVGshader);

  str[2] = vshader;
  glShaderSource(shader->vert, 3, str, 0);
  str[2] = fshader;
  glShaderSource(shader->frag, 3, str, 0);

  glCompileShader(shader->vert);
  GLint status;
  glGetShaderiv(shader->vert, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpShaderError(shader->vert, name, "vert");
    return {};
  }

  glCompileShader(shader->frag);
  glGetShaderiv(shader->frag, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpShaderError(shader->frag, name, "frag");
    return {};
  }

  glAttachShader(shader->prog, shader->vert);
  glAttachShader(shader->prog, shader->frag);

  glBindAttribLocation(shader->prog, 0, "vertex");
  glBindAttribLocation(shader->prog, 1, "tcoord");

  glLinkProgram(shader->prog);
  glGetProgramiv(shader->prog, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    glnvg__dumpProgramError(shader->prog, name);
    return {};
  }

  return shader;
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
