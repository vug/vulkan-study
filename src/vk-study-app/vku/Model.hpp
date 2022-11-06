#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vulkan/vulkan.hpp>

#include <filesystem>

namespace vku {
  struct DefaultVertex {
    glm::vec3 position{ 0, 0, 0 };
    glm::vec2 texCoord{ 0, 0 };
    glm::vec3 normal{ 0, 0, 0 };
    glm::vec4 color{ 1, 1, 1, 1 };
    // tangent
    // custom1 (glm::vec4), custom2 (glm::ivec4)
  };

  struct MeshData {
    std::vector<DefaultVertex> vertices;
    std::vector<uint32_t> indices;
  };

  class VertexInputStateCreateInfo : public vk::PipelineVertexInputStateCreateInfo {
  private:
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

  public:
    VertexInputStateCreateInfo(const std::vector<vk::VertexInputBindingDescription>& bindingDescs, const std::vector<vk::VertexInputAttributeDescription>& attributeDescs);
    VertexInputStateCreateInfo(); // Meaningful default constructor
  };

  MeshData makeQuad(const glm::vec2& dimensions);
  MeshData makeBox(const glm::vec3& dimensions);
  MeshData makeTorus(float outerRadius, uint32_t outerSegments, float innerRadius, uint32_t innerSegments);
  MeshData loadOBJ(const std::filesystem::path& filepath);
}