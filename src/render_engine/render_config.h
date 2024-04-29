#pragma once
#include <vector>

struct RenderConfig {
  struct Resolution {
    size_t width, height;
  } resolution;

  struct VulkanInfo {
    std::vector<const char*> required_extensions;
    std::vector<const char*> requested_layers;
  } vulkan;
};