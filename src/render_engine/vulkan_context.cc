#include "vulkan_context.h"
#include <algorithm>
#include <fmt/core.h>
#include <fmt/color.h>
#include <set>
#include "../application/application.h"

#ifdef NDEBUG
  constexpr bool enable_validation_layers = false;
#else
  constexpr bool enable_validation_layers = true;
#endif

VulkanContext::VulkanContext(const RenderConfig& _config, const Application& application)
    : config { _config } {
  init_instance();
  init_debug_messenger();
  init_window_surface(application);
  select_physical_device();
  init_logical_device();
  init_queues();
}

void VulkanContext::init_instance() {
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

void VulkanContext::check_required_extensions_support() {
  auto available_extension = context.enumerateInstanceExtensionProperties();
  for (const char* name : config.vulkan.required_extensions) {
    bool found = std::ranges::any_of(available_extension, [name] (const vk::ExtensionProperties property) {
      return std::strcmp(name, property.extensionName.data());
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find required extension: {}", name));
    }
  }
}

void VulkanContext::check_validation_layers_support() {
  auto available_layers = context.enumerateInstanceLayerProperties();
  for (const char* name : config.vulkan.requested_layers) {
    bool found = std::ranges::any_of(available_layers, [name] (const vk::LayerProperties property) {
      return std::strcmp(name, property.layerName.data());
    });
    if (!found) {
      throw std::runtime_error(fmt::format("Cannot find requested validation layer: {}", name));
    }
  }
}

void VulkanContext::init_debug_messenger() {
  auto create_info = get_debug_messenger_create_info();
  debug_messenger = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(*instance, create_info);
}

auto VulkanContext::get_debug_messenger_create_info() -> vk::DebugUtilsMessengerCreateInfoEXT {
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

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debug_callback(
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

void VulkanContext::init_window_surface(const Application& application) {
  VkSurfaceKHR _surface;
  application.create_window_surface(**instance, _surface);
  surface = std::make_unique<vk::raii::SurfaceKHR>(*instance, _surface);
}

void VulkanContext::select_physical_device() {
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
}

bool VulkanContext::is_device_suitable(const vk::raii::PhysicalDevice& phyiscal_device) {
  auto indices = get_queue_family_indices(phyiscal_device);
  return indices.is_complete();
}

auto VulkanContext::get_queue_family_indices(const vk::raii::PhysicalDevice& phyiscal_device)
   -> QueueFamilyIndices {
  QueueFamilyIndices indices;

  auto properties = phyiscal_device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < static_cast<uint32_t>(properties.size()); ++i) {
    if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphics_family = i;
    }

    if (phyiscal_device.getSurfaceSupportKHR(i, *surface)) {
      indices.present_family = i;
    }

    if (indices.is_complete()) {
      break;
    }
  }

  return indices;
}

void VulkanContext::init_logical_device() {
  queue_family_indices = get_queue_family_indices(*physical_device);

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
    .pEnabledFeatures = &device_features
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

void VulkanContext::init_queues() {
  graphics_queue = std::make_unique<vk::raii::Queue>(
    device->getQueue(queue_family_indices.graphics_family.value(), 0)
  );

  present_queue = std::make_unique<vk::raii::Queue>(
    device->getQueue(queue_family_indices.present_family.value(), 0)
  );
}