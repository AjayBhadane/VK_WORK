#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_xcb.h>
#include <wayland-client-core.h>

#include <set>

#include "vkutil.hpp"

const std::vector<const char*> _validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData ) 
{   
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

App::App(int width, int height, const char* title){
    this->window = initWindow(width, height, title);
    // Try creating a vulkan instance
    if(! mVkCreateInstance()){
        // Probs no vulkan support
        std::cerr << "Vulkan initialisation failed, existing now" << std::endl;
        return;
    };

    if( !  mSetupDebugMessenger()){
        std::cerr << "Setting up debug messenger failed" << std::endl;
        return;
    }

    if(! mCreateSurface()){
        std::cerr << "Setting up drawing surface failed" << std::endl;
        return;
    }

    if (! mPickPhysicalDevice()){
        std::cerr << "Could not find a suitable device" << std::endl;
        return;
    }

    if(! mCreateLogicalDevice()){
        std::cerr << "Createing logical device failed" << std::endl;
        return;
    } 

    if(! mCreateSwapChain()) {
        std::cerr << "SwapChain creation failed" << std::endl;
        return;
    }

    if(! mCreateImageViews()){
        std::cerr << "Setting up image views failed" << std::endl;
        return;
    }
}

void App::loop(){    
    while(!glfwWindowShouldClose(this->window.handle)){                       
        this->draw();
        glfwPollEvents();
        glfwSwapBuffers(App::window.handle);
        calculateDeltaTime();
    }    
}

void App::start(){	
    this->initDraw();
    this->loop();
}

/**
 * Calculates time spent since last frame in seconds
 * NOTE TO SELF: While implementing animator convert back to ms by 
 *               multiplying with 1000
**/

void App::calculateDeltaTime(){
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;                
    }

bool App::keyPressed(int keyCode) const{
    return glfwGetKey(this->window.handle, keyCode) == GLFW_PRESS;
}

void App::terminate() const{
    glfwSetWindowShouldClose(window.handle, true);
}

static WindowInfo initWindow(int width, int height, const char* title){    
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(  width, height,
                                            title,
                                            nullptr, nullptr);


    WindowInfo windowStrct{
        nullptr,
        width,
        height
    };

    if(window == NULL){
        std::cout << "Failed to create window" << std::endl;
    }

    glfwMakeContextCurrent(window);

    windowStrct.handle = window;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    return windowStrct;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    
}

double App::getDeltaTime(){
    return this->deltaTime;
}

// Vulkan API setup

bool App::mVkCreateInstance(){

    // See if validation layer enable was requested
    if( mEnableValidationLayers && 
        !checkValidationSupport(_validationLayers)){
        
        std::cerr << "Validation layre requested but, not found" << std::endl;
        return false;

    }

    // See if vulkan instance can actually do gpu work
    std::vector<const char*> extensions = 
    getRequiredVkExtensions(mEnableValidationLayers);
    
    VkApplicationInfo appInfo{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Hello, tidy render",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No engine yet",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0
    };

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};    

    VkInstanceCreateInfo createInfo{
    	.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    	        .pNext                      = nullptr,
    	        .pApplicationInfo           = &appInfo,
    	        .enabledLayerCount          = 0 ,
    	        .ppEnabledLayerNames        = nullptr,
    	        .enabledExtensionCount      = static_cast<uint32_t>(extensions.size()),
    	        .ppEnabledExtensionNames    = extensions.data()
    };

    if(mEnableValidationLayers){
        std::cout << "Setting up debug " << std::endl;

        populateDebugMessengerCreateInfo(debugMessengerInfo, debugCallback);

        createInfo.pNext = &debugMessengerInfo;
        createInfo.enabledLayerCount = \
        static_cast<uint32_t>(_validationLayers.size());

        createInfo.ppEnabledLayerNames = _validationLayers.data();
    }

    if(vkCreateInstance(&createInfo, nullptr, &mInstance.instance) != VK_SUCCESS){
        throw std::runtime_error("failed to create instance");
    }
    
    return true;
}

bool App::mSetupDebugMessenger(){                                       
    if(!mEnableValidationLayers) return true;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo, debugCallback);

    if(CreateDebugUtilsMessengerEXT
        (mInstance.instance, &createInfo, nullptr, &debugMessenger)
        != VK_SUCCESS){

        throw std::runtime_error("failed to setup dabug message");
    }
    
    return true;
}

bool App::mCreateSurface(){    
    if( glfwCreateWindowSurface(mInstance.instance,
                                window.handle,
                                nullptr,
                                &mSurface.surface)!= VK_SUCCESS){
        return false;
    }

    // What is happening in the background
    // .......................................................

    //     _XDisplay* display = (_XDisplay*)glfwGetX11Display();
    //
    //     if(display == NULL){
    //         std::cerr << "No surface found" << std::endl;
    //         return false;
    //     }
    //
    //     xcb_connection_t* _Lconnection = xcb_connect(display->display_name, &(display->default_screen));
    //     xcb_window_t _LWindow = getActiveWindow(_Lconnection);
    //
    //     int errorCode = 0;
    //
    //     if( (errorCode = xcb_connection_has_error(_Lconnection)) != 0){
    //         std::cout << "Error code " << errorCode << std::endl;
    //     }
    //
    //     VkXcbSurfaceCreateInfoKHR createInfo{
    //         .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
    //         .pNext = NULL,
    //         .flags = 0,
    //         .connection = _Lconnection,
    //         .window = _LWindow
    //     };
    //
    //     if(vkCreateXcbSurfaceKHR(mInstance.instance, &createInfo, nullptr, &mSurface.surface) != VK_SUCCESS){
    //         throw std::runtime_error("failed to create window surface");
    //     }
    //...........................................................

    return true;
}

bool App::mPickPhysicalDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance.instance, &deviceCount, nullptr);

    if(deviceCount == 0){
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance.instance, &deviceCount, devices.data());

    for(const auto& device : devices){
        if(isDeviceSuitable(device, mSurface.surface)){            
            mInstance.physicalDevice = device;
            break;
        }
    }    

    if(mInstance.physicalDevice == VK_NULL_HANDLE){
        throw std::runtime_error("failed to find suitable GPU!");
        return false;
    }

    return true;
}

bool App::mCreateLogicalDevice(){

    QueueFamilyIndices indices = findQueueFamilies( mInstance.physicalDevice, 
                                                    mSurface.surface);

    mQueue.queueFamilyIndex     = indices.graphicsFamily.value();
    mQueue.presentFamilyIndex   = indices.presentFamily.value();

    // Separate update
    // .........................................................................
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueueQueueFamilies = {mQueue.queueFamilyIndex ,
                                                mQueue.presentFamilyIndex};

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType                  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex       = queueFamily,
            .queueCount             = 1,
            .pQueuePriorities       = &queuePriority,
        };

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // .........................................................................
    // Separate update
        
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (mEnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
        createInfo.ppEnabledLayerNames = _validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice( mInstance.physicalDevice,
                        &createInfo,
                        NULL,
                        &mInstance.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(   mInstance.device, 
                        indices.presentFamily.value(), 
                        0, 
                        &mQueue.presentQueue);

    vkGetDeviceQueue(   mInstance.device, 
                        indices.graphicsFamily.value(), 
                        0, 
                        &mQueue.graphicsQueue);

    return true;
}

bool App::mCreateSwapChain(){
    SwapChainSupportDetails swapChainDetails = querySwapChainSupport(mInstance.physicalDevice, mSurface.surface);

    VkSurfaceFormatKHR surfaceFormat    =   chooseSwapSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode        =   chooseSwapPresentMode(swapChainDetails.presentMode);
    VkExtent2D extent                   =   chooseSwapExtent(swapChainDetails.capabilities, window.handle);

    uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

    if(swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount){
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = mSurface.surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(mInstance.physicalDevice);    

    uint32_t queueFamilyIndices[]     =   { mQueue.queueFamilyIndex, 
                                            mQueue.presentFamilyIndex};    
    
    if(indices.graphicsFamily != indices.presentFamily){
        createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount    = 2;
        createInfo.pQueueFamilyIndices      = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount    = 0;
        createInfo.pQueueFamilyIndices      = nullptr;
    }

    createInfo.preTransform     = swapChainDetails.capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if( vkCreateSwapchainKHR(mInstance.device, &createInfo, nullptr, &mSwapChain.swapChain)!=VK_SUCCESS ){
        throw::std::runtime_error("Failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(mInstance.device, mSwapChain.swapChain, &imageCount, nullptr);
    mSwapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mInstance.device, mSwapChain.swapChain, &imageCount, mSwapChain.swapChainImages.data());

    mSwapChain.swapChainImageFormat = surfaceFormat.format;
    mSwapChain.swapChainExtent      = extent;

    return true;
}

bool App::mCreateImageViews(){
    mSwapChain.swapChainImageViews.resize(mSwapChain.swapChainImages.size());    

    for(int i = 0; i < mSwapChain.swapChainImageViews.size(); i++){
        VkImageViewCreateInfo createInfo{
            .sType          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image          = mSwapChain.swapChainImages[i],
            .viewType       = VK_IMAGE_VIEW_TYPE_2D,
            .format         = mSwapChain.swapChainImageFormat,
            .components{
                .r   = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g   = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b   = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a   = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };

        if(vkCreateImageView(mInstance.device, &createInfo, nullptr, &mSwapChain.swapChainImageViews[i]) != VK_SUCCESS){
            throw std::runtime_error("failed to create image views !");
        }
    }

    return true;
}

bool App::mCreateGraphicsPipeline(){
    
}

void App::cleanup(){
    for(auto imageView : mSwapChain.swapChainImageViews){
        vkDestroyImageView(mInstance.device, imageView, nullptr);
    }
}
