#include "01-ClearStudy.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

void ClearStudy::onInit([[maybe_unused]] const vku::AppSettings appSettings, [[maybe_unused]] const vku::VulkanContext& vc) { }

void ClearStudy::onUpdate([[maybe_unused]] float deltaTime, [[maybe_unused]] const vku::Window& win) { }

void ClearStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  // Clearing inside a RenderPass via a vkCmdClearAttachments
  const vk::Rect2D renderArea = { {0,0}, vc.swapchainExtent };
  vk::ClearRect clearRect = vk::ClearRect(renderArea, 0, 1);
  

  const std::array<float, 4> col = { 0.5f, 0.5f, 1.0f, 1.0f };
  vk::ClearAttachment clearColorAttachment = vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0, vk::ClearColorValue{ col });
  cmdBuf.clearAttachments(clearColorAttachment, clearRect);

  if (vc.appSettings.hasPresentDepth) {
    vk::ClearAttachment clearDepthAttachment = vk::ClearAttachment(vk::ImageAspectFlagBits::eDepth, 0, vk::ClearDepthStencilValue{ 1.0f, 0 });
    cmdBuf.clearAttachments(clearDepthAttachment, clearRect);
  }
    
  cmdBuf.endRenderPass();
}

void ClearStudy::onDeinit() { }