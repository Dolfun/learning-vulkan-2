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
    std::vector<const char*> requested_layers;
  };

  RenderEngine(const Config&);

  void render();

private:
  void create_instance();
  void check_required_extensions_support() const;
  void check_validation_layers_support() const;

  Config config;
  vk::raii::Context context;
  std::unique_ptr<vk::raii::Instance> instance;
};