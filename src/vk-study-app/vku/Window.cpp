#include "Window.hpp"

#include <iostream>

namespace vku {
  Window::Window(const AppSettings& appSettings) {
    glfwInit();
    glfwSetErrorCallback([](int error, const char* msg) { std::cerr << "GLFW ERROR: " << "(" << error << ") " << msg << std::endl; });
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window.reset(glfwCreateWindow(appSettings.width, appSettings.height, appSettings.name.c_str(), nullptr, nullptr));
  }

  Window::~Window() {
    window.release(); // need to explicitly delete, otherwise the unique_ptr delete will be called after glfwTerminate
    glfwTerminate();
  }

  vk::raii::SurfaceKHR Window::createSurface(const vk::raii::Instance& instance) const {
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(*instance, window.get(), nullptr, &vkSurface);
    return vk::raii::SurfaceKHR{ instance, vkSurface };
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

  glm::vec2 Window::getSize() const {
    int width, height;
    glfwGetWindowSize(window.get(), &width, &height);
    return { width, height };
  }

  bool Window::isKeyHeld(int glfwKey) const {
    const int state = glfwGetKey(window.get(), glfwKey);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }

  bool Window::isMouseButtonPressed(int glfwButton) const {
    const int state = glfwGetMouseButton(window.get(), glfwButton);
    return state == GLFW_PRESS;
  }

  glm::vec2 Window::getMouseCursorPosition() const {
    double xpos, ypos;
    glfwGetCursorPos(window.get(), &xpos, &ypos);
    return glm::vec2{ static_cast<float>(xpos), static_cast<float>(ypos) };
  }

  DragHelper::DragHelper(const Window& win, int glfwMouseButton, std::function<void()> onEnterDraggingCallback, std::function<void(const glm::vec2& drag)> onBeingDraggedCallback) :
    win(win), glfwMouseButton(glfwMouseButton), onEnterDraggingCallback(onEnterDraggingCallback), onBeingDraggedCallback(onBeingDraggedCallback) {}

  void DragHelper::checkDragging() {
    if (win.isMouseButtonPressed(glfwMouseButton)) {
      // enter dragging
      if (!isBeingDragged) {
        isBeingDragged = true;
        cursor0 = win.getMouseCursorPosition();
        onEnterDraggingCallback(); // for storing values at the beginning
        isBeingPressed = true;
      }
      // being dragged
      else {
        const glm::vec2 drag = win.getMouseCursorPosition() - cursor0;
        onBeingDraggedCallback(drag); // for updating values while mouse is being dragged
      }
    }
    // exit dragging
    else if (isBeingPressed) {
      isBeingDragged = false;
      isBeingPressed = false;
      // onExitDraggingCallback(); should come here if every needed
    }
  }
}