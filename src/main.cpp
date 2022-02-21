#include "MyApp.hpp"
#include <cstdlib>

MyApp::MyApp(int width, int height, const char* title) : 
            App(width, height, title)
{

}

void MyApp::initDraw(){

}

void MyApp::draw(){
    vkWaitForFences(mInstance.device, 1, &mRenderPass.inFlightFences[mRenderPass.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(mInstance.device,
                          mSwapChain.swapChain,
                          UINT64_MAX,
                          mRenderPass.imageAvailableSemaphores[mRenderPass.currentFrame],
                          VK_NULL_HANDLE,
                          &imageIndex);

    // Check if a previous frame is using this image
    if(mRenderPass.imagesInFlight[imageIndex] != VK_NULL_HANDLE){
        vkWaitForFences(mInstance.device, 1, &mRenderPass.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark this image as now being in use by this frame
    mRenderPass.imagesInFlight[imageIndex] = mRenderPass.inFlightFences[mRenderPass.currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]        = {mRenderPass.imageAvailableSemaphores[mRenderPass.currentFrame]};
    VkPipelineStageFlags waitStages[]   = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount       = 1;
    submitInfo.pWaitSemaphores          = waitSemaphores;
    submitInfo.pWaitDstStageMask        = waitStages;

    submitInfo.commandBufferCount       = 1;
    submitInfo.pCommandBuffers          = &mRenderPass.commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[]       = {mRenderPass.renderFinishedSemaphores[mRenderPass.currentFrame]};
    submitInfo.signalSemaphoreCount     = 1;
    submitInfo.pSignalSemaphores        = signalSemaphores;

    vkResetFences(mInstance.device, 1, &mRenderPass.inFlightFences[mRenderPass.currentFrame]);

    if(vkQueueSubmit(mQueue.graphicsQueue, 1, &submitInfo, mRenderPass.inFlightFences[mRenderPass.currentFrame]) != VK_SUCCESS){
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType                   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount      = 1;
    presentInfo.pWaitSemaphores         = signalSemaphores;

    VkSwapchainKHR swapchains[] = {mSwapChain.swapChain};
    presentInfo.swapchainCount          = 1;
    presentInfo.pSwapchains             = swapchains;
    presentInfo.pImageIndices           = &imageIndex;

    presentInfo.pResults                = nullptr;
    vkQueuePresentKHR(mQueue.presentQueue, &presentInfo);

    vkQueueWaitIdle(mQueue.presentQueue);

    mRenderPass.currentFrame = (mRenderPass.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void MyApp::processInput(){
    if(keyPressed(GLFW_KEY_ESCAPE)){
        terminate();
    }
}

void MyApp::processMouse(){

}

int main(){
    MyApp myapp(1024, 768, "VK");
    myapp.start();
    return EXIT_SUCCESS;
}
