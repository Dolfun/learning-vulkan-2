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

  void render();
  void cleanup();

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
  void init_physical_device();
  bool is_device_suitable(const vk::raii::PhysicalDevice&);
  std::vector<const char*> required_device_extensions;
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

  // Swap Chain
  struct SwapChainInfo {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
  } swap_chain_info;
  auto get_swap_chain_info(const vk::raii::PhysicalDevice&)
    -> SwapChainInfo;
  auto select_swap_chain_extent(const vk::SurfaceCapabilitiesKHR&, const Application&)
    -> vk::Extent2D;
  auto select_surface_format(const std::vector<vk::SurfaceFormatKHR>&)
    -> vk::SurfaceFormatKHR;
  auto select_present_mode(const std::vector<vk::PresentModeKHR>&)
    -> vk::PresentModeKHR;
  void init_swap_chain(const Application&);
  std::unique_ptr<vk::raii::SwapchainKHR> swap_chain;
  std::vector<vk::Image> swap_chain_images;
  vk::Format swap_chain_image_format;
  vk::Extent2D swap_chain_extent;

  // Image Views
  std::vector<vk::raii::ImageView> swap_chain_image_views;
  void init_swap_chain_image_views();

  // Render Pass
  void create_render_pass();
  std::unique_ptr<vk::raii::RenderPass> render_pass;

  // Graphics Pipeline
  void create_graphics_pipeline();
  std::vector<std::byte> read_file(const std::string&);
  auto create_shader_module(const std::vector<std::byte>&) -> std::unique_ptr<vk::raii::ShaderModule>;
  std::unique_ptr<vk::raii::PipelineLayout> pipeline_layout;
  std::unique_ptr<vk::raii::Pipeline> graphics_pipeline;

  // Framebuffers
  void create_framebuffers();
  std::vector<vk::raii::Framebuffer> swap_chain_framebuffers;

  // Command Pool
  void create_command_pool();
  std::unique_ptr<vk::raii::CommandPool> command_pool;

  // Command Buffer
  void create_command_buffer();
  void record_command_buffer(vk::raii::CommandBuffer&, uint32_t);
  std::unique_ptr<vk::raii::CommandBuffer> command_buffer;

  // Rendering
  std::unique_ptr<vk::raii::Semaphore> image_available_semaphore, render_finished_semaphore;
  std::unique_ptr<vk::raii::Fence> in_flight_fence;
  void create_sync_objects();
};