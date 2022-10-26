#pragma once

#include "../StudyApp/Study.hpp"

class ClearStudy : public vku::Study {
private:
public:
  virtual ~ClearStudy() = default;

  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};