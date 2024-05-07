set(
  render_engine_sources
  render_config.h
  render_engine.h
  render_engine.cc
)

add_library(render_engine ${render_engine_sources})
target_compile_features(render_engine PUBLIC cxx_std_20)
target_link_libraries(render_engine PUBLIC Vulkan::Vulkan PRIVATE fmt::fmt)
target_include_directories(render_engine PUBLIC .)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(render_engine PUBLIC -Wall -Wextra -Wpedantic -Werror)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(render_engine PUBLIC /W4 /WX)
endif()

find_program(GLSLC glslc REQUIRED)
set(SHADER_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(
  SHADER_SOURCES 
  main.vert
  main.frag
)

foreach(SHADER_SOURCE ${SHADER_SOURCES})
  set(SHADER_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_SOURCE}")
  set(SHADER_OUTPUT_PATH "${SHADER_OUTPUT_DIR}/${SHADER_SOURCE}.spv")

  add_custom_command(
    OUTPUT ${SHADER_OUTPUT_PATH}
    COMMAND ${GLSLC} ${SHADER_SOURCE_PATH} -o ${SHADER_OUTPUT_PATH}
    DEPENDS ${SHADER_SOURCE_PATH}
    VERBATIM
    COMMENT "Compiling ${SHADER_SOURCE}"
  )

  target_sources(render_engine PRIVATE ${SHADER_OUTPUT_PATH})
endforeach()