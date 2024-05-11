#include "render_engine.h"
#include <algorithm>
#include <fmt/core.h>
#include <fmt/color.h>
#include <fstream>
#include <limits>
#include <array>
#include <set>
#include "../application/application.h"

#ifdef NDEBUG
  constexpr bool enable_validation_layers = false;
#else
  constexpr bool enable_validation_layers = true;
#endif

RenderEngine::RenderEngine(const RenderConfig& _config, const Application& application)
    : config { _config }, current_frame { 0 } {
  init_instance();
  init_debug_messenger();
  init_window_surface(application);
  init_physical_device();
  init_logical_device();
  init_queues();
  init_swap_chain(application);
  init_swap_chain_image_views();
  create_render_pass();
  create_graphics_pipeline();
  create_framebuffers();
  create_command_pool();
  create_command_buffer();
  create_sync_objects();
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
  swap_chain_images = swap_chain->getImages();
  swap_chain_extent = extent;
  swap_chain_image_format = surface_format.format;
}

void RenderEngine::init_swap_chain_image_views() {
  auto size = swap_chain_images.size();
  swap_chain_image_views.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    vk::ImageViewCreateInfo create_info {
      .sType = vk::StructureType::eImageViewCreateInfo,
      .image = swap_chain_images[i],
      .viewType = vk::ImageViewType::e2D,
      .format = swap_chain_image_format,
      .components = {
        .r = vk::ComponentSwizzle::eIdentity,
        .g = vk::ComponentSwizzle::eIdentity,
        .b = vk::ComponentSwizzle::eIdentity,
        .a = vk::ComponentSwizzle::eIdentity,
      },
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    swap_chain_image_views.emplace_back(*device, create_info);
  }
}

void RenderEngine::create_render_pass() {
  vk::AttachmentDescription color_attachment {
    .format = swap_chain_image_format,
    .samples = vk::SampleCountFlagBits::e1,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
    .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
    .initialLayout = vk::ImageLayout::eUndefined,
    .finalLayout = vk::ImageLayout::ePresentSrcKHR
  };

  vk::AttachmentReference color_attachment_reference {
    .attachment = 0,
    .layout = vk::ImageLayout::eColorAttachmentOptimal
  };

  vk::SubpassDescription subpass_description {
    .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_reference,
  };

  vk::SubpassDependency subpass_dependency {
    .srcSubpass = vk::SubpassExternal,
    .dstSubpass = 0,
    .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
    .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
    .srcAccessMask = vk::AccessFlagBits::eNone,
    .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite
  };

  vk::RenderPassCreateInfo create_info {
    .sType = vk::StructureType::eRenderPassCreateInfo,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass_description,
    .dependencyCount = 1,
    .pDependencies = &subpass_dependency
  };

  render_pass = std::make_unique<vk::raii::RenderPass>(*device, create_info);
}

void RenderEngine::create_graphics_pipeline() {
  // Shader Stage
  auto vertex_shader_code = read_file("shaders/main.vert.spv");
  auto fragment_shader_code = read_file("shaders/main.frag.spv");
  auto vertex_shader_module = create_shader_module(vertex_shader_code);
  auto fragment_shader_module = create_shader_module(fragment_shader_code);

  vk::PipelineShaderStageCreateInfo vertex_shader_stage_create_info {
    .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
    .stage = vk::ShaderStageFlagBits::eVertex,
    .module = *vertex_shader_module,
    .pName = "main"
  };

  vk::PipelineShaderStageCreateInfo fragment_shader_stage_create_info {
    .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
    .stage = vk::ShaderStageFlagBits::eFragment,
    .module = *fragment_shader_module,
    .pName = "main"
  };

  std::array shader_stages = {
    vertex_shader_stage_create_info, fragment_shader_stage_create_info
  };

  // Vertex Input
  vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info {
    .sType = vk::StructureType::ePipelineVertexInputStateCreateInfo,
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = nullptr,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = nullptr
  };

  // Input Assembly
  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info {
    .sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo,
    .topology = vk::PrimitiveTopology::eTriangleList,
    .primitiveRestartEnable = false
  };

  // Dynamic State
  std::array dynamic_states {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamic_state_create_info {
    .sType = vk::StructureType::ePipelineDynamicStateCreateInfo,
    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    .pDynamicStates = dynamic_states.data()
  };

  vk::PipelineViewportStateCreateInfo viewport_state_create_info {
    .sType = vk::StructureType::ePipelineViewportStateCreateInfo,
    .viewportCount = 1,
    .scissorCount = 1
  };

  vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info {
    .sType = vk::StructureType::ePipelineRasterizationStateCreateInfo,
    .depthClampEnable = false,
    .rasterizerDiscardEnable = false,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eClockwise,
    .depthBiasEnable = false,
    .lineWidth = 1.0f
  };

  vk::PipelineMultisampleStateCreateInfo multisample_state_create_info {
    .sType = vk::StructureType::ePipelineMultisampleStateCreateInfo,
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = false,
  };

  using enum vk::ColorComponentFlagBits;
  vk::PipelineColorBlendAttachmentState color_blend_attachment_state {
    .blendEnable = false,
    .colorWriteMask = eR | eG | eB | eA
  };

  vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info {
    .sType = vk::StructureType::ePipelineColorBlendStateCreateInfo,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state
  };

  vk::PipelineLayoutCreateInfo pipeline_layout_create_info {
    .sType = vk::StructureType::ePipelineLayoutCreateInfo
  };
  pipeline_layout = std::make_unique<vk::raii::PipelineLayout>(*device, pipeline_layout_create_info);

  vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info {
    .sType = vk::StructureType::eGraphicsPipelineCreateInfo,
    .stageCount = static_cast<uint32_t>(shader_stages.size()),
    .pStages = shader_stages.data(),
    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &input_assembly_state_create_info,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pDepthStencilState = nullptr,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = *pipeline_layout,
    .renderPass = *render_pass,
    .subpass = 0,
  };

  graphics_pipeline = std::make_unique<vk::raii::Pipeline>(*device, nullptr, graphics_pipeline_create_info);
}

std::vector<std::byte> RenderEngine::read_file(const std::string& path) {
  std::ifstream file { path, std::ios::ate | std::ios::binary };
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  auto size = static_cast<size_t>(file.tellg());
  std::vector<std::byte> buffer(size);
  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), size);
  
  return buffer;
}

auto RenderEngine::create_shader_module(const std::vector<std::byte>& code) 
  -> std::unique_ptr<vk::raii::ShaderModule> {
  vk::ShaderModuleCreateInfo create_info {
    .sType = vk::StructureType::eShaderModuleCreateInfo,
    .codeSize = code.size(),
    .pCode = reinterpret_cast<const uint32_t*>(code.data())
  };

  return std::make_unique<vk::raii::ShaderModule>(*device, create_info);
}

void RenderEngine::create_framebuffers() {
  swap_chain_framebuffers.reserve(swap_chain_image_views.size());
  for (size_t i = 0; i < swap_chain_image_views.size(); ++i) {
    std::array attachments = {
      *swap_chain_image_views[i]
    };
    
    vk::FramebufferCreateInfo create_info {
      .sType = vk::StructureType::eFramebufferCreateInfo,
      .renderPass = *render_pass,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .width = swap_chain_extent.width,
      .height = swap_chain_extent.height,
      .layers = 1
    };

    swap_chain_framebuffers.emplace_back(*device, create_info);
  }
}

void RenderEngine::create_command_pool() {
  vk::CommandPoolCreateInfo create_info {
    .sType = vk::StructureType::eCommandPoolCreateInfo,
    .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = queue_family_indices.graphics_family.value()
  };

  command_pool = std::make_unique<vk::raii::CommandPool>(*device, create_info);
}

void RenderEngine::create_command_buffer() {
  vk::CommandBufferAllocateInfo allocate_info {
    .sType = vk::StructureType::eCommandBufferAllocateInfo,
    .commandPool = *command_pool,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = config.max_frames_in_flight
  };

  vk::raii::CommandBuffers _command_buffers { *device, allocate_info };
  for (auto& command_buffer : _command_buffers) {
    command_buffers.emplace_back(std::move(command_buffer));
  }
}

void RenderEngine::record_command_buffer(vk::raii::CommandBuffer& command_buffer, uint32_t image_index) {
  vk::CommandBufferBeginInfo command_buffer_begin_info {
    .sType = vk::StructureType::eCommandBufferBeginInfo,
  };
  command_buffer.begin(command_buffer_begin_info);

  vk::ClearValue clear_color {{ std::array { 0.0f, 0.0f, 0.0f, 1.0f }}};
  vk::RenderPassBeginInfo render_pass_begin_info {
    .sType = vk::StructureType::eRenderPassBeginInfo,
    .renderPass = *render_pass,
    .framebuffer = swap_chain_framebuffers[image_index],
    .renderArea = {
      .offset = { 0, 0 },
      .extent = swap_chain_extent
    },
    .clearValueCount = 1,
    .pClearValues = &clear_color
  };
  command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);

  vk::Viewport viewport {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(swap_chain_extent.width),
    .height = static_cast<float>(swap_chain_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  command_buffer.setViewport(0, viewport);

  vk::Rect2D scissor {
    .offset = { 0, 0 },
    .extent = swap_chain_extent
  };
  command_buffer.setScissor(0, scissor);

  command_buffer.draw(3, 1, 0, 0);

  command_buffer.endRenderPass();
  command_buffer.end();
}

void RenderEngine::create_sync_objects() {
  vk::SemaphoreCreateInfo semaphore_create_info {
    .sType = vk::StructureType::eSemaphoreCreateInfo
  };

  vk::FenceCreateInfo fence_create_info {
    .sType = vk::StructureType::eFenceCreateInfo,
    .flags = vk::FenceCreateFlagBits::eSignaled
  };

  image_available_semaphores.reserve(config.max_frames_in_flight);
  render_finished_semaphores.reserve(config.max_frames_in_flight);
  in_flight_fences.reserve(config.max_frames_in_flight);
  for (uint32_t i = 0; i < config.max_frames_in_flight; ++i) {
    image_available_semaphores.emplace_back(*device, semaphore_create_info);
    render_finished_semaphores.emplace_back(*device, semaphore_create_info);
    in_flight_fences.emplace_back(*device, fence_create_info);
  }
}

void RenderEngine::render() {
  (void)device->waitForFences(*in_flight_fences[current_frame], true, UINT64_MAX);
  device->resetFences(*in_flight_fences[current_frame]);

  auto [result, image_index] 
    = swap_chain->acquireNextImage(UINT32_MAX, *image_available_semaphores[current_frame]);

  command_buffers[current_frame].reset();
  record_command_buffer(command_buffers[current_frame], image_index);

  vk::Semaphore wait_semaphores[] = { *image_available_semaphores[current_frame] };
  vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
  vk::Semaphore signal_semaphores[] = { *render_finished_semaphores[current_frame] };
  vk::CommandBuffer _command_buffers[] = { command_buffers[current_frame] };
  vk::SubmitInfo submit_info {
    .sType = vk::StructureType::eSubmitInfo,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = _command_buffers,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signal_semaphores
  };
  graphics_queue->submit(submit_info, *in_flight_fences[current_frame]);

  vk::SwapchainKHR swap_chains[] = { **swap_chain };
  vk::PresentInfoKHR present_info {
    .sType = vk::StructureType::ePresentInfoKHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signal_semaphores,
    .swapchainCount = 1,
    .pSwapchains = swap_chains,
    .pImageIndices = &image_index
  };
  (void)present_queue->presentKHR(present_info);

  current_frame = (current_frame + 1) % config.max_frames_in_flight;
}

void RenderEngine::cleanup() {
  device->waitIdle();
}