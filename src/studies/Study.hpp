#pragma once
#include "VulkanContext.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class Study {
  public:
    virtual ~Study() = default;

    virtual void onInit(const VulkanContext& vc, const vku::AppSettings appSettings) = 0;
    //virtual void onRender(const VulkanContext& vc = 0;
    virtual void recordCommandBuffer(const vk::raii::CommandBuffer& cmdBuf, const vk::RenderPassBeginInfo& defaultFullScreenRenderPassBeginInfo) = 0;
    virtual void onDeinit() = 0;
    // on ImGuiRender, OnUpdate
  };
}