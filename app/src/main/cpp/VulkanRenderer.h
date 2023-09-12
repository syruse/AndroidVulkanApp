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
    void init(ANativeWindow *newWindow, AAssetManager *newManager);

    void render();

    void cleanup();

    void cleanupSwapChain();

    void setHSVFactors(float hue, float saturation, float intensity) {
        m_hsvFactors.HSV[0] = std::clamp(hue, 0.0f, 1.0f);;
        m_hsvFactors.HSV[1] = std::clamp(saturation, 0.0f, 1.0f);;
        m_hsvFactors.HSV[2] = std::clamp(intensity, 0.0f, 1.0f);;
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

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void recreateSwapChain();

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
