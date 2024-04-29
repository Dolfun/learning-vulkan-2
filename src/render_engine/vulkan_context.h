#pragma once

#include <memory>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "render_config.h"

class VulkanContext {
public:
  VulkanContext(const RenderConfig&);

private:
  const RenderConfig& config;

  vk::raii::Context context;

  void init_instance();
  void check_required_extensions_support();
  void check_validation_layers_support();
  std::unique_ptr<vk::raii::Instance> instance;

  void init_debug_messenger();
  void init_debug_messenger_create_info();
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*);
  vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
  std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debug_messenger;
};