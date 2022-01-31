//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Create flags

enum NVGcreateFlags {
  // Flag indicating if geometry based anti-aliasing is used (may not be needed
  // when using MSAA).
  NVG_ANTIALIAS = 1 << 0,
  // Flag indicating if strokes should be drawn using stencil buffer. The
  // rendering will be a little
  // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be
  // drawn just once.
  NVG_STENCIL_STROKES = 1 << 1,
  // Flag indicating that additional debug checks are done.
  NVG_DEBUG = 1 << 2,
};

// Creates NanoVG contexts for different OpenGL (ES) versions.
// Flags should be combination of the create flags above.
struct NVGcontext *nvgCreateGL3(int flags);
void nvgDeleteGL3(NVGcontext *ctx);

int nvglCreateImageFromHandleGL3(NVGcontext *ctx, unsigned int textureId, int w,
                                 int h, int flags);
unsigned int nvglImageHandleGL3(NVGcontext *ctx, int image);

// These are additional flags on top of NVGimageFlags.
enum NVGimageFlagsGL {
  NVG_IMAGE_NODELETE = 1 << 16, // Do not delete GL texture handle.
};

#ifdef __cplusplus
}
#endif
