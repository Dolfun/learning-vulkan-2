#pragma once

#include <memory>
#include <optional>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "render_config.h"

class Application;

class RenderEngine {
public:
  RenderEngine(const RenderConfig&, const Application&);

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
  auto get_debug_messenger_create_info() 
    -> vk::DebugUtilsMessengerCreateInfoEXT;
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*);
  std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debug_messenger;

  // Window Surface
  void init_window_surface(const Application&);
  std::unique_ptr<vk::raii::SurfaceKHR> surface;

  // Physical Device
  void select_physical_device();
  bool is_device_suitable(const vk::raii::PhysicalDevice&);
  std::unique_ptr<vk::raii::PhysicalDevice> physical_device;

  // Queue Family
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
      return graphics_family.has_value() && present_family.has_value();
    }
  } queue_family_indices;
  auto get_queue_family_indices(const vk::raii::PhysicalDevice&) 
    -> QueueFamilyIndices;

  // Logical Device
  void init_logical_device();
  std::unique_ptr<vk::raii::Device> device;

  // Queues
  void init_queues();
  std::unique_ptr<vk::raii::Queue> graphics_queue;
  std::unique_ptr<vk::raii::Queue> present_queue;
};