#include "utils.hpp"
#include "VulkanContext.hpp"

#include <VkBootstrap.h>

namespace vku {
  VulkanContext::VulkanContext(vku::Window& window) : 
    window(window), 
    instance(constructInstance()),
    surface(window.createSurface(instance)),
    physicalDevice(constructPhysicalDevice()),
    device(constructDevice()),
    swapchain(constructSwapchain())
  { }

  VulkanContext::~VulkanContext() {
    vkb::destroy_debug_utils_messenger(vkbInstance->instance, vkbInstance->debug_messenger, vkbInstance->allocation_callbacks);
  }

  vk::raii::Instance VulkanContext::constructInstance() {
    std::vector<std::string> vulkanInstanceExtensions = window.getRequiredInstanceExtensions();

    vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value(); // has methods about available layers and extensions
    for (const auto& ext : vulkanInstanceExtensions)
      assert(systemInfo.is_extension_available(ext.c_str()));

    vkbInstanceBuilder = new vkb::InstanceBuilder();
    vkbInstanceBuilder
      ->set_app_name("Example Vulkan Application")
      .require_api_version(1, 3, 0)
      .enable_validation_layers(vku::isDebugBuild) // == .enable_layer("VK_LAYER_KHRONOS_validation")
      // TODO: can create my own callback function and set it via set_debug_callback()
      // See debugUtilsMessengerCallback in utils.cpp
      .use_default_debug_messenger();
    for (const auto& ext : vulkanInstanceExtensions)
      vkbInstanceBuilder->enable_extension(ext.c_str());

    vkbInstance = new vkb::Instance();
    *vkbInstance = vkbInstanceBuilder
      ->build()
      .value();

    vk::raii::Context context;
    return vk::raii::Instance { context, vkbInstance->instance };
  }

  vk::raii::PhysicalDevice VulkanContext::constructPhysicalDevice() {
    vkb::PhysicalDeviceSelector phys_device_selector(*vkbInstance);
    vkbPhysicalDevice = phys_device_selector
      .set_surface(*surface)
      .select()
      .value();
    return vk::raii::PhysicalDevice { instance, vkbPhysicalDevice.physical_device };
  }

  vk::raii::Device VulkanContext::constructDevice() {
    vkbDevice = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value();
    return vk::raii::Device { physicalDevice, vkbDevice.device };
  }

  vk::raii::SwapchainKHR VulkanContext::constructSwapchain() {
    const uint32_t NUM_IMAGES = 3;
    // TODO: find a better format picking scheme // can get available formats via: auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapchainColorFormat = vk::Format::eB8G8R8A8Unorm; // or vk::Format::eB8G8R8A8Srgb;
    swapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vkb::Swapchain vkbSwapchain = vkb::SwapchainBuilder{ vkbDevice }
      .set_desired_format({ static_cast<VkFormat>(swapchainColorFormat), static_cast<VkColorSpaceKHR>(swapchainColorSpace) }) // default
      .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) // default. other: VK_PRESENT_MODE_FIFO_KHR
      .set_required_min_image_count(NUM_IMAGES)
      .build().value();
    assert(vkbSwapchain.image_format == static_cast<VkFormat>(swapchainColorFormat));
    assert(vkbSwapchain.color_space == static_cast<VkColorSpaceKHR>(swapchainColorSpace));
    swapchainExtent = vkbSwapchain.extent;
    return vk::raii::SwapchainKHR { device, vkbSwapchain.swapchain };
    // TODO: implement recreateSwapchain which recreates FrameBuffers, CommandPools, and CommandBuffers with it    
  }
}