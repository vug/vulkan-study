#include "ImGuiHelper.hpp"

#include "VulkanContext.hpp"

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
//#include "dependencies/imnodes.h"

#include <array>
#include <cassert>

namespace vku {
	ImGuiHelper::ImGuiHelper(const VulkanContext& vc, const Window& win)
		: vc(vc), win(win) {

		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		{
			VkDescriptorPoolCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			info.maxSets = 1000;
			info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
			info.pPoolSizes = pool_sizes;

			VkResult result = vkCreateDescriptorPool(*vc.device, &info, nullptr, &imguiPool);
			assert(result == VK_SUCCESS);
		}


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImNodes::CreateContext();
		ImGui_ImplGlfw_InitForVulkan(win.getGLFWWindow(), true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = *vc.instance;
		init_info.PhysicalDevice = *vc.physicalDevice;
		init_info.Device = *vc.device;
		//init_info.QueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
		init_info.Queue = *vc.graphicsQueue;
		//init_info.PipelineCache = g_PipelineCache;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = static_cast<uint32_t>(vc.NUM_IMAGES);
		init_info.ImageCount = static_cast<uint32_t>(vc.NUM_IMAGES);
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		//init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, *vc.renderPass);

		// Upload fonts
		{
			VkCommandPool command_pool = *vc.commandPool;
			VkCommandBuffer command_buffer = *vc.copyCommandBuffer;

			vkResetCommandPool(*vc.device, command_pool, 0);
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(command_buffer, &begin_info);

			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			vkEndCommandBuffer(command_buffer);
			vkQueueSubmit(*vc.graphicsQueue, 1, &end_info, VK_NULL_HANDLE);

			vkDeviceWaitIdle(*vc.device);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	ImGuiHelper::~ImGuiHelper() {
		vkDestroyDescriptorPool(*vc.device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		//ImNodes::DestroyContext();
		ImGui::DestroyContext();
	}

	void ImGuiHelper::Begin() const {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiHelper::End() const {
		ImGui::Render();
	}

	void ImGuiHelper::AddDrawCalls(const VkCommandBuffer& cmdBuf) const {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
	}

	void ImGuiHelper::ShowDemoWindow() {
		static bool isDemoWindowOpen = true;
		ImGui::ShowDemoWindow(&isDemoWindowOpen);
	}
}
