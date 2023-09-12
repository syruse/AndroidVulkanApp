//
// Created by s_y_r on 11.09.2023.
//

#ifndef ANDROIDVULKAN_UTILS_H
#define ANDROIDVULKAN_UTILS_H

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <vector>
#include <vulkan/vulkan.h>

#define LOG_TAG "my_engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define VK_CHECK(err)                                                                   \
  do {                                                                                  \
    if (err != VK_SUCCESS) {                                                            \
      LOGE("Detected Vulkan error{%d} at file{%s}, line{%d}", err, __FILE__, __LINE__); \
      abort();                                                                          \
    }                                                                                   \
  } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace Utils {

    struct VulkanPhysicalDevices {
        std::vector<VkPhysicalDevice> m_devices;
        std::vector<VkPhysicalDeviceProperties> m_devProps;
        std::vector<std::vector<VkQueueFamilyProperties>> m_qFamilyProps;
        std::vector<std::vector<VkBool32>> m_qSupportsPresent;
        std::vector<std::vector<VkSurfaceFormatKHR>> m_surfaceFormats;
        std::vector<VkSurfaceCapabilitiesKHR> m_surfaceCaps;
        std::vector<std::vector<VkPresentModeKHR>> m_presentModes;
    };

    VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t> &code);

    uint32_t findMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    // mapping required memory property into a VK memory type
    // memory type is an index of 32 entries; or the bit index
    // for the memory type ( each BIT of an 32 bit integer is a type ).
    VkResult allocateMemoryTypeFromProperties(VkPhysicalDevice physDevice, uint32_t typeBits,
                                              VkFlags requirements_mask, uint32_t *typeIndex);

    void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                        VkPipelineStageFlags srcStages,
                        VkPipelineStageFlags destStages);

    void VulkanCheckValidationLayerSupport();

    void VulkanEnumExtProps(std::vector<VkExtensionProperties> &ExtProps);

    void VulkanGetPhysicalDevices(VkInstance inst, VkSurfaceKHR Surface,
                                  VulkanPhysicalDevices &PhysDevices);

    size_t VulkanFindMemoryType(VkPhysicalDevice physicalDevice,
                                const VkMemoryRequirements &memRequirements,
                                VkMemoryPropertyFlags properties);

    std::vector<uint8_t> LoadBinaryFileToVector(const char *file_path,
                                                AAssetManager *assetManager);

    /*
    * getPrerotationMatrix handles screen rotation with 3 hardcoded rotation
    * matrices (detailed below). We skip the 180 degrees rotation.
    */
    void getPrerotationMatrix(const VkSurfaceCapabilitiesKHR &capabilities,
                              const VkSurfaceTransformFlagBitsKHR &pretransformFlag,
                              std::array<float, 16> &mat);

}  // namespace Utils

#endif //ANDROIDVULKAN_UTILS_H
