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

EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage);
bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, std::string const& glslShader, std::vector<unsigned int>& spvShader);
const TLimits limits = {
  /* .nonInductiveForLoops = */ 1,
  /* .whileLoops = */ 1,
  /* .doWhileLoops = */ 1,
  /* .generalUniformIndexing = */ 1,
  /* .generalAttributeMatrixVectorIndexing = */ 1,
  /* .generalVaryingIndexing = */ 1,
  /* .generalSamplerIndexing = */ 1,
  /* .generalVariableIndexing = */ 1,
  /* .generalConstantMatrixVectorIndexing = */ 1,
};
TBuiltInResource DefaultTBuiltInResource = {
  /* .MaxLights = */ 32,
  /* .MaxClipPlanes = */ 6,
  /* .MaxTextureUnits = */ 32,
  /* .MaxTextureCoords = */ 32,
  /* .MaxVertexAttribs = */ 64,
  /* .MaxVertexUniformComponents = */ 4096,
  /* .MaxVaryingFloats = */ 64,
  /* .MaxVertexTextureImageUnits = */ 32,
  /* .MaxCombinedTextureImageUnits = */ 80,
  /* .MaxTextureImageUnits = */ 32,
  /* .MaxFragmentUniformComponents = */ 4096,
  /* .MaxDrawBuffers = */ 32,
  /* .MaxVertexUniformVectors = */ 128,
  /* .MaxVaryingVectors = */ 8,
  /* .MaxFragmentUniformVectors = */ 16,
  /* .MaxVertexOutputVectors = */ 16,
  /* .MaxFragmentInputVectors = */ 15,
  /* .MinProgramTexelOffset = */ -8,
  /* .MaxProgramTexelOffset = */ 7,
  /* .MaxClipDistances = */ 8,
  /* .MaxComputeWorkGroupCountX = */ 65535,
  /* .MaxComputeWorkGroupCountY = */ 65535,
  /* .MaxComputeWorkGroupCountZ = */ 65535,
  /* .MaxComputeWorkGroupSizeX = */ 1024,
  /* .MaxComputeWorkGroupSizeY = */ 1024,
  /* .MaxComputeWorkGroupSizeZ = */ 64,
  /* .MaxComputeUniformComponents = */ 1024,
  /* .MaxComputeTextureImageUnits = */ 16,
  /* .MaxComputeImageUniforms = */ 8,
  /* .MaxComputeAtomicCounters = */ 8,
  /* .MaxComputeAtomicCounterBuffers = */ 1,
  /* .MaxVaryingComponents = */ 60,
  /* .MaxVertexOutputComponents = */ 64,
  /* .MaxGeometryInputComponents = */ 64,
  /* .MaxGeometryOutputComponents = */ 128,
  /* .MaxFragmentInputComponents = */ 128,
  /* .MaxImageUnits = */ 8,
  /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
  /* .MaxCombinedShaderOutputResources = */ 8,
  /* .MaxImageSamples = */ 0,
  /* .MaxVertexImageUniforms = */ 0,
  /* .MaxTessControlImageUniforms = */ 0,
  /* .MaxTessEvaluationImageUniforms = */ 0,
  /* .MaxGeometryImageUniforms = */ 0,
  /* .MaxFragmentImageUniforms = */ 8,
  /* .MaxCombinedImageUniforms = */ 8,
  /* .MaxGeometryTextureImageUnits = */ 16,
  /* .MaxGeometryOutputVertices = */ 256,
  /* .MaxGeometryTotalOutputComponents = */ 1024,
  /* .MaxGeometryUniformComponents = */ 1024,
  /* .MaxGeometryVaryingComponents = */ 64,
  /* .MaxTessControlInputComponents = */ 128,
  /* .MaxTessControlOutputComponents = */ 128,
  /* .MaxTessControlTextureImageUnits = */ 16,
  /* .MaxTessControlUniformComponents = */ 1024,
  /* .MaxTessControlTotalOutputComponents = */ 4096,
  /* .MaxTessEvaluationInputComponents = */ 128,
  /* .MaxTessEvaluationOutputComponents = */ 128,
  /* .MaxTessEvaluationTextureImageUnits = */ 16,
  /* .MaxTessEvaluationUniformComponents = */ 1024,
  /* .MaxTessPatchComponents = */ 120,
  /* .MaxPatchVertices = */ 32,
  /* .MaxTessGenLevel = */ 64,
  /* .MaxViewports = */ 16,
  /* .MaxVertexAtomicCounters = */ 0,
  /* .MaxTessControlAtomicCounters = */ 0,
  /* .MaxTessEvaluationAtomicCounters = */ 0,
  /* .MaxGeometryAtomicCounters = */ 0,
  /* .MaxFragmentAtomicCounters = */ 8,
  /* .MaxCombinedAtomicCounters = */ 8,
  /* .MaxAtomicCounterBindings = */ 1,
  /* .MaxVertexAtomicCounterBuffers = */ 0,
  /* .MaxTessControlAtomicCounterBuffers = */ 0,
  /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
  /* .MaxGeometryAtomicCounterBuffers = */ 0,
  /* .MaxFragmentAtomicCounterBuffers = */ 1,
  /* .MaxCombinedAtomicCounterBuffers = */ 1,
  /* .MaxAtomicCounterBufferSize = */ 16384,
  /* .MaxTransformFeedbackBuffers = */ 4,
  /* .MaxTransformFeedbackInterleavedComponents = */ 64,
  /* .MaxCullDistances = */ 8,
  /* .MaxCombinedClipAndCullDistances = */ 8,
  /* .MaxSamples = */ 4,
  /* .maxMeshOutputVerticesNV = */ 256,
  /* .maxMeshOutputPrimitivesNV = */ 512,
  /* .maxMeshWorkGroupSizeX_NV = */ 32,
  /* .maxMeshWorkGroupSizeY_NV = */ 1,
  /* .maxMeshWorkGroupSizeZ_NV = */ 1,
  /* .maxTaskWorkGroupSizeX_NV = */ 32,
  /* .maxTaskWorkGroupSizeY_NV = */ 1,
  /* .maxTaskWorkGroupSizeZ_NV = */ 1,
  /* .maxMeshViewCountNV = */ 4,
  /* .maxMeshOutputVerticesEXT = */ 256,
  /* .maxMeshOutputPrimitivesEXT = */ 256,
  /* .maxMeshWorkGroupSizeX_EXT = */ 128,
  /* .maxMeshWorkGroupSizeY_EXT = */ 128,
  /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
  /* .maxTaskWorkGroupSizeX_EXT = */ 128,
  /* .maxTaskWorkGroupSizeY_EXT = */ 128,
  /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
  /* .maxMeshViewCountEXT = */ 4,
  /* .maxDualSourceDrawBuffersEXT = */ 1,

  ///* .limits = */ limits, 
};


int main() {
  DefaultTBuiltInResource.limits = limits;

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


  // END
  glfwDestroyWindow(window);
  glfwTerminate();

  // Need to be destroyed explicitly becomes raii instance does not own it apparently.
  vkb::destroy_debug_utils_messenger(vkbInstance.instance, vkbInstance.debug_messenger, vkbInstance.allocation_callbacks);

  std::cout << "Bye, Vulkan!\n";
  return 0;
}


EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage)
{
  switch (stage)
  {
  case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
  case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
  case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
  case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
  case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
  case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
  case vk::ShaderStageFlagBits::eRaygenNV: return EShLangRayGenNV;
  case vk::ShaderStageFlagBits::eAnyHitNV: return EShLangAnyHitNV;
  case vk::ShaderStageFlagBits::eClosestHitNV: return EShLangClosestHitNV;
  case vk::ShaderStageFlagBits::eMissNV: return EShLangMissNV;
  case vk::ShaderStageFlagBits::eIntersectionNV: return EShLangIntersectNV;
  case vk::ShaderStageFlagBits::eCallableNV: return EShLangCallableNV;
  case vk::ShaderStageFlagBits::eTaskNV: return EShLangTaskNV;
  case vk::ShaderStageFlagBits::eMeshNV: return EShLangMeshNV;
  default: assert(false && "Unknown shader stage"); return EShLangVertex;
  }
}

bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, std::string const& glslShader, std::vector<unsigned int>& spvShader)
{
  EShLanguage stage = translateShaderStage(shaderType);

  const char* shaderStrings[1];
  shaderStrings[0] = glslShader.data();

  glslang::TShader shader(stage);
  shader.setStrings(shaderStrings, 1);

  // Enable SPIR-V and Vulkan rules when parsing GLSL
  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

  if (!shader.parse(&DefaultTBuiltInResource, 100, false, messages))
  {
    puts(shader.getInfoLog());
    puts(shader.getInfoDebugLog());
    return false;  // something didn't work
  }

  glslang::TProgram program;
  program.addShader(&shader);

  //
  // Program-level processing...
  //

  if (!program.link(messages))
  {
    puts(shader.getInfoLog());
    puts(shader.getInfoDebugLog());
    fflush(stdout);
    return false;
  }

  glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
  return true;
}