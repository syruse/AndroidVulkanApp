//
// Created by s_y_r_u_s_e on 11.09.2023.
//

#ifndef ANDROIDVULKAN_VULKANCORE_H
#define ANDROIDVULKAN_VULKANCORE_H

#include "Utils.h"
#include <assert.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

class VulkanCore {
    struct ANativeWindowDeleter {
        void operator()(ANativeWindow *window) { ANativeWindow_release(window); }
    };

public:
    ~VulkanCore();

    void setANativeData(ANativeWindow *window, AAssetManager *assetManager);

    void init();

    VkPhysicalDevice getPhysDevice() const;

    const VkSurfaceFormatKHR &getSurfaceFormat() const;

    VkSurfaceCapabilitiesKHR getSurfaceCaps();

    VkSurfaceKHR getSurface() const {
        return m_surface;
    }

    int getQueueFamily() const {
        return m_gfxQueueFamily;
    }

    VkInstance getInstance() const {
        return m_inst;
    }

    VkDevice getDevice() const {
        return m_device;
    }

    ANativeWindow *getWindow() const {
        return m_winController.get();
    }

    AAssetManager *getAssetManager() const {
        return m_assetManager;
    }

    void createSurface();

private:
    void createInstance();

    void selectPhysicalDevice();

    void createLogicalDevice();

    std::unique_ptr<ANativeWindow, ANativeWindowDeleter> m_winController = nullptr;
    AAssetManager *m_assetManager = nullptr;

    // Vulkan stuff
    VkInstance m_inst = nullptr;
    VkSurfaceKHR m_surface = 0u;
    Utils::VulkanPhysicalDevices m_physDevices{};
    VkDevice m_device = nullptr;

    // Internal stuff
    int m_gfxDevIndex = -1;
    int m_gfxQueueFamily = -1;
};


#endif //ANDROIDVULKAN_VULKANCORE_H
