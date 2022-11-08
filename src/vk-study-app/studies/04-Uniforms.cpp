#include "04-Uniforms.hpp"

#include "../vku/Model.hpp"
#include "../vku/SpirvHelper.hpp"
#include "../vku/utils.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vivid/vivid.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

void UniformsStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  std::cout << vivid::ansi::lightBlue << "Hi from Vivid at UniformsStudy" << vivid::ansi::reset << std::endl;
  //---- Vertex Data
  const vivid::ColorMap cmap = vivid::ColorMap::Preset::Viridis;
  vku::MeshData boxMeshData = vku::makeBox({ 0.6f, 0.9f, 1.5f });
  vku::MeshData torusMeshData = vku::makeTorus(1.f, 17, .5f, 6);
  vku::MeshData quadMeshData = vku::makeQuad({ 1, 1 });
  // TODO: do not hard-code absolute paths, instead have a global "assets root folder"
  vku::MeshData objMeshData = vku::loadOBJ(vku::assetsRootFolder / "models/suzanne.obj");
  
  vku::MeshData& md = objMeshData;

  uint32_t vboSizeBytes = (uint32_t)(md.vertices.size() * sizeof(vku::DefaultVertex));
  vbo = vku::Buffer(vc, md.vertices.data(), vboSizeBytes, vk::BufferUsageFlagBits::eVertexBuffer);

  uint32_t iboSizeBytes = (uint32_t)(md.indices.size() * sizeof(uint32_t));
  indexCount = (uint32_t)md.indices.size();
  ibo = vku::Buffer(vc, md.indices.data(), iboSizeBytes, vk::BufferUsageFlagBits::eIndexBuffer);

  //---- Uniform Data
  // create UBO and connect it to a Uniforms instance
  ubo = vku::UniformBuffer(vc, &uniforms, sizeof(Uniforms));
  uniforms.projectionMatrix = glm::perspective(glm::radians(45.0f), (float)800 / (float)800, 0.1f, 256.0f);
  // will set the view and model matrices at every frame

  //---- Descriptor Set Layout
  vk::DescriptorSetLayoutBinding layoutBinding = { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
  vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(vc.device, { {}, 1, &layoutBinding });

  //---- Descriptor Set
  vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo(*vc.descriptorPool, 1, &(*descriptorSetLayout));
  descriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);

  // Binding 0 : Uniform buffer
  vk::WriteDescriptorSet writeDescriptorSet;
  writeDescriptorSet.dstSet = *descriptorSets[0];
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
  writeDescriptorSet.pBufferInfo = &ubo.descriptor;
  writeDescriptorSet.dstBinding = 0;

  vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

  //---- Pipeline
  const std::string vertexShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec4 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outColor = inColor;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}
)";

  const std::string fragmentShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = inColor;
}
)";

  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")
  };

  vku::VertexInputStateCreateInfo vertexInputStateCreateInfo; // default one without arguments

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

  std::array<vk::Viewport, 1> viewports = { vk::Viewport{0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f} };
  std::array<vk::Rect2D, 1> scissors = { vk::Rect2D{vk::Offset2D{0, 0}, vc.swapchainExtent} };
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, viewports, scissors);

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({}, // flags
    false,                        // depthClampEnable
    false,                        // rasterizerDiscardEnable
    vk::PolygonMode::eFill,       // polygonMode
    vk::CullModeFlagBits::eNone,  // cullMode {eBack}
    vk::FrontFace::eCounterClockwise,    // frontFace
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

void UniformsStudy::onUpdate([[maybe_unused]] float deltaTime, [[maybe_unused]] const vku::Window& win) { }

void UniformsStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  static float t = 0.0f;
  const glm::vec3 up = { 0, 1, 0 };
  const float r{ 5.0f };
  uniforms.viewMatrix = glm::lookAt(glm::vec3(r * std::cos(t), 0, r * std::sin(t)), glm::vec3(0, 0, 0), up);
  t += 0.001f;

  uniforms.modelMatrix = glm::translate(glm::mat4(1), glm::vec3(0, std::sin(t) * 0.5f, 0));
  ubo.update(); // don't forget to call update after uniform data changes

  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vbo.buffer, offsets);
  cmdBuf.bindIndexBuffer(*ibo.buffer, 0, vk::IndexType::eUint32);
  cmdBuf.drawIndexed(indexCount, 1, 0, 0, 1);

  cmdBuf.endRenderPass();
}

void UniformsStudy::onDeinit() { }