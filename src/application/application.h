#pragma once
#include <GLFW/glfw3.h>
#include <cstddef>
#include <memory>
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

private:
  static void key_callback(GLFWwindow*, int, int, int, int);

  void init_glfw();
  void init_render_engine();

  const ApplicationInfo& info;
  GLFWwindow* window;
  std::unique_ptr<RenderEngine> render_engine;
};