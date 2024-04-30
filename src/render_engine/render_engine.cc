#include "render_engine.h"
#include <algorithm>
#include <fmt/core.h>
#include <fmt/color.h>
#include <limits>
#include <set>
#include "../application/application.h"

#ifdef NDEBUG
  constexpr bool enable_validation_layers = false;
#else
  constexpr bool enable_validation_layers = true;
#endif

RenderEngine::RenderEngine(const RenderConfig& _config, const Application& application)
    : config { _config } {
  init_instance();
  init_debug_messenger();
  init_window_surface(application);
  init_physical_device();
  init_logical_device();
  init_queues();
  init_swap_chain(application);
}

void RenderEngine::init_instance() {
  vk::ApplicationInfo application_info {
    .sType = vk::StructureType::eApplicationInfo,
    .pApplicationName = "learning-vulkan-c++",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "null",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };

  vk::InstanceCreateInfo create_info {
    .sType = vk::StructureType::eInstanceCreateInfo,
    .pApplicationInfo = &application_info,
  };

  if constexpr (enable_validation_layers) {
    check_validation_layers_support();
    auto debug_messenger_create_info = get_debug_messenger_create_info();
    create_info.enabledLayerCount = static_cast<uint32_t>(config.vulkan.requested_layers.size());
    create_info.ppEnabledLayerNames = config.vulkan.requested_layers.data();
    create_info.pNext = static_cast<const void*>(&debug_messenger_create_info);
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  auto required_extensions = config.vulkan.required_extensions;
  if constexpr (enable_validation_layers) {
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  check_required_extensions_support();
  create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
  create_info.ppEnabledExtensionNames = required_extensions.data();

  instance = std::make_unique<vk::raii::Instance>(context, create_info);
}

void RenderEngine::check_required_extensions_support() {
  auto available_extensions = context.enumerateInstanceExtensionProperties();
  for (const char* name : config.vulkan.required_extensions) {
    bool found = std::ranges::any_of(available_extensions, [name] (const vk::ExtensionProperties property) {
      return std::strcmp(name, property.extensionName.data()) == 0;
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find required extension: {}", name));
    }
  }
}

void RenderEngine::check_validation_layers_support() {
  auto available_layers = context.enumerateInstanceLayerProperties();
  for (const char* name : config.vulkan.requested_layers) {
    bool found = std::ranges::any_of(available_layers, [name] (const vk::LayerProperties property) {
      return std::strcmp(name, property.layerName.data()) == 0;
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find requested validation layer: {}", name));
    }
  }
}

void RenderEngine::init_debug_messenger() {
  auto create_info = get_debug_messenger_create_info();
  debug_messenger = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(*instance, create_info);
}

auto RenderEngine::get_debug_messenger_create_info() -> vk::DebugUtilsMessengerCreateInfoEXT {
  using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
  vk::DebugUtilsMessengerCreateInfoEXT create_info = {
    .sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT,
    .messageSeverity = eWarning | eError,
    .messageType = eGeneral | eValidation | ePerformance,
    .pfnUserCallback = debug_callback,
    .pUserData = nullptr
  };
  return create_info;
}

VKAPI_ATTR VkBool32 VKAPI_CALL RenderEngine::debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT,
  const VkDebugUtilsMessengerCallbackDataEXT* data,
  void*) {

  fmt::color color { fmt::color::white };
  switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      color = fmt::color::white;
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      color = fmt::color::green;
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      color = fmt::color::yellow;
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      color = fmt::color::red;
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
      color = fmt::color::dark_red;
      break;
  }

  std::string error = fmt::format("[[{}]] {}\n", data->pMessageIdName, data->pMessage);
  fmt::print(fmt::fg(color), "{}", error);

  return VK_FALSE;
}

void RenderEngine::init_window_surface(const Application& application) {
  VkSurfaceKHR _surface;
  application.create_window_surface(**instance, _surface);
  surface = std::make_unique<vk::raii::SurfaceKHR>(*instance, _surface);
}

void RenderEngine::init_physical_device() {
  required_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  vk::raii::PhysicalDevices physical_devices { *instance };
  physical_device = std::make_unique<vk::raii::PhysicalDevice>(
    *std::ranges::max_element(physical_devices, {}, [this] (const vk::raii::PhysicalDevice& device) {
      if (!is_device_suitable(device) || device.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
        return 0;
      } else {
        return 1;
      }
    })
  );

  if (!is_device_suitable(*physical_device)) {
    throw std::runtime_error("No suitable device found");
  }
}

bool RenderEngine::is_device_suitable(const vk::raii::PhysicalDevice& _device) {
  // Queue Family is complete
  auto indices = get_queue_family_indices(_device);
  if (!indices.is_complete()) {
    return false;
  }

  // All required device extensions are available
  auto available_extensions = _device.enumerateDeviceExtensionProperties();
  for (const char* name : required_device_extensions) {
    if(!std::ranges::any_of(available_extensions, [name] (const vk::ExtensionProperties property) {
      return std::strcmp(name, property.extensionName.data()) == 0;
    })) {
      return false;
    }
  }

  // Swap Chain is adequate
  auto _swap_chain_info = get_swap_chain_info(_device);
  if (_swap_chain_info.formats.empty() || _swap_chain_info.present_modes.empty()) {
    return false;
  }

  return true;
}

auto RenderEngine::get_queue_family_indices(const vk::raii::PhysicalDevice& _device)
    -> QueueFamilyIndices {
  QueueFamilyIndices indices;

  auto properties = _device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < static_cast<uint32_t>(properties.size()); ++i) {
    if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphics_family = i;
    }

    if (_device.getSurfaceSupportKHR(i, *surface)) {
      indices.present_family = i;
    }

    if (indices.is_complete()) {
      break;
    }
  }

  return indices;
}

void RenderEngine::init_logical_device() {
  queue_family_indices = get_queue_family_indices(*physical_device);
  swap_chain_info = get_swap_chain_info(*physical_device);

  std::set<uint32_t> unique_queue_families {
    queue_family_indices.graphics_family.value(),
    queue_family_indices.present_family.value()
  };

  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.reserve(unique_queue_families.size());

  for (auto queue_family : unique_queue_families) {
    float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info {
      .sType = vk::StructureType::eDeviceQueueCreateInfo,
      .queueFamilyIndex = queue_family,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos.emplace_back(queue_create_info);
  }

  vk::PhysicalDeviceFeatures device_features {};

  vk::DeviceCreateInfo create_info {
    .sType = vk::StructureType::eDeviceCreateInfo,
    .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
    .pQueueCreateInfos = queue_create_infos.data(),
    .enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size()),
    .ppEnabledExtensionNames = required_device_extensions.data(),
    .pEnabledFeatures = &device_features,
  };

  if constexpr (enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(config.vulkan.requested_layers.size());
    create_info.ppEnabledLayerNames = config.vulkan.requested_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;
  }

  device = std::make_unique<vk::raii::Device>(*physical_device, create_info);
}

void RenderEngine::init_queues() {
  graphics_queue = std::make_unique<vk::raii::Queue>(
    device->getQueue(queue_family_indices.graphics_family.value(), 0)
  );

  present_queue = std::make_unique<vk::raii::Queue>(
    device->getQueue(queue_family_indices.present_family.value(), 0)
  );
}

auto RenderEngine::get_swap_chain_info(const vk::raii::PhysicalDevice& _device) 
    -> SwapChainInfo {
  SwapChainInfo info = {
    .capabilities = _device.getSurfaceCapabilitiesKHR(*surface),
    .formats = _device.getSurfaceFormatsKHR(*surface),
    .present_modes = _device.getSurfacePresentModesKHR(*surface)
  };

  return info;
}

auto RenderEngine::select_swap_chain_extent(const vk::SurfaceCapabilitiesKHR& capabilities,
    const Application& application)
     -> vk::Extent2D {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    auto [width, height] = application.get_framebuffer_size();
    vk::Extent2D extent = { 
      static_cast<uint32_t>(width), 
      static_cast<uint32_t>(height)
    };

    auto min_extent = capabilities.minImageExtent;
    auto max_extent = capabilities.maxImageExtent;
    extent.width = std::clamp(extent.width, min_extent.width, max_extent.width);
    extent.height = std::clamp(extent.height, min_extent.height, max_extent.height);
    return extent;
  }
}

auto RenderEngine::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& formats)
    -> vk::SurfaceFormatKHR {
  for (const auto& format : formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }

  return formats.front();
}

auto RenderEngine::select_present_mode(const std::vector<vk::PresentModeKHR>& present_modes)
    -> vk::PresentModeKHR {
  for (const auto& mode : present_modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

void RenderEngine::init_swap_chain(const Application& application) {
  auto extent = select_swap_chain_extent(swap_chain_info.capabilities, application);
  auto surface_format = select_surface_format(swap_chain_info.formats);
  auto present_mode = select_present_mode(swap_chain_info.present_modes);

  auto capabilities = swap_chain_info.capabilities;
  uint32_t image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
    image_count = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR create_info = {
    .sType = vk::StructureType::eSwapchainCreateInfoKHR,
    .surface = *surface,
    .minImageCount = image_count,
    .imageFormat = surface_format.format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = present_mode,
    .clipped = vk::True,
    .oldSwapchain = nullptr
  };

  uint32_t indices[] = { 
    queue_family_indices.graphics_family.value(),
    queue_family_indices.present_family.value()
  };

  if (queue_family_indices.graphics_family != queue_family_indices.present_family) {
    create_info.imageSharingMode = vk::SharingMode::eConcurrent;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = indices;
  } else {
    create_info.imageSharingMode = vk::SharingMode::eExclusive;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = nullptr;
  }

  swap_chain = std::make_unique<vk::raii::SwapchainKHR>(*device, create_info);
}