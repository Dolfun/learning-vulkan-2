add_library(render_engine render_engine.h render_engine.cc)
target_compile_features(render_engine PUBLIC cxx_std_20)
target_include_directories(render_engine PUBLIC .)