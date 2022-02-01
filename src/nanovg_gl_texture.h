#pragma once
#include <memory>

class GLNVGtexture {
  int _id = {};
  unsigned int _handle = {};
  int _width = {};
  int _height = {};
  int _type = {};
  int _flags = {};

  GLNVGtexture();

public:
  ~GLNVGtexture();
  static std::shared_ptr<GLNVGtexture> load(int w, int h, int type,
                                            const void *data, int imageFlags);
  static std::shared_ptr<GLNVGtexture> fromHandle(unsigned int handle, int w,
                                                  int h, int imageFlags);
  void update(int x, int y, int w, int h, const void *data);
  unsigned int handle() const { return _handle; }
  void bind();
  void unbind();
  int id() const { return _id; }
  int width() const { return _width; }
  int height() const { return _height; }
  int flags() const { return _flags; }
  int type() const { return _type; }
};
