#pragma once
#include "VulkanContext.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace vku {
  class StudyRunner;

  class Study {
  protected:
    const VulkanContext& vc;
    const vku::AppSettings& appSettings;
  public:
    Study(const StudyRunner& studyRunner);
    virtual ~Study() = default;

    virtual void onInit() = 0;
    //virtual void onRender(const VulkanContext& vc = 0;
    virtual void recordCommandBuffer(const vk::raii::CommandBuffer& cmdBuf, const vk::raii::Framebuffer& framebuffer) = 0;
    virtual void onDeinit() = 0;
    // on ImGuiRender, OnUpdate
  };
}