#include "nanovg_gl_shader.h"

#include <assert.h>
#include <glad/glad.h>
#include <stdio.h>

#include <memory>

// TODO: mediump float may not be enough for GLES2 in iOS.
// see the following discussion: https://github.com/memononen/nanovg/issues/46
auto shaderHeader = R"(#version 150 core
#define USE_UNIFORMBUFFER 1
)";

auto fillVertShader = R"(
	uniform vec2 viewSize;
	in vec2 vertex;
	in vec2 tcoord;
	out vec2 ftcoord;
	out vec2 fpos;

void main(void) {
	ftcoord = tcoord;
	fpos = vertex;
	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);
}
)";

auto fillFragShader = R"(
  precision mediump float;
	layout(std140) uniform frag {
		mat3 scissorMat;
		mat3 paintMat;
		vec4 innerCol;
		vec4 outerCol;
		vec2 scissorExt;
		vec2 scissorScale;
		vec2 extent;
		float radius;
		float feather;
		float strokeMult;
		float strokeThr;
		int texType;
		int type;
	};
	uniform sampler2D tex;
	in vec2 ftcoord;
	in vec2 fpos;
	out vec4 outColor;

float sdroundrect(vec2 pt, vec2 ext, float rad) {
	vec2 ext2 = ext - vec2(rad,rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
}

// Scissoring
float scissorMask(vec2 p) {
	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
	sc = vec2(0.5,0.5) - sc * scissorScale;
	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}
#ifdef EDGE_AA
// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask() {
	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);
}
#endif

void main(void) {
  vec4 result;
	float scissor = scissorMask(fpos);
#ifdef EDGE_AA
	float strokeAlpha = strokeMask();
	if (strokeAlpha < strokeThr) discard;
#else
	float strokeAlpha = 1.0;
#endif
	if (type == 0) {			// Gradient
		// Calculate gradient color using box gradient
		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;
		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
		vec4 color = mix(innerCol,outerCol,d);
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	} else if (type == 1) {		// Image
		// Calculate color fron texture
		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;
		vec4 color = texture(tex, pt);
		if (texType == 1) color = vec4(color.xyz*color.w,color.w);
		if (texType == 2) color = vec4(color.x);
		// Apply color tint and alpha.
		color *= innerCol;
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	} else if (type == 2) {		// Stencil fill
		result = vec4(1,1,1,1);
	} else if (type == 3) {		// Textured tris
		vec4 color = texture(tex, ftcoord);
		if (texType == 1) color = vec4(color.xyz*color.w,color.w);
		if (texType == 2) color = vec4(color.x);
		color *= scissor;
		result = color * innerCol;
	}
	outColor = result;
};
)";

static void glnvg__dumpShaderError(GLuint shader, const char *name,
                                   const char *type) {
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetShaderInfoLog(shader, 512, &len, str);
    if (len > 512) len = 512;
    str[len] = '\0';
    printf("Shader %s/%s error:\n%s\n", name, type, str);
}

static void glnvg__dumpProgramError(GLuint prog, const char *name) {
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetProgramInfoLog(prog, 512, &len, str);
    if (len > 512) len = 512;
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
    if (prog != 0) glDeleteProgram(prog);
    if (vert != 0) glDeleteShader(vert);
    if (frag != 0) glDeleteShader(frag);
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
