#include "Model.hpp"

#include <vulkan/vulkan.hpp>

#include <array>

namespace vku {
  std::vector<DefaultVertex> makeBox(const glm::vec3& dimensions) {
    const glm::vec3 halfDim = dimensions * 0.5f;
    const float width = halfDim.x, height = halfDim.y, depth = halfDim.z;

    // corners
    struct PartialVertex { glm::vec3 position; glm::vec4 color; };
    const PartialVertex p000 = { { -width, -height, -depth }, {0.0, 0.0, 0.0, 1.0} };
    const PartialVertex p001 = { { -width, -height, +depth }, {0.0, 0.0, 1.0, 1.0} };
    const PartialVertex p010 = { { -width, +height, -depth }, {0.0, 1.0, 0.0, 1.0} };
    const PartialVertex p011 = { { -width, +height, +depth }, {0.0, 1.0, 1.0, 1.0} };
    const PartialVertex p100 = { { +width, -height, -depth }, {1.0, 0.0, 0.0, 1.0} };
    const PartialVertex p101 = { { +width, -height, +depth }, {1.0, 0.0, 1.0, 1.0} };
    const PartialVertex p110 = { { +width, +height, -depth }, {1.0, 1.0, 0.0, 1.0} };
    const PartialVertex p111 = { { +width, +height, +depth }, {1.0, 1.0, 1.0, 1.0} };

    // normals
    const glm::vec3 nFront = { 0.0f, 0.0f, 1.0f };
    const glm::vec3 nBack = -nFront;
    const glm::vec3 nLeft = { 1.0f, 0.0, 0.0 };
    const glm::vec3 nRight = -nLeft;
    const glm::vec3 nUp = { 0.0f, 1.0f, 0.0f };
    const glm::vec3 nDown = -nUp;

    // faces (four corners in CCW, 1 normal)
    struct Face { std::array<PartialVertex, 4> corners; glm::vec3 normal; };
    const Face fBack = { { p010, p110, p100, p000, }, nBack };
    const Face fFront = { { p001, p101, p111, p011, }, nFront };
    const Face fLeft = { { p110, p111, p101, p100, }, nLeft };
    const Face fRight = { { p000, p001, p011, p010, }, nRight };
    const Face fUp = { { p010, p011, p111, p110 }, nUp };
    const Face fDown = { { p100, p101, p001, p000 }, nDown };

    std::vector<DefaultVertex> vertices;
    const int indices[] = { 0, 1, 2,    // triangle 1 of quad
                            0, 2, 3, }; // triangle 2 of quad
    const glm::vec2 uvs[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
    const Face faces[] = { fBack, fFront, fLeft, fRight, fUp, fDown };
    for (auto& face : faces) {
      for (int ix : indices) {
        const PartialVertex& pv = face.corners[ix];
        vertices.emplace_back(pv.position, uvs[ix], face.normal, pv.color);
      }
    }
    return vertices;
  }

  VertexAttributesInfo::VertexAttributesInfo(std::vector<vk::VertexInputBindingDescription>& bindingDescs, std::vector<vk::VertexInputAttributeDescription>& attrDescs) : 
    bindingDescriptions(bindingDescs), 
    attributeDescriptions(attrDescs), 
    createInfo({}, bindingDescriptions, attributeDescriptions) {}

  VertexAttributesInfo getVertexAttributesInfo() {
    std::vector<vk::VertexInputBindingDescription> bindingDescs = { { 0, sizeof(vku::DefaultVertex), vk::VertexInputRate::eVertex } };
    std::vector<vk::VertexInputAttributeDescription> attrDescs = {
      { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(DefaultVertex, position) },
      { 1, 0, vk::Format::eR32G32Sfloat, offsetof(DefaultVertex, texCoord) },
      { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(DefaultVertex, normal) },
      { 3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(DefaultVertex, color) },
    };
    return VertexAttributesInfo{bindingDescs, attrDescs};
  }
}