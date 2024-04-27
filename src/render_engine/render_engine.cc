#include "render_engine.h"
#include "create_instance.h"

RenderEngine::RenderEngine(const RenderConfig& _config) : config { _config } {
  instance = create_instance(config, context);
}

void RenderEngine::render() {
  
}