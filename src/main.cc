#include <fmt/core.h>
#include "application.h"

int main() {

  try {
    Application app;
    app.run();
    
  } catch (const std::exception& e) {
    fmt::println("Exception Occured: {}", e.what());
    return -1;
  }

  return 0;
}