#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class VulkanContext;

  // A GPU Buffer for vertex data
  class UniformBuffer {
  private:
    void* srcData;
    void* dstData;
    uint32_t sizeBytes;

  public:
    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DescriptorBufferInfo descriptor;

    UniformBuffer() = default;
    UniformBuffer(const VulkanContext& vc, void* srcData, uint32_t sizeBytes);
    void update() const;
  };
}