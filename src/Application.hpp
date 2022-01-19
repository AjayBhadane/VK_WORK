#pragma once

#include <cstdint>
#include <vector>
 #define NDEBUG
#define VK_USE_PLATFORM_XCB_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include <iostream>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

#include <shaderc/shaderc.hpp>

void framebuffer_size_callback(GLFWwindow*, int, int);

struct VulkanShader{
    VkShaderModule  vertexShaderModule;
    VkShaderModule  fragmentShaderModule;
};

struct WindowInfo{
    GLFWwindow* handle;
    int width, height;
};

struct VulkanInstance{
    VkInstance          instance        = VK_NULL_HANDLE;
    VkPhysicalDevice    physicalDevice  = VK_NULL_HANDLE;
    VkDevice            device          = VK_NULL_HANDLE;    
};

struct VulkanQueue{
    VkQueue     graphicsQueue;
    VkQueue     presentQueue;
    uint32_t    graphicsFamilyIndex;
    uint32_t    presentFamilyIndex;
};

struct VulkanSwapChain{
    VkSwapchainKHR              swapChain;
    std::vector<VkImage>        swapChainImages;
    VkFormat                    swapChainImageFormat;
    VkExtent2D                  swapChainExtent;
    std::vector<VkImageView>    swapChainImageViews;
    std::vector<VkFramebuffer>  swapChainFramebuffers;
};

struct VulkanSurface{
    VkSurfaceKHR surface;
};

struct VulkanRenderPass{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffer;
};

static WindowInfo initWindow(int, int, const char*);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*,
    void*
);

class App{
    private:
        WindowInfo window;

        // Vulkan vars
        VulkanInstance  mInstance;
        VulkanQueue     mQueue;
        VulkanSurface   mSurface;
        VulkanSwapChain mSwapChain;
        VulkanShader    mShader;
        VulkanRenderPass mRenderPass;

        #ifdef NDEBUG
            bool mEnableValidationLayers = false;
        #else
            bool mEnableValidationLayers = true;
        #endif

        VkDebugUtilsMessengerEXT debugMessenger = NULL;

        double currentFrame = 0;
        double lastFrame = currentFrame;
        double deltaTime;        
        
        void loop();
        void calculateDeltaTime();

        // Vulkan setup
        bool mVkCreateInstance();
        bool mSetupDebugMessenger();
        bool mCreateSurface();
        bool mPickPhysicalDevice();
        bool mCreateLogicalDevice();
        bool mCreateSwapChain();
        bool mCreateImageViews();
        bool mCreateRenderPass();
        bool mCreateGraphicsPipeline();
        bool mCreateFrameBuffers();
        bool mCreateCommandpool();
        void mCreateCommandBuffers();

        // vulkan cleanup
        void cleanup();

    public:
        App(int, int, const char*);
        
        virtual void initDraw() = 0;
        virtual void draw() = 0;

        void start();

        WindowInfo getWindow() const;

        bool keyPressed(int) const;
        void terminate() const;

        double getDeltaTime();
};
