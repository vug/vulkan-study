#include "Model.hpp"

#include <glm/geometric.hpp>
#include <vulkan/vulkan.hpp>

#include <array>
#include <numbers>
#include <numeric>

namespace vku {
  MeshData makeQuad(const glm::vec2& dimensions) {
    const float halfWidth = .5f * dimensions.x;
    const float halfHeight = .5f * dimensions.y;

    const glm::vec3 normal{ 0, 0, -1 };

    const glm::vec4 red{ 1, 0, 0, 1 };
    const glm::vec4 green{ 0, 1, 0, 1 };
    const glm::vec4 blue{ 0, 0, 1, 1 };
    const glm::vec4 white{ 1, 1, 1, 1 };

    const DefaultVertex topLeft{ .position = {-halfWidth, halfHeight, 0}, .texCoord = {0, 1}, .normal = normal, .color = red };
    const DefaultVertex bottomLeft{ .position = {-halfWidth, -halfHeight, 0}, .texCoord = {0, 0}, .normal = normal, .color = green };
    const DefaultVertex bottomRight{ .position = {halfWidth, -halfHeight, 0}, .texCoord = {1, 0}, .normal = normal, .color = blue };
    const DefaultVertex topRight{ .position = {halfWidth, halfHeight, 0}, .texCoord = {1, 1}, .normal = normal, .color = white };
    return MeshData{
      .vertices = { topLeft, bottomLeft, bottomRight, topRight },
      .indices = {
        0, 1, 2,
        0, 2, 3,
      }
    };
  }

  MeshData makeBox(const glm::vec3& dimensions) {
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

    MeshData meshData;
    const int faceIndices[] = { 0, 1, 2,    // triangle 1 of quad
                                0, 2, 3, }; // triangle 2 of quad
    const glm::vec2 uvs[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
    const Face faces[] = { fBack, fFront, fLeft, fRight, fUp, fDown };
    for (auto& face : faces) {
      for (int ix : faceIndices) {
        const PartialVertex& pv = face.corners[ix];
        meshData.vertices.emplace_back(pv.position, uvs[ix], face.normal, pv.color);
      }
    }

    // Since all 3 vertices at the same corner have different normals we couldn't reuse vertices with given DefaultVertex schema
    meshData.indices.resize(meshData.vertices.size());
    std::iota(meshData.indices.begin(), meshData.indices.end(), 0);
    return meshData;
  }

  MeshData makeTorus(float outerRadius, int outerSegments, float innerRadius, int innerSegments) {
    MeshData meshData;
    for (int i = 0; i < outerSegments; i++) {
      const float u = (float)i / (outerSegments - 1);
      const float outerAngle = 2.f * std::numbers::pi * u;
      const glm::vec3 innerCenter = glm::vec3{ cosf(outerAngle), sinf(outerAngle), 0.0f } * outerRadius;
      for (int j = 0; j < innerSegments; j++) {
        const float v = (float)j / (innerSegments - 1);
        const float innerAngle = 2.f * std::numbers::pi * v;
        const glm::vec3 innerPos = glm::vec3{ cosf(innerAngle) * cosf(outerAngle),  cosf(innerAngle) * sinf(outerAngle), sinf(innerAngle) } *innerRadius;

        const glm::vec3 pos = innerCenter + innerPos;
        const glm::vec3 norm = glm::normalize(innerPos);
        const glm::vec2 uv = { u, v };
        const float pattern = static_cast<float>((i % 2) ^ (j % 2));
        const glm::vec4 col = glm::vec4{ 1.0, 1.0, 0.0, 1.0 } * pattern + glm::vec4{ 0.0, 1.0, 1.0, 1.0 } * (1.0f - pattern);

        meshData.vertices.emplace_back(DefaultVertex{ pos, uv, norm, col });
      }
    }

    for (size_t i = 0; i < outerSegments; i++) {
      for (size_t j = 0; j < innerSegments; j++) {
        const size_t i1 = (i + 1) % outerSegments;
        const size_t j1 = (j + 1) % innerSegments;
        const uint32_t ix1 = i * innerSegments + j;
        const uint32_t ix2 = i * innerSegments + j1;
        const uint32_t ix3 = i1 * innerSegments + j;
        const uint32_t ix4 = i1 * innerSegments + j1;

        meshData.indices.insert(meshData.indices.begin(), { ix3, ix2, ix1 }); // triangle-1
        meshData.indices.insert(meshData.indices.begin(), { ix2, ix3, ix4 }); // triangle-2
      }
    }

    return meshData;
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
