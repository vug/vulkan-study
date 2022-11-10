#pragma once
#include "../StudyApp/AppSettings.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <functional>
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
    // Call once a frame
    void pollEvents() const;
    glm::vec2 getSize() const;
    // TODO: Make friends with ImGuiHelper
    GLFWwindow* getGLFWWindow() const { return window.get(); }

    // Inputs
    bool isKeyHeld(int glfwKey) const;
    bool isMouseButtonPressed(int glfwButton) const;
    glm::vec2 getMouseCursorPosition() const;
  };

  // A state machine that keeps track of mouse dragging input by the given mouse button.
  // calls given callbacks at state changes
  class DragHelper {
  private:
    const Window& win;
    int glfwMouseButton;
    std::function<void()> onEnterDraggingCallback;
    std::function<void(const glm::vec2& drag)> onBeingDraggedCallback;
    bool isBeingDragged{};
    bool isBeingPressed{};
    glm::vec2 cursor0{};
  public:
    DragHelper(const Window& win, int glfwMouseButton, std::function<void()> onEnterDraggingCallback, std::function<void(const glm::vec2& drag)> onBeingDraggedCallback);
    // update function to call every frame
    void checkDragging();
  };
}
