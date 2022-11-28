#include "07-TransformsCompute.hpp"

#include "../vku/Model.hpp"
#include "../vku/SpirvHelper.hpp"
#include "../vku/utils.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <imgui.h>
#include <vivid/vivid.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <numbers>
#include <random>
#include <ranges>
#include <string>

TransformGPUConstructionStudy::PushConstants TransformGPUConstructionStudy::Entity::getPushConstants() const {
  PushConstants pc = TransformGPUConstructionStudy::PushConstants{.worldFromObject = transform.getTransform(), .color = color};
  pc.dualWorldFromObject = glm::transpose(glm::inverse(pc.worldFromObject));
  return pc;
}

void TransformGPUConstructionStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  std::cout << vivid::ansi::lightBlue << "Hi from Vivid at UniformsStudy" << vivid::ansi::reset << std::endl;

  //---- Vertex Data
  const vivid::ColorMap cmap = vivid::ColorMap::Preset::Viridis;
  uint32_t instanceBufferSize{};
  {
    vku::MeshData allMeshesData;
    auto insertMeshData = [&](const vku::MeshData& newMesh) -> Mesh {
      Mesh mesh = {static_cast<uint32_t>(allMeshesData.indices.size()), static_cast<uint32_t>(newMesh.indices.size())};
      std::ranges::copy(newMesh.vertices, std::back_inserter(allMeshesData.vertices));
      std::ranges::transform(newMesh.indices, std::back_inserter(allMeshesData.indices), [&](uint32_t ix) { return ix + mesh.offset; });
      return mesh;
    };

    meshes.resize(3);
    meshes[MeshId::Box] = insertMeshData(vku::makeBox({0.2f, 0.5f, 0.7f}));
    meshes[MeshId::Axes] = insertMeshData(vku::makeAxes());
    meshes[MeshId::Monkey] = insertMeshData(vku::loadOBJ(vku::assetsRootFolder / "models/suzanne.obj"));

    entities.emplace_back(meshes[MeshId::Box], vku::Transform{{-2, 0, 0}, {0, 0, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 0, 0, 1});
    entities.emplace_back(meshes[MeshId::Axes], vku::Transform{{0, 0, 0}, {1, 1, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 1, 1, 1});

    numMonkeyInstances = 10;
    std::vector<InstanceData> monkeyInstances(numMonkeyInstances);
    const float pi = std::numbers::pi_v<float>;
    for (int i = 0; i < numMonkeyInstances; ++i) {
      const auto transform = vku::Transform{
          glm::vec3{std::cos(i * 2.0f * pi / numMonkeyInstances), 0, std::sin(i * 2.0f * pi / numMonkeyInstances)} * 3.0f,
          {},
          0,
          glm::vec3{1, 1, 1} * 0.75f};
      const glm::mat4 model = transform.getTransform();
      monkeyInstances[i] = InstanceData{
          .worldFromObject = model,
          .dualWorldFromObject = glm::transpose(glm::inverse(model)),
          .color = glm::vec4{0, 0, 1, 1},
      };
    }
    instanceBufferSize = static_cast<uint32_t>(monkeyInstances.size() * sizeof(InstanceData));
    instanceBuffer = vku::Buffer(vc, monkeyInstances.data(),
                                 instanceBufferSize,
                                 vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

    uint32_t vboSizeBytes = (uint32_t)(allMeshesData.vertices.size() * sizeof(vku::DefaultVertex));
    vbo = vku::Buffer(vc, allMeshesData.vertices.data(), vboSizeBytes, vk::BufferUsageFlagBits::eVertexBuffer);

    uint32_t iboSizeBytes = (uint32_t)(allMeshesData.indices.size() * sizeof(uint32_t));
    indexCount = (uint32_t)allMeshesData.indices.size();
    ibo = vku::Buffer(vc, allMeshesData.indices.data(), iboSizeBytes, vk::BufferUsageFlagBits::eIndexBuffer);
  }

  //---- Uniform Data
  // create UBO and connect it to a Uniforms instance
  entityUniformBuffer = vku::UniformBuffer(vc, &entityUniforms, sizeof(EntityUniforms));
  computeUniformBuffer = vku::UniformBuffer(vc, &computeUniforms, sizeof(ComputeUniforms));

  //---- Descriptor Set - Graphics
  {
    const std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings = {
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}};
    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, layoutBindings);
    const vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(vc.device, descriptorSetLayoutCreateInfo);
    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo(*vc.descriptorPool, 1, &(*descriptorSetLayout));
    graphicsDescriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);

    // Binding 0 : Uniform buffer
    vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.dstSet = *graphicsDescriptorSets[0];
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptorSet.pBufferInfo = &entityUniformBuffer.descriptor;
    writeDescriptorSet.dstBinding = 0;

    vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);
    initPipelineWithPushConstant(appSettings, vc, descriptorSetLayout);
    initPipelineWithInstances(appSettings, vc, descriptorSetLayout);
  }

  //---- Descriptor Set - Compute
  {
    // Define only one resource (a Storage Buffer) to be bound at binding point 0 and be used at a compute shader
    // layout(std140, binding = 0) buffer buf;
    const std::array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = {
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute}};
    const vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(
        vc.device, vk::DescriptorSetLayoutCreateInfo({}, layoutBindings));
    // Allocate descriptor set from the pool
    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo(*vc.descriptorPool, 1, &(*descriptorSetLayout));
    computeDescriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);

    // Binding 0 : Storage Buffer
    vk::DescriptorBufferInfo descriptorBufferInfoStorage(*instanceBuffer.buffer, 0, instanceBufferSize);
    vk::WriteDescriptorSet writeDescriptorSetStorage;
    writeDescriptorSetStorage.dstSet = *computeDescriptorSets[0];
    writeDescriptorSetStorage.descriptorCount = 1;
    writeDescriptorSetStorage.descriptorType = vk::DescriptorType::eStorageBuffer;
    writeDescriptorSetStorage.pBufferInfo = &descriptorBufferInfoStorage;
    writeDescriptorSetStorage.dstBinding = 0;
    vc.device.updateDescriptorSets(writeDescriptorSetStorage, nullptr);

    // Binding 1 : Uniform Buffer
    vk::WriteDescriptorSet writeDescriptorSetUniform;
    writeDescriptorSetUniform.dstSet = *computeDescriptorSets[0];
    writeDescriptorSetUniform.descriptorCount = 1;
    writeDescriptorSetUniform.descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptorSetUniform.pBufferInfo = &computeUniformBuffer.descriptor;
    writeDescriptorSetUniform.dstBinding = 1;
    vc.device.updateDescriptorSets(writeDescriptorSetUniform, nullptr);

    initPipelineWithCompute(appSettings, vc, descriptorSetLayout);
  }
}

void TransformGPUConstructionStudy::initPipelineWithPushConstant(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout) {
  const std::string vertexShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Vertex attributes
layout (location = 0) in vec3 inObjectPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inObjectNormal;
layout (location = 3) in vec4 inColor;

layout(push_constant) uniform PushConstants
{
	mat4 worldFromObjectMatrix;
	mat4 dualWorldFromObjectMatrix;
  vec4 color;
} pushConstants;


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
  const mat4 transform = pushConstants.worldFromObjectMatrix;
  const vec4 worldPosition4 = transform * vec4(inObjectPosition.xyz, 1.0);
  v2f.worldPosition = worldPosition4.xyz;

  v2f.worldNormal = mat3(pushConstants.dualWorldFromObjectMatrix) * inObjectNormal;

  v2f.objectNormal = inObjectNormal;

  gl_Position = ubo.projectionFromWorldMatrix * worldPosition4;

  //v2f.color = pushConstants.color;
  //v2f.color = inColor;
  v2f.color = inColor * pushConstants.color;
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
  vec3 lightPos = vec3(0, 10, 0);
  vec3 fragToLightDir = normalize(lightPos - v2f.worldPosition); // not light to frag
  float diffuse = max(dot(normalize(v2f.worldNormal), fragToLightDir), 0);  
  
  outFragColor = vec4(v2f.color.xyz, 1) * diffuse; // lit
  //outFragColor = vec4(v2f.color.xyz, 1); // unlit
}
)";

  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")};

  vku::VertexInputStateCreateInfo vertexInputStateCreateInfo(
      {
          {0, sizeof(vku::DefaultVertex), vk::VertexInputRate::eVertex},
      },
      {{
          {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, position)},
          {1, 0, vk::Format::eR32G32Sfloat, offsetof(vku::DefaultVertex, texCoord)},
          {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, normal)},
          {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vku::DefaultVertex, color)},
      }});

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

  std::array<vk::Viewport, 1> viewports = {vk::Viewport{0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f}};
  std::array<vk::Rect2D, 1> scissors = {vk::Rect2D{vk::Offset2D{0, 0}, vc.swapchainExtent}};
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, viewports, scissors);

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({},                                // flags
                                                                        false,                             // depthClampEnable
                                                                        false,                             // rasterizerDiscardEnable
                                                                        vk::PolygonMode::eFill,            // polygonMode
                                                                        vk::CullModeFlagBits::eNone,       // cullMode {eBack}
                                                                        vk::FrontFace::eCounterClockwise,  // frontFace
                                                                        false,                             // depthBiasEnable
                                                                        0.0f,                              // depthBiasConstantFactor
                                                                        0.0f,                              // depthBiasClamp
                                                                        0.0f,                              // depthBiasSlopeFactor
                                                                        1.0f                               // lineWidth
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
  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(false,                   // blendEnable
                                                                  vk::BlendFactor::eZero,  // srcColorBlendFactor, defaults...
                                                                  vk::BlendFactor::eZero,  // dstColorBlendFactor
                                                                  vk::BlendOp::eAdd,       // colorBlendOp
                                                                  vk::BlendFactor::eZero,  // srcAlphaBlendFactor
                                                                  vk::BlendFactor::eZero,  // dstAlphaBlendFactor
                                                                  vk::BlendOp::eAdd,       // alphaBlendOp
                                                                  colorComponentFlags      // colorWriteMask
  );
  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo({},                         // flags
                                                                  false,                      // logicOpEnable
                                                                  vk::LogicOp::eNoOp,         // logicOp
                                                                  colorBlendAttachmentState,  // attachments
                                                                  {{0.f, 0.f, 0.f, 0.f}}      // blendConstants
  );

  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo({}, dynamicStates);

  vk::PushConstantRange pushConstant{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)};
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  pipelineLayoutCreateInfo.setSetLayouts(*descriptorSetLayout);
  pipelineLayoutCreateInfo.setPushConstantRanges(pushConstant);
  pipelineLayoutPushConstant = {vc.device, pipelineLayoutCreateInfo};  // { flags, descriptorSetLayout }

  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      {},
      shaderStageCreateInfos,
      &vertexInputStateCreateInfo,
      &inputAssemblyStateCreateInfo,
      nullptr,  // *vk::PipelineTessellationStateCreateInfo
      &viewportStateCreateInfo,
      &rasterizationStateCreateInfo,
      &multisampleStateCreateInfo,
      appSettings.hasPresentDepth ? &depthStencilStateCreateInfo : nullptr,
      &colorBlendStateCreateInfo,
      &dynamicStateCreateInfo,      // *vk::PipelineDynamicStateCreateInfo
      *pipelineLayoutPushConstant,  // vk::PipelineLayout
      *vc.renderPass                // vk::RenderPass
                                    //{}, // uint32_t subpass_ = {},
  );

  pipelinePushConstant = std::make_unique<vk::raii::Pipeline>(vc.device, nullptr, graphicsPipelineCreateInfo);
  assert(pipelinePushConstant->getConstructorSuccessCode() == vk::Result::eSuccess);
}

void TransformGPUConstructionStudy::initPipelineWithInstances(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout) {
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
  //v2f.color = inColor;
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
  
  outFragColor = vec4(v2f.color.xyz, 1) * diffuse; // lit
  //outFragColor = vec4(v2f.color.xyz, 1); // unlit
}
)";

  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
      vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")};

  vku::VertexInputStateCreateInfo vertexInputStateCreateInfo(
      {
          {0, sizeof(vku::DefaultVertex), vk::VertexInputRate::eVertex},
          {1, sizeof(InstanceData), vk::VertexInputRate::eInstance},
      },
      {{
          {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, position)},
          {1, 0, vk::Format::eR32G32Sfloat, offsetof(vku::DefaultVertex, texCoord)},
          {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(vku::DefaultVertex, normal)},
          {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(vku::DefaultVertex, color)},

          {4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0},
          {5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1},
          {6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2},
          {7, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3},
          {8, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 4},
          {9, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 5},
          {10, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 6},
          {11, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 7},
          {12, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 8},
      }});

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

  std::array<vk::Viewport, 1> viewports = {vk::Viewport{0.f, 0.f, static_cast<float>(vc.swapchainExtent.width), static_cast<float>(vc.swapchainExtent.height), 0.f, 1.f}};
  std::array<vk::Rect2D, 1> scissors = {vk::Rect2D{vk::Offset2D{0, 0}, vc.swapchainExtent}};
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, viewports, scissors);

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({},                                // flags
                                                                        false,                             // depthClampEnable
                                                                        false,                             // rasterizerDiscardEnable
                                                                        vk::PolygonMode::eFill,            // polygonMode
                                                                        vk::CullModeFlagBits::eNone,       // cullMode {eBack}
                                                                        vk::FrontFace::eCounterClockwise,  // frontFace
                                                                        false,                             // depthBiasEnable
                                                                        0.0f,                              // depthBiasConstantFactor
                                                                        0.0f,                              // depthBiasClamp
                                                                        0.0f,                              // depthBiasSlopeFactor
                                                                        1.0f                               // lineWidth
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
  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(false,                   // blendEnable
                                                                  vk::BlendFactor::eZero,  // srcColorBlendFactor, defaults...
                                                                  vk::BlendFactor::eZero,  // dstColorBlendFactor
                                                                  vk::BlendOp::eAdd,       // colorBlendOp
                                                                  vk::BlendFactor::eZero,  // srcAlphaBlendFactor
                                                                  vk::BlendFactor::eZero,  // dstAlphaBlendFactor
                                                                  vk::BlendOp::eAdd,       // alphaBlendOp
                                                                  colorComponentFlags      // colorWriteMask
  );
  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo({},                         // flags
                                                                  false,                      // logicOpEnable
                                                                  vk::LogicOp::eNoOp,         // logicOp
                                                                  colorBlendAttachmentState,  // attachments
                                                                  {{0.f, 0.f, 0.f, 0.f}}      // blendConstants
  );

  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo({}, dynamicStates);

  // vk::PipelineLayout pipelineLayout = (*vc.device).createPipelineLayout(vk::PipelineLayoutCreateInfo({}, 1, &descriptorSetLayout));
  pipelineLayoutInstance = {vc.device, {{}, 1, &(*descriptorSetLayout)}};  // { flags, descriptorSetLayout }

  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      {},
      shaderStageCreateInfos,
      &vertexInputStateCreateInfo,
      &inputAssemblyStateCreateInfo,
      nullptr,  // *vk::PipelineTessellationStateCreateInfo
      &viewportStateCreateInfo,
      &rasterizationStateCreateInfo,
      &multisampleStateCreateInfo,
      appSettings.hasPresentDepth ? &depthStencilStateCreateInfo : nullptr,
      &colorBlendStateCreateInfo,
      &dynamicStateCreateInfo,  // *vk::PipelineDynamicStateCreateInfo
      *pipelineLayoutInstance,  // vk::PipelineLayout
      *vc.renderPass            // vk::RenderPass
                                //{}, // uint32_t subpass_ = {},
  );

  pipelineInstance = std::make_unique<vk::raii::Pipeline>(vc.device, nullptr, graphicsPipelineCreateInfo);
  assert(pipelineInstance->getConstructorSuccessCode() == vk::Result::eSuccess);
}

void TransformGPUConstructionStudy::initPipelineWithCompute(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const vk::raii::DescriptorSetLayout& descriptorSetLayout) {
  const std::string computeShaderStr = R"(
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 10, local_size_y = 1, local_size_z = 1 ) in;

struct Transform {
  mat4 worldFromObject;
  mat4 dualWorldFromObject;
  vec4 color;
};

layout (std140, binding = 0) buffer buf
{
  Transform transforms[];
};

layout (binding = 1) uniform Target 
{
	vec4 position;
} target;

void main() 
{
  const int numMonkeyInstances = 10;
  const float pi = 3.14159265358979f;
  const uint ix = gl_GlobalInvocationID.x;

  //vec3 trans = vec3(cos(ix * 2.0f * pi / numMonkeyInstances), 0, sin(ix * 2.0f * pi / numMonkeyInstances)) * 3.0f;
  //mat4 model = mat4(1);
  //model[3][0] = trans.x;
  //model[3][1] = trans.y;
  //model[3][2] = trans.z;
  //transforms[ix].worldFromObject = model;
  //transforms[ix].dualWorldFromObject = transpose(inverse(model));
  //transforms[ix].color = vec4(0, 1, 1, 1);

  // TODO: Remove. Temporarily only moving monkeys in a direction with fixed speed to demonstrate compute shader is active
  //mat4 incr = mat4(0);
  //incr[3][0] = 0.001f;
  //incr[3][1] = 0.0f;
  //incr[3][2] = 0.0f;

  vec3 monkeyPos = vec3(transforms[ix].worldFromObject[3][0], transforms[ix].worldFromObject[3][1], transforms[ix].worldFromObject[3][2]);
  vec3 dir = normalize(target.position.xyz - monkeyPos);

  // TODO: Remove. Temporarily demonstrating that uniform bufer is functioning, we get target position here
  monkeyPos += dir * 0.001;
  transforms[ix].worldFromObject[3][0] = monkeyPos.x;
  transforms[ix].worldFromObject[3][1] = monkeyPos.y;
  transforms[ix].worldFromObject[3][2] = monkeyPos.z;

  // TODO: calculate and update monkey rotations based on target position
}
)";

  vk::raii::ShaderModule computeShader = vku::spirv::makeShaderModule(vc.device, vk::ShaderStageFlagBits::eCompute, computeShaderStr);

  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  pipelineLayoutCreateInfo.setSetLayouts(*descriptorSetLayout);
  pipelineLayoutCompute = {vc.device, pipelineLayoutCreateInfo};  // { flags, descriptorSetLayout }

  vk::PipelineShaderStageCreateInfo shaderStageCreateInfo({}, vk::ShaderStageFlagBits::eCompute, *computeShader, "main");

  pipelineCompute = std::make_unique<vk::raii::Pipeline>(vc.device, nullptr, 
    vk::ComputePipelineCreateInfo({}, shaderStageCreateInfo, *pipelineLayoutCompute));
  assert(pipelineCompute->getConstructorSuccessCode() == vk::Result::eSuccess);
}

void TransformGPUConstructionStudy::onUpdate(float deltaTime, const vku::Window& win) {
  static float t = 0.0f;

  ImGui::Begin("Scene");
  ImGui::Text("Entities");
  static bool isBoxInteractive = false;
  ImGui::Checkbox("Interactive Box", &isBoxInteractive);
  if (isBoxInteractive) {
    ImGui::DragFloat3("Box Pos", glm::value_ptr(entities[0].transform.position));
  } else {
    static float r = 2.0f;
    float theta = t * 0.5f;
    float sx = r * std::cos(theta);
    float sy = r * std::sin(theta);
    entities[0].transform.position = glm::vec3{
                                         glm::perlin(glm::vec3{sx, sy, -2}),
                                         glm::perlin(glm::vec3{sx, sy, 0}),
                                         glm::perlin(glm::vec3{sx, sy, 2})} *
                                     3.0f;
  }
  ImGui::DragFloat3("Axes Pos", glm::value_ptr(entities[1].transform.position));

  static bool shouldTurnInstantly = true;
  ImGui::Checkbox("Instant Turn", &shouldTurnInstantly);
  const glm::vec3 up{0, 1, 0};
  if (shouldTurnInstantly) {
    for (size_t ix = 1; ix < entities.size(); ++ix) {
      const glm::quat targetRotation = glm::normalize(glm::quatLookAt(entities[ix].transform.position - entities[0].transform.position, up));
      entities[ix].transform.rotation = targetRotation;
    }
  } else {
    static float turningSpeed = 2.5f;
    ImGui::SliderFloat("Turning Speed", &turningSpeed, 0.0f, 10.0f);
    float maxAngle = turningSpeed * deltaTime;
    ImGui::Text("maxAngle: %f", maxAngle);
    for (size_t ix = 1; ix < entities.size(); ++ix) {
      const glm::quat targetRotation = glm::normalize(glm::quatLookAt(entities[ix].transform.position - entities[0].transform.position, up));
      entities[ix].transform.rotation = vku::rotateTowards(entities[ix].transform.rotation, targetRotation, maxAngle);
    }
  }

  ImGui::Text("Axes Rot (Quat) {%.1f, %.1f, %.1f, %.1f}, norm: %.2f", entities[1].transform.rotation.x, entities[1].transform.rotation.y, entities[1].transform.rotation.z, entities[1].transform.rotation.w, glm::length(entities[1].transform.rotation));
  const glm::vec3 axis = glm::axis(entities[1].transform.rotation);
  ImGui::Text("Axes Rot (AA) %.1f, {%.1f, %.1f, %.1f}", glm::angle(entities[1].transform.rotation), axis.x, axis.y, axis.z);
  const glm::vec3 euler = glm::eulerAngles(entities[1].transform.rotation);
  ImGui::Text("Axes Rot (Euler) {%.1f, %.1f, %.1f}", glm::angle(entities[1].transform.rotation), euler.x, euler.y, euler.z);
  ImGui::Separator();

  ImGui::Text("Camera");
  if (false) {
    static vku::FirstPersonCameraViewInputController cc(camera, win);
    cc.update(deltaTime);
  } else {
    static auto cc = [&]() { 
      vku::FirstPersonCameraViewOrbitingController ret{ camera }; 
      ret.radius = 8.5f; ret.speed = 0.0f;
      return ret; }();
    cc.update(deltaTime);
  }
  ImGui::SliderFloat("FoV", &camera.fov, 15, 180, "%.1f");  // TODO: PerspectiveCameraController, OrthographicCameraController

  //
  entityUniforms.viewFromWorld = camera.getViewFromWorld();
  entityUniforms.projectionFromView = camera.getProjectionFromView();
  entityUniforms.projectionFromWorld = entityUniforms.projectionFromView * entityUniforms.viewFromWorld;
  entityUniformBuffer.update();  // don't forget to call update after uniform data changes

  //
  computeUniforms.targetPosition = glm::vec4(entities[0].transform.position, 0);
  computeUniformBuffer.update();

  ImGui::Text(std::format("yaw: {}, pitch: {}\n", camera.yaw, camera.pitch).c_str());
  ImGui::End();

  t += deltaTime;
}

void TransformGPUConstructionStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{{0, 0}, vc.swapchainExtent}, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;

  // compute monkey transforms
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineLayoutCompute, 0, *computeDescriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, **pipelineCompute);
  cmdBuf.dispatch(10, 1, 1);
  vk::MemoryBarrier memBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eVertexAttributeRead);
  cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput, {}, memBarrier, nullptr, nullptr);

  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vbo.buffer, offsets);
  cmdBuf.bindIndexBuffer(*ibo.buffer, 0, vk::IndexType::eUint32);

  // Draw entities
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayoutPushConstant, 0, *graphicsDescriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipelinePushConstant);
  for (auto& e : entities) {
    const PushConstants& pco = e.getPushConstants();
    const Mesh& mesh = e.mesh;
    assert(sizeof(pco) <= vc.physicalDevice.getProperties().limits.maxPushConstantsSize);  // Push constant data too big
    cmdBuf.pushConstants<PushConstants>(*pipelineLayoutPushConstant, vk::ShaderStageFlagBits::eVertex, 0u, pco);
    cmdBuf.drawIndexed(mesh.size, 1, mesh.offset, 0, 0);
  }

  // Draw monkey instances
  cmdBuf.bindVertexBuffers(1, *instanceBuffer.buffer, offsets);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayoutInstance, 0, *graphicsDescriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipelineInstance);
  cmdBuf.drawIndexed(indexCount, numMonkeyInstances, 0, 0, 0);

  cmdBuf.endRenderPass();
}

void TransformGPUConstructionStudy::onDeinit() {}