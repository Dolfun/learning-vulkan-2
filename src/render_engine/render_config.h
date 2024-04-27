#pragma once
#include <vector>

struct RenderConfig {
  struct Resolution {
    size_t width, height;
  } const resolution;

  std::vector<const char*> required_extensions;
  std::vector<const char*> requested_layers;
};