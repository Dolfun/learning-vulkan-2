add_library(application application.h application.cc)
target_compile_features(application PUBLIC cxx_std_20)
target_link_libraries(application PUBLIC render_engine glfw PRIVATE fmt::fmt)
target_include_directories(application PUBLIC .)