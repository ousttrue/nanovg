# NVGcontext

```c
struct NVGcontext {
	NVGparams params;
	float* commands;
	int ccommands;
	int ncommands;
	float commandx, commandy;
	NVGstate states[NVG_MAX_STATES];
	int nstates;
	NVGpathCache* cache;
	float tessTol;
	float distTol;
	float fringeWidth;
	float devicePxRatio;
	struct FONScontext* fs;
	int fontImages[NVG_MAX_FONTIMAGES];
	int fontImageIdx;
	int drawCallCount;
	int fillTriCount;
	int strokeTriCount;
	int textTriCount;
};

struct NVGparams {
	void* userPtr;
	int edgeAntiAlias;
	int (*renderCreate)(void* uptr);
	int (*renderCreateTexture)(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
	int (*renderDeleteTexture)(void* uptr, int image);
	int (*renderUpdateTexture)(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
	int (*renderGetTextureSize)(void* uptr, int image, int* w, int* h);
	void (*renderViewport)(void* uptr, float width, float height, float devicePixelRatio);
	void (*renderCancel)(void* uptr);
	void (*renderFlush)(void* uptr);
	void (*renderFill)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths);
	void (*renderStroke)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths);
	void (*renderTriangles)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe);
	void (*renderDelete)(void* uptr);
};
typedef struct NVGparams NVGparams;

NVGcontext* nvgCreateGL3(int flags)
{
	NVGparams params;
	NVGcontext* ctx = NULL;
	GLNVGcontext* gl = (GLNVGcontext*)malloc(sizeof(GLNVGcontext));
	if (gl == NULL) goto error;
	memset(gl, 0, sizeof(GLNVGcontext));

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

	ctx = nvgCreateInternal(&params);
	if (ctx == NULL) goto error;

	return ctx;
}
```
