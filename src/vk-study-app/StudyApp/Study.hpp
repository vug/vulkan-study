#pragma once
#include "../vku/VulkanContext.hpp"
#include "../StudyApp/AppSettings.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace vku {
  struct FrameDrawer;

  class Study {
  public:
    virtual ~Study() = default;

    inline virtual std::string getName() = 0;
    virtual void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) = 0;
    //virtual void onRender(const VulkanContext& vc) = 0;
    virtual void recordCommandBuffer(const VulkanContext& vc, const FrameDrawer& frameDrawer) = 0;
    virtual void onDeinit() = 0;
    // on ImGuiRender, OnUpdate
  };
}