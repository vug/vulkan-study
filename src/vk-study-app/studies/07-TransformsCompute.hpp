#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/Math.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class TransformGPUConstructionStudy : public vku::Study {
  struct Mesh {
    uint32_t offset;
    uint32_t size;
  };

  struct PushConstants {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
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

  struct InstanceData {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
  };

  struct ComputeUniforms {
    glm::vec4 targetPosition;
    glm::vec4 maxAngleToTurn;
    glm::ivec4 shouldTurnInstantly;
  };

  struct PerFrameUniform {
    glm::vec4 cameraPositionWorld;
    glm::mat4 viewFromWorld;
    glm::mat4 projectionFromView;
    glm::mat4 projectionFromWorld;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  vku::Buffer instanceBuffer;
  vku::Buffer transformBuffer;
  std::vector<Mesh> meshes;
  std::vector<Entity> entities;
  uint32_t indexCount;
  //
  std::vector<vku::UniformBuffer<PerFrameUniform>> perFrameUniform;
  std::vector<vk::raii::DescriptorSets> descriptorSetsGraphics;
  //
  vku::UniformBuffer<ComputeUniforms> computeUniformBuffer;
  vk::raii::DescriptorSets computeDescriptorSets = nullptr;
  // for rendering entities
  vk::raii::PipelineLayout pipelineLayoutPushConstant = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelinePushConstant;
  // for rendering monkey instances
  uint32_t numMonkeyInstances;
  vk::raii::PipelineLayout pipelineLayoutInstance = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelineInstance;
  // for computing monkey transforms
  vk::raii::PipelineLayout pipelineLayoutCompute = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelineCompute;
  vku::FirstPersonPerspectiveCamera camera;

 public:
  virtual ~TransformGPUConstructionStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;

 private:
  void initPipelineWithPushConstant(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts);
  void initPipelineWithInstances(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts);
  void initPipelineWithCompute(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout);
};