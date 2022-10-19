#include "utils.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

static std::string AppName = "04_InitCommandBufferRAII";
static std::string EngineName = "Vulkan.hpp";

int main() {
	std::cout << "Hello, Vulkan!\n";

	vk::raii::Context context;
	vk::ApplicationInfo applicationInfo(AppName.c_str(), 1, EngineName.c_str(), 1, VK_API_VERSION_1_3);
	vk::raii::Instance instance = vk::raii::su::makeInstance(context, AppName, EngineName);
#if !defined( NDEBUG )
	vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(instance, vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif
	vk::raii::PhysicalDevice physicalDevice = vk::raii::PhysicalDevices(instance).front();
	uint32_t graphicsQueueFamilyIndex = vk::su::findGraphicsQueueFamilyIndex(physicalDevice.getQueueFamilyProperties());
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