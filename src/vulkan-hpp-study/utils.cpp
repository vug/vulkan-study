#include "utils.hpp"

namespace vk {
    namespace su {
        std::vector<std::string> getInstanceExtensions()
        {
            std::vector<std::string> extensions;
            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
            extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_IOS_MVK )
            extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_MACOS_MVK )
            extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_MIR_KHR )
            extensions.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_VI_NN )
            extensions.push_back(VK_NN_VI_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
            extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
            extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XCB_KHR )
            extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
            extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
            extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
#endif
            return extensions;
        }

        vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT()
        {
            return { {},
                     vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                     &vk::su::debugUtilsMessengerCallback };
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
            void* /*pUserData*/)
        {
            if (vku::isDebugBuild) {
                if (pCallbackData->messageIdNumber == 648835635)
                {
                    // UNASSIGNED-khronos-Validation-debug-build-warning-message
                    return VK_FALSE;
                }
                if (pCallbackData->messageIdNumber == 767975156)
                {
                    // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
                    return VK_FALSE;
                }
            }

            std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
                << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
            std::cerr << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
            std::cerr << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
            std::cerr << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
            if (0 < pCallbackData->queueLabelCount)
            {
                std::cerr << std::string("\t") << "Queue Labels:\n";
                for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
                {
                    std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
                }
            }
            if (0 < pCallbackData->cmdBufLabelCount)
            {
                std::cerr << std::string("\t") << "CommandBuffer Labels:\n";
                for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
                {
                    std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
                }
            }
            if (0 < pCallbackData->objectCount)
            {
                std::cerr << std::string("\t") << "Objects:\n";
                for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
                {
                    std::cerr << std::string("\t\t") << "Object " << i << "\n";
                    std::cerr << std::string("\t\t\t") << "objectType   = " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
                        << "\n";
                    std::cerr << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
                    if (pCallbackData->pObjects[i].pObjectName)
                    {
                        std::cerr << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
                    }
                }
            }
            return VK_TRUE;
        }

        std::vector<char const*> gatherLayers(std::vector<std::string> const& layers, std::vector<vk::LayerProperties> const& layerProperties)
        {
            std::vector<char const*> enabledLayers;
            enabledLayers.reserve(layers.size());
            for (auto const& layer : layers)
            {
                assert(std::find_if(layerProperties.begin(), layerProperties.end(), [layer](vk::LayerProperties const& lp) { return layer == lp.layerName; }) !=
                    layerProperties.end());
                enabledLayers.push_back(layer.data());
            }

            if (vku::isDebugBuild) {
                // Enable standard validation layer to find as much errors as possible!
                const char* standard_validation_layer = "VK_LAYER_KHRONOS_validation";
                if (std::find(layers.begin(), layers.end(), standard_validation_layer) == layers.end() &&
                    std::find_if(layerProperties.begin(),
                        layerProperties.end(),
                        [&](vk::LayerProperties const& lp)
                        { return (strcmp(standard_validation_layer, lp.layerName) == 0); }) != layerProperties.end())
                {
                    enabledLayers.push_back(standard_validation_layer);
                }
            }

            return enabledLayers;
        }

        std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions, std::vector<vk::ExtensionProperties> const& extensionProperties)
        {
            std::vector<char const*> enabledExtensions;
            enabledExtensions.reserve(extensions.size());
            for (auto const& ext : extensions)
            {
                assert(std::find_if(extensionProperties.begin(),
                    extensionProperties.end(),
                    [ext](vk::ExtensionProperties const& ep) { return ext == ep.extensionName; }) != extensionProperties.end());
                enabledExtensions.push_back(ext.data());
            }

            if (vku::isDebugBuild) {
                if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extensions.end() &&
                    std::find_if(extensionProperties.begin(),
                        extensionProperties.end(),
                        [](vk::ExtensionProperties const& ep)
                        { return (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0); }) != extensionProperties.end())
                {
                    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }
            }
            return enabledExtensions;
        }

        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
            makeInstanceCreateInfoChainWithDebug(vk::ApplicationInfo const& applicationInfo,
                std::vector<char const*> const& layers,
                std::vector<char const*> const& extensions)
        {
            // in debug mode, addionally use the debugUtilsMessengerCallback in instance creation!
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
            vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
                { {}, &applicationInfo, layers, extensions }, { {}, severityFlags, messageTypeFlags, &vk::su::debugUtilsMessengerCallback });
            return instanceCreateInfo;
        }


        uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties)
        {
            // get the first index into queueFamiliyProperties which supports graphics
            std::vector<vk::QueueFamilyProperties>::const_iterator graphicsQueueFamilyProperty =
                std::find_if(queueFamilyProperties.begin(),
                    queueFamilyProperties.end(),
                    [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });
            assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());
            return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
        }

        std::vector<std::string> getDeviceExtensions()
        {
            return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        }

        //----------- WindowData
        WindowData::WindowData(GLFWwindow* wnd, std::string const& name, vk::Extent2D const& extent) : handle{ wnd }, name{ name }, extent{ extent } {}

        WindowData::WindowData(WindowData&& other) : handle{}, name{}, extent{}, vulkanExtensions{}
        {
            std::swap(handle, other.handle);
            std::swap(name, other.name);
            std::swap(extent, other.extent);
            std::swap(vulkanExtensions, other.vulkanExtensions);
        }

        WindowData::~WindowData() noexcept
        {
            glfwDestroyWindow(handle);
        }

        WindowData createWindow(std::string const& windowName, vk::Extent2D const& extent)
        {
            struct glfwContext
            {
                glfwContext()
                {
                    glfwInit();
                    glfwSetErrorCallback(
                        [](int error, const char* msg)
                        {
                            std::cerr << "glfw: "
                                << "(" << error << ") " << msg << std::endl;
                        });                        
                }

                ~glfwContext()
                {
                    glfwTerminate();
                }
            };

            static auto glfwCtx = glfwContext();
            (void)glfwCtx;

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            GLFWwindow* window = glfwCreateWindow(extent.width, extent.height, windowName.c_str(), nullptr, nullptr);
            auto wd = WindowData(window, windowName, extent);

            uint32_t count;
            const char** extensions = glfwGetRequiredInstanceExtensions(&count);
            for (size_t ix = 0; ix < count; ++ix)
                wd.vulkanExtensions.push_back(extensions[ix]);
                
            return wd;
        }
    }
}

namespace vk {
    namespace raii {
        namespace su {
            vk::raii::Instance makeInstance(vk::raii::Context const& context,
                std::string const& appName,
                std::string const& engineName,
                std::vector<std::string> const& layers,
                std::vector<std::string> const& extensions,
                uint32_t                         apiVersion)
            {
                vk::ApplicationInfo       applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, apiVersion);
                const auto layerProperties = vku::isDebugBuild ? context.enumerateInstanceLayerProperties() : std::vector<vk::LayerProperties>{};
                std::vector<char const*> enabledLayers = vk::su::gatherLayers(layers, layerProperties);
                const auto extensionProperties = vku::isDebugBuild ? context.enumerateInstanceExtensionProperties() : std::vector<vk::ExtensionProperties>{};
                std::vector<char const*> enabledExtensions = vk::su::gatherExtensions(extensions, extensionProperties);

                if (vku::isDebugBuild) {
                    const auto instanceCreateInfoChain = vk::su::makeInstanceCreateInfoChainWithDebug(applicationInfo, enabledLayers, enabledExtensions);
                    return vk::raii::Instance(context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
                }
                else {
                    return vk::raii::Instance(context, vk::InstanceCreateInfo{ {}, &applicationInfo, enabledLayers, enabledExtensions });
                }                   
            }

            vk::raii::Device makeDevice(vk::raii::PhysicalDevice const& physicalDevice,
                uint32_t queueFamilyIndex,
                std::vector<std::string> const& extensions,
                vk::PhysicalDeviceFeatures const* physicalDeviceFeatures,
                void const* pNext)
            {
                std::vector<char const*> enabledExtensions;
                enabledExtensions.reserve(extensions.size());
                for (auto const& ext : extensions)
                {
                    enabledExtensions.push_back(ext.data());
                }

                float                     queuePriority = 0.0f;
                vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority);
                vk::DeviceCreateInfo      deviceCreateInfo(vk::DeviceCreateFlags(), deviceQueueCreateInfo, {}, enabledExtensions, physicalDeviceFeatures, pNext);
                return vk::raii::Device(physicalDevice, deviceCreateInfo);
            }
        }
    }
}