#pragma once
#include "../StudyApp/AppSettings.hpp"
#include "../vku/Window.hpp"

#include <VkBootstrap.h>
#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <vector>

namespace vku {
struct Image;

struct FrameDrawer {
  const vk::raii::CommandBuffer& commandBuffer;
  const uint32_t imageIndex;  // if kept as a reference (i.e. uint32_t&) its value changes when command buffer methods are called
  const vk::Image image;      // same as above
  const uint32_t frameNo;
  const vk::raii::Framebuffer& framebuffer;
};

class VulkanContext {
 public:
  // TODO: make it tunable with its default being "surface capabilities -> minimum image count + 1
  const uint32_t NUM_IMAGES = 3;
  // Values larger than 2 might cause latency. More frames will be recorded than being presented by GPU.
  const int MAX_FRAMES_IN_FLIGHT = 2;
  AppSettings appSettings;

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
  const vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

 private:
  vkb::Device vkbDevice;

 public:
  vk::raii::Device device;
  vk::Format swapchainColorFormat;
  vk::ColorSpaceKHR swapchainColorSpace;
  vk::Format swapchainDepthFormat;
  vk::SampleCountFlagBits swapchainSamples;
  vk::Extent2D swapchainExtent;
  vk::raii::SwapchainKHR swapchain;
  std::vector<vku::Image> depthImages;
  std::vector<vk::raii::ImageView> swapchainImageViews;
  vk::raii::Queue graphicsQueue;
  vk::raii::Queue presentQueue;
  vk::raii::Queue computeQueue;
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  uint32_t computeQueueFamilyIndex;
  vk::raii::RenderPass renderPass;
  std::vector<vk::raii::Framebuffer> framebuffers;
  vk::raii::CommandPool commandPool;
  vk::raii::CommandBuffers commandBuffers;
  // for copying buffers from host to device etc
  vk::raii::CommandBuffer copyCommandBuffer;
  vk::raii::DescriptorPool descriptorPool;

 private:
  //---- Synchronization
  // A semaphore is used to add order between queue operations on the GPU
  // Same semaphore is a "signal" semaphors in one queue operation and a "wait" semaphors in another one.
  // Queue Ops-A will signal Semaphore-S when it finishes executing and Queue Ops-B will wait on Semaphore-S before executing
  // Once B starts S returns to "unsignaled" state to be reused again
  std::vector<vk::raii::Semaphore> imageAvailableForRenderingSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  // A fense is used to introduce order on CPU execution. It's usually used for CPU to wait a GPU operation to complete.
  // A GPU work is submitted with a fence. When GPU work is done fence is signaled.
  // Fences block the host. Any CPU execution waiting for that fence will stop until the signal arrives.
  std::vector<vk::raii::Fence> commandBufferAvailableFences;  // aka commandBufferAvailableFences
  // Note that, having an array of each sync object is to allow recording of one frame while next one is being recorded
 public:
  uint32_t currentFrame = 0;

 private:
  vk::raii::Instance constructInstance();
  vk::raii::PhysicalDevice constructPhysicalDevice();
  vk::raii::Device constructDevice();
  vk::raii::SwapchainKHR constructSwapchain();
  std::vector<vk::raii::ImageView> constructSwapchainImageViews();
  vk::raii::RenderPass constructRenderPass();
  std::vector<vk::raii::Framebuffer> constructFramebuffers();
  vk::raii::DescriptorPool constructDescriptorPool();
  // To be called when app window is resized
  void recreateSwapchain();

 public:
  VulkanContext(Window& window, const AppSettings& appSettings = {});
  ~VulkanContext();

  FrameDrawer drawFrameBegin();
  void drawFrameEnd(const FrameDrawer& frameDrawer);

  // utilities
  uint32_t getMemoryType(uint32_t requirementTypeBits, const vk::MemoryPropertyFlags& flags) const;
};
}  // namespace vku