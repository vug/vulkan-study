#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class InstancingStudy : public vku::Study {
  struct InstanceData {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
  };

  struct Uniforms {
    glm::mat4 viewFromWorld;
    glm::mat4 projectionFromView;
    glm::mat4 projectionFromWorld;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  uint32_t indexCount;
  vku::Buffer instanceBuffer;
  uint32_t instanceCount;
  std::vector<Uniforms> uniforms;
  std::vector<vku::UniformBuffer> ubos;
  std::vector<vk::raii::DescriptorSets> descriptorSets;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipeline;
  vku::FirstPersonPerspectiveCamera camera;

 public:
  virtual ~InstancingStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};