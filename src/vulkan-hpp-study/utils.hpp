#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <iostream>

/*
* Taken from 
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/RAII_Samples/utils/utils.hpp
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/utils/utils.hpp
- https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/utils/utils.cpp
*/ 

namespace vk {
    namespace su {
        VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
            void* /*pUserData*/);

        std::vector<char const*> gatherLayers(std::vector<std::string> const& layers
#if !defined( NDEBUG )
            ,
            std::vector<vk::LayerProperties> const& layerProperties
#endif
        );

        std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions
#if !defined( NDEBUG )
            ,
            std::vector<vk::ExtensionProperties> const& extensionProperties
#endif
        );

#if defined( NDEBUG )
        vk::StructureChain<vk::InstanceCreateInfo>
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
            makeInstanceCreateInfoChain(vk::ApplicationInfo const& applicationInfo,
                std::vector<char const*> const& layers,
                std::vector<char const*> const& extensions);
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

