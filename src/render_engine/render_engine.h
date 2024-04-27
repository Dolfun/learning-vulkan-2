#pragma once
#include <cstddef>

struct RenderInfo {
  std::size_t width, height;
};

class RenderEngine {
public:
  RenderEngine(const RenderInfo&);

  void render();

private:
  const RenderInfo& info;
};