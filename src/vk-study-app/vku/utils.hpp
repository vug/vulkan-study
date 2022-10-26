#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vku {
#if !defined( NDEBUG )
  constexpr bool isDebugBuild = true;
#else
  constexpr bool isDebugBuild = false;
#endif

  void setImageLayout(vk::raii::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout);

}