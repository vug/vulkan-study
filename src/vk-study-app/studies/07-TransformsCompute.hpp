#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/Math.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class TransformGPUConstructionStudy : public vku::Study {
  struct PushConstants {
    glm::mat4x4 worldFromObject;
    glm::mat4x4 dualWorldFromObject;
    glm::vec4 color;
  };

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

  struct Mesh {
    uint32_t offset;
    uint32_t size;
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

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  vku::Buffer instanceBuffer;
  std::vector<Mesh> meshes;
  std::vector<Entity> entities;
  uint32_t indexCount;
  Uniforms uniforms;
  vku::UniformBuffer ubo;
  vk::raii::DescriptorSets graphicsDescriptorSets = nullptr;
  vk::raii::PipelineLayout pipelineLayoutPushConstant = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelinePushConstant;
  uint32_t numMonkeyInstances;
  vk::raii::PipelineLayout pipelineLayoutInstance = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelineInstance;
  vku::FirstPersonPerspectiveCamera camera;

 public:
  virtual ~TransformGPUConstructionStudy() = default;

  inline std::string getName() final { return "VertexBuffer upload to GPU, bind to pipeline/shader."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(float deltaTime, const vku::Window& win) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;

 private:
  void initPipelineWithPushConstant(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout);
  void initPipelineWithInstances(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout);
};