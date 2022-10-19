#include "utils.hpp"

namespace vk {
    namespace su {
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
#if !defined( NDEBUG )
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
#endif

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

        std::vector<char const*> gatherLayers(std::vector<std::string> const& layers
#if !defined( NDEBUG )
            ,
            std::vector<vk::LayerProperties> const& layerProperties
#endif
        )
        {
            std::vector<char const*> enabledLayers;
            enabledLayers.reserve(layers.size());
            for (auto const& layer : layers)
            {
                assert(std::find_if(layerProperties.begin(), layerProperties.end(), [layer](vk::LayerProperties const& lp) { return layer == lp.layerName; }) !=
                    layerProperties.end());
                enabledLayers.push_back(layer.data());
            }
#if !defined( NDEBUG )
            // Enable standard validation layer to find as much errors as possible!
            if (std::find(layers.begin(), layers.end(), "VK_LAYER_KHRONOS_validation") == layers.end() &&
                std::find_if(layerProperties.begin(),
                    layerProperties.end(),
                    [](vk::LayerProperties const& lp)
                    { return (strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0); }) != layerProperties.end())
            {
                enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
            }
#endif
            return enabledLayers;
        }

        std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions
#if !defined( NDEBUG )
            ,
            std::vector<vk::ExtensionProperties> const& extensionProperties
#endif
        )
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
#if !defined( NDEBUG )
            if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extensions.end() &&
                std::find_if(extensionProperties.begin(),
                    extensionProperties.end(),
                    [](vk::ExtensionProperties const& ep)
                    { return (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0); }) != extensionProperties.end())
            {
                enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
#endif
            return enabledExtensions;
        }


#if defined( NDEBUG )
        vk::StructureChain<vk::InstanceCreateInfo>
#else
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
            makeInstanceCreateInfoChain(vk::ApplicationInfo const& applicationInfo,
                std::vector<char const*> const& layers,
                std::vector<char const*> const& extensions)
        {
#if defined( NDEBUG )
            // in non-debug mode just use the InstanceCreateInfo for instance creation
            vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo({ {}, &applicationInfo, layers, extensions });
#else
            // in debug mode, addionally use the debugUtilsMessengerCallback in instance creation!
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
            vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
                { {}, &applicationInfo, layers, extensions }, { {}, severityFlags, messageTypeFlags, &vk::su::debugUtilsMessengerCallback });
#endif
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
                std::vector<char const*> enabledLayers = vk::su::gatherLayers(layers
#if !defined( NDEBUG )
                    ,
                    context.enumerateInstanceLayerProperties()
#endif
                );
                std::vector<char const*> enabledExtensions = vk::su::gatherExtensions(extensions
#if !defined( NDEBUG )
                    ,
                    context.enumerateInstanceExtensionProperties()
#endif
                );
#if defined( NDEBUG )
                vk::StructureChain<vk::InstanceCreateInfo>
#else
                vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
                    instanceCreateInfoChain = vk::su::makeInstanceCreateInfoChain(applicationInfo, enabledLayers, enabledExtensions);

                return vk::raii::Instance(context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>());
            }
        }
    }
}