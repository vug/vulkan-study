#pragma once

#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
        template <class T>
        VULKAN_HPP_INLINE constexpr const T& clamp(const T& v, const T& lo, const T& hi)
        {
            return v < lo ? lo : hi < v ? hi : v;
        }

        template <typename TargetType, typename SourceType>
        VULKAN_HPP_INLINE TargetType checked_cast(SourceType value)
        {
            static_assert(sizeof(TargetType) <= sizeof(SourceType), "No need to cast from smaller to larger type!");
            static_assert(std::numeric_limits<SourceType>::is_integer, "Only integer types supported!");
            static_assert(!std::numeric_limits<SourceType>::is_signed, "Only unsigned types supported!");
            static_assert(std::numeric_limits<TargetType>::is_integer, "Only integer types supported!");
            static_assert(!std::numeric_limits<TargetType>::is_signed, "Only unsigned types supported!");
            assert(value <= std::numeric_limits<TargetType>::max());
            return static_cast<TargetType>(value);
        }

        // Instead of this I implemented `WindowData::getVulkanInstanceExtensions` which relies on `glfwGetRequiredInstanceExtensions()`
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

        std::vector<std::string> getDeviceExtensions();

        // TODO: This API is bad, copy-pasted and then quickly added logic for acquiring vulkanExtensions. Moved fast. 
        class WindowData
        {
        public:
            WindowData(GLFWwindow* wnd, std::string const& name, vk::Extent2D const& extent);
            WindowData(const WindowData&) = delete;
            // Used when `createWindow()` returns. Don't know why = default does not work.
            WindowData(WindowData&& other);
            ~WindowData() noexcept;

            GLFWwindow* handle;
            std::string  name;
            vk::Extent2D extent;
            std::vector<std::string> vulkanExtensions;

            const std::vector<std::string>& getVulkanInstanceExtensions() const { return vulkanExtensions; }
        };

        WindowData createWindow(std::string const& windowName, vk::Extent2D const& extent);
    }

    namespace raii {
        namespace su {
            vk::raii::Instance makeInstance(vk::raii::Context const& context,
                std::string const& appName,
                std::string const& engineName,
                std::vector<std::string> const& layers = {},
                std::vector<std::string> const& extensions = {},
                uint32_t                         apiVersion = VK_API_VERSION_1_3);

            vk::raii::Device makeDevice(vk::raii::PhysicalDevice const& physicalDevice,
                uint32_t queueFamilyIndex,
                std::vector<std::string> const& extensions = {},
                vk::PhysicalDeviceFeatures const* physicalDeviceFeatures = nullptr,
                void const* pNext = nullptr);
        }
    }
}

