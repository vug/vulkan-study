#include "08-Outlines.hpp"

OutlinesViaDepthBuffer::PushConstants OutlinesViaDepthBuffer::Entity::getPushConstants() const {
  PushConstants pc = OutlinesViaDepthBuffer::PushConstants{.worldFromObject = transform.getTransform(), .color = color};
  pc.dualWorldFromObject = glm::transpose(glm::inverse(pc.worldFromObject));
  return pc;
}

void OutlinesViaDepthBuffer::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
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
