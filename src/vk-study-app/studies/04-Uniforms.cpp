#include "04-Uniforms.hpp"

#include "../vku/SpirvHelper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

struct Vertex {
  float pos[3];
  float col[3];
};

void UniformsStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
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

  vertices = vku::Buffer(vc, vertexBuffer.data(), vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer);
  indices = vku::Buffer(vc, indexBuffer.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer);

  //---- Uniform Data
  // Update matrices
  uniforms.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)800 / (float)800, 0.1f, 256.0f);
  const float zoom{ 2.5f };
  auto vec = glm::vec3(0.0f, 0.0f, zoom);
  uniforms.viewMatrix = glm::translate(glm::mat4(1), vec);
  uniforms.modelMatrix = glm::translate(glm::mat4(1), glm::vec3(0.1, 0.2, 0));

  ubo = vku::UniformBuffer(vc, &uniforms, sizeof(Uniforms));

  //---- Descriptor Set Layout
  vk::DescriptorSetLayoutBinding layoutBinding = { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
  vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(vc.device, { {}, 1, &layoutBinding });

  //---- Descriptor Set
  vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo(*vc.descriptorPool, 1, &(*descriptorSetLayout));
  descriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);


  vk::WriteDescriptorSet writeDescriptorSet;
  // Binding 0 : Uniform buffer
  writeDescriptorSet.dstSet = *descriptorSets[0];
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
  writeDescriptorSet.pBufferInfo = &ubo.descriptor;
  // Binds this uniform buffer to binding point 0
  writeDescriptorSet.dstBinding = 0;

  vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

  //---- Pipeline
  const std::string vertexShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outColor = inColor;
	//gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
  gl_Position = ubo.modelMatrix * vec4(inPos.xyz, 1.0);
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

  //vk::PipelineLayout pipelineLayout = (*vc.device).createPipelineLayout(vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout));
  pipelineLayout = { vc.device, { {}, 1, &(*descriptorSetLayout) } }; // { flags, descriptorSetLayout }

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
  assert(pipeline->getConstructorSuccessCode() == vk::Result::eSuccess);
}

void UniformsStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vertices.buffer, offsets);
  cmdBuf.bindIndexBuffer(*indices.buffer, 0, vk::IndexType::eUint32);
  cmdBuf.drawIndexed(indexCount, 1, 0, 0, 1);
  
  cmdBuf.endRenderPass();
}

void UniformsStudy::onDeinit() { }