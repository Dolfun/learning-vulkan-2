#pragma once
#include <memory>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "render_config.h"

auto create_instance(const RenderConfig&, const vk::raii::Context&)
  -> std::unique_ptr<vk::raii::Instance>;