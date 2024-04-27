#include "render_engine.h"
#include "utility.h"

RenderEngine::RenderEngine(const RenderConfig& _config) : config { _config } {
  instance = create_instance(config, context);
}

void RenderEngine::render() {
  
}