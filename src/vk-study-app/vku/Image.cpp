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

  // taken from https://github.com/KhronosGroup/Vulkan-Hpp/blob/fc63beb5962128c263647375b46693da271e7ceb/RAII_Samples/utils/utils.hpp#L103
  void setImageLayout(
    vk::raii::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout)
  {
    vk::AccessFlags sourceAccessMask;
    switch (oldImageLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal: sourceAccessMask = vk::AccessFlagBits::eTransferWrite; break;
    case vk::ImageLayout::ePreinitialized: sourceAccessMask = vk::AccessFlagBits::eHostWrite; break;
    case vk::ImageLayout::eColorAttachmentOptimal: sourceAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break; // or Read? Not sure, my addition.
    case vk::ImageLayout::eGeneral:  // sourceAccessMask is empty
    case vk::ImageLayout::eUndefined: break;
    default: assert(false); break;
    }

    vk::PipelineStageFlags sourceStage;
    switch (oldImageLayout)
    {
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::ePreinitialized: sourceStage = vk::PipelineStageFlagBits::eHost; break;
    case vk::ImageLayout::eTransferDstOptimal: sourceStage = vk::PipelineStageFlagBits::eTransfer; break;
    case vk::ImageLayout::eUndefined: sourceStage = vk::PipelineStageFlagBits::eTopOfPipe; break;
    case vk::ImageLayout::eColorAttachmentOptimal: sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput; break; // Not sure, my addition.
    default: assert(false); break;
    }

    vk::AccessFlags destinationAccessMask;
    switch (newImageLayout)
    {
    case vk::ImageLayout::eColorAttachmentOptimal: destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      destinationAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      break;
    case vk::ImageLayout::eGeneral:  // empty destinationAccessMask
    case vk::ImageLayout::ePresentSrcKHR: break;
    case vk::ImageLayout::eShaderReadOnlyOptimal: destinationAccessMask = vk::AccessFlagBits::eShaderRead; break;
    case vk::ImageLayout::eTransferSrcOptimal: destinationAccessMask = vk::AccessFlagBits::eTransferRead; break;
    case vk::ImageLayout::eTransferDstOptimal: destinationAccessMask = vk::AccessFlagBits::eTransferWrite; break;
    default: assert(false); break;
    }

    vk::PipelineStageFlags destinationStage;
    switch (newImageLayout)
    {
    case vk::ImageLayout::eColorAttachmentOptimal: destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput; break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal: destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests; break;
    case vk::ImageLayout::eGeneral: destinationStage = vk::PipelineStageFlagBits::eHost; break;
    case vk::ImageLayout::ePresentSrcKHR: destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe; break;
    case vk::ImageLayout::eShaderReadOnlyOptimal: destinationStage = vk::PipelineStageFlagBits::eFragmentShader; break;
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal: destinationStage = vk::PipelineStageFlagBits::eTransfer; break;
    default: assert(false); break;
    }

    vk::ImageAspectFlags aspectMask;
    if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
      aspectMask = vk::ImageAspectFlagBits::eDepth;
      if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
      {
        aspectMask |= vk::ImageAspectFlagBits::eStencil;
      }
    }
    else
    {
      aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, 1, 0, 1);
    vk::ImageMemoryBarrier    imageMemoryBarrier(sourceAccessMask,
      destinationAccessMask,
      oldImageLayout,
      newImageLayout,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      image,
      imageSubresourceRange);
    return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
  }
}