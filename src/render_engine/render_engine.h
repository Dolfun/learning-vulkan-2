#pragma once
#include "vulkan_context.h"

class RenderEngine {
public:
  RenderEngine(const RenderConfig&);

  void render();

private:
  RenderConfig config;
  VulkanContext vulkan_context;
};