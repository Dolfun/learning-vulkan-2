#include "vulkan_context.h"
#include <fmt/core.h>
#include <fmt/color.h>

#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
#endif

VulkanContext::VulkanContext(const RenderConfig& _config) : config { _config } {
  init_instance();
  if constexpr (enable_validation_layers) {
    init_debug_messenger();
  }
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
    init_debug_messenger_create_info();
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
  debug_messenger = std::make_unique<vk::raii::DebugUtilsMessengerEXT>(
    *instance,
    debug_messenger_create_info
  );
}

void VulkanContext::init_debug_messenger_create_info() {
  using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
  debug_messenger_create_info = {
    .sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT,
    .messageSeverity = eWarning | eError,
    .messageType = eGeneral | eValidation | ePerformance,
    .pfnUserCallback = debug_callback,
    .pUserData = nullptr
  };
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