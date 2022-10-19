#include "utils.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

static std::string AppName = "04_InitCommandBufferRAII";
static std::string EngineName = "Vulkan.hpp";

int main() {
	std::cout << "Hello, Vulkan!\n";

	uint32_t           width = 64;
	uint32_t           height = 64;
	vk::su::WindowData window = vk::su::createWindow(AppName, { width, height });

	vk::raii::Context context;
	vk::raii::Instance instance = vk::raii::su::makeInstance(context, AppName, EngineName, {}, window.getVulkanInstanceExtensions());
	if(vku::isDebugBuild)
		vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(instance, vk::su::makeDebugUtilsMessengerCreateInfoEXT());
	vk::raii::PhysicalDevice physicalDevice = vk::raii::PhysicalDevices(instance).front();

	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	uint32_t graphicsQueueFamilyIndex = vk::su::findGraphicsQueueFamilyIndex(physicalDevice.getQueueFamilyProperties());

	// TODO: Helper to create vk::raii::SurfaceKHR from vk::su::WindowData
	VkSurfaceKHR       _surface;
	glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window.handle, nullptr, &_surface);
	vk::raii::SurfaceKHR surface(instance, _surface);

	// determine a queueFamilyIndex that suports present
	// first check if the graphicsQueueFamiliyIndex is good enough
	uint32_t presentQueueFamilyIndex = physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, *surface)
		? graphicsQueueFamilyIndex
		: vk::su::checked_cast<uint32_t>(queueFamilyProperties.size());

	float queuePriority = 0.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, graphicsQueueFamilyIndex, 1, &queuePriority);
	vk::DeviceCreateInfo deviceCreateInfo({}, deviceQueueCreateInfo);
	vk::raii::Device device(physicalDevice, deviceCreateInfo);

	// create a CommandPool to allocate a CommandBuffer from
	vk::CommandPoolCreateInfo commandPoolCreateInfo({}, graphicsQueueFamilyIndex);
	vk::raii::CommandPool commandPool(device, commandPoolCreateInfo);
	// allocate a CommandBuffer from the CommandPool
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, commandBufferAllocateInfo).front());

	// simple test that physical device found
	auto features = physicalDevice.getFeatures();
	std::cout << "physicalDevice has multi-viewport: " << features.multiViewport << ", graphicsQueueFamilyIndex: " << graphicsQueueFamilyIndex << '\n';

	std::cout << "Bye, Vulkan!\n";
	return 0;
}
