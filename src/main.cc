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
    
  } catch (const std::exception& e) {
    fmt::println("Exception Occured: {}", e.what());
    return -1;
  }

  return 0;
}