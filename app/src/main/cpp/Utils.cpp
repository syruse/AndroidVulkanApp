//
// Created by s_y_r on 11.09.2023.
//

#include "Utils.h"

#include <array>
#include <cassert>

namespace Utils {
    std::vector<uint8_t> LoadBinaryFileToVector(const char *file_path,
                                                AAssetManager *assetManager) {
        std::vector<uint8_t> file_content;
        assert(assetManager);
        AAsset *file =
                AAssetManager_open(assetManager, file_path, AASSET_MODE_BUFFER);
        size_t file_length = AAsset_getLength(file);

        file_content.resize(file_length);

        AAsset_read(file, file_content.data(), file_length);
        AAsset_close(file);
        return file_content;
    }

    void VulkanCheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const auto &layer: availableLayers) {
            LOGI("%s", layer.layerName);
        }
    }

    void VulkanEnumExtProps(std::vector<VkExtensionProperties> &ExtProps) {
        uint32_t NumExt = 0;
        VkResult res = vkEnumerateInstanceExtensionProperties(nullptr, &NumExt, nullptr);
        LOGI("vkEnumerateInstanceExtensionProperties error %d", res);
        VK_CHECK(res);

        LOGI("Found %d extensions", NumExt);

        ExtProps.resize(NumExt);

        res = vkEnumerateInstanceExtensionProperties(nullptr, &NumExt, &ExtProps[0]);
        LOGI("vkEnumerateInstanceExtensionProperties error %d", res);
        VK_CHECK(res);

        for (decltype(NumExt) i = 0; i < NumExt; ++i) {
            LOGI("Instance extension %d - %s", i, ExtProps[i].extensionName);
        }
    }

    void VulkanPrintImageUsageFlags(const VkImageUsageFlags &flags) {
        if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            LOGI("Image usage transfer src is supported");
        }

        if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            LOGI("Image usage transfer dest is supported");
        }

        if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
            LOGI("Image usage sampled is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            LOGI("Image usage color attachment is supported");
        }

        if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            LOGI("Image usage depth stencil attachment is supported");
        }

        if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
            LOGI("Image usage transient attachment is supported");
        }

        if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
            LOGI("Image usage input attachment is supported");
        }
    }

    void VulkanGetPhysicalDevices(VkInstance inst, VkSurfaceKHR Surface,
                                  VulkanPhysicalDevices &PhysDevices) {
        uint32_t NumDevices = 0;

        VkResult res = vkEnumeratePhysicalDevices(inst, &NumDevices, nullptr);
        LOGI("vkEnumeratePhysicalDevices error %d", res);
        VK_CHECK(res);

        LOGI("Num physical devices %d", NumDevices);

        PhysDevices.m_devices.resize(NumDevices);
        PhysDevices.m_devProps.resize(NumDevices);
        PhysDevices.m_qFamilyProps.resize(NumDevices);
        PhysDevices.m_qSupportsPresent.resize(NumDevices);
        PhysDevices.m_surfaceFormats.resize(NumDevices);
        PhysDevices.m_surfaceCaps.resize(NumDevices);
        PhysDevices.m_presentModes.resize(NumDevices);

        res = vkEnumeratePhysicalDevices(inst, &NumDevices, &PhysDevices.m_devices[0]);
        LOGI("vkEnumeratePhysicalDevices error %d", res);
        VK_CHECK(res);

        for (size_t i = 0; i < NumDevices; ++i) {
            const VkPhysicalDevice &PhysDev = PhysDevices.m_devices[i];
            vkGetPhysicalDeviceProperties(PhysDev, &PhysDevices.m_devProps[i]);

            LOGI("Device name: %s", PhysDevices.m_devProps[i].deviceName);
            uint32_t apiVer = PhysDevices.m_devProps[i].apiVersion;
            LOGI("API version: %d.%d.%d", VK_VERSION_MAJOR(apiVer), VK_VERSION_MINOR(apiVer),
                 VK_VERSION_PATCH(apiVer));
            uint32_t NumQFamily = 0;

            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamily, nullptr);

            LOGI("Num of family queues: %d", NumQFamily);

            PhysDevices.m_qFamilyProps[i].resize(NumQFamily);
            PhysDevices.m_qSupportsPresent[i].resize(NumQFamily);

            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamily,
                                                     &(PhysDevices.m_qFamilyProps[i][0]));

            for (size_t q = 0; q < NumQFamily; q++) {
                res = vkGetPhysicalDeviceSurfaceSupportKHR(PhysDev, q, Surface,
                                                           &(PhysDevices.m_qSupportsPresent[i][q]));
                LOGI("vkGetPhysicalDeviceSurfaceSupportKHR %d", res);
            }

            uint32_t NumFormats = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats, nullptr);
            assert(NumFormats > 0);

            PhysDevices.m_surfaceFormats[i].resize(NumFormats);

            res = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats,
                                                       &(PhysDevices.m_surfaceFormats[i][0]));
            LOGI("vkGetPhysicalDeviceSurfaceFormatsKHR %d", res);
            VK_CHECK(res);

            for (size_t j = 0; j < NumFormats; j++) {
                const VkSurfaceFormatKHR &SurfaceFormat = PhysDevices.m_surfaceFormats[i][j];
                LOGI("Format %d color space %d\n", SurfaceFormat.format, SurfaceFormat.colorSpace);
            }

            res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDev, Surface,
                                                            &(PhysDevices.m_surfaceCaps[i]));
            LOGI("vkGetPhysicalDeviceSurfaceCapabilitiesKHR error", res);
            VK_CHECK(res);

            VulkanPrintImageUsageFlags(PhysDevices.m_surfaceCaps[i].supportedUsageFlags);

            uint32_t NumPresentModes = 0;

            res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes,
                                                            nullptr);
            LOGI("vkGetPhysicalDeviceSurfacePresentModesKHR %d", res);
            VK_CHECK(res);

            assert(NumPresentModes != 0);

            LOGI("Number of presentation modes %d", NumPresentModes);
            PhysDevices.m_presentModes[i].resize(NumPresentModes);
            res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes,
                                                            &(PhysDevices.m_presentModes[i][0]));
            VK_CHECK(res);
        }
    }

    void getPrerotationMatrix(const VkSurfaceCapabilitiesKHR &capabilities,
                              const VkSurfaceTransformFlagBitsKHR &pretransformFlag,
                              std::array<float, 16> &mat) {
        // mat is initialized to the identity matrix
        mat = {1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.};
        if (pretransformFlag & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
            // mat is set to a 90 deg rotation matrix
            mat = {0., 1., 0., 0., -1., 0, 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.};
        } else if (pretransformFlag & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
            // mat is set to 270 deg rotation matrix
            mat = {0., -1., 0., 0., 1., 0, 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.};
        }
    }
}  // namespace Utils