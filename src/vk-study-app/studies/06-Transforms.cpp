#include "06-Transforms.hpp"

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

TransformConstructionStudy::PushConstants TransformConstructionStudy::Entity::getPushConstants() const {
  PushConstants pc = TransformConstructionStudy::PushConstants{.worldFromObject = transform.getTransform(), .color = color};
  pc.dualWorldFromObject = glm::transpose(glm::inverse(pc.worldFromObject));
  return pc;
}

void TransformConstructionStudy::onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) {
  std::cout << vivid::ansi::lightBlue << "Hi from Vivid at UniformsStudy" << vivid::ansi::reset << std::endl;

  //---- Vertex Data
  const vivid::ColorMap cmap = vivid::ColorMap::Preset::Viridis;
  vku::MeshData boxMeshData = vku::makeBox({0.2f, 0.5f, 0.7f});
  vku::MeshData axesMeshData = vku::makeAxes();
  vku::MeshData objMeshData = vku::loadOBJ(vku::assetsRootFolder / "models/suzanne.obj");

  {
    vku::MeshData allMeshesData;
    auto insertMeshData = [&](const vku::MeshData& newMesh) -> Mesh {
      Mesh mesh = {static_cast<uint32_t>(allMeshesData.indices.size()), static_cast<uint32_t>(newMesh.indices.size())};
      std::ranges::copy(newMesh.vertices, std::back_inserter(allMeshesData.vertices));
      std::ranges::transform(newMesh.indices, std::back_inserter(allMeshesData.indices), [&](uint32_t ix) { return ix + mesh.offset; });
      return mesh;
    };

    meshes.emplace_back(insertMeshData(boxMeshData));
    meshes.emplace_back(insertMeshData(axesMeshData));
    meshes.emplace_back(insertMeshData(objMeshData));
    entities.emplace_back(meshes[MeshId::Box], vku::Transform{{-2, 0, 0}, {0, 0, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 0, 0, 1});
    entities.emplace_back(meshes[MeshId::Axes], vku::Transform{{0, 0, 0}, {1, 1, 1}, std::numbers::pi_v<float> * 0.f, {1, 1, 1}}, glm::vec4{1, 1, 1, 1});

    const int numMonkeys = 10;
    const float pi = std::numbers::pi_v<float>;
    for (int i = 0; i < numMonkeys; ++i) {
      entities.emplace_back(
          meshes[MeshId::Monkey],
          vku::Transform{glm::vec3{std::cos(i * 2.0f * pi / numMonkeys), 0, std::sin(i * 2.0f * pi / numMonkeys)} * 3.0f, {}, 0, glm::vec3{1, 1, 1} * 0.75f},
          glm::vec4{0, 0, 1, 1});
    }

    uint32_t vboSizeBytes = (uint32_t)(allMeshesData.vertices.size() * sizeof(vku::DefaultVertex));
    vbo = vku::Buffer(vc, allMeshesData.vertices.data(), vboSizeBytes, vk::BufferUsageFlagBits::eVertexBuffer);

    uint32_t iboSizeBytes = (uint32_t)(allMeshesData.indices.size() * sizeof(uint32_t));
    indexCount = (uint32_t)allMeshesData.indices.size();
    ibo = vku::Buffer(vc, allMeshesData.indices.data(), iboSizeBytes, vk::BufferUsageFlagBits::eIndexBuffer);
  }

  //---- Uniform Data
  // create UBO and connect it to a Uniforms instance
  ubo = vku::UniformBuffer(vc, &uniforms, sizeof(Uniforms));

  //---- Descriptor Set Layout
  vk::DescriptorSetLayoutBinding layoutBinding = {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex};
  vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(vc.device, {{}, 1, &layoutBinding});

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
  pipelineLayout = {vc.device, pipelineLayoutCreateInfo};  // { flags, descriptorSetLayout }

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
      *pipelineLayout,          // vk::PipelineLayout
      *vc.renderPass            // vk::RenderPass
                                //{}, // uint32_t subpass_ = {},
  );

  pipeline = std::make_unique<vk::raii::Pipeline>(vc.device, nullptr, graphicsPipelineCreateInfo);
  assert(pipeline->getConstructorSuccessCode() == vk::Result::eSuccess);
}

void TransformConstructionStudy::onUpdate(float deltaTime, const vku::Window& win) {
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
  ImGui::DragFloat3("Monkey Pos", glm::value_ptr(entities[2].transform.position));

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

  uniforms.viewFromWorld = camera.getViewFromWorld();
  uniforms.projectionFromView = camera.getProjectionFromView();
  uniforms.projectionFromWorld = uniforms.projectionFromView * uniforms.viewFromWorld;

  ubo.update();  // don't forget to call update after uniform data changes
  t += deltaTime;

  ImGui::Text(std::format("yaw: {}, pitch: {}\n", camera.yaw, camera.pitch).c_str());
  ImGui::End();
}

void TransformConstructionStudy::recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) {
  const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{{0, 0}, vc.swapchainExtent}, {});

  const vk::raii::CommandBuffer& cmdBuf = frameDrawer.commandBuffer;
  cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descriptorSets[0], nullptr);
  cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);

  vk::DeviceSize offsets = 0;
  cmdBuf.bindVertexBuffers(0, *vbo.buffer, offsets);
  cmdBuf.bindIndexBuffer(*ibo.buffer, 0, vk::IndexType::eUint32);
  for (auto& e : entities) {
    const PushConstants& pco = e.getPushConstants();
    const Mesh& mesh = e.mesh;
    assert(sizeof(pco) <= vc.physicalDevice.getProperties().limits.maxPushConstantsSize);  // Push constant data too big
    cmdBuf.pushConstants<PushConstants>(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0u, pco);
    cmdBuf.drawIndexed(mesh.size, 1, mesh.offset, 0, 0);
  }

  cmdBuf.endRenderPass();
}

void TransformConstructionStudy::onDeinit() {}