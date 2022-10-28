#include "UniformBuffer.hpp"

#include "VulkanContext.hpp"

namespace vku {
  UniformBuffer::UniformBuffer(const VulkanContext& vc, void* srcData, uint32_t sizeBytes) :
    srcData(srcData), sizeBytes(sizeBytes) {
    vk::MemoryAllocateInfo memAlloc;
    vk::MemoryRequirements memReqs;

    // Create a host-visible (CPU) buffer & memory for uniform data
    buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, sizeBytes, vk::BufferUsageFlagBits::eUniformBuffer));
    memReqs = buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent); // HostVisible is enough if we unmap here
    memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };
    const vk::Device& dev = *vc.device;
    dev.bindBufferMemory(*buffer, *memory, 0);

    descriptor.buffer = *buffer;
    descriptor.offset = 0;
    descriptor.range = sizeBytes;

    dstData = dev.mapMemory(*memory, 0, memAlloc.allocationSize, vk::MemoryMapFlags());
    memcpy(dstData, srcData, sizeBytes);
    //dev.unmapMemory(*memory);
  }

  void UniformBuffer::update() const {
    memcpy(dstData, srcData, sizeBytes);
  }
}