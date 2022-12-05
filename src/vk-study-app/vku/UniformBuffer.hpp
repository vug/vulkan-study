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
  }

  void update() const {
    memcpy(dstData, &src, sizeBytes);
  }

  void unmap() {
    // I don't think that unmap needs to be called at destruction
    dev.unmapMemory(*memory);
  }
};

template <typename TUniformStruct>
class UniformDescriptor {
 public:
  vku::UniformBuffer<TUniformStruct> ubo;
  vk::raii::DescriptorSets descriptorSets = nullptr;

  UniformDescriptor(const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout, uint32_t binding)
      : ubo(vku::UniformBuffer<TUniformStruct>(vc)) {
    vk::DescriptorSetAllocateInfo allocateInfo{*vc.descriptorPool, 1, &(*descriptorSetLayout)};
    descriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);

    // Binding 0 : Uniform buffer
    vk::WriteDescriptorSet writeDescriptorSet;  // connects indiviudal concrete uniform buffer to descriptor set with the abstract layout that can refer to it
    writeDescriptorSet.dstSet = *(descriptorSets[0]);
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptorSet.pBufferInfo = &ubo.descriptor;
    writeDescriptorSet.dstBinding = binding;
    vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);
  }

  TUniformStruct& getStructRef() {
    return ubo.src;
  }

  // call after changing struct members to upload it to GPU memory
  void upload() {
    ubo.update();
  }
};

}  // namespace vku