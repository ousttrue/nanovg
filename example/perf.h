#pragma once
#include "nanovg.h"
#include <string>

enum GraphrenderStyle {
  GRAPH_RENDER_FPS,
  GRAPH_RENDER_MS,
  GRAPH_RENDER_PERCENT,
};

#define GRAPH_HISTORY_COUNT 100
class PerfGraph {
  int _style = 0;
  std::string _name;
  float _values[GRAPH_HISTORY_COUNT] = {0};
  int _head = 0;

public:
  PerfGraph(int style, const char *name);
  ~PerfGraph();
  void updateGraph(float frameTime);
  void renderGraph(NVGcontext *vg, float x, float y);
  float getGraphAverage();
};

#define GPU_QUERY_COUNT 5
class GPUtimer {
  int cur = 0;
  int ret = 0;
  unsigned int queries[GPU_QUERY_COUNT] = {0};

public:
  int supported = 0;
  void startGPUTimer();
  int stopGPUTimer(float *times, int maxTimes);
};
