#include "VulkanCore.h"
#include <array>
#include <string_view>

class VulkanRenderer {
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr VkFormat TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr std::string_view TEXTURE_NAME = "texture.png";

    struct Texture {
        VkSampler sampler;
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory mem;
        VkImageView view;
        int32_t width;
        int32_t height;
    };

    struct UBO_Data {
        alignas(16) std::array<float, 16> MVP; // aligned as vec4 or 16bytes
    };

    struct PushConstant_Data {
        alignas(16) std::array<float, 3> HSV; // HSV factors for modifying
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

    void loadTextureFromFile(const char *filePath,
                             Texture *texture,
                             VkImageUsageFlags usage, VkFlags required_props);

    void createTexture();

    VkResult allocateMemoryTypeFromProperties(uint32_t typeBits,
                                              VkFlags requirements_mask,
                                              uint32_t *typeIndex);

private:
    VulkanCore m_core;
    bool m_initialized{false};
    VkSwapchainKHR m_swapChain{0u};
    std::vector<VkImage> m_swapChainImages{};
    std::vector<VkImageView> m_swapChainImageViews{};
    std::vector<VkFramebuffer> m_swapChainFramebuffers{};
    VkCommandPool m_commandPool{0u};
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};

    PushConstant_Data m_hsvFactors{0.5f, 0.5f, 0.5f};
    Texture m_texture;
    VkQueue m_queue{nullptr};

    VkRenderPass m_renderPass{0u};
    VkDescriptorSetLayout m_descriptorSetLayout{0u};
    VkPipelineLayout m_pipelineLayout{0u};
    VkPipeline m_graphicsPipeline{0u};

    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers{};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_uniformBuffersMemory{};

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderFinishedSemaphores{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences{};
    VkDescriptorPool m_descriptorPool{0u};
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets{};

    uint32_t m_currentFrame{0u};
    bool m_orientationChanged{false};
    VkSurfaceTransformFlagBitsKHR m_pretransformFlag;
};
