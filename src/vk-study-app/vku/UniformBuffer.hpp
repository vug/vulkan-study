#pragma once

#include "../vku/VulkanContext.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vku {

template <typename T>
class UniformBuffer {
 private:
  void* dstData = nullptr;
  uint32_t sizeBytes = 0;
  vk::Device dev = nullptr;

 public:
  T src{};
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
  vk::DescriptorBufferInfo descriptor;

  UniformBuffer() = default;
  UniformBuffer(const VulkanContext& vc)
      : UniformBuffer<T>(vc, T()) {}

  UniformBuffer(const VulkanContext& vc, const T& data)
      : src(data), sizeBytes(sizeof(T)) {
    vk::MemoryAllocateInfo memAlloc;
    vk::MemoryRequirements memReqs;

    // Create a host-visible (CPU) buffer & memory for uniform data
    buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, sizeBytes, vk::BufferUsageFlagBits::eUniformBuffer));
    memReqs = buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent);  // HostVisible is enough if we unmap here
    memory = vk::raii::DeviceMemory{vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex)};
    dev = *vc.device;
    dev.bindBufferMemory(*buffer, *memory, 0);

    descriptor.buffer = *buffer;
    descriptor.offset = 0;
    descriptor.range = sizeBytes;

    dstData = dev.mapMemory(*memory, 0, memAlloc.allocationSize, vk::MemoryMapFlags());
    memcpy(dstData, &src, sizeBytes);
    // TODO: call unmap at destructor. (adding a destructor makes generates an issue with vk::raii::Buffer's some functions missing)
    // 'vk::raii::Buffer& vk::raii::Buffer::operator=(const vk::raii::Buffer& buf)': function was explicitly deleted
    // dev.unmapMemory(*memory);
  }

  void update() const {
    memcpy(dstData, &src, sizeBytes);
  }
};
}  // namespace vku