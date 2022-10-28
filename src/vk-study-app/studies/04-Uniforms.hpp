#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"

#include <memory>

class UniformsStudy : public vku::Study {
private:
  vku::Buffer vertices;
  vku::Buffer indices;
  uint32_t indexCount;
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  virtual ~UniformsStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};