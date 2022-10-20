cmake_minimum_required(VERSION 3.20)

#set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.224.1/")

project(VulkanStudy
  VERSION 0.1
  DESCRIPTION "Studying Vulkan by writing small apps")

# dependencies
# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)
# vk-bootstrap
add_subdirectory(dependencies/vk-bootstrap)

# projects
add_subdirectory(src/vulkan-hpp-study)
add_subdirectory(src/vk-bootstrap-study)