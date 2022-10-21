#include "SpirvHelper.hpp"

//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glslang/SPIRV/GlslangToSpv.h>
//#include <glslang/Include/ResourceLimits.h>
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

  // GLFW init
  glfwInit();
  glfwSetErrorCallback([](int error, const char* msg) { std::cerr << "glfw: " << "(" << error << ") " << msg << std::endl; });
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 800, "Example Vulkan Application", nullptr, nullptr);

  uint32_t count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  std::vector<std::string> vulkanInstanceExtensions;
  for (size_t ix = 0; ix < count; ++ix)
    vulkanInstanceExtensions.push_back(extensions[ix]);

  // Instance
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

  // Surface
  VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
  glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &vkSurface);
  vk::raii::SurfaceKHR surface{ instance, vkSurface };

  // Physical Device
  vkb::PhysicalDeviceSelector phys_device_selector(vkbInstance);
  auto vkbPhysicalDevice = phys_device_selector
    .set_surface(*surface)
    .select()
    .value();
  vk::raii::PhysicalDevice physicalDevice{ instance, vkbPhysicalDevice.physical_device };

  // Logical Device
  auto vkbDevice = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value();
  vk::raii::Device device{ physicalDevice, vkbDevice.device };

  // Swapchain
  // TODO: find a better format picking scheme // can get available formats via: auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
  auto desiredColorFormat = vk::Format::eB8G8R8A8Unorm; // or vk::Format::eB8G8R8A8Srgb;
  auto desiredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  auto vkbSwapchain = vkb::SwapchainBuilder{ vkbDevice }
    .set_desired_format({ static_cast<VkFormat>(desiredColorFormat), static_cast<VkColorSpaceKHR>(desiredColorSpace) }) // default
    .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) // default. other: VK_PRESENT_MODE_FIFO_KHR
    .build().value();
  assert(vkbSwapchain.image_format == static_cast<VkFormat>(desiredColorFormat));
  assert(vkbSwapchain.color_space == static_cast<VkColorSpaceKHR>(desiredColorSpace));
  vk::raii::SwapchainKHR swapChain{ device, vkbSwapchain.swapchain };
  // TODO: implement recreateSwapchain which recreates FrameBuffers, CommandPools, and CommandBuffers with it

  // Queues
  vk::raii::Queue graphicsQueue{ device, vkbDevice.get_queue(vkb::QueueType::graphics).value() };
  vk::raii::Queue presentQueue{ device, vkbDevice.get_queue(vkb::QueueType::present).value() };

  // RenderPass
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
  std::vector<unsigned int> vertexShaderSpv;
  std::vector<unsigned int> fragmentShaderSpv;
  const bool vertexShaderCompilationSuccessful = vku::spirv::GLSLtoSPV(vk::ShaderStageFlagBits::eVertex, vertexShaderStr, vertexShaderSpv);
  const bool fragmentShaderCompilationSuccessful = vku::spirv::GLSLtoSPV(vk::ShaderStageFlagBits::eFragment, fragmentShaderStr, fragmentShaderSpv);
  std::cout << "vert successful: " << vertexShaderCompilationSuccessful << ", frag succesful: " << fragmentShaderCompilationSuccessful << '\n';
  vku::spirv::finalize();

  // END
  glfwDestroyWindow(window);
  glfwTerminate();

  // Need to be destroyed explicitly becomes raii instance does not own it apparently.
  vkb::destroy_debug_utils_messenger(vkbInstance.instance, vkbInstance.debug_messenger, vkbInstance.allocation_callbacks);

  std::cout << "Bye, Vulkan!\n";
  return 0;
}
