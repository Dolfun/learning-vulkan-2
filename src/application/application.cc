#include "Application.h"
#include <fmt/core.h>
#include <stdexcept>

Application::Application(const ApplicationInfo& _info) : info { _info } {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  auto monitor = (info.window.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
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