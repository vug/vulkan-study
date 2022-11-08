#include "05-Instanced.hpp"

#include "../vku/Model.hpp"
#include "../vku/SpirvHelper.hpp"
#include "../vku/utils.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vivid/vivid.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <numbers>
#include <random>
#include <string>

void InstancingStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
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

  std::vector<InstanceData> instances;
  instanceCount = 50'000; // quads: (5M, 30FPS), (2.5M, 60FPS). box: (1M, 80FPS), (2M, 40FPS). suzanne: (100K, 30FPS), (50K, 60FPS).
  std::default_random_engine rndGenerator(0); // (unsigned)time(nullptr)
  std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
  std::uniform_real_distribution<float> uniformN11(-1.0f, 1.0f);
  const auto& u1 = [&rndGenerator, &uniform01]() { return uniform01(rndGenerator); };
  const auto& u2 = [&rndGenerator, &uniformN11]() { return uniformN11(rndGenerator); };
  //std::uniform_int_distribution<uint32_t> rndTextureIndex(0, textures.rocks.layerCount);
  for (uint32_t i = 0; i < instanceCount; ++i) {
    const auto& scale = glm::scale(glm::mat4(1), glm::vec3{ 0.5f + 0.3f * u1(), 0.5f + 0.3f * u1(), 0.5f + 0.3f * u1() } * 0.02f);
    const auto& rotate = glm::rotate(glm::mat4(1), 2.f * std::numbers::pi_v<float> * u1(), {u2(), u2(), u2()});
    const auto& translate = glm::translate(glm::mat4(1), { u2(), u2(), u2()});
    const glm::mat4 transform = translate * rotate * scale;
    const glm::mat4 dualTransform = glm::transpose(glm::inverse(transform));
    const glm::vec4 color{ u1(), u1(), u1(), 1 };
    instances.emplace_back(transform, dualTransform, color);
  }
  instanceBuffer = vku::Buffer(vc, instances.data(), static_cast<uint32_t>(instances.size() * sizeof(InstanceData)), vk::BufferUsageFlagBits::eVertexBuffer);

  //---- Uniform Data
  // create UBO and connect it to a Uniforms instance
  ubo = vku::UniformBuffer(vc, &uniforms, sizeof(Uniforms));

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

// Vertex attributes
layout (location = 0) in vec3 inObjectPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inObjectNormal;
layout (location = 3) in vec4 inColor;

// Instanced attributes
layout (location = 4) in mat4 instanceWorldFromObjectMatrix;
layout (location = 8) in mat4 instanceDualWorldFromObjectMatrix;
layout (location = 12) in vec4 instanceColor;

layout (binding = 0) uniform UBO 
{
	mat4 viewFromWorldMatrix;
  mat4 projectionFromViewMatrix;
  mat4 projectionFromWorldMatrix;
} ubo;

layout (location = 0) out struct {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 objectNormal;
    vec4 color;
} v2f;


void main() 
{
  const mat4 transform = instanceWorldFromObjectMatrix; // {ubo.WorldFromObjectMatrix, instanceWorldFromObjectMatrix}
  const vec4 worldPosition4 = transform * vec4(inObjectPosition.xyz, 1.0);
  v2f.worldPosition = worldPosition4.xyz;

  v2f.worldNormal = mat3(instanceDualWorldFromObjectMatrix) * inObjectNormal;

  v2f.objectNormal = inObjectNormal;

  gl_Position = ubo.projectionFromWorldMatrix * worldPosition4;

  v2f.color = instanceColor;
}
)";

  const std::string fragmentShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in struct {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 objectNormal;
    vec4 color;
} v2f;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec3 lightPos = vec3(0, 0, 0);
  vec3 fragToLightDir = normalize(lightPos - v2f.worldPosition); // not light to frag
  float diffuse = max(dot(normalize(v2f.worldNormal), fragToLightDir), 0);  
  
  outFragColor = vec4(v2f.color.xyz, 1) * diffuse;
}
)";

  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")
  };

  vku::VertexInputStateCreateInfo vertexInputStateCreateInfo(
    { 
      { 0, sizeof(vku::DefaultVertex), vk::VertexInputRate::eVertex },
      { 1, sizeof(InstanceData), vk::VertexInputRate::eInstance },
    },
    {
      {
        { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, position) },
        { 1, 0, vk::Format::eR32G32Sfloat, offsetof(vku::DefaultVertex, texCoord) },
        { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, normal) },
        { 3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vku::DefaultVertex, color) },

        { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
        { 5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
        { 6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
        { 7, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
        { 8, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 4 },
        { 9, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 5 },
        { 10, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 6 },
        { 11, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 7 },
        { 12, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 8 },
      }
    }
   );

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

void InstancingStudy::onUpdate(float deltaTime, [[maybe_unused]] const vku::Window& win) {
  static float t = 0.0f;

  camera.yaw = t; // TODO: make yaw private again.
  uniforms.viewFromWorld = camera.getViewFromWorld();
  uniforms.projectionFromView = camera.getProjectionFromView();
  uniforms.projectionFromWorld = uniforms.projectionFromView * uniforms.viewFromWorld;

  ubo.update(); // don't forget to call update after uniform data changes
  t += deltaTime;
}

void InstancingStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vbo.buffer, offsets);
  cmdBuf.bindVertexBuffers(1, *instanceBuffer.buffer, offsets);
  cmdBuf.bindIndexBuffer(*ibo.buffer, 0, vk::IndexType::eUint32);
  cmdBuf.drawIndexed(indexCount, instanceCount, 0, 0, 0);

  cmdBuf.endRenderPass();
}

void InstancingStudy::onDeinit() { }