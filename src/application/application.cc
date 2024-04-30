#include "Application.h"
#include <fmt/core.h>
#include <stdexcept>
#include <algorithm>
#include "../timeit.h"

Application::Application(const ApplicationInfo& _info) : info { _info } {
  timeit("init_glfw", [this] { init_glfw(); });
  timeit("init_render_engine", [this] { init_render_engine(); });
}

void Application::init_glfw() {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  auto monitor = (info.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
  window = glfwCreateWindow(
    static_cast<int>(info.window.width), 
    static_cast<int>(info.window.height), 
    "Vulkan", monitor, nullptr
  );
  
  if (window == nullptr) {
    throw std::runtime_error("Failed to create window");
  }

  glfwSetKeyCallback(window, key_callback);
}

void Application::init_render_engine() {
  RenderConfig render_config {
    .resolution = {
      .width = info.window.width,
      .height = info.window.height
    },
    .vulkan = {
      .requested_layers = { "VK_LAYER_KHRONOS_validation" }
    }
  };

  uint32_t required_extension_count;
  const char** required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
  render_config.vulkan.required_extensions.resize(required_extension_count);
  std::copy(
    required_extensions,
    required_extensions + required_extension_count,
    render_config.vulkan.required_extensions.begin()
  );

  render_engine = std::make_unique<RenderEngine>(render_config, *this);
}

void Application::key_callback(GLFWwindow* window, int key, int, int action, int) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

void Application::run() {
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void Application::create_window_surface(const VkInstance& instance, VkSurfaceKHR& surface) const {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface)) {
    throw std::runtime_error("Failed to create window surface");
  }
}

std::pair<int, int> Application::get_framebuffer_size() const {
  std::pair<int, int> framebuffer_size;
  glfwGetFramebufferSize(window, &framebuffer_size.first, &framebuffer_size.second);
  return framebuffer_size;
}

Application::~Application() {
  glfwDestroyWindow(window);
  glfwTerminate();
}