#include "SpirvHelper.hpp"
#include "utils.hpp"
#include "VulkanContext.hpp"
#include "Window.hpp"


#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << "Hello, Vulkan!\n";

  vku::Window window; // Window (GLFW)
  // Instance, Surface, Physical Device, Logical Device, Swapchain, Queues
  vku::VulkanContext vc(window);

  //---- RenderPass
  if (false)
  {
    std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
    attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
      vc.swapchainColorFormat,
      vk::SampleCountFlagBits::e1, // TODO: try MSAA
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR);
    attachmentDescriptions[1] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
      vk::Format::eD16Unorm, // TODO: what other options are there for depth?
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);
    vk::raii::RenderPass renderPass{ vc.device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };
  }
  std::array<vk::AttachmentDescription, 1> attachmentDescriptions;
  attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
    vc.swapchainColorFormat,
    vk::SampleCountFlagBits::e1, // TODO: try MSAA
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::ePresentSrcKHR);
  vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
  vk::SubpassDescription subpass(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, nullptr);
  vk::raii::RenderPass renderPass{ vc.device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };

  //---- Pipeline
  const std::string vertexShaderStr = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](vec2 (0.0, -0.5), vec2 (0.5, 0.5), vec2 (-0.5, 0.5));

vec3 colors[3] = vec3[](vec3 (1.0, 0.0, 0.0), vec3 (0.0, 1.0, 0.0), vec3 (0.0, 0.0, 1.0));

void main ()
{
	gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}
)";

  const std::string fragmentShaderStr = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragColor;

layout (location = 0) out vec4 outColor;

void main () { outColor = vec4 (fragColor, 1.0); }
)";

  vku::spirv::init();
  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")
  };

  // No vertexBindingDescription and no vertexAttributeDescriptions for this example
  vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo({}, {}, {});

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

  std::array<vk::Viewport, 1> viewports = { vk::Viewport{0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f} };
  std::array<vk::Rect2D, 1> scissors = { vk::Rect2D{vk::Offset2D{0, 0}, vc.swapchainExtent} };
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, viewports, scissors);

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({}, // flags
    false,                        // depthClampEnable
    false,                        // rasterizerDiscardEnable
    vk::PolygonMode::eFill,       // polygonMode
    vk::CullModeFlagBits::eBack,  // cullMode
    vk::FrontFace::eClockwise,    // frontFace
    false,                        // depthBiasEnable
    0.0f,                         // depthBiasConstantFactor
    0.0f,                         // depthBiasClamp
    0.0f,                         // depthBiasSlopeFactor
    1.0f                          // lineWidth
  );

  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

  // Not used in vk-bootstrap example actually.
  //vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
  //vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo({},                           // flags
  //  true,                         // depthTestEnable
  //  true,                         // depthWriteEnable
  //  vk::CompareOp::eLessOrEqual,  // depthCompareOp
  //  false,                        // depthBoundTestEnable
  //  false,                        // stencilTestEnable
  //  stencilOpState,               // front
  //  stencilOpState                // back
  //);

  vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(false, // blendEnable
    vk::BlendFactor::eZero,  // srcColorBlendFactor, defaults...
    vk::BlendFactor::eZero,  // dstColorBlendFactor
    vk::BlendOp::eAdd,       // colorBlendOp
    vk::BlendFactor::eZero,  // srcAlphaBlendFactor
    vk::BlendFactor::eZero,  // dstAlphaBlendFactor
    vk::BlendOp::eAdd,       // alphaBlendOp
    colorComponentFlags      // colorWriteMask
  );
  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo({},                                 // flags
    false,                              // logicOpEnable
    vk::LogicOp::eNoOp,                 // logicOp
    colorBlendAttachmentState,  // attachments
    { { 0.f, 0.f, 0.f, 0.f } }      // blendConstants
  );

  std::array<vk::DynamicState, 2> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
  vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo({}, dynamicStates);

  vk::raii::PipelineLayout pipelineLayout(vc.device, { {}, {} }); // { flags, descriptorSetLayout }

  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
    {},
    shaderStageCreateInfos,
    &vertexInputStateCreateInfo,
    &inputAssemblyStateCreateInfo,
    nullptr, // *vk::PipelineTessellationStateCreateInfo
    &viewportStateCreateInfo,
    &rasterizationStateCreateInfo,
    &multisampleStateCreateInfo,
    nullptr, // *vk::PipelineDepthStencilStateCreateInfo
    &colorBlendStateCreateInfo,
    &dynamicStateCreateInfo, // *vk::PipelineDynamicStateCreateInfo
    *pipelineLayout, // vk::PipelineLayout
    *renderPass // vk::RenderPass
    //{}, // uint32_t subpass_ = {},
    //{}, // VULKAN_HPP_NAMESPACE::Pipeline basePipelineHandle_ = {},
    //{}, // int32_t basePipelineIndex_ = {},
    //nullptr // const void* pNext
  );
  vk::raii::Pipeline pipeline(vc.device, nullptr, graphicsPipelineCreateInfo);
  switch (pipeline.getConstructorSuccessCode())
  {
  case vk::Result::eSuccess:
    std::cout << "Pipeline compilation successful\n";
    break;
  case vk::Result::ePipelineCompileRequiredEXT:
    std::cerr << "Pipeline compilation failed(?)";
    break;
  default: std::unreachable();
  }

  //---- Framebuffer
  const std::vector<VkImage>& swapchainImages = vc.swapchain.getImages();

  // vkbSwapchain.get_image_views() is actually not a getter but creator. Instead let's create imageViews ourselves
  std::vector<vk::raii::ImageView> swapchainImageViews;
  for (const VkImage& img : swapchainImages) {
    const vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity };
    const vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    vk::ImageViewCreateInfo imageViewCreateInfo({}, img, vk::ImageViewType::e2D, vc.swapchainColorFormat, components, imageSubresourceRange);
    swapchainImageViews.emplace_back(vc.device, imageViewCreateInfo);
  }
    
  std::vector<vk::raii::Framebuffer> framebuffers;
  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    std::array<vk::ImageView, 1> attachments = { *swapchainImageViews[i] };
    vk::FramebufferCreateInfo framebufferCreateInfo({}, *renderPass, attachments, vc.swapchainExtent.width, vc.swapchainExtent.height, 1);
    framebuffers.push_back(vk::raii::Framebuffer(vc.device, framebufferCreateInfo));
  }

  //---- CommandBuffer
  vk::CommandPoolCreateInfo commandPoolCreateInfo({}, vc.graphicsQueueFamilyIndex);
  vk::raii::CommandPool commandPool(vc.device, commandPoolCreateInfo);
 
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 3);
  vk::raii::CommandBuffers commandBuffers(vc.device, commandBufferAllocateInfo);

  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    vk::raii::CommandBuffer& cmdBuf = commandBuffers[i];

    vk::CommandBufferBeginInfo cmdBufBeginInfo{};
    vk::raii::Framebuffer& framebuffer = framebuffers[i];
    vk::Rect2D renderArea = { {0,0}, vc.swapchainExtent };
    vk::ClearColorValue clearColorValue = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<vk::ClearValue, 1> clearColor = { clearColorValue };
    vk::RenderPassBeginInfo renderPassBeginInfo(*renderPass, *framebuffer, renderArea, clearColor);

    cmdBuf.begin(cmdBufBeginInfo);
    //std::array<vk::Viewport, 1> viewports = { vk::Viewport{ 0.f, 0.f, static_cast<float>(vkbSwapchain.extent.width), static_cast<float>(vkbSwapchain.extent.height), 0.f, 1.f } };
    //std::array<vk::Rect2D, 1> scissors = { vk::Rect2D{ vk::Offset2D{0, 0}, vkbSwapchain.extent } };
    //cmdBuf.setViewport(0, viewports);
    //cmdBuf.setScissor(0, scissors);
    cmdBuf.setViewport(0, vk::Viewport{ 0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f });
    cmdBuf.setScissor(0, vk::Rect2D{ vk::Offset2D{0, 0}, vc.swapchainExtent });
    cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    cmdBuf.draw(3, 1, 0, 0); // 3 vertices. their positions and colors are hard-coded in the vertex shader code.
    cmdBuf.endRenderPass();
    cmdBuf.end();
  }

  //---- Synchronization
  const int MAX_FRAMES_IN_FLIGHT = 2;
  std::vector<vk::raii::Semaphore> availableSemaphores;
  std::vector<vk::raii::Semaphore> finishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;
  std::vector<vk::raii::Fence*> imageInFlight;
  imageInFlight.resize(swapchainImages.size(), nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    availableSemaphores.emplace_back(vc.device, vk::SemaphoreCreateInfo());
    finishedSemaphores.emplace_back(vc.device, vk::SemaphoreCreateInfo());
    inFlightFences.emplace_back(vc.device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
  }

  //---- Main Loop
  size_t currentFrame = 0;
  while (!window.shouldClose()) {
    window.pollEvents();

    // Draw Frame
    vk::Result result;
    uint32_t imageIndex = 0;

    result = vc.device.waitForFences(*inFlightFences[currentFrame], true, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
    std::tie(result, imageIndex) = vc.swapchain.acquireNextImage(UINT64_MAX, *availableSemaphores[currentFrame]);
    assert(result == vk::Result::eSuccess || result == vk::Result::eErrorOutOfDateKHR);
    assert(imageIndex < vc.swapchain.getImages().size());
    //if (result == vk::Result::eErrorOutOfDateKHR) recreate_swapchain(...);

    if (imageInFlight[imageIndex]) {
      result = vc.device.waitForFences(**imageInFlight[imageIndex], true, UINT64_MAX);
      assert(result == vk::Result::eSuccess);
    }
    imageInFlight[imageIndex] = &inFlightFences[currentFrame];

    vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(*availableSemaphores[currentFrame], waitStages, *commandBuffers[imageIndex], *finishedSemaphores[currentFrame]);
    vc.device.resetFences(*inFlightFences[currentFrame]);
    vc.graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

    vk::PresentInfoKHR presentInfo(*finishedSemaphores[currentFrame], *vc.swapchain, imageIndex);
    result = vc.presentQueue.presentKHR(presentInfo);
    //if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) recreateSwapChain();
    //else assert(result == vk::Result::eSuccess);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  // END
  vc.device.waitIdle();

  vku::spirv::finalize();
  // Need to be destroyed explicitly becomes raii instance does not own it apparently.

  std::cout << "Bye, Vulkan!\n";
  return 0;
}

