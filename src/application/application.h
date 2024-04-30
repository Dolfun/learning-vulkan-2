#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstddef>
#include <memory>
#include <utility>
#include "render_engine.h"

struct ApplicationInfo {
  struct WindowSize {
    size_t width, height;
  } window;
  bool fullscreen;
};

class Application {
public:
  Application(const ApplicationInfo&);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

  void create_window_surface(const VkInstance&, VkSurfaceKHR&) const;
  std::pair<int, int> get_framebuffer_size() const;

private:
  static void key_callback(GLFWwindow*, int, int, int, int);

  void init_glfw();
  void init_render_engine();

  const ApplicationInfo& info;
  GLFWwindow* window;
  std::unique_ptr<RenderEngine> render_engine;
};