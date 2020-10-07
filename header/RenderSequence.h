#pragma once

namespace scatter {
    class VulkanRenderSequence {
    public:

    private:
        VkSemaphore semaphore;
        VkFramebuffer framebuffer;
        VkRenderPass renderPass;
    };
}