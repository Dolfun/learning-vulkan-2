#include <fmt/core.h>
#include "application.h"

int main() {

  try {
    ApplicationInfo info {
      .window = {
        .width = 1280,
        .height = 720
      },
      .fullscreen = false
    };
    
    Application app { info };
    app.run();
    
  } catch (const vk::SystemError& e) {
    fmt::println("vk::SystemError -> {}", e.what());
    return -1;
  } catch (const std::exception& e) {
    fmt::println("std::exception-> {}", e.what());
    return -1;
  } catch (...) {
    fmt::println("Unknown Error");
    return -1;
  }

  return 0;
}