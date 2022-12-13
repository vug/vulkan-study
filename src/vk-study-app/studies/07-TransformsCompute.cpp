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
  uint32_t transformBufferSize{};
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
    meshes[MeshId::Monkey] = insertMeshData(vku::loadOBJ(vku::assetsRootFolder / "models/suzanne_smooth.obj"));

    entities.emplace_back(meshes[MeshId::Box], vku::Transform{{-2, 0, 0}, {0, 0, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 0, 0, 1});
    entities.emplace_back(meshes[MeshId::Axes], vku::Transform{{0, 0, 0}, {1, 1, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 1, 1, 1});

    numMonkeyInstances = 50'000;
    std::vector<InstanceData> monkeyInstances(numMonkeyInstances);
    std::vector<vku::TransformGPU> monkeyTransformsToGPU(numMonkeyInstances);

    std::default_random_engine rndGenerator(0);  // (unsigned)time(nullptr)
    std::uniform_real_distribution<float> uniformDist(-1.0f, 1.0f);
    const auto& u = [&rndGenerator, &uniformDist]() { return uniformDist(rndGenerator); };

    for (uint32_t i = 0; i < numMonkeyInstances; ++i) {
      const vku::Transform transform = vku::Transform{
          glm::vec3{u(), u(), u()} * 10.0f,
          {},
          0,
          glm::vec3{1.0f} * 0.05f};
      monkeyTransformsToGPU[i] = transform.toGPULayout();
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

    transformBufferSize = static_cast<uint32_t>(monkeyTransformsToGPU.size() * sizeof(vku::TransformGPU));
    transformBuffer = vku::Buffer(vc, monkeyTransformsToGPU.data(), transformBufferSize, vk::BufferUsageFlagBits::eStorageBuffer);

    uint32_t vboSizeBytes = (uint32_t)(allMeshesData.vertices.size() * sizeof(vku::DefaultVertex));
    vbo = vku::Buffer(vc, allMeshesData.vertices.data(), vboSizeBytes, vk::BufferUsageFlagBits::eVertexBuffer);

    uint32_t iboSizeBytes = (uint32_t)(allMeshesData.indices.size() * sizeof(uint32_t));
    indexCount = (uint32_t)allMeshesData.indices.size();
    ibo = vku::Buffer(vc, allMeshesData.indices.data(), iboSizeBytes, vk::BufferUsageFlagBits::eIndexBuffer);
  }

  //---- Graphics
  {
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayoutsRaii;
    // set = 0 Per Frame Descriptor Set Layout
    {
      const std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{
          vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
      };
      const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{{}, layoutBindings};
      descriptorSetLayoutsRaii.emplace_back(vc.device, layoutCreateInfo);
    }
    // set = 1 Per Pass Descriptor Set Layout
    {
      const std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{
          vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
      };
      const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{{}, layoutBindings};
      descriptorSetLayoutsRaii.emplace_back(vc.device, layoutCreateInfo);
    }
    // init pipelineLayoutPerFrameAndPass
    {
      std::vector<vk::DescriptorSetLayout> descriptorSetLayoutsCommon;
      std::ranges::transform(descriptorSetLayoutsRaii, std::back_inserter(descriptorSetLayoutsCommon), [&](const vk::raii::DescriptorSetLayout& dsRaii) { return *dsRaii; });
      vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)};
      vk::PipelineLayoutCreateInfo pipelineLayoutCommoneCreateInfo({}, descriptorSetLayoutsCommon, pushConstantRange);
      pipelineLayoutPerFrameAndPass = vk::raii::PipelineLayout(vc.device, pipelineLayoutCommoneCreateInfo);
    }
    // set = 2 Per Material Descriptor Set Layout
    {
      const std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{
          vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
      };
      const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{{}, layoutBindings};
      descriptorSetLayoutsRaii.emplace_back(vc.device, layoutCreateInfo);
    }

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::ranges::transform(descriptorSetLayoutsRaii, std::back_inserter(descriptorSetLayouts), [&](const vk::raii::DescriptorSetLayout& dsRaii) { return *dsRaii; });
    vk::DescriptorSetAllocateInfo allocateInfo{*vc.descriptorPool, descriptorSetLayouts};

    descriptorSetsGraphics.reserve(vc.MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < vc.MAX_FRAMES_IN_FLIGHT; i++) {
      perFrameUniform.emplace_back(vc);
      perPassUniform.emplace_back(vc);
      perMaterialUniform.emplace_back(vc);

      descriptorSetsGraphics.emplace_back(vc.device, allocateInfo);
      std::vector<vk::DescriptorSet> descriptorSets;
      std::ranges::transform(descriptorSetsGraphics.back(), std::back_inserter(descriptorSets), [&](const vk::raii::DescriptorSet& dsRaii) { return *dsRaii; });

      vk::WriteDescriptorSet writeDescriptorSet;  // connects indiviudal concrete uniform buffer to descriptor set with the abstract layout that can refer to it

      // per Frame uniform and descriptor connections
      writeDescriptorSet.dstSet = descriptorSets[0];  // set = 0
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
      writeDescriptorSet.pBufferInfo = &perFrameUniform.back().descriptor;
      writeDescriptorSet.dstBinding = 0;  // binding = 0
      vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

      // per Pass uniform and descriptor connections
      writeDescriptorSet.dstSet = descriptorSets[1];  // set = 1
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
      writeDescriptorSet.pBufferInfo = &perPassUniform.back().descriptor;
      writeDescriptorSet.dstBinding = 0;  // binding = 0
      vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

      // per Material uniform and descriptor connections
      writeDescriptorSet.dstSet = descriptorSets[2];  // set = 2
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
      writeDescriptorSet.pBufferInfo = &perMaterialUniform.back().descriptor;
      writeDescriptorSet.dstBinding = 0;  // binding = 0
      vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);
    }

    initPipelineWithPushConstant(appSettings, vc, descriptorSetLayouts);
    initPipelineWithInstances(appSettings, vc, descriptorSetLayouts);
  }

  //---- Compute Uniform Data
  computeUniformBuffer = vku::UniformBuffer<ComputeUniforms>(vc);

  //---- Descriptor Set - Compute
  {
    const std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},  // layout(set=0, binding=0) buffer buf; // transforms
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},  // layout(set=0, binding=1) buffer buf; // transformMatrices
        vk::DescriptorSetLayoutBinding{2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute},  // layout(set=0, binding=2)
    };
    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{{}, layoutBindings};
    const vk::raii::DescriptorSetLayout descriptorSetLayout{vc.device, descriptorSetLayoutCreateInfo};

    // Allocate descriptor set from the pool
    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo(*vc.descriptorPool, 1, &(*descriptorSetLayout));
    computeDescriptorSets = vk::raii::DescriptorSets(vc.device, allocateInfo);

    vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.dstSet = *computeDescriptorSets[0];

    // Binding 0 and 1: Storage Buffer for transforms and transform matrices
    std::array<vk::DescriptorBufferInfo, 2> descriptorBufferInfosStorage{
        vk::DescriptorBufferInfo(*transformBuffer.buffer, 0, transformBufferSize),
        vk::DescriptorBufferInfo(*instanceBuffer.buffer, 0, instanceBufferSize),
    };
    writeDescriptorSet.descriptorCount = 2;
    writeDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
    writeDescriptorSet.pBufferInfo = descriptorBufferInfosStorage.data();
    writeDescriptorSet.dstBinding = 0;  // {0, 1}
    vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

    // Binding 2: Uniform Buffer for target position
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptorSet.pBufferInfo = &computeUniformBuffer.descriptor;
    writeDescriptorSet.dstBinding = 2;  // dstBindingPrev + descriptorCountPrev;
    vc.device.updateDescriptorSets(writeDescriptorSet, nullptr);

    initPipelineWithCompute(appSettings, vc, descriptorSetLayout);
  }
}

void TransformGPUConstructionStudy::initPipelineWithPushConstant(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts) {
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

layout (set = 1, binding = 0) uniform PerPass {
  vec4 cameraPositionWorld;
	mat4 viewFromWorldMatrix;
  mat4 projectionFromViewMatrix;
  mat4 projectionFromWorldMatrix;
} perPass;

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

  gl_Position = perPass.projectionFromWorldMatrix * worldPosition4;

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
                                                                        vk::CullModeFlagBits::eBack,       // cullMode {eBack}
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
  pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayouts);
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

void TransformGPUConstructionStudy::initPipelineWithInstances(const vku::AppSettings appSettings, const vku::VulkanContext& vc, const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts) {
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

layout (set = 1, binding = 0) uniform PerPass {
  vec4 cameraPositionWorld;
	mat4 viewFromWorldMatrix;
  mat4 projectionFromViewMatrix;
  mat4 projectionFromWorldMatrix;
} perPass;

layout (location = 0) out struct {
    vec3 worldPosition;
    vec3 worldNormal;
    vec3 objectNormal;
    vec4 color;
} v2f;


void main() {
  const mat4 transform = instanceWorldFromObjectMatrix; // {perPass.WorldFromObjectMatrix, instanceWorldFromObjectMatrix}
  const vec4 worldPosition4 = transform * vec4(inObjectPosition.xyz, 1.0);
  v2f.worldPosition = worldPosition4.xyz;

  v2f.worldNormal = mat3(instanceDualWorldFromObjectMatrix) * inObjectNormal;

  v2f.objectNormal = inObjectNormal;

  gl_Position = perPass.projectionFromWorldMatrix * worldPosition4;

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

layout (set = 0, binding = 0) uniform PerFrame {
  vec4 time;
  vec4 lightPos;
} perFrame;

layout (set = 1, binding = 0) uniform PerPass {
  vec4 cameraPositionWorld;
	mat4 viewFromWorldMatrix;
  mat4 projectionFromViewMatrix;
  mat4 projectionFromWorldMatrix;
} perPass;

layout (set = 2, binding = 0) uniform PerMaterial {
  vec4 specularParams; // x: specularExponent/smoothness
  vec4 goochCool;
  vec4 goochWarm;
  ivec4 shouldUseGooch;
} perMaterial;

layout (location = 0) out vec4 outFragColor;

void main() {
  // from vertex
  const vec3 normal = normalize(v2f.worldNormal);
  // from uniforms
  const vec3 camPosWorld = perPass.cameraPositionWorld.xyz;
  const vec3 lightPos = perFrame.lightPos.xyz;
  const vec3 goochCoolColor = perMaterial.goochCool.xyz;
  const vec3 goochWarmColor = perMaterial.goochWarm.xyz;
  const bool shouldUseGooch = perMaterial.shouldUseGooch.x != 0;

  // directions
  const vec3 fragToCamDir = normalize(camPosWorld - v2f.worldPosition);
  const vec3 fragToLightDir = normalize(lightPos - v2f.worldPosition); // not light to frag
  const vec3 reflectionDir = reflect(-fragToLightDir, normal);

  // illuminations
  const float diffuse = max(dot(fragToLightDir, normal), 0);
  const float gooch = (1.0f + dot(fragToLightDir, normal)) * 0.5f;
  const vec3 goochDiffuse = gooch * goochWarmColor + (1 - gooch) * goochCoolColor;
  const float specular0 = max(dot(fragToCamDir, reflectionDir), 0);
  const float specular = pow(specular0, perMaterial.specularParams.x);
  
  if (shouldUseGooch)
    outFragColor = vec4(goochDiffuse + specular * vec3(1), 1); // lit
  else
    outFragColor = vec4(v2f.color.xyz, 1) * (diffuse + specular); // lit
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

  // TODO: fix obj loading. Looks like face orientations are incorrect? I had to cull front faces. :-O
  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({},                                // flags
                                                                        false,                             // depthClampEnable
                                                                        false,                             // rasterizerDiscardEnable
                                                                        vk::PolygonMode::eFill,            // polygonMode
                                                                        vk::CullModeFlagBits::eBack,       // cullMode {eBack}
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

  // Note that, thie pipeline/shaders do not use push constants but in order to make the two pipelines' (entities and instance) layouts compatible
  // needed to add the push constants. See "Pipeline Layout Compatibility" https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#descriptorsets-compatibility
  vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)};
  const vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{{}, descriptorSetLayouts, pushConstantRange};  // { flags, descriptorSetLayouts }

  pipelineLayoutInstance = vk::raii::PipelineLayout{vc.device, pipelineLayoutCreateInfo};

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
  vec4 position;
  vec4 rotation;
  vec4 scale;
};

struct TransformMatrices {
  mat4 worldFromObject;
  mat4 dualWorldFromObject;
  vec4 color;
};

layout (std140, set = 0, binding = 0) buffer buf0 {
  Transform transforms[];
};

layout (std140, set = 0, binding = 1) buffer buf1 {
  TransformMatrices transformMatrices[];
};

layout (set = 0, binding = 2) uniform ComputeParameters {
	vec4 targetPosition;
  vec4 maxAngleToTurn;
  ivec4 shouldTurnInstantly;
} params;

mat4 dirToRot(vec3 dir, vec3 up) {
  const vec3 zaxis = dir; // local forward
  const vec3 xaxis = normalize(cross(zaxis, up)); // local right
  const vec3 yaxis = cross(xaxis, zaxis); // local up
  const mat4 rotate = {
    vec4(xaxis.x, xaxis.y, xaxis.z, 0),
    vec4(yaxis.x, yaxis.y, yaxis.z, 0),
    vec4(zaxis.x, zaxis.y, zaxis.z, 0),
    vec4(0, 0, 0, 1)
  };
  return rotate;
}

// from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
vec4 rotToQuat(mat4 rot) {
  vec4 q = vec4(0);
  q.w = sqrt(1.0 + rot[0][0] + rot[1][1] + rot[2][2]) / 2.0;
  float w4 = (4.0 * q.w);
	q.x = (rot[2][1] - rot[1][2]) / w4 ;
	q.y = (rot[0][2] - rot[2][0]) / w4 ;
	q.z = (rot[1][0] - rot[0][1]) / w4 ;
	//q.x = (rot[1][2] - rot[2][1]) / w4 ;
	//q.y = (rot[2][0] - rot[0][2]) / w4 ;
	//q.z = (rot[0][1] - rot[1][0]) / w4 ;
  return normalize(q);

  //float tr = rot[0][0] + rot[1][1] + rot[2][2];
  //vec4 q;
  //if (tr > 0) { 
  //  float S = sqrt(tr+1.0) * 2; // S=4*qw 
  //  float qw = 0.25 * S;
  //  float qx = (rot[2][1] - rot[1][2]) / S;
  //  float qy = (rot[0][2] - rot[2][0]) / S; 
  //  float qz = (rot[1][0] - rot[0][1]) / S; 
  //  q = vec4(qx, qy, qz, qw);
  //} else if ((rot[0][0] > rot[1][1]) && (rot[0][0] > rot[2][2])) { 
  //  float S = sqrt(1.0 + rot[0][0] - rot[1][1] - rot[2][2]) * 2; // S=4*qx 
  //  float qw = (rot[2][1] - rot[1][2]) / S;
  //  float qx = 0.25 * S;
  //  float qy = (rot[0][1] + rot[1][0]) / S; 
  //  float qz = (rot[0][2] + rot[2][0]) / S; 
  //  q = vec4(qx, qy, qz, qw);
  //} else if (rot[1][1] > rot[2][2]) { 
  //  float S = sqrt(1.0 + rot[1][1] - rot[0][0] - rot[2][2]) * 2; // S=4*qy
  //  float qw = (rot[0][2] - rot[2][0]) / S;
  //  float qx = (rot[0][1] + rot[1][0]) / S; 
  //  float qy = 0.25 * S;
  //  float qz = (rot[1][2] + rot[2][1]) / S; 
  //  q = vec4(qx, qy, qz, qw);
  //} else { 
  //  float S = sqrt(1.0 + rot[2][2] - rot[0][0] - rot[1][1]) * 2; // S=4*qz
  //  float qw = (rot[1][0] - rot[0][1]) / S;
  //  float qx = (rot[0][2] + rot[2][0]) / S;
  //  float qy = (rot[1][2] + rot[2][1]) / S;
  //  float qz = 0.25 * S;
  //  q = normalize(vec4(qx, qy, qz, qw));
  //}
  //return q;
}

// turn quaternion1 towards q2 at most by the amount of maxAngle
vec4 rotateTowards(vec4 q1, vec4 q2, float maxAngle) {
  if (maxAngle < 0.00001f)
    return q1;

  float cosTheta = dot(q1, q2);

  if (cosTheta > 0.99999f)
    return q2;

  // take shorter path on the sphere
  if (cosTheta < 0) {
    q1 *= -1.f;
    cosTheta *= -1.f;
  }
  float angle = acos(cosTheta);

  if (angle < maxAngle)
    return q2;

  // because we make sure shorter path is taken above, we can use mix instead of slerp (which ensures shorter path)
  const float m = maxAngle / angle;
  return normalize(mix(q1, q2, m));
}

// not sure. from https://stackoverflow.com/questions/52413464/look-at-quaternion-using-up-vector
vec4 dirToQuat(vec3 dir, vec3 up) {
  vec4 q;
  const vec3 F = dir; // local forward
  const vec3 R = normalize(cross(F, up)); // local right
  const vec3 U = cross(R, F); // local up

  float trace = R.x + U.y + F.z;
  if (trace > 0.0) {
    float s = 0.5 / sqrt(trace + 1.0);
    q.w = 0.25 / s;
    q.x = (U.z - F.y) * s;
    q.y = (F.x - R.z) * s;
    q.z = (R.y - U.x) * s;
  } 
  else {
    if (R.x > U.y && R.x > F.z) {
      float s = 2.0 * sqrt(1.0 + R.x - U.y - F.z);
      q.w = (U.z - F.y) / s;
      q.x = 0.25 * s;
      q.y = (U.x + R.y) / s;
      q.z = (F.x + R.z) / s;
    } else if (U.y > F.z) {
      float s = 2.0 * sqrt(1.0 + U.y - R.x - F.z);
      q.w = (F.x - R.z) / s;
      q.x = (U.x + R.y) / s;
      q.y = 0.25 * s;
      q.z = (F.y + U.z) / s;
    } else {
      float s = 2.0 * sqrt(1.0 + F.z - R.x - U.y);
      q.w = (R.y - U.x) / s;
      q.x = (F.x + R.z) / s;
      q.y = (F.y + U.z) / s;
      q.z = 0.25 * s;
    }
  }
  return q;
}

// not working properly. from http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm and http://www.songho.ca/opengl/gl_quaternion.html
mat4 quatToRot(vec4 q){
  float xx      = q.x * q.x;
  float xy      = q.x * q.y;
  float xz      = q.x * q.z;
  float xw      = q.x * q.w;
  float yy      = q.y * q.y;
  float yz      = q.y * q.z;
  float yw      = q.y * q.w;
  float zz      = q.z * q.z;
  float zw      = q.z * q.w;
 
  // one of them is made of column vectors other row vectors. not sure which one is which.
  const mat4 rot1 = {
    vec4(1 - 2 * ( yy + zz ),     2 * ( xy - zw ),     2 * ( xz + yw ), 0),
    vec4(    2 * ( xy + zw ), 1 - 2 * ( xx + zz ),     2 * ( yz - xw ), 0),
    vec4(    2 * ( xz - yw ),     2 * ( yz + xw ), 1 - 2 * ( xx + yy ), 0),
    vec4(                  0,                   0,                   0, 1)
  };

  //const mat4 rot2 = {
  //  vec4(1 - 2 * ( yy + zz ),     2 * ( xy + zw ),     2 * ( xz - yw ), 0),
  //  vec4(    2 * ( xy - zw ), 1 - 2 * ( xx + zz ),     2 * ( yz + xw ), 0),
  //  vec4(    2 * ( xz + yw ),     2 * ( yz - xw ), 1 - 2 * ( xx + yy ), 0),
  //  vec4(                  0,                   0,                   0, 1)
  //};

  return rot1;
}

vec4 axisAngleToQuat(vec3 axis, float angle) {
  const float half_angle = angle * 0.5;
  const vec4 q = vec4(
    axis.x * sin(half_angle),
    axis.y * sin(half_angle),
    axis.z * sin(half_angle),
    cos(half_angle)
  );
  return q;
}

vec4 quatConj(vec4 q) { 
  return vec4(-q.x, -q.y, -q.z, q.w); 
}

vec4 quatInv(vec4 q) {
  float norm = length(q);
  vec4 invQ = quatConj(q) / norm;
  return invQ;
}

vec4 quatMult(vec4 q1, vec4 q2) { 
  vec4 qr;
  qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
  qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
  qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
  qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
  return qr;
}

// from https://gist.github.com/mattatz/40a91588d5fb38240403f198a938a593
#define QUATERNION_IDENTITY vec4(0, 0, 0, 1)
vec4 q_look_at(vec3 forward, vec3 up)
{
    vec3 right = normalize(cross(forward, up));
    up = normalize(cross(forward, right));

    float m00 = right.x;
    float m01 = right.y;
    float m02 = right.z;
    float m10 = up.x;
    float m11 = up.y;
    float m12 = up.z;
    float m20 = forward.x;
    float m21 = forward.y;
    float m22 = forward.z;

    float num8 = (m00 + m11) + m22;
    vec4 q = QUATERNION_IDENTITY;
    if (num8 > 0.0)
    {
        float num = sqrt(num8 + 1.0);
        q.w = num * 0.5;
        num = 0.5 / num;
        q.x = (m12 - m21) * num;
        q.y = (m20 - m02) * num;
        q.z = (m01 - m10) * num;
        return q;
    }

    if ((m00 >= m11) && (m00 >= m22))
    {
        float num7 = sqrt(((1.0 + m00) - m11) - m22);
        float num4 = 0.5 / num7;
        q.x = 0.5 * num7;
        q.y = (m01 + m10) * num4;
        q.z = (m02 + m20) * num4;
        q.w = (m12 - m21) * num4;
        return q;
    }

    if (m11 > m22)
    {
        float num6 = sqrt(((1.0 + m11) - m00) - m22);
        float num3 = 0.5 / num6;
        q.x = (m10 + m01) * num3;
        q.y = 0.5 * num6;
        q.z = (m21 + m12) * num3;
        q.w = (m20 - m02) * num3;
        return q;
    }

    float num5 = sqrt(((1.0 + m22) - m00) - m11);
    float num2 = 0.5 / num5;
    q.x = (m20 + m02) * num2;
    q.y = (m21 + m12) * num2;
    q.z = 0.5 * num5;
    q.w = (m01 - m10) * num2;
    return q;
}

// from: https://gist.github.com/mattatz/40a91588d5fb38240403f198a938a593
mat4 quaternion_to_matrix(vec4 quat)
{
    mat4 m = mat4(vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0));

    float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    m[0][0] = 1.0 - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;

    m[1][0] = xy + wz;
    m[1][1] = 1.0 - (xx + zz);
    m[1][2] = yz - wx;

    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0 - (xx + yy);

    m[3][3] = 1.0;

    return transpose(m); // needed to get correct orientations
}


void main() 
{
  const float pi = 3.14159265358979f;
  const uint ix = gl_GlobalInvocationID.x;

  const float maxAngle = params.maxAngleToTurn.x;
  const vec3 targetPosition = params.targetPosition.xyz;

  // Extract Translation
  const vec3 inPos = transforms[ix].position.xyz;
  const vec4 inRot = transforms[ix].rotation;
  const vec3 inScale = transforms[ix].scale.xyz;

  const vec3 targetDir = normalize(targetPosition - inPos);
  const vec3 up = vec3(0, 1, 0);

  // TRANSLATE
  mat4 translate = mat4(1);
  translate[3][0] = inPos.x;
  translate[3][1] = inPos.y;
  translate[3][2] = inPos.z;

  // ROTATE
  // Method A: construct rotation quaternion from direction (and up), then rotation matrix from quaternion
  const vec4 targetRotQuat = q_look_at(targetDir, up);
  //const mat4 targetRotMat = quaternion_to_matrix(targetRotQuat);

  // Method B: construct rotation matrix from direction, then rotation quaternion from matrix
  //const mat4 targetRotMat = dirToRot(targetDir, up);
  //const vec4 targetRotQuat = rotToQuat(targetRotMat);

  // SCALE
  mat4 scale = mat4(1);
  scale[0][0] = inScale.x;
  scale[1][1] = inScale.y;
  scale[2][2] = inScale.z;  

  const bool shouldTurnInstantly = params.shouldTurnInstantly.x != 0;
  const vec4 rotateQ = shouldTurnInstantly ? 
    targetRotQuat :
    rotateTowards(transforms[ix].rotation, targetRotQuat, maxAngle);

  const mat4 rotateM = quaternion_to_matrix(rotateQ); // targetRotMat;

  // Update transforms uniform
  transforms[ix].rotation = rotateQ;
  // Update transformMatrices uniform
  const mat4 model = translate * rotateM * scale;
  transformMatrices[ix].worldFromObject = model;
  transformMatrices[ix].dualWorldFromObject = transpose(inverse(model));
  transformMatrices[ix].color = vec4(0.1, 0.2, 1, 1);
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

void TransformGPUConstructionStudy::onUpdate(const vku::UpdateParams& params) {
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
                                     10.0f;
  }
  ImGui::DragFloat3("Axes Pos", glm::value_ptr(entities[1].transform.position));

  static bool shouldTargetCamera = false;
  ImGui::Checkbox("Target Camera?", &shouldTargetCamera);
  const glm::vec3 targetPosition = shouldTargetCamera ? camera.getPosition() : entities[0].transform.position;

  static bool shouldTurnInstantly = true;
  ImGui::Checkbox("Instant Turn", &shouldTurnInstantly);
  const glm::vec3 up{0, 1, 0};
  float maxAngle = 0;
  if (!shouldTurnInstantly) {
    static float turningSpeed = 2.5f;
    ImGui::SliderFloat("Turning Speed", &turningSpeed, 0.0f, 10.0f);
    maxAngle = turningSpeed * params.deltaTime;
    ImGui::Text("maxAngle: %f", maxAngle);
  }

  ImGui::Text("Axes Rot (Quat) {%.1f, %.1f, %.1f, %.1f}, norm: %.2f", entities[1].transform.rotation.x, entities[1].transform.rotation.y, entities[1].transform.rotation.z, entities[1].transform.rotation.w, glm::length(entities[1].transform.rotation));
  const glm::vec3 axis = glm::axis(entities[1].transform.rotation);
  ImGui::Text("Axes Rot (AA) %.1f, {%.1f, %.1f, %.1f}", glm::angle(entities[1].transform.rotation), axis.x, axis.y, axis.z);
  const glm::vec3 euler = glm::eulerAngles(entities[1].transform.rotation);
  ImGui::Text("Axes Rot (Euler) {%.1f, %.1f, %.1f}", glm::angle(entities[1].transform.rotation), euler.x, euler.y, euler.z);
  ImGui::Separator();

  ImGui::Text("Camera");
  static bool shouldUseFreeCamera = true;
  ImGui::Checkbox("Free Camera?", &shouldUseFreeCamera);
  static vku::FirstPersonCameraViewInputController freeCameraController(camera, params.win);
  static auto orbitingCameraController = [&]() { 
      vku::FirstPersonCameraViewOrbitingController ret{ camera }; 
      ret.radius = 20.f; ret.speed = 0.1f;
      return ret; }();
  if (shouldUseFreeCamera)
    freeCameraController.update(params.deltaTime);
  else
    orbitingCameraController.update(params.deltaTime);
  ImGui::SliderFloat("FoV", &camera.fov, 15, 180, "%.1f");  // TODO: PerspectiveCameraController, OrthographicCameraController
  ImGui::Text(std::format("yaw: {}, pitch: {}\n", camera.yaw, camera.pitch).c_str());

  ImGui::Separator();

  ImGui::Text("Lights");
  PerFrameUniform& frameUniform = perFrameUniform[params.frameInFlightNo].src;
  static glm::vec3 pointLightPos{frameUniform.lightPos};
  if (ImGui::DragFloat3("Point Light Pos", glm::value_ptr(pointLightPos))) {
    frameUniform.lightPos = {pointLightPos, 0};
    perFrameUniform[params.frameInFlightNo].update();
  }
  ImGui::Separator();


  // View / Projection matrix updates
  PerPassUniform& uni = perPassUniform[params.frameInFlightNo].src;
  uni.cameraPositionWorld = glm::vec4(camera.getPosition(), 0);
  uni.viewFromWorld = camera.getViewFromWorld();
  uni.projectionFromView = camera.getProjectionFromView();
  uni.projectionFromWorld = uni.projectionFromView * uni.viewFromWorld;
  perPassUniform[params.frameInFlightNo].update();  // don't forget to call upload after uniform data changes

  ImGui::Text("Material");
  MaterialUniform& mat = perMaterialUniform[params.frameInFlightNo].src;
  static float specularExponent = mat.specularParams.x;
  ImGui::SliderFloat("Specular Exponent (Smoothness)", &specularExponent, 0, 255);
  mat.specularParams.x = specularExponent;
  static bool shouldUseGooch = static_cast<bool>(mat.shouldUseGooch.x);
  ImGui::Checkbox("Goochify", &shouldUseGooch);
  mat.shouldUseGooch.x = static_cast<int32_t>(shouldUseGooch);
  perMaterialUniform[params.frameInFlightNo].update();
  ImGui::Separator();

  //
  computeUniformBuffer.src.targetPosition = glm::vec4(targetPosition, 0);
  computeUniformBuffer.src.maxAngleToTurn = glm::vec4(maxAngle, 0, 0, 0);
  computeUniformBuffer.src.shouldTurnInstantly = glm::ivec4(static_cast<int>(shouldTurnInstantly), 0, 0, 0);
  computeUniformBuffer.update();

  ImGui::End();

  t += params.deltaTime;
}

void TransformGPUConstructionStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  // compute monkey transforms
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineLayoutCompute, 0, *computeDescriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, **pipelineCompute);
  cmdBuf.dispatch(numMonkeyInstances, 1, 1);
  vk::MemoryBarrier memBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eVertexAttributeRead);
  cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexInput, {}, memBarrier, nullptr, nullptr);

  // Bind per-frame data
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayoutPerFrameAndPass, 0, *descriptorSetsGraphics[frameDrawer.frameNo][0], nullptr);

  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{{0, 0}, vc.swapchainExtent}, {});
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  // Bind per-pass data
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayoutPerFrameAndPass, 1, *descriptorSetsGraphics[frameDrawer.frameNo][1], nullptr);

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vbo.buffer, offsets);
  cmdBuf.bindIndexBuffer(*ibo.buffer, 0, vk::IndexType::eUint32);
  // Draw entities
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
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipelineInstance);
  // Bind per-material data
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayoutInstance, 2, *descriptorSetsGraphics[frameDrawer.frameNo][2], nullptr);
  cmdBuf.drawIndexed(meshes[MeshId::Monkey].size, numMonkeyInstances, meshes[MeshId::Monkey].offset, 0, 0);

  cmdBuf.endRenderPass();
}

void TransformGPUConstructionStudy::onDeinit() {}