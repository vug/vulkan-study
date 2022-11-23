#pragma once

#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan_raii.hpp>

namespace vku {
namespace spirv {
//---- API
// Construct a shader module from given GLSL code
vk::raii::ShaderModule makeShaderModule(vk::raii::Device const& device, vk::ShaderStageFlagBits shaderStage, std::string const& glsl);

// has to be called before shader operations
void init();

// has to be called while shutting down
void finalize();

//---- Internal
// Default Resources
void initResources(TBuiltInResource& Resources);
// Translate ShaderStage input from vulkan.hpp type to glslang type
EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage);
// Compile GLSL into SPV
bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType, std::string const& glsl, std::vector<unsigned int>& spv);
}  // namespace spirv
}  // namespace vku