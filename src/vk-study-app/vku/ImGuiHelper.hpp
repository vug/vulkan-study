#pragma once

#include <vulkan/vulkan.h>

namespace vku {
	class VulkanContext;
	class Window;

	class ImGuiHelper {
	public:
		ImGuiHelper(const VulkanContext& vc, const Window& win);
		~ImGuiHelper();
		void Begin() const;
		void End() const;

		void AddDrawCalls(const VkCommandBuffer& cmdBuf) const;
		void ShowDemoWindow();
	private:
		const VulkanContext& vc;
		const Window& win;
		VkDescriptorPool imguiPool;
	};
}
