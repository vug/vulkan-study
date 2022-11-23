#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class TransformConstructionStudy : public vku::Study {
  struct PushConstants {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
  };

  struct Uniforms {
    glm::mat4 viewFromWorld;
    glm::mat4 projectionFromView;
    glm::mat4 projectionFromWorld;
  };

  struct Mesh {
    const uint32_t offset;
    const uint32_t size;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  std::vector<Mesh> meshes;
  std::vector<PushConstants> pcos;
  uint32_t indexCount;
  Uniforms uniforms;
  vku::UniformBuffer ubo;
  vk::raii::DescriptorSets descriptorSets = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipeline;
  vku::FirstPersonPerspectiveCamera camera;

 public:
  virtual ~TransformConstructionStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(float deltaTime, const vku::Window& win) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};