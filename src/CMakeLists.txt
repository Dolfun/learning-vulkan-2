add_subdirectory(application)

add_executable(main main.cc)
target_link_libraries(main PRIVATE application fmt::fmt)