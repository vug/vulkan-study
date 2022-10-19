set(APP VulkanHppStudy)
add_executable(${APP}
  main.cpp
  utils.hpp utils.cpp
)

find_package(Vulkan REQUIRED FATAL_ERROR)
message(STATUS "vulkan installation found at: ${Vulkan_INCLUDE_DIRS}")

target_include_directories(${APP}
  PUBLIC ${Vulkan_INCLUDE_DIRS}
)

#target_link_libraries(target ${Vulkan_LIBRARIES})

if(MSVC)
  add_compile_options(/W4) # /WX if warnings should be treated as errors

  # set cwd to ${GAME}/Debug instead of just ${GAME}/
  set_property(TARGET ${GAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()