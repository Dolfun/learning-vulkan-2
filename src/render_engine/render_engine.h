#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "render_config.h"

class RenderEngine {
public:
  RenderEngine(const RenderConfig&);

  void render();

private:
  RenderConfig config;
  vk::raii::Context context;
  std::unique_ptr<vk::raii::Instance> instance;
};