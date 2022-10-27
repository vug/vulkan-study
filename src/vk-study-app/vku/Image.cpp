#include "Image.hpp"

#include "VulkanContext.hpp"

namespace vku {
  Image::Image(const VulkanContext& vc, vk::Format format, vk::Extent2D extent, vk::SampleCountFlagBits samples, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageAspectFlagBits aspect) :
    format(format),
    image([&]() {
    vk::ImageCreateInfo imageCreateInfo({}, vk::ImageType::e2D, format, vk::Extent3D(extent, 1), 1, 1, samples, tiling, usage);
    return vk::raii::Image{ vc.device, imageCreateInfo };
      }()),
    memory([&]() {
        vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
        const uint32_t typeBits = memoryRequirements.memoryTypeBits;
        uint32_t typeIndex = vc.getMemoryType(typeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        assert(typeIndex != uint32_t(~0));
        vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, typeIndex);
        return vk::raii::DeviceMemory{ vc.device, memoryAllocateInfo };
      }()),
        imageView([&]() {
        image.bindMemory(*memory, 0);
        vk::ImageSubresourceRange imageSubresourceRange{ aspect, 0, 1, 0, 1 };
        vk::ImageViewCreateInfo imageViewCreateInfo({}, *image, vk::ImageViewType::e2D, format, {}, imageSubresourceRange);
        return vk::raii::ImageView(vc.device, imageViewCreateInfo);
          }())
  {
  }
}