#pragma once

#include "Study.hpp"
#include "VulkanContext.hpp"

#include <memory>

class SecondStudy : public vku::Study {
private:
  // LATER: if more objects to be shared between init() and render() methods, then put all of them into a unique_ptr<RenderData>
  // OR alternatively initialize them in Constructor
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  virtual ~SecondStudy() = default;

  void onInit(const vku::VulkanContext& vc, const vku::AppSettings appSettings) final;
  void recordCommandBuffer(const vk::raii::CommandBuffer& cmdBuf) final;
  //void onRender(const vku::VulkanContext& vc) final;
  void onDeinit() final;
};