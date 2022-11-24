#pragma once

#include "../StudyApp/Study.hpp"

#include "../vku/Buffer.hpp"
#include "../vku/Camera.hpp"
#include "../vku/UniformBuffer.hpp"

#include <glm/mat4x4.hpp>

#include <memory>

class Transform {
 public:
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;

  Transform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& sca);
  Transform(const glm::vec3& pos, const glm::vec3 axis, const float angle, const glm::vec3& sca);

  glm::mat4 getTranslateMatrix() const;
  glm::mat4 getRotationMatrix() const;
  glm::mat4 getScaleMatrix() const;
  glm::mat4 getTransform() const;
};

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

  struct Entity {
    Mesh mesh;
    Transform transform;
    glm::vec4 color;

    PushConstants getPushConstants() const;
  };

 private:
  vku::Buffer vbo;
  vku::Buffer ibo;
  std::vector<Entity> entities;
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