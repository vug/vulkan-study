#include "Buffer.hpp"

#include "VulkanContext.hpp"

namespace vku {
  Buffer::Buffer(const VulkanContext& vc, void* srcData, uint32_t sizeBytes, vk::BufferUsageFlagBits usage) {
    //---- Vertex Data Upload via Staging buffers
    // Static data like vertex and index buffer should be stored on the device memory
    // for optimal (and fastest) access by the GPU
    //
    // To achieve this we use so-called "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    vk::MemoryAllocateInfo memAlloc;
    vk::MemoryRequirements memReqs;

    // Create a host-visible (CPU) staging buffer & memory to copy the vertex/index data
    vk::raii::Buffer stagingBuffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, sizeBytes, vk::BufferUsageFlagBits::eTransferSrc));
    memReqs = stagingBuffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
    vk::raii::DeviceMemory stagingMmory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };

    const vk::Device& dev = *vc.device;
    // Map and copy
    void* dstData = dev.mapMemory(*stagingMmory, 0, memAlloc.allocationSize, vk::MemoryMapFlags());
    memcpy(dstData, srcData, sizeBytes);
    dev.unmapMemory(*stagingMmory);
    dev.bindBufferMemory(*stagingBuffer, *stagingMmory, 0);

    // Create the destination buffer with device only visibility (overwrite member variable)
    buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, sizeBytes, usage | vk::BufferUsageFlagBits::eTransferDst));
    memReqs = buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };
    dev.bindBufferMemory(*buffer, *memory, 0);

    // TODO: get copy command buffer from VulkanContext
    // vk::Buffer copies are done on the queue, so we need a command buffer for them
    auto copyCmdBuffers = vk::raii::CommandBuffers(vc.device, vk::CommandBufferAllocateInfo(*vc.commandPool, vk::CommandBufferLevel::ePrimary, 1));
    vk::raii::CommandBuffer copyCmdBuf(std::move(copyCmdBuffers[0]));
    copyCmdBuf.begin(vk::CommandBufferBeginInfo());
    vk::BufferCopy copyRegion;
    copyRegion.setSize(sizeBytes);
    copyCmdBuf.copyBuffer(*stagingBuffer, *buffer, copyRegion);
    copyCmdBuf.end();

    vk::SubmitInfo copySubmitInfo(nullptr, nullptr, *copyCmdBuf, nullptr);
    vc.graphicsQueue.submit(copySubmitInfo, {});
    vc.graphicsQueue.waitIdle();
  }
}