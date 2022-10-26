#pragma once

#include "Study.hpp"
#include "VulkanContext.hpp"

#include <memory>

class SecondStudy : public vku::Study {
private:
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  virtual ~SecondStudy() = default;

  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  //void onRender(const vku::VulkanContext& vc) final;
  void onDeinit() final;
};