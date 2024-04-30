#pragma once
#include "vulkan_context.h"

class Application;

class RenderEngine {
public:
  RenderEngine(const RenderConfig&, const Application&);

  void render();

private:
  const RenderConfig& config;
  VulkanContext vulkan_context;
};