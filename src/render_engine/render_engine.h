#pragma once
#include <memory>
#include <vector>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class RenderEngine {
public:
  struct Config {
    struct Resolution {
      size_t width, height;
    } const resolution;

    std::vector<const char*> required_extensions;
  };

  RenderEngine(const Config&);

  void render();

private:
  Config config;

  vk::raii::Context context;
  std::unique_ptr<vk::raii::Instance> instance;
};