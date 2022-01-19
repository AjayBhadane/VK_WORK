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

    mSetupDebugMessenger();

    if(! mCreateSurface()){
        std::cerr << "Setting up drawing surface failed" << std::endl;
        return;
    }

    mPickPhysicalDevice();
    mCreateLogicalDevice();
    mCreateSwapChain();
    mCreateImageViews();
    mCreateRenderPass();
    mCreateGraphicsPipeline();
    mCreateFrameBuffers();
    mCreateCommandpool();
}

void App::loop(){    
    while(!glfwWindowShouldClose(this->window.handle)){                       
        this->draw();
        glfwPollEvents();
        glfwSwapBuffers(App::window.handle);
        calculateDeltaTime();
    }

    this->cleanup();
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
        
        std::cerr << "Validation layer requested but, not found" << std::endl;
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

    mQueue.graphicsFamilyIndex      = indices.graphicsFamily.value();
    mQueue.presentFamilyIndex       = indices.presentFamily.value();

    // Separate update
    // .........................................................................
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueueQueueFamilies = {mQueue.graphicsFamilyIndex ,
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
                        nullptr,
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

    uint32_t queueFamilyIndices[2] = {mQueue.graphicsFamilyIndex, mQueue.presentFamilyIndex };

    if(mQueue.graphicsFamilyIndex != mQueue.presentFamilyIndex){
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

bool App::mCreateRenderPass(){
    // Attachment description
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format          = mSwapChain.swapChainImageFormat;
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment   = 0;
    colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;    

    VkPipelineLayout pipelineLayout;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount    = 1;
    renderPassCreateInfo.pAttachments       = &colorAttachment;
    renderPassCreateInfo.subpassCount       = 1;
    renderPassCreateInfo.pSubpasses         = &subpass;

    if(vkCreateRenderPass(mInstance.device, &renderPassCreateInfo, nullptr, &mRenderPass.renderPass) != VK_SUCCESS){
        throw std::runtime_error("failed to create render pass!");
    }

    return true; 
}

bool App::mCreateGraphicsPipeline(){
    // Fixed function
    auto vertShaderCode = readFile("/shader/spv/test.vert.spv");
    auto fragShaderCode = readFile("/shader/spv/test.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(mInstance.device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(mInstance.device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) mSwapChain.swapChainExtent.width;
    viewport.height = (float) mSwapChain.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = mSwapChain.swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(mInstance.device, &pipelineLayoutInfo, nullptr, &mRenderPass.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = mRenderPass.pipelineLayout;
    pipelineInfo.renderPass = mRenderPass.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(mInstance.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mRenderPass.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(mInstance.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(mInstance.device, vertShaderModule, nullptr);

    return true;
}

bool App::mCreateFrameBuffers(){
    mSwapChain.swapChainFramebuffers.resize(mSwapChain.swapChainImageViews.size());

    for(size_t i = 0; i < mSwapChain.swapChainImageViews.size(); i++){
        VkImageView attachments[] = {
            mSwapChain.swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType             = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass        = mRenderPass.renderPass;
        framebufferCreateInfo.attachmentCount   = 1;
        framebufferCreateInfo.pAttachments      = attachments;
        framebufferCreateInfo.width             = mSwapChain.swapChainExtent.width;
        framebufferCreateInfo.height            = mSwapChain.swapChainExtent.height;
        framebufferCreateInfo.layers            = 1;

        if(vkCreateFramebuffer(mInstance.device, &framebufferCreateInfo, nullptr, &mSwapChain.swapChainFramebuffers[i])
            != VK_SUCCESS){
            throw std::runtime_error("failed to create frame buffer");
        }
    }

    return true;
}

bool App::mCreateCommandpool(){
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex   = mQueue.graphicsFamilyIndex;
    poolInfo.flags              = 0; // optional

    if(vkCreateCommandPool(mInstance.device, &poolInfo, nullptr, &mRenderPass.commandPool) != VK_SUCCESS){
        throw std::runtime_error("failed to create command pool");
    }

    return true;
}

void App::mCreateCommandBuffers(){
    mRenderPass.commandBuffer.resize(mSwapChain.swapChainFramebuffers.size());

}

void App::cleanup(){

    vkDestroyCommandPool(mInstance.device, mRenderPass.commandPool, nullptr);

    for(auto framebuffer : mSwapChain.swapChainFramebuffers){
        vkDestroyFramebuffer(mInstance.device, framebuffer, nullptr);
    }

    vkDestroyPipeline(mInstance.device, mRenderPass.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mInstance.device, mRenderPass.pipelineLayout, nullptr);
    vkDestroyRenderPass(mInstance.device, mRenderPass.renderPass, nullptr);

    for (auto imageView : mSwapChain.swapChainImageViews) {
        vkDestroyImageView(mInstance.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(mInstance.device, mSwapChain.swapChain, nullptr);
    vkDestroyDevice(mInstance.device, nullptr);

    if (mEnableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(mInstance.instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(mInstance.instance, mSurface.surface, nullptr);
    vkDestroyInstance(mInstance.instance, nullptr);

    glfwDestroyWindow(window.handle);

    glfwTerminate();
}
