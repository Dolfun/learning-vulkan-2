#pragma once
#include <GLFW/glfw3.h>
#include <cstddef>

struct ApplicationInfo {
  struct WindowInfo {
    std::size_t width, height;
    bool fullscreen;
  } window;
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

  const ApplicationInfo& info;
  GLFWwindow* window;
};