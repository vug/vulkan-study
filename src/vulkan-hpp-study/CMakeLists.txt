set(APP VulkanHppStudy)
add_executable(${APP}
  main.cpp
  utils.hpp utils.cpp
)

# CMake can find VulkanSDK if it was first installed from https://www.lunarg.com/vulkan-sdk/ 
find_package(Vulkan REQUIRED FATAL_ERROR)
message("Vulkan installation/includes found at: ${Vulkan_INCLUDE_DIRS}")
get_target_property(glfw_interface_includes glfw INTERFACE_INCLUDE_DIRECTORIES)
message("GLFW include dirs: ${glfw_interface_includes}")


target_link_libraries(${APP} PRIVATE
	glfw
	#${Vulkan_LIBRARIES}
)

target_include_directories(${APP} PRIVATE 
  # Makes Vulkan headers such as <vulkan/vulkan.hpp> or <vulkan/vulkan_raii.hpp> available
  ${Vulkan_INCLUDE_DIRS}
  # There must have been a bug, until I've added include dir below VS was not able to locate glfw.h in the IDE (purple wiggly lines)
  #$<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_features(${APP} PRIVATE cxx_std_23)

if(MSVC)
  add_compile_options(/W4) # /WX if warnings should be treated as errors

  # set cwd to ${GAME}/Debug instead of just ${GAME}/
  set_property(TARGET ${GAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()