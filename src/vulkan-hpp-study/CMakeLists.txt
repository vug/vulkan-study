set(APP VulkanHppStudy)
add_executable(${APP}
  main.cpp
)

if(MSVC)
  add_compile_options(/W4) # /WX if warnings should be treated as errors

  # set cwd to ${GAME}/Debug instead of just ${GAME}/
  set_property(TARGET ${GAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
else()
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()