#include "VulkanContext.hpp"

#include "Image.hpp"
#include "utils.hpp"

#include <VkBootstrap.h>

namespace vku {
  VulkanContext::VulkanContext(vku::Window& window, const AppSettings& appSettings) :
    appSettings(appSettings),
    window(window),
    instance(constructInstance()),
    surface(window.createSurface(instance)),
    physicalDevice(constructPhysicalDevice()),
    device(constructDevice()),
    // TODO: find a better format picking scheme // can get available formats via: auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapchainColorFormat(vk::Format::eB8G8R8A8Unorm), // or vk::Format::eB8G8R8A8Srgb;
    swapchainColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear),
    swapchainDepthFormat(vk::Format::eD16Unorm),
    swapchainSamples(vk::SampleCountFlagBits::e1),
    swapchain(constructSwapchain()),
    swapchainImageViews(constructSwapchainImageViews()),
    graphicsQueue{ device, vkbDevice.get_queue(vkb::QueueType::graphics).value() },
    presentQueue{ device, vkbDevice.get_queue(vkb::QueueType::present).value() },
    graphicsQueueFamilyIndex(vkbDevice.get_queue_index(vkb::QueueType::graphics).value()),
    renderPass(constructRenderPass()),
    framebuffers(constructFramebuffers()),
    commandPool(device, vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamilyIndex)),
    commandBuffers(device, vk::CommandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT))
  { 
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // (Semaphores begin their lifetime at "unsignaled" state)
      // Image Available -> Semaphore -> Submit Draw Calls for rendering
      imageAvailableForRenderingSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
      renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
      // Start the fence in signaled state, so that we won't wait indefinitely for frame=-1 CommandBuffer to be done
      commandBufferAvailableFences.emplace_back(device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
  }

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
    return vk::raii::Instance{ context, vkbInstance->instance };
  }

  vk::raii::PhysicalDevice VulkanContext::constructPhysicalDevice() {
    vkb::PhysicalDeviceSelector phys_device_selector(*vkbInstance);
    vkbPhysicalDevice = phys_device_selector
      .set_surface(*surface)
      .select()
      .value();
    return vk::raii::PhysicalDevice{ instance, vkbPhysicalDevice.physical_device };
  }

  vk::raii::Device VulkanContext::constructDevice() {
    vkbDevice = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value();
    return vk::raii::Device{ physicalDevice, vkbDevice.device };
  }

  vk::raii::SwapchainKHR VulkanContext::constructSwapchain() {
    vkb::Swapchain vkbSwapchain = vkb::SwapchainBuilder{ vkbDevice }
      .set_desired_format({ static_cast<VkFormat>(swapchainColorFormat), static_cast<VkColorSpaceKHR>(swapchainColorSpace) }) // default
      .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) // default. other: VK_PRESENT_MODE_FIFO_KHR
      .set_required_min_image_count(NUM_IMAGES)
      .build().value();
    assert(vkbSwapchain.image_format == static_cast<VkFormat>(swapchainColorFormat));
    assert(vkbSwapchain.color_space == static_cast<VkColorSpaceKHR>(swapchainColorSpace));
    swapchainExtent = vkbSwapchain.extent;
    return vk::raii::SwapchainKHR{ device, vkbSwapchain.swapchain };  
  }

  std::vector<vk::raii::ImageView> VulkanContext::constructSwapchainImageViews() {
    const std::vector<VkImage>& swapchainImages = swapchain.getImages();

    std::vector<vk::raii::ImageView> imgViews;
    // vkbSwapchain.get_image_views() is actually not a getter but creator. Instead let's create imageViews ourselves
    for (const VkImage& img : swapchainImages) {
      const vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity };
      const vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
      vk::ImageViewCreateInfo imageViewCreateInfo({}, img, vk::ImageViewType::e2D, swapchainColorFormat, components, imageSubresourceRange);
      imgViews.emplace_back(device, imageViewCreateInfo);

      // Note that Swapchain comes with images for color attachment but by default no images for depth attachment
      if (appSettings.hasPresentDepth)
        depthImages.emplace_back(*this, swapchainDepthFormat, swapchainExtent, swapchainSamples, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageAspectFlagBits::eDepth);
    }
    return imgViews;
  }

  vk::raii::RenderPass VulkanContext::constructRenderPass() {
    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
      swapchainColorFormat,
      swapchainSamples,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR);
    vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);

    if (!appSettings.hasPresentDepth) {
      vk::SubpassDescription subpass(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {});
      return vk::raii::RenderPass{ device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };
    }

    attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
      swapchainDepthFormat,
      swapchainSamples,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass(vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);
    return vk::raii::RenderPass{ device, vk::RenderPassCreateInfo{vk::RenderPassCreateFlags(), attachmentDescriptions, subpass} };
  }

  std::vector<vk::raii::Framebuffer> VulkanContext::constructFramebuffers() {
    std::vector<vk::raii::Framebuffer> fbs;
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
      std::vector<vk::ImageView> attachments = { *swapchainImageViews[i] };
      if (appSettings.hasPresentDepth)
        attachments.push_back(*depthImages[i].imageView);
      vk::FramebufferCreateInfo framebufferCreateInfo({}, *renderPass, attachments, swapchainExtent.width, swapchainExtent.height, 1);
      fbs.push_back(vk::raii::Framebuffer(device, framebufferCreateInfo));
    }
    return fbs; // probably unneccessary copy
  }

  void VulkanContext::recreateSwapchain() {
    // This makes rendering stop before creating the swapchain. Actually, it's possible to create new swapchain while drawing commands on the image from the old one are still in-flight
    // TODO: if recreating use "set old swapchain" option when creating new swapchain and delete old one once presentation is done. (Maybe give a flag to `constructSwapchain`)
    device.waitIdle();

    framebuffers.clear();
    swapchainImageViews.clear();
    swapchain.clear();

    swapchain = constructSwapchain();
    swapchainImageViews = constructSwapchainImageViews();
    // Recreation of the RenderpPass is usually not necessary. 
    // Only needed when image format changes during application lifetime e.g. Moving app window from standard range to HDR monitor.
    renderPass = constructRenderPass();
    framebuffers = constructFramebuffers();
  }
}