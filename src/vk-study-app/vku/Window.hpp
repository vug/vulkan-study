#pragma once
#include "../StudyApp/AppSettings.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <string>
#include <vector>

namespace vku {
  class Window {
  private:
    std::unique_ptr < GLFWwindow, decltype([](GLFWwindow* win) { glfwDestroyWindow(win); }) > window;
  public:
    Window(const AppSettings& appSettings = {});
    ~Window();
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance) const;
    std::vector<std::string> getRequiredInstanceExtensions() const;
    bool shouldClose() const;
    void pollEvents() const;
    // TODO: Make friends with ImGuiHelper
    GLFWwindow* getGLFWWindow() const { return window.get(); }
  };
}
