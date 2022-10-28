set(APP Studies)

add_executable(${APP}
  main.cpp
  vku/SpirvHelper.hpp vku/SpirvHelper.cpp
  vku/utils.hpp vku/utils.cpp
  vku/VulkanContext.hpp vku/VulkanContext.cpp
  vku/Window.hpp vku/Window.cpp
  vku/Image.hpp vku/Image.cpp
  vku/Buffer.hpp vku/Buffer.cpp
  vku/UniformBuffer.hpp vku/UniformBuffer.cpp
  StudyApp/AppSettings.hpp
  StudyApp/StudyRunner.hpp StudyApp/StudyRunner.cpp
  StudyApp/Study.hpp 
  studies/01-ClearStudy.hpp studies/01-ClearStudy.cpp
  studies/02-FirstStudy.hpp studies/02-FirstStudy.cpp
  studies/02b-SecondStudy.hpp studies/02b-SecondStudy.cpp
  studies/03-Vertices.hpp studies/03-Vertices.cpp
  studies/04-Uniforms.hpp studies/04-Uniforms.cpp
)

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

# One way of finding include directory of a library
get_target_property(glfw_interface_includes glfw INTERFACE_INCLUDE_DIRECTORIES)
message("GLFW include dirs: ${glfw_interface_includes}")

# SPIRV-Tools-opt is not included in FindVulkan module and we need it for runtime Shader compilation.
# Therefore need to target its .lib file manually based.
# The debug version of the lib file comes with "Debuggable Shader API Libraries - 64-bit" component of VulkanSDK installation.
# We need to target correct version based on the CMake config (Debug, Release)
message("CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  set(SPIRV-Tools-Opt_Library $ENV{VULKAN_SDK}/Lib/SPIRV-Tools-optd.lib)
ELSE() # Release OR RelWithDebInfo
  set(SPIRV-Tools-Opt_Library $ENV{VULKAN_SDK}/Lib/SPIRV-Tools-opt.lib)
ENDIF()
message("SPIRV-Tools-Opt_Library set to: ${SPIRV-Tools-Opt_Library}")

target_link_libraries(${APP} PRIVATE
	glfw
  glm::glm
	vk-bootstrap::vk-bootstrap
  Vulkan::Vulkan # Makes Vulkan headers such as <vulkan/vulkan.hpp> or <vulkan/vulkan_raii.hpp> available
  Vulkan::glslang
  Vulkan::SPIRV-Tools
  ${SPIRV-Tools-Opt_Library}
  # believe that this is https://github.com/KhronosGroup/glslang/blob/980ac50813fb567b6af6b89282eae850b328c967/StandAlone/ResourceLimits.cpp
  # where DefaultTBuiltInResource is defined
  # but there is no header file for it in VulkanSDK distribution, hence I cannot use it directly and need to copy-paste that logic
  #glslang-default-resource-limits
  # $ENV{VULKAN_SDK}/Lib/glslang-default-resource-limits.lib # No corresponding header file :-(
)

# No need to include library headers explicitly after targeting it above, if it was properly setup by the library author
target_include_directories(${APP} PRIVATE 
  # There must have been a bug, until I've added include dir below Visual Studio was not able to locate glfw.h in the IDE (purple wiggly lines). But then it was fixed by itself.
  #$<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_features(${APP} PRIVATE cxx_std_23)

if(MSVC)
  add_compile_options(/W4) # /WX if warnings should be treated as errors

  # set cwd to ${APP}/Debug instead of just ${APP}/
  set_property(TARGET ${APP} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()