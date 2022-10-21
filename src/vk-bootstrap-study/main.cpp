#include "SpirvHelper.hpp"

//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

#include <iostream>
#include <string>
#include <vector>


namespace vku {
#if !defined( NDEBUG )
  constexpr bool isDebugBuild = true;
#else
  constexpr bool isDebugBuild = false;
#endif
}

int main() {
  std::cout << "Hello, Vulkan!\n";

  //---- GLFW init
  glfwInit();
  glfwSetErrorCallback([](int error, const char* msg) { std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl; });
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 800, "Example Vulkan Application", nullptr, nullptr);

  uint32_t count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  std::vector<std::string> vulkanInstanceExtensions;
  for (size_t ix = 0; ix < count; ++ix)
    vulkanInstanceExtensions.push_back(extensions[ix]);

  //---- Instance
  auto systemInfo = vkb::SystemInfo::get_system_info().value(); // has methods about available layers and extensions
  for (const auto& ext : vulkanInstanceExtensions)
    assert(systemInfo.is_extension_available(ext.c_str()));

  auto vkbInstanceBuilder = vkb::InstanceBuilder{}
    .set_app_name("Example Vulkan Application")
    .require_api_version(1, 3, 0)
    .enable_validation_layers(vku::isDebugBuild) // == .enable_layer("VK_LAYER_KHRONOS_validation")
    // TODO: can create my own callback function and set it via set_debug_callback()
    // See debugUtilsMessengerCallback in utils.cpp
    .use_default_debug_messenger();
  for (const auto& ext : vulkanInstanceExtensions)
    vkbInstanceBuilder.enable_extension(ext.c_str());

  auto vkbInstance = vkbInstanceBuilder
    .build()
    .value();

  vk::raii::Context context;
  vk::raii::Instance instance{ context, vkbInstance.instance };

  //---- Surface
  VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
  glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &vkSurface);
  vk::raii::SurfaceKHR surface{ instance, vkSurface };

  //---- Physical Device
  vkb::PhysicalDeviceSelector phys_device_selector(vkbInstance);
  auto vkbPhysicalDevice = phys_device_selector
    .set_surface(*surface)
    .select()
    .value();
  vk::raii::PhysicalDevice physicalDevice{ instance, vkbPhysicalDevice.physical_device };

  //---- Logical Device
  auto vkbDevice = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value();
  vk::raii::Device device{ physicalDevice, vkbDevice.device };

  //---- Swapchain
  const uint32_t NUM_IMAGES = 3;
  // TODO: find a better format picking scheme // can get available formats via: auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
  auto desiredColorFormat = vk::Format::eB8G8R8A8Unorm; // or vk::Format::eB8G8R8A8Srgb;
  auto desiredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  auto vkbSwapchain = vkb::SwapchainBuilder{ vkbDevice }
    .set_desired_format({ static_cast<VkFormat>(desiredColorFormat), static_cast<VkColorSpaceKHR>(desiredColorSpace) }) // default
    .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) // default. other: VK_PRESENT_MODE_FIFO_KHR
    .set_required_min_image_count(NUM_IMAGES)
    .build().value();
  assert(vkbSwapchain.image_format == static_cast<VkFormat>(desiredColorFormat));
  assert(vkbSwapchain.color_space == static_cast<VkColorSpaceKHR>(desiredColorSpace));
  vk::raii::SwapchainKHR swapChain{ device, vkbSwapchain.swapchain };
  // TODO: implement recreateSwapchain which recreates FrameBuffers, CommandPools, and CommandBuffers with it

  //---- Queues

  vk::raii::Queue graphicsQueue{ device, vkbDevice.get_queue(vkb::QueueType::graphics).value() };
  vk::raii::Queue presentQueue{ device, vkbDevice.get_queue(vkb::QueueType::present).value() };
  uint32_t graphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  //---- RenderPass
  if (false)
  {
    std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
    attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
      desiredColorFormat,
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
    vk::raii::RenderPass renderPass{ device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };
  }
  std::array<vk::AttachmentDescription, 1> attachmentDescriptions;
  attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
    desiredColorFormat,
    vk::SampleCountFlagBits::e1, // TODO: try MSAA
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::ePresentSrcKHR);
  vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
  vk::SubpassDescription subpass(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, nullptr);
  vk::raii::RenderPass renderPass{ device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };

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
  vk::raii::ShaderModule vertexShader = vku::spirv::makeShaderModule(device, vk::ShaderStageFlagBits::eVertex, vertexShaderStr);
  vk::raii::ShaderModule fragmentShader = vku::spirv::makeShaderModule(device, vk::ShaderStageFlagBits::eFragment, fragmentShaderStr);
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos = {
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShader, "main"),
    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShader, "main")
  };

  // No vertexBindingDescription and no vertexAttributeDescriptions for this example
  vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo({}, {}, {});

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

  std::array<vk::Viewport, 1> viewports = { vk::Viewport{0.f, 0.f, static_cast<float>(vkbSwapchain.extent.width), static_cast<float>(vkbSwapchain.extent.height), 0.f, 1.f} };
  std::array<vk::Rect2D, 1> scissors = { vk::Rect2D{vk::Offset2D{0, 0}, vkbSwapchain.extent} };
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

  vk::raii::PipelineLayout pipelineLayout(device, { {}, {} }); // { flags, descriptorSetLayout }

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
  vk::raii::Pipeline pipeline(device, nullptr, graphicsPipelineCreateInfo);
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
  const std::vector<VkImage>& swapchainImages = swapChain.getImages();

  // vkbSwapchain.get_image_views() is actually not a getter but creator. Instead let's create imageViews ourselves
  std::vector<vk::raii::ImageView> swapchainImageViews;
  for (const VkImage& img : swapchainImages) {
    const vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity };
    const vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    vk::ImageViewCreateInfo imageViewCreateInfo({}, img, vk::ImageViewType::e2D, desiredColorFormat, components, imageSubresourceRange);
    swapchainImageViews.emplace_back(device, imageViewCreateInfo);
  }
    
  std::vector<vk::raii::Framebuffer> framebuffers;
  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    std::array<vk::ImageView, 1> attachments = { *swapchainImageViews[i] };
    vk::FramebufferCreateInfo framebufferCreateInfo({}, *renderPass, attachments, vkbSwapchain.extent.width, vkbSwapchain.extent.height, 1);
    framebuffers.push_back(vk::raii::Framebuffer(device, framebufferCreateInfo));
  }

  //---- CommandBuffer
  vk::CommandPoolCreateInfo commandPoolCreateInfo({}, graphicsQueueFamilyIndex);
  vk::raii::CommandPool commandPool(device, commandPoolCreateInfo);
  
  
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 3);
  vk::raii::CommandBuffers commandBuffers(device, commandBufferAllocateInfo);


  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    vk::raii::CommandBuffer& cmdBuf = commandBuffers[i];

    vk::CommandBufferBeginInfo cmdBufBeginInfo{};
    vk::raii::Framebuffer& framebuffer = framebuffers[i];
    vk::Rect2D renderArea = { {0,0}, vkbSwapchain.extent };
    vk::ClearColorValue clearColorValue = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<vk::ClearValue, 1> clearColor = { clearColorValue };
    vk::RenderPassBeginInfo renderPassBeginInfo(*renderPass, *framebuffer, renderArea, clearColor);

    cmdBuf.begin(cmdBufBeginInfo);
    //std::array<vk::Viewport, 1> viewports = { vk::Viewport{ 0.f, 0.f, static_cast<float>(vkbSwapchain.extent.width), static_cast<float>(vkbSwapchain.extent.height), 0.f, 1.f } };
    //std::array<vk::Rect2D, 1> scissors = { vk::Rect2D{ vk::Offset2D{0, 0}, vkbSwapchain.extent } };
    //cmdBuf.setViewport(0, viewports);
    //cmdBuf.setScissor(0, scissors);
    cmdBuf.setViewport(0, vk::Viewport{ 0.f, 0.f, static_cast<float>(vkbSwapchain.extent.width), static_cast<float>(vkbSwapchain.extent.height), 0.f, 1.f });
    cmdBuf.setScissor(0, vk::Rect2D{ vk::Offset2D{0, 0}, vkbSwapchain.extent });
    cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    cmdBuf.draw(3, 1, 0, 0); // 3 vertices
    cmdBuf.endRenderPass();
    cmdBuf.end();
  }



  // END
  glfwDestroyWindow(window);
  glfwTerminate();

  vku::spirv::finalize();
  // Need to be destroyed explicitly becomes raii instance does not own it apparently.
  vkb::destroy_debug_utils_messenger(vkbInstance.instance, vkbInstance.debug_messenger, vkbInstance.allocation_callbacks);

  std::cout << "Bye, Vulkan!\n";
  return 0;
}

