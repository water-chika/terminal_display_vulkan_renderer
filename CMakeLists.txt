cmake_minimum_required(VERSION 3.20)

project(terminal_display_vulkan_renderer)

find_package(Vulkan REQUIRED)

add_library(terminal_display_vulkan_renderer
    renderer.hpp
    renderer.cpp)
set_target_properties(terminal_display_vulkan_renderer PROPERTIES CXX_STANDARD 23)

target_link_libraries(terminal_display_vulkan_renderer PUBLIC Vulkan::Vulkan)
target_include_directories(terminal_display_vulkan_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
