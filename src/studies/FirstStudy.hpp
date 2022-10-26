#pragma once

#include "Study.hpp"
#include "VulkanContext.hpp"

#include <memory>

class FirstStudy : public vku::Study {
private:
  // LATER: if more objects to be shared between init() and render() methods, then put all of them into a unique_ptr<RenderData>
  // OR alternatively initialize them in Constructor
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  virtual ~FirstStudy() = default;

  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};