#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/Math.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class TransformConstructionStudy : public vku::Study {
  struct PushConstants {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
  };

  struct PerFrameUniforms {
    glm::mat4 viewFromWorld;
    glm::mat4 projectionFromView;
    glm::mat4 projectionFromWorld;
  };

  struct Mesh {
    const uint32_t offset;
    const uint32_t size;
  };

  struct Entity {
    Mesh mesh;
    vku::Transform transform;
    glm::vec4 color;

    PushConstants getPushConstants() const;
  };

  struct MeshId {
    static const size_t Box = 0;
    static const size_t Axes = 1;
    static const size_t Monkey = 2;
  };

  struct PerFrameUniformDescriptor {
    vku::UniformBuffer<PerFrameUniforms> ubo;
    vk::raii::DescriptorSets descriptorSets = nullptr;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  std::vector<Mesh> meshes;
  std::vector<Entity> entities;
  uint32_t indexCount;
  std::vector<PerFrameUniformDescriptor> perFrameData;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipeline;
  vku::FirstPersonPerspectiveCamera camera;

 public:
  virtual ~TransformConstructionStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};