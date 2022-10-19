#include "utils.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <iostream>
#include <string>

static std::string AppName = "03_InitDeviceRAII";
static std::string EngineName = "Vulkan.hpp";

int main() {
	std::cout << "Hello, Vulkan!\n";

	vk::raii::Context context;
	vk::ApplicationInfo applicationInfo(AppName.c_str(), 1, EngineName.c_str(), 1, VK_API_VERSION_1_3);
	vk::raii::Instance instance = vk::raii::su::makeInstance(context, AppName, EngineName);
#if !defined( NDEBUG )
	vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger(instance, vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif
	vk::raii::PhysicalDevices physicalDevices(instance);
	// simple test that physical device found
	auto features = physicalDevices.front().getFeatures();
	std::cout << "numDevices: " << physicalDevices.size() << ", physicalDevice[0] has multi-viewport: " << features.multiViewport << '\n';

	std::cout << "Bye, Vulkan!\n";
	return 0;
}