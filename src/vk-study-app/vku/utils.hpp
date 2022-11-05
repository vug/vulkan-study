#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vku {
#if !defined( NDEBUG )
  constexpr bool isDebugBuild = true;
#else
  constexpr bool isDebugBuild = false;
#endif

  void setImageLayout(vk::raii::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout);

  template <class T>
  inline void hash_combine(std::size_t& s, const T& v)
  {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
  }
}