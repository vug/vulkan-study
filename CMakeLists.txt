cmake_minimum_required(VERSION 3.24) # needed for improved FindVulkan module

project(VulkanStudy
  VERSION 0.1
  DESCRIPTION "Studying Vulkan by writing small apps")

# dependencies
# GLFW https://github.com/glfw/glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)
# CMake can find VulkanSDK if it was first installed from https://www.lunarg.com/vulkan-sdk/ 
# using FindVulkan module. `include(FindVulkan)` not needed
# See https://cmake.org/cmake/help/latest/module/FindVulkan.html for docs and https://github.com/Kitware/CMake/blob/master/Modules/FindVulkan.cmake for code
# CMake v3.24 has optional COMPONENTS using which we can check existense of components we'll need
# It not only makes Vulkan::Vulkan Target available but also other ones that comes with VulkanSDK such as Vulkan::glslang
find_package(Vulkan REQUIRED COMPONENTS glslang SPIRV-Tools)
message("Vulkan SDK location: $ENV{VULKAN_SDK}") # finds the location of LunarG VulkanSDK installation set into PATH
# Find Vulkan comes with some Result Variables
message("Found? Vulkan: ${Vulkan_FOUND}, glslang: ${Vulkan_glslang_FOUND}, SPIRV-Tools: ${Vulkan_SPIRV-Tools_FOUND}")
message("Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
message("Libraries. Vulkan: ${Vulkan_LIBRARIES}, glslang: ${Vulkan_glslang_LIBRARY}, SPIRV-Tools: ${Vulkan_SPIRV-Tools_LIBRARY}")
# GLM https://github.com/g-truc/glm
add_subdirectory(dependencies/glm)
# ImGui https://github.com/ocornut/imgui/tree/docking
add_library(
  imgui
  dependencies/imgui/imgui.h dependencies/imgui/imgui.cpp
  dependencies/imgui/imgui_draw.cpp
  dependencies/imgui/imgui_widgets.cpp
  dependencies/imgui/imgui_tables.cpp
  dependencies/imgui/imgui_demo.cpp # for ImGui::ShowDemoWindow()
  dependencies/imgui/backends/imgui_impl_glfw.h dependencies/imgui/backends/imgui_impl_glfw.cpp
  dependencies/imgui/backends/imgui_impl_vulkan.h dependencies/imgui/backends/imgui_impl_vulkan.cpp
  dependencies/imgui/misc/cpp/imgui_stdlib.h dependencies/imgui/misc/cpp/imgui_stdlib.cpp
)
target_link_libraries(
  imgui PRIVATE
  glfw
)
target_include_directories(
  imgui PUBLIC
  dependencies/imgui/
  dependencies/imgui/backends
  dependencies/imgui/misc/cpp
  PRIVATE
  ${Vulkan_INCLUDE_DIRS}
)
# tinyobjloader https://github.com/tinyobjloader/tinyobjloader
add_subdirectory(dependencies/tinyobjloader)
# vivid https://github.com/gurki/vivid
set(FETCHCONTENT_SOURCE_DIR_GLM "dependencies/glm" CACHE STRING "" FORCE) # Throus a warning about relative directory unsafe. Ignore.
add_subdirectory(dependencies/vivid)
# vk-bootstrap
add_subdirectory(dependencies/vk-bootstrap)

# projects
#add_subdirectory(src/vk-bootstrap-study)
#add_subdirectory(src/vk-bootstrap-variation)
add_subdirectory(src/vk-study-app)