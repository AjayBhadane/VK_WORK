#ifndef __VK_UTILS_HPP
#define __VK_UTILS_HPP

#include <GLFW/glfw3.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <optional>

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentMode;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


// Get supported swapchain

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

inline bool checkValidationSupport(std::vector<const char*> validationLayers){
    uint32_t layerCount; // Available validation layers
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Data regarding the validation layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char* layerName : validationLayers){
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers){
            if(strcmp(layerName, layerProperties.layerName)){
                layerFound = true;
                break;
            }
        }

        if(!layerFound){
            return false;
        }
    }

    return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice,  VkSurfaceKHR surface){
    SwapChainSupportDetails details;
    return details;
}

inline std::vector<const char*> getRequiredVkExtensions(bool pEnableValidationLayers){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if(pEnableValidationLayers){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

inline void populateDebugMessengerCreateInfo
    (VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo,
    PFN_vkDebugUtilsMessengerCallbackEXT pDebugCallback){
        
    pCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    pCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    pCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    pCreateInfo.pfnUserCallback = pDebugCallback;
}

inline VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"
    );

    if(func != nullptr){
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = NULL){
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties
        (device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    VkBool32 presentSupport = false;    

    for(const auto& queueFamily : queueFamilies){

        if(surface != NULL){
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            

            if(presentSupport) {
                indices.presentFamily = i;                
            }
        }
        
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
        }

        if(indices.isComplete()){
            break;
        }                

        i++;
    }    

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device){

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions (deviceExtensions.begin(), deviceExtensions.end());


    for(const auto& extension : availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

inline bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface = NULL){
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    return indices.isComplete() && checkDeviceExtensionSupport(device);

}

inline uint32_t getQueueFamily(VkPhysicalDevice physicalDevice){
    uint32_t queueFamilyIndex;
    uint32_t queueFamilyCount;
    
    vkGetPhysicalDeviceQueueFamilyProperties
        (physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties
        (physicalDevice, &queueFamilyCount, queueProperties.data());

    uint32_t i = 0;

    for(auto queueProperty : queueProperties){
        if(queueProperty.queueFlags == VK_QUEUE_GRAPHICS_BIT){
            queueFamilyIndex = queueProperty.queueFlags;
            break;
        }

        i++;
    }

    return i;
}

#endif /** __VK_UTILS_HPP */
