cmake_minimum_required(VERSION 3.10)
project(learning-vulkan-2)

find_package(Vulkan REQUIRED)

add_subdirectory(extern)
add_subdirectory(src)