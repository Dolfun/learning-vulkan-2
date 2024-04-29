#pragma once

#include <memory>
#include <optional>
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

  // Instance
  void init_instance();
  void check_required_extensions_support();
  void check_validation_layers_support();
  std::unique_ptr<vk::raii::Instance> instance;

  // Debug Messenger
  void init_debug_messenger();
  void init_debug_messenger_create_info();
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*);
  vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
  std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debug_messenger;

  // Physical Device
  void select_physical_device();
  bool is_device_suitable(const vk::raii::PhysicalDevice&);
  std::unique_ptr<vk::raii::PhysicalDevice> physical_device;

  // Queue Family
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;

    bool is_complete() {
      return graphics_family.has_value();
    }
  };
  auto get_queue_family_indices(const vk::raii::PhysicalDevice&) -> QueueFamilyIndices;
};