#pragma once
#include "VulkanContext.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class FrameDrawer;

  class Study {
  public:
    virtual ~Study() = default;

    virtual void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) = 0;
    //virtual void onRender(const VulkanContext& vc) = 0;
    virtual void recordCommandBuffer(const VulkanContext& vc, const FrameDrawer& frameDrawer) = 0;
    virtual void onDeinit() = 0;
    // on ImGuiRender, OnUpdate
  };
}