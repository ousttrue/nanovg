#pragma once

#include "example/demo.h"
#include "nanovg.h"

struct NVGcontext;
class DemoData {
  NVGcontext *_vg = nullptr;
  int _fontNormal = 0;
  int _fontBold = 0;
  int _fontIcons = 0;
  int _fontEmoji = 0;
  int _images[12] = {};

public:
  DemoData(NVGcontext *vg);
  ~DemoData();
  bool load();
  void render(float mx, float my, float width, float height, float t,
              int blowup);
  void saveScreenShot(int w, int h, int premult, const char *name);
};
