//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

	glfwInit();
	glfwSetErrorCallback(
		[](int error, const char* msg)
		{
			std::cerr << "glfw: "
				<< "(" << error << ") " << msg << std::endl;
		});
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 800, "Example Vulkan Application", nullptr, nullptr);

	uint32_t count;
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);
	std::vector<std::string> vulkanInstanceExtensions;
	for (size_t ix = 0; ix < count; ++ix)
		vulkanInstanceExtensions.push_back(extensions[ix]);

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

	VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
	glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &vkSurface);
	vk::raii::SurfaceKHR surface{ instance, vkSurface };

	vkb::PhysicalDeviceSelector phys_device_selector(vkbInstance);
	auto vkbPhysicalDevice = phys_device_selector
		.set_surface(*surface)
		.select()
		.value();
	vk::raii::PhysicalDevice physicalDevice{ instance, vkbPhysicalDevice.physical_device };

	auto vkbDevice = vkb::DeviceBuilder { vkbPhysicalDevice }.build().value();
	vk::raii::Device device{ physicalDevice, vkbDevice.device};

	auto vkbSwapchain = vkb::SwapchainBuilder{ vkbDevice }.build().value();
	vk::raii::SwapchainKHR swapChain{ device, vkbSwapchain.swapchain };
	// TODO: implement recreateSwapchain which recreates FrameBuffers, CommandPools, and CommandBuffers with it


	// END

	glfwDestroyWindow(window);
	glfwTerminate();

	// Need to be destroyed explicitly becomes raii instance does not own it apparently.
	vkb::destroy_debug_utils_messenger(vkbInstance.instance, vkbInstance.debug_messenger, vkbInstance.allocation_callbacks);

	std::cout << "Bye, Vulkan!\n";
	return 0;
}
