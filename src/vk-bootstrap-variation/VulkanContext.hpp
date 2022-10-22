#pragma once
#include "Window.hpp"

#include <VkBootstrap.h>
#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class VulkanContext {
  private:
    vku::Window& window;
    vk::raii::Context context;
  public:
    vk::raii::Instance instance;
  private:
    // For some weird reason these are "deleted" when they stay in the scope of constructInstance and also when they are scope variables
    vkb::InstanceBuilder* vkbInstanceBuilder;
    vkb::Instance* vkbInstance;
  public:
    vk::raii::SurfaceKHR surface;
  private:
    vkb::PhysicalDevice vkbPhysicalDevice;
  public:
    vk::raii::PhysicalDevice physicalDevice;
  private:
    vkb::Device vkbDevice;
  public:
    vk::raii::Device device;
    vk::Format swapchainColorFormat;
    vk::ColorSpaceKHR swapchainColorSpace;
    vk::Extent2D swapchainExtent;
    vk::raii::SwapchainKHR swapchain;
    vk::raii::Queue graphicsQueue;
    vk::raii::Queue presentQueue;
    uint32_t graphicsQueueFamilyIndex;

  private:
    vk::raii::Instance constructInstance();
    vk::raii::PhysicalDevice constructPhysicalDevice();
    vk::raii::Device constructDevice();
    vk::raii::SwapchainKHR constructSwapchain();

  public:
    VulkanContext(Window& window);
    ~VulkanContext();
  };

  struct Image {
    vk::Format format = vk::Format::eUndefined;
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::ImageView imageView = nullptr;

    Image(const VulkanContext& vc, vk::Format format, vk::Extent2D extent, vk::SampleCountFlagBits samples, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageAspectFlagBits aspect);
  };
}