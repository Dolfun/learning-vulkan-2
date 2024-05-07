set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(render_engine)
add_subdirectory(application)

add_executable(main main.cc)
target_link_libraries(main PRIVATE application fmt::fmt)