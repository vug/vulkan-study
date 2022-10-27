#include "03-Vertices.hpp"

#include "../vku/SpirvHelper.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

void VerticesStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  //---- Vertex Data
  std::vector<Vertex> vertexBuffer = { { { 1.0f, 1.0f, 0.0f },
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

  std::vector<uint32_t> indexBuffer = { 0, 1, 2 };

  uint32_t indexBufferSize = (uint32_t)(indexBuffer.size() * sizeof(uint32_t));
  indexCount = (uint32_t)indexBuffer.size();

  //---- CommandBuffer for copying
  // vk::Buffer copies are done on the queue, so we need a command buffer for them
  auto copyCmdBuffers = vk::raii::CommandBuffers(vc.device, vk::CommandBufferAllocateInfo(*vc.commandPool, vk::CommandBufferLevel::ePrimary, 1));
  vk::raii::CommandBuffer& copyCmdBuf = copyCmdBuffers[0];

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

  vk::MemoryAllocateInfo memAlloc;
  vk::MemoryRequirements memReqs;

  const vk::Device& dev = *vc.device;
  {
    struct {
      Buffer vertices;
      Buffer indices;
    } stagingBuffers;

    // Vertex buffer
    // Create a host-visible buffer to copy the vertex data to (staging buffer)
    stagingBuffers.vertices.buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc));
    memReqs = stagingBuffers.vertices.buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
    stagingBuffers.vertices.memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };

    // Map and copy
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

    // Index Buffer
    stagingBuffers.indices.buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc));
    memReqs = stagingBuffers.indices.buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible);
    stagingBuffers.indices.memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };

    data = dev.mapMemory(*stagingBuffers.indices.memory, 0, memAlloc.allocationSize, vk::MemoryMapFlags());
    memcpy(data, indexBuffer.data(), vertexBufferSize);
    dev.unmapMemory(*stagingBuffers.indices.memory);
    dev.bindBufferMemory(*stagingBuffers.indices.buffer, *stagingBuffers.indices.memory, 0);

    indices.buffer = vk::raii::Buffer(vc.device, vk::BufferCreateInfo({}, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst));
    memReqs = indices.buffer.getMemoryRequirements();
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = vc.getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    indices.memory = vk::raii::DeviceMemory{ vc.device, vk::MemoryAllocateInfo(memReqs.size, memAlloc.memoryTypeIndex) };
    dev.bindBufferMemory(*indices.buffer, *indices.memory, 0);

    // Command
    copyCmdBuf.begin(vk::CommandBufferBeginInfo());
    vk::BufferCopy copyRegion;

    copyRegion.setSize(vertexBufferSize);
    copyCmdBuf.copyBuffer(*stagingBuffers.vertices.buffer, *vertices.buffer, copyRegion);
    copyRegion.setSize(indexBufferSize);
    copyCmdBuf.copyBuffer(*stagingBuffers.indices.buffer, *indices.buffer, copyRegion);
    copyCmdBuf.end();

    vk::SubmitInfo copySubmitInfo(nullptr, nullptr, *copyCmdBuf, nullptr);
    vc.graphicsQueue.submit(copySubmitInfo, {});
    vc.graphicsQueue.waitIdle();
  } // destroy and free staging buffers?



  //---- Pipeline
  const std::string vertexShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

//layout (binding = 0) uniform UBO 
//{
//	mat4 projectionMatrix;
//	mat4 modelMatrix;
//	mat4 viewMatrix;
//} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outColor = inColor;
	//gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
  gl_Position = vec4(inPos.xyz, 1.0);
}
)";

  const std::string fragmentShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = vec4(inColor, 1.0);
}
)";

  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")
  };

  // No vertexBindingDescription and no vertexAttributeDescriptions for this example
  std::vector<vk::VertexInputBindingDescription> bindingDescs = { { 0, sizeof(Vertex), vk::VertexInputRate::eVertex } };
  std::vector<vk::VertexInputAttributeDescription> attrDescs(2);
  attrDescs[0] = { 0, 0, vk::Format::eR32G32B32Sfloat, 0 };
  attrDescs[1] = { 1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3 };
  vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo({}, bindingDescs, attrDescs);

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

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vertices.buffer, offsets);
  cmdBuf.bindIndexBuffer(*indices.buffer, 0, vk::IndexType::eUint32);
  cmdBuf.drawIndexed(indexCount, 1, 0, 0, 1);
  
  cmdBuf.endRenderPass();
}

void VerticesStudy::onDeinit() { }