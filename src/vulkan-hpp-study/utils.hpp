#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <iostream>

/*
* Taken from 
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/RAII_Samples/utils/utils.hpp
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/utils/utils.hpp
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/utils/utils.cpp
*/ 

namespace vku {   
#if !defined( NDEBUG )
    constexpr bool isDebugBuild = true;
#else
    constexpr bool isDebugBuild = false;
#endif
}

namespace vk {
    namespace su {
        std::vector<std::string> getInstanceExtensions();

        vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT();

        VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
            void* /*pUserData*/);

        std::vector<char const*> gatherLayers(std::vector<std::string> const& layers, std::vector<vk::LayerProperties> const& layerProperties);

        std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions, std::vector<vk::ExtensionProperties> const& extensionProperties);

        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
            makeInstanceCreateInfoChainWithDebug(vk::ApplicationInfo const& applicationInfo,
                std::vector<char const*> const& layers,
                std::vector<char const*> const& extensions);

        uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties);
    }

    namespace raii {
        namespace su {
            vk::raii::Instance makeInstance(vk::raii::Context const& context,
                std::string const& appName,
                std::string const& engineName,
                std::vector<std::string> const& layers = {},
                std::vector<std::string> const& extensions = {},
                uint32_t                         apiVersion = VK_API_VERSION_1_3);
        }

    }
}

