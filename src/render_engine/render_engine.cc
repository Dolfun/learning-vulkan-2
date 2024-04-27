#include "render_engine.h"
#include <fmt/core.h>
#include <algorithm>
#include <cstring>

RenderEngine::RenderEngine(const Config& _config) : config { _config } {
  // Application info
  vk::ApplicationInfo application_info {
    .sType = vk::StructureType::eApplicationInfo,
    .pApplicationName = "learning-vulkan-c++",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "null",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };

  // Check availability of required extensions
  auto available_extension = vk::enumerateInstanceExtensionProperties();
  std::ranges::for_each(config.required_extensions, [&available_extension] (const char* name) {
    bool found = std::ranges::any_of(available_extension, [name] (const vk::ExtensionProperties property) {
      return std::strcmp(name, property.extensionName.data());
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find required extension: {}", name));
    }
  });

  // Instance Create info
  vk::InstanceCreateInfo instance_create_info {
    .sType = vk::StructureType::eInstanceCreateInfo,
    .pApplicationInfo = &application_info,
    .enabledLayerCount = 0,
    .enabledExtensionCount = static_cast<uint32_t>(config.required_extensions.size()),
    .ppEnabledExtensionNames = config.required_extensions.data()
  };

  // Create instance
  instance = std::make_unique<vk::raii::Instance>(context, instance_create_info);
}

void RenderEngine::render() {
  
}