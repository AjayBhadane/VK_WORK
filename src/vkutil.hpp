#ifndef __VK_UTILS_HPP
#define __VK_UTILS_HPP

#include <GLFW/glfw3.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <optional>
#include <algorithm>

#include "../configuration/root_directory.h"

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> & code){
    VkShaderModuleCreateInfo createInfo{};
    VkShaderModule shaderModule;
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
        throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
}

inline std::vector<char> readFile(const char* path){
    std::string _path = logl_root;
    _path += path;

    FILE *f = fopen(_path.c_str(), "rb");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<char> buffer (fsize);
    fread(buffer.data(), 1, fsize, f);
    fclose(f);    
    
    return buffer;
}


struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentMode;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily, presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


// Get supported swapchain

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
    for(const auto& availableFormat : availableFormats){
        if(availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    for(const auto& availablePresentMode : availablePresentModes){
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window){
    if(capabilities.currentExtent.width != UINT32_MAX){
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, 
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, 
                                         capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if(formatCount !=0){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, nullptr);
    

    if(presentModes != 0){ 
        details.presentMode.resize(presentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModes, details.presentMode.data());
    }

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

        if(surface != nullptr){
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
    bool swapChainAdequate = false;
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    if(extensionsSupported){
        SwapChainSupportDetails swapchainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentMode.empty();
    }

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    return indices.isComplete() && extensionsSupported && swapChainAdequate;

}

#endif /** __VK_UTILS_HPP */
