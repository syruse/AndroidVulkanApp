#include "VulkanCore.h"

#include <array>

class VulkanRenderer {
    struct UBO_Data {
        alignas(16) std::array<float, 16> MVP; // aligned as vec4 or 16bytes
    };

    struct PushConstant_Data {
    };

public:
    void init();

    void render();

    void cleanup();

    void cleanupSwapChain();

    void reset(ANativeWindow *newWindow, AAssetManager *newManager);

    bool isInitialized() {
        return m_initialized;
    }

private:
    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffer();

    void createSyncObjects();

    VkShaderModule createShaderModule(const std::vector<uint8_t> &code);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void recreateSwapChain();

    void onOrientationChange();

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);

    void createUniformBuffers();

    void updateUniformBuffer(uint32_t currentImage);

    void createDescriptorPool();

    void createDescriptorSets();

    void establishDisplaySizeIdentity();

private:
    uint32_t m_max_frames_in_flight = 2;
    VulkanCore m_core;
    bool m_initialized{false};
    VkSwapchainKHR m_swapChain{0u};
    std::vector<VkImage> m_swapChainImages{};
    std::vector<VkImageView> m_swapChainImageViews{};
    std::vector<VkFramebuffer> m_swapChainFramebuffers{};
    VkCommandPool m_commandPool{0u};
    std::vector<VkCommandBuffer> m_commandBuffers{};

    VkQueue m_queue{nullptr};

    VkRenderPass m_renderPass{0u};
    VkDescriptorSetLayout m_descriptorSetLayout{0u};
    VkPipelineLayout m_pipelineLayout{0u};
    VkPipeline m_graphicsPipeline{0u};

    std::vector<VkBuffer> m_uniformBuffers{};
    std::vector<VkDeviceMemory> m_uniformBuffersMemory{};

    std::vector<VkSemaphore> m_imageAvailableSemaphores{};
    std::vector<VkSemaphore> m_renderFinishedSemaphores{};
    std::vector<VkFence> m_inFlightFences{};
    VkDescriptorPool m_descriptorPool{0u};
    std::vector<VkDescriptorSet> m_descriptorSets{};

    uint32_t m_currentFrame{0u};
    bool m_orientationChanged{false};
    VkSurfaceTransformFlagBitsKHR m_pretransformFlag;
};
