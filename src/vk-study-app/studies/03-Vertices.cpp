#include "03-Vertices.hpp"

#include "../vku/SpirvHelper.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

//void prepareVertices() {
//  void* data;
//
//  // Index buffer
//  vk::BufferCreateInfo indexbufferInfo;
//  indexbufferInfo.size = indexBufferSize;
//  indexbufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
//  // Copy index data to a buffer visible to the host (staging buffer)
//  stagingBuffers.indices.buffer = device.createBuffer(indexbufferInfo);
//  memReqs = device.getBufferMemoryRequirements(stagingBuffers.indices.buffer);
//  memAlloc.allocationSize = memReqs.size;
//  memAlloc.memoryTypeIndex = context.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
//  stagingBuffers.indices.memory = device.allocateMemory(memAlloc);
//  data = device.mapMemory(stagingBuffers.indices.memory, 0, indexBufferSize, vk::MemoryMapFlags());
//  memcpy(data, indexBuffer.data(), indexBufferSize);
//  device.unmapMemory(stagingBuffers.indices.memory);
//  device.bindBufferMemory(stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);
//
//  // Create destination buffer with device only visibility
//  indexbufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
//  indices.buffer = device.createBuffer(indexbufferInfo);
//  memReqs = device.getBufferMemoryRequirements(indices.buffer);
//  memAlloc.allocationSize = memReqs.size;
//  memAlloc.memoryTypeIndex = context.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
//  indices.memory = device.allocateMemory(memAlloc);
//  device.bindBufferMemory(indices.buffer, indices.memory, 0);
//
//  vk::CommandBufferBeginInfo cmdBufferBeginInfo;
//  vk::BufferCopy copyRegion;
//
//  // Put buffer region copies into command buffer
//  // Note that the staging buffer must not be deleted before the copies
//  // have been submitted and executed
//  copyCommandBuffer.begin(cmdBufferBeginInfo);
//
//  // Vertex buffer
//  copyRegion.size = vertexBufferSize;
//  copyCommandBuffer.copyBuffer(stagingBuffers.vertices.buffer, vertices.buffer, copyRegion);
//  // Index buffer
//  copyRegion.size = indexBufferSize;
//  copyCommandBuffer.copyBuffer(stagingBuffers.indices.buffer, indices.buffer, copyRegion);
//  copyCommandBuffer.end();
//
//  // Submit copies to the queue
//  vk::SubmitInfo copySubmitInfo;
//  copySubmitInfo.commandBufferCount = 1;
//  copySubmitInfo.pCommandBuffers = &copyCommandBuffer;
//
//  vk::Fence nullFence;
//  queue.submit(copySubmitInfo, nullFence);
//  queue.waitIdle();
//
//  device.freeCommandBuffers(cmdPool, copyCommandBuffer);
//
//  // Destroy staging buffers
//  device.destroyBuffer(stagingBuffers.vertices.buffer);
//  device.freeMemory(stagingBuffers.vertices.memory);
//  device.destroyBuffer(stagingBuffers.indices.buffer);
//  device.freeMemory(stagingBuffers.indices.memory);
//
//  // Binding description
//  bindingDescriptions.resize(1);
//  bindingDescriptions[0].binding = 0;
//  bindingDescriptions[0].stride = sizeof(Vertex);
//  bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;
//
//  // Attribute descriptions
//  // Describes memory layout and shader attribute locations
//  attributeDescriptions.resize(2);
//  // Location 0 : Position
//  attributeDescriptions[0].binding = 0;
//  attributeDescriptions[0].location = 0;
//  attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
//  attributeDescriptions[0].offset = 0;
//  // Location 1 : Color
//  attributeDescriptions[1].binding = 0;
//  attributeDescriptions[1].location = 1;
//  attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
//  attributeDescriptions[1].offset = sizeof(float) * 3;
//
//  // Assign to vertex input state
//  inputState.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
//  inputState.pVertexBindingDescriptions = bindingDescriptions.data();
//  inputState.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
//  inputState.pVertexAttributeDescriptions = attributeDescriptions.data();
//}

void VerticesStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  //---- Vertex Data
  vertexBuffer = { { { 1.0f, 1.0f, 0.0f },
                                         { 1.0f,
                                           0.0f,
                                           0.0f } },
                                       { { -1.0f, 1.0f, 0.0f },
                                         { 0.0f,
                                           1.0f,
                                           0.0f } },
                                       { { 0.0f, -1.0f, 0.0f },
                                         { 0.0f,
                                           0.0f,
                                           1.0f } } };

  uint32_t vertexBufferSize = (uint32_t)(vertexBuffer.size() * sizeof(Vertex));

  indexBuffer = { 0, 1, 2 };

  uint32_t indexBufferSize = (uint32_t)(indexBuffer.size() * sizeof(uint32_t));
  uint32_t indexCount = (uint32_t)indexBuffer.size();

  //---- CommandBuffer for copying
  // vk::Buffer copies are done on the queue, so we need a command buffer for them
  auto buffers = vk::raii::CommandBuffers(vc.device, vk::CommandBufferAllocateInfo(*vc.commandPool, vk::CommandBufferLevel::ePrimary, 1));
  copyCmdBuf = std::make_unique<vk::raii::CommandBuffer>(std::move(buffers[0]));

  //---- Vertex Data Upload via Staging buffers
  // Static data like vertex and index buffer should be stored on the device memory
  // for optimal (and fastest) access by the GPU
  //
  // To achieve this we use so-called "staging buffers" :
  // - Create a buffer that's visible to the host (and can be mapped)
  // - Copy the data to this buffer
  // - Create another buffer that's local on the device (VRAM) with the same size
  // - Copy the data from the host to the device using a command buffer
  // - Delete the host visible (staging) buffer
  // - Use the device local buffers for rendering
  struct {
    Buffer vertices;
    Buffer indices;
  } stagingBuffers;

  Buffer vertices;
  Buffer indices;

  vk::MemoryAllocateInfo memAlloc;
  vk::MemoryRequirements memReqs;

  // Vertex buffer
  // Create a host-visible buffer to copy the vertex data to (staging buffer)
  stagingBuffers.vertices.buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc));
  memReqs = stagingBuffers.vertices.buffer.getMemoryRequirements();
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
  stagingBuffers.vertices.memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };

  //// Map and copy
  const vk::Device& dev = *vc.device;
  void* data = dev.mapMemory(*stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, vk::MemoryMapFlags());
  memcpy(data, vertexBuffer.data(), vertexBufferSize);
  dev.unmapMemory(*stagingBuffers.vertices.memory);
  dev.bindBufferMemory(*stagingBuffers.vertices.buffer, *stagingBuffers.vertices.memory, 0);

  // Create the destination buffer with device only visibility
  // vk::Buffer will be used as a vertex buffer and is the copy destination
  vertices.buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst));
  memReqs = vertices.buffer.getMemoryRequirements();
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
  vertices.memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };
  dev.bindBufferMemory(*vertices.buffer, *vertices.memory, 0);

  copyCmdBuf->begin(vk::CommandBufferBeginInfo());
  vk::BufferCopy copyRegion;
  copyRegion.setSize(vertexBufferSize);
  copyCmdBuf->copyBuffer(*stagingBuffers.vertices.buffer, *vertices.buffer, copyRegion);
  copyCmdBuf->end();

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

  pipeline = std::make_unique<vk::raii::Pipeline>(vc.device, nullptr, graphicsPipelineCreateInfo);
  switch (pipeline->getConstructorSuccessCode())
  {
  case vk::Result::eSuccess:
    std::cout << "Pipeline compilation successful\n";
    break;
  case vk::Result::ePipelineCompileRequiredEXT:
    std::cerr << "Pipeline compilation failed(?)";
    break;
  default: std::unreachable();
  }

}
void VerticesStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);
  cmdBuf.draw(3, 1, 0, 0); // 3 vertices. their positions and colors are hard-coded in the vertex shader code.

  cmdBuf.endRenderPass();
}

void VerticesStudy::onDeinit() { }