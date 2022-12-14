#include "08-Outlines.hpp"

#include "../vku/utils.hpp"

namespace vku {
Mesh MeshStore::insertMeshData(const DefaultMeshData& newMesh) {
  Mesh mesh = {static_cast<uint32_t>(allMeshesData.indices.size()), static_cast<uint32_t>(newMesh.indices.size())};
  std::ranges::copy(newMesh.vertices, std::back_inserter(allMeshesData.vertices));
  std::ranges::transform(newMesh.indices, std::back_inserter(allMeshesData.indices), [&](uint32_t ix) { return ix + mesh.offset; });
  return mesh;
}

void MeshStore::upload(const vku::VulkanContext& vc) {
  uint32_t vboSizeBytes = (uint32_t)(allMeshesData.vertices.size() * sizeof(vku::DefaultVertex));
  vertexBuffer = vku::Buffer(vc, allMeshesData.vertices.data(), vboSizeBytes, vk::BufferUsageFlagBits::eVertexBuffer);

  uint32_t iboSizeBytes = (uint32_t)(allMeshesData.indices.size() * sizeof(uint32_t));
  indexBuffer = vku::Buffer(vc, allMeshesData.indices.data(), iboSizeBytes, vk::BufferUsageFlagBits::eIndexBuffer);
}
}  // namespace vku

OutlinesViaDepthBuffer::PushConstants OutlinesViaDepthBuffer::Entity::getPushConstants() const {
  PushConstants pc = OutlinesViaDepthBuffer::PushConstants{.worldFromObject = transform.getTransform(), .color = color};
  pc.dualWorldFromObject = glm::transpose(glm::inverse(pc.worldFromObject));
  return pc;
}

void OutlinesViaDepthBuffer::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  meshes.axes = meshStore.insertMeshData(vku::makeAxes());
  meshes.box = meshStore.insertMeshData(vku::makeBox());
  meshes.monkeyFlat = meshStore.insertMeshData(vku::loadOBJ(vku::assetsRootFolder / "models/suzanne.obj"));
  meshes.monkeySmooth = meshStore.insertMeshData(vku::loadOBJ(vku::assetsRootFolder / "models/suzanne_smooth.obj"));
  meshStore.upload(vc);

  entities.emplace_back(meshes.box, vku::Transform{{-2, 0, 0}, {0, 0, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 0, 0, 1});
  entities.emplace_back(meshes.axes, vku::Transform{{0, 0, 0}, {1, 1, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 1, 1, 1});
}
void OutlinesViaDepthBuffer::onUpdate(const vku::UpdateParams& params) {
}
void OutlinesViaDepthBuffer::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
}
void OutlinesViaDepthBuffer::onDeinit() {
}
void OutlinesViaDepthBuffer::initPipeline(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts) {
}
void OutlinesViaDepthBuffer::initPipelineOutline(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout) {
}
