#pragma once

#include "../StudyApp/Study.hpp"

#include <memory>

class SecondStudy : public vku::Study {
 private:
  std::unique_ptr<vk::raii::Pipeline> pipeline;

 public:
  virtual ~SecondStudy() = default;

  inline std::string getName() final { return "Clone of 01-FirstStudy."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  // void onRender(const vku::VulkanContext& vc) final;
  void onDeinit() final;
};