#include "Window.hpp"

#include <iostream>

namespace vku {
Window::Window(const AppSettings& appSettings) {
  glfwInit();
  glfwSetErrorCallback([](int error, const char* msg) { std::cerr << "GLFW ERROR: "
                                                                  << "(" << error << ") " << msg << std::endl; });
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window.reset(glfwCreateWindow(appSettings.width, appSettings.height, appSettings.name.c_str(), nullptr, nullptr));
}

Window::~Window() {
  window.release();  // need to explicitly delete, otherwise the unique_ptr delete will be called after glfwTerminate
  glfwTerminate();
}

vk::raii::SurfaceKHR Window::createSurface(const vk::raii::Instance& instance) const {
  VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
  glfwCreateWindowSurface(*instance, window.get(), nullptr, &vkSurface);
  return vk::raii::SurfaceKHR{instance, vkSurface};
}

std::vector<std::string> Window::getRequiredInstanceExtensions() const {
  uint32_t count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  std::vector<std::string> instanceExtensions;
  for (size_t ix = 0; ix < count; ++ix)
    instanceExtensions.push_back(extensions[ix]);
  return instanceExtensions;
}

bool Window::shouldClose() const {
  return glfwWindowShouldClose(window.get());
}

void Window::pollEvents() const {
  glfwPollEvents();
}
}  // namespace vku