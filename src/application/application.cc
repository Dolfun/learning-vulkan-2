#include "Application.h"
#include <fmt/core.h>
#include <stdexcept>
#include <algorithm>

Application::Application(const ApplicationInfo& _info) : info { _info } {
  // Initialize GLFW
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

  // RenderEngine::Config
  RenderEngine::Config render_config {
    .resolution = {
      .width = info.window.width,
      .height = info.window.height
    }
  };

  // Find extensions required by GLFW
  uint32_t required_extension_count;
  const char** required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
  render_config.required_extensions.resize(required_extension_count);
  std::copy(
    required_extensions,
    required_extensions + required_extension_count,
    render_config.required_extensions.begin()
  );

  // Create render_engine
  render_engine = std::make_unique<RenderEngine>(render_config);
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

Application::~Application() {
  glfwDestroyWindow(window);
  glfwTerminate();
}