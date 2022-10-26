#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class VulkanContext;

  struct Image {
    vk::Format format = vk::Format::eUndefined;
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::raii::ImageView imageView = nullptr;

    // TODO: not sure about needing vc here. Maybe VC can be a friend and only VC can create images?
    Image(const VulkanContext& vc, vk::Format format, vk::Extent2D extent, vk::SampleCountFlagBits samples, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageAspectFlagBits aspect);
  };
}