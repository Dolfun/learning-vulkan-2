#pragma once
#include <vector>

struct RenderConfig {
  struct Resolution {
    uint32_t width, height;
  } resolution;

  struct VulkanInfo {
    std::vector<const char*> required_extensions;
    std::vector<const char*> requested_layers;
  } vulkan;

  uint32_t max_frames_in_flight;
};