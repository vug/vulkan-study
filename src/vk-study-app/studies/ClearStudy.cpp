#include "ClearStudy.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

void ClearStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) { }

void ClearStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  // Clearing inside a RenderPass via a vkCmdClearAttachments
  const std::array<float, 4> col = { 0.5f, 0.5f, 1.0f, 1.0f };
  vk::ClearAttachment clearAttachment = vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0, vk::ClearColorValue{ col });
  const vk::Rect2D renderArea = { {0,0}, vc.swapchainExtent };
  vk::ClearRect clearRect = vk::ClearRect(renderArea, 0, 1);
  cmdBuf.clearAttachments(clearAttachment, clearRect);

  cmdBuf.endRenderPass();
}

void ClearStudy::onDeinit() { }