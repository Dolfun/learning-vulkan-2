#include "render_engine.h"

RenderEngine::RenderEngine(const RenderConfig& _config, const Application& application)
  : config { _config }, vulkan_context { config, application } {

}

void RenderEngine::render() {
  
}