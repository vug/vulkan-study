#pragma once

#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan.hpp>

namespace vku {
	namespace spirv
	{
		void init();
		void finalize();
		void initResources(TBuiltInResource& Resources);
		EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage);
		bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, std::string const& glsl, std::vector<unsigned int>& spv);
	}
}