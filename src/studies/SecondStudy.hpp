#pragma once

#include "Study.hpp"
#include "VulkanContext.hpp"

#include <memory>

namespace vku {
  class StudyRunner;
}

class SecondStudy : public vku::Study {
private:
  // LATER: if more objects to be shared between init() and render() methods, then put all of them into a unique_ptr<RenderData>
  // OR alternatively initialize them in Constructor
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  SecondStudy(const vku::StudyRunner& studyRunner);
  virtual ~SecondStudy() = default;

  void onInit() final;
  void recordCommandBuffer(const vk::raii::CommandBuffer& cmdBuf, const vk::raii::Framebuffer& framebuffer) final;
  //void onRender(const vku::VulkanContext& vc) final;
  void onDeinit() final;
};