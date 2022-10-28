#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class VulkanContext;

  // A GPU Buffer for vertex data
  struct UniformBuffer {
    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DescriptorBufferInfo descriptor;

    UniformBuffer() = default;
    UniformBuffer(const VulkanContext& vc, void* srcData, uint32_t sizeBytes, vk::BufferUsageFlagBits usage);
  };
}