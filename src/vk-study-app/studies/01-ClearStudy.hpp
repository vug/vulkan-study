#pragma once

#include "../StudyApp/Study.hpp"

class ClearStudy : public vku::Study {
private:
public:
  virtual ~ClearStudy() = default;

  inline std::string getName() final { return "Just a VkCmdClearAttachments call in a RenderPass"; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};