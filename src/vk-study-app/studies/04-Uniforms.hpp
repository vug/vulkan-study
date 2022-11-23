#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class UniformsStudy : public vku::Study {
  struct Uniforms {
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  uint32_t indexCount;
  Uniforms uniforms;
  vku::UniformBuffer ubo;
  vk::raii::DescriptorSets descriptorSets = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipeline;

 public:
  virtual ~UniformsStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(float deltaTime, const vku::Window& win) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};