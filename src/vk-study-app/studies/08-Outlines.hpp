#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/Math.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>

class OutlinesViaDepthBuffer : public vku::Study {
  struct MeshId {
    static const size_t Box = 0;
    static const size_t Axes = 1;
    static const size_t Monkey = 2;
  };

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

  // struct Material {DescriptorSet textureSet, Pipeline, PipelineLayout  };

  struct PerFrameUniform {
    glm::vec4 time;
    glm::vec4 lightPos;
  };

  struct PerPassUniform {
    glm::vec4 cameraPositionWorld;
    glm::mat4 viewFromWorld;
    glm::mat4 projectionFromView;
    glm::mat4 projectionFromWorld;
  };

  struct MaterialUniform {
    glm::vec4 specularParams{5.0f, 0, 0, 0};  // x: specularExponent/smoothness
    glm::vec4 goochCool{0.1f, 0.2f, 0.9f, 1.0f};
    glm::vec4 goochWarm{0.9f, 0.2f, 0.1f, 1.0f};
    glm::ivec4 shouldUseGooch{0, 0, 0, 0};
  };

 private:
  vku::Buffer vertexBuffer;
  vku::Buffer indexBuffer;
  std::vector<Mesh> meshes;
  std::vector<Entity> entities;
  uint32_t indexCount;
  //
  std::vector<vku::UniformBuffer<PerFrameUniform>> perFrameUniform;
  std::vector<vku::UniformBuffer<PerPassUniform>> perPassUniform;
  std::vector<vku::UniformBuffer<MaterialUniform>> perMaterialUniform;
  std::vector<vk::raii::DescriptorSets> descriptorSets;
  // for layout that's common to every pipeline (per frame and per pass data)
  vk::raii::PipelineLayout pipelineLayoutPerFrameAndPass = nullptr;
  // for rendering entities
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipeline;
  // for outlines
  vk::raii::PipelineLayout pipelineLayoutOutline = nullptr;
  std::unique_ptr<vk::raii::Pipeline> pipelineOutline;

 public:
  virtual ~OutlinesViaDepthBuffer() = default;

  inline std::string getName() final { return "Drawing object outlines using gradients of depth buffer."; }
  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void onUpdate(const vku::UpdateParams& params) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;

 private:
  void initPipeline(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts);
  void initPipelineOutline(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout);
};