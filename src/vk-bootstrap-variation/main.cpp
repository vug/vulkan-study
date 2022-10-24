#include "SpirvHelper.hpp"
#include "utils.hpp"
#include "VulkanContext.hpp"
#include "Window.hpp"


#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main() {
  std::cout << "Hello, Vulkan!\n";

  vku::AppSettings appSettings = {
    .name = "vk-bootstrap variation study",
    .hasPresentDepth = false,
  };

  vku::Window window; // Window (GLFW)
  // Instance, Surface, Physical Device, Logical Device, Swapchain, Queues, RenderPass, Framebuffer, CommandBuffer
  vku::VulkanContext vc(window, appSettings);

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

  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo({}, vc.swapchainSamples);

  vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
  vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo({},                           // flags
    true,                         // depthTestEnable
    true,                         // depthWriteEnable
    vk::CompareOp::eLessOrEqual,  // depthCompareOp
    false,                        // depthBoundTestEnable
    false,                        // stencilTestEnable
    stencilOpState,               // front
    stencilOpState                // back
  );

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
    appSettings.hasPresentDepth ? &depthStencilStateCreateInfo : nullptr,
    &colorBlendStateCreateInfo,
    &dynamicStateCreateInfo, // *vk::PipelineDynamicStateCreateInfo
    *pipelineLayout, // vk::PipelineLayout
    *vc.renderPass // vk::RenderPass
    //{}, // uint32_t subpass_ = {},
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

  //---- Synchronization
  // A semaphore is used to add order between queue operations on the GPU
  // Same semaphore is a "signal" semaphors in one queue operation and a "wait" semaphors in another one.
  // Queue Ops-A will signal Semaphore-S when it finishes executing and Queue Ops-B will wait on Semaphore-S before executing
  // Once B starts S returns to "unsignaled" state to be reused again
  std::vector<vk::raii::Semaphore> imageAvailableForRenderingSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  // A fense is used to introduce order on CPU execution. It's usually used for CPU to wait a GPU operation to complete. 
  // A GPU work is submitted with a fence. When GPU work is done fence is signaled. 
  // Fences block the host. Any CPU execution waiting for that fence will stop until the signal arrives.
  std::vector<vk::raii::Fence> commandBufferAvailableFences; // aka commandBufferAvailableFences
  // Note that, having an array of each sync object is to allow recording of one frame while next one is being recorded

  for (int i = 0; i < vc.MAX_FRAMES_IN_FLIGHT; ++i) {
    // (Semaphores begin their lifetime at "unsignaled" state)
    // Image Available -> Semaphore -> Submit Draw Calls for rendering
    imageAvailableForRenderingSemaphores.emplace_back(vc.device, vk::SemaphoreCreateInfo());
    renderFinishedSemaphores.emplace_back(vc.device, vk::SemaphoreCreateInfo());
    // Start the fence in signaled state, so that we won't wait indefinitely for frame=-1 CommandBuffer to be done
    commandBufferAvailableFences.emplace_back(vc.device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
  }

  //---- Main Loop
  size_t currentFrame = 0;
  while (!window.shouldClose()) {
    window.pollEvents();

    // Draw Frame
    vk::Result result = vk::Result::eErrorUnknown;

    // Wait for previous frame's CommandBuffer processing to finish, so that we don't write next image's commands into the same CommandBuffer
    // Maximum int value "disables" timeout. 
    result = vc.device.waitForFences(*commandBufferAvailableFences[currentFrame], true, std::numeric_limits<uint64_t>::max());
    assert(result == vk::Result::eSuccess);

    // Acquire an image available for rendering from the Swapchain, then signal availability (i.e. readiness for executing draw calls)
    uint32_t imageIndex = 0; // index/position of the image in Swapchain
    try {
      std::tie(result, imageIndex) = vc.swapchain.acquireNextImage(std::numeric_limits<uint64_t>::max(), *imageAvailableForRenderingSemaphores[currentFrame]);
    }
    catch (vk::OutOfDateKHRError& e) {
      assert(result == vk::Result::eErrorOutOfDateKHR); // to see whether result gets a wrong value as it happens with presentKHR
      vc.recreateSwapchain();
      continue;
    }
    assert(result == vk::Result::eSuccess); // or vk::Result::eSuboptimalKHR
    assert(imageIndex < vc.swapchain.getImages().size());

    // Record a command buffer which draws the scene into acquired/available image
    vk::raii::CommandBuffer& cmdBuf = vc.commandBuffers[currentFrame];
    {
      // clean up, don't reuse existing commands
      cmdBuf.reset();

      cmdBuf.begin(vk::CommandBufferBeginInfo{});

      const vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f };
      cmdBuf.setViewport(0, viewport);

      vk::Rect2D renderArea{{0,0}, vc.swapchainExtent };
      cmdBuf.setScissor(0, renderArea);

      const vk::raii::Framebuffer& framebuffer = vc.framebuffers[imageIndex];
      const vk::ClearColorValue clearColorValue = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
      const vk::ClearDepthStencilValue clearDepthValue{ 1.f, 0 };
      const std::vector<vk::ClearValue> clearValues = (appSettings.hasPresentDepth) ?
        std::vector<vk::ClearValue>{ clearColorValue } :
        std::vector<vk::ClearValue>{ clearColorValue, clearDepthValue };
      const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *framebuffer, renderArea, clearValues);
      cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
      {
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
        cmdBuf.draw(3, 1, 0, 0); // 3 vertices. their positions and colors are hard-coded in the vertex shader code.
      }
      cmdBuf.endRenderPass();
      cmdBuf.end();
    }

    // Submit recorded command buffer to graphics queue
    // Once previous fence is passed and image is available, submit commands and do graphics calculations, signal finishedSemaphore after execution
    const vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(*imageAvailableForRenderingSemaphores[currentFrame], waitStages, *cmdBuf, *renderFinishedSemaphores[currentFrame]);

    // Fences must be reset manually to go back into unsignaled state.
    // Reset fence only just before when we are submitting the queue (not immediately after we waited for it at the beginning of the frame drawing)
    // otherwise an early return from "Out of Date" acquired image might keep the fence in unsignaled state eternally
    vc.device.resetFences(*commandBufferAvailableFences[currentFrame]);
    // Submit recorded CommanBuffer. Signal fence indicating we are done with this CommandBuffer.
    vc.graphicsQueue.submit(submitInfo, *commandBufferAvailableFences[currentFrame]);

    // Waits for finishedSemaphore before execution, then Present the Swapchain image, no signal thereafter
    vk::PresentInfoKHR presentInfo(*renderFinishedSemaphores[currentFrame], *vc.swapchain, imageIndex);
    try {
      result = vc.presentQueue.presentKHR(presentInfo);
    }
    catch (vk::OutOfDateKHRError& e) {
      // for some reason, even though "out of date" exception was thrown result is still vk::eSuccess.
      // Setting it to correct value manually just in case result will be used below later.
      result = vk::Result::eErrorOutOfDateKHR;
      vc.recreateSwapchain();
      continue;
    }
    assert(result == vk::Result::eSuccess); // or vk::Result::eSuboptimalKHR

    currentFrame = (currentFrame + 1) % vc.MAX_FRAMES_IN_FLIGHT;
  }

  // END
  vc.device.waitIdle();

  vku::spirv::finalize();
  // Need to be destroyed explicitly becomes raii instance does not own it apparently.

  std::cout << "Bye, Vulkan!\n";
  return 0;
}

