#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vulkan/vulkan.hpp>

namespace vku {
  struct DefaultVertex {
    glm::vec3 position{ 0, 0, 0 };
    glm::vec2 texCoord{ 0, 0 };
    glm::vec3 normal{ 0, 0, 0 };
    glm::vec4 color{ 1, 1, 1, 1 };
    // tangent
    // custom1 (glm::vec4), custom2 (glm::ivec4)
  };

  class VertexAttributesInfo {
  private:
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
  public:
    vk::PipelineVertexInputStateCreateInfo createInfo;
    
    VertexAttributesInfo(std::vector<vk::VertexInputBindingDescription>& bindingDescs, std::vector<vk::VertexInputAttributeDescription>& attrDescs);
  };

  std::vector<DefaultVertex> makeBox(const glm::vec3&);
  std::vector<DefaultVertex> makeTorus(float outerRadius, int outerSegments, float innerRadius, int innerSegments);

  VertexAttributesInfo getVertexAttributesInfo();
}