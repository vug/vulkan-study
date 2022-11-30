#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"

#include <memory>

class VerticesStudy : public vku::Study {
 private:
  vku::Buffer vertices;
  vku::Buffer indices;
  uint32_t indexCount;
  std::unique_ptr<vk::raii::Pipeline> pipeline;

 public:
  virtual ~VerticesStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};