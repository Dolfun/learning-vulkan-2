#include "render_engine.h"
#include <fmt/core.h>
#include <algorithm>
#include <cstring>

#ifdef NDEBUG
  constexpr bool enable_validation_layers = false;
#else
  constexpr bool enable_validation_layers = true;
#endif

RenderEngine::RenderEngine(const Config& _config) : config { _config } {
  create_instance();
}

void RenderEngine::create_instance() {
  vk::ApplicationInfo application_info {
    .sType = vk::StructureType::eApplicationInfo,
    .pApplicationName = "learning-vulkan-c++",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "null",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };

  vk::InstanceCreateInfo instance_create_info {
    .sType = vk::StructureType::eInstanceCreateInfo,
    .pApplicationInfo = &application_info,
  };

  if (enable_validation_layers) {
    check_validation_layers_support();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(config.requested_layers.size());
    instance_create_info.ppEnabledLayerNames = config.requested_layers.data();
  } else {
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.pNext = nullptr;
  }

  check_required_extensions_support();
  instance_create_info.enabledExtensionCount = static_cast<uint32_t>(config.required_extensions.size());
  instance_create_info.ppEnabledExtensionNames = config.required_extensions.data();

  instance = std::make_unique<vk::raii::Instance>(context, instance_create_info);
}

void RenderEngine::check_required_extensions_support() const {
  auto available_extension = vk::enumerateInstanceExtensionProperties();
  for (const char* name : config.required_extensions) {
    bool found = std::ranges::any_of(available_extension, [name] (const vk::ExtensionProperties property) {
      return std::strcmp(name, property.extensionName.data());
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find required extension: {}", name));
    }
  }
}

void RenderEngine::check_validation_layers_support() const {
  auto available_layers = vk::enumerateInstanceLayerProperties();
  for (const char* name : config.requested_layers) {
    bool found = std::ranges::any_of(available_layers, [name] (const vk::LayerProperties property) {
      return std::strcmp(name, property.layerName.data());
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find requested validation layer: {}", name));
    }
  }
}

void RenderEngine::render() {
  
}