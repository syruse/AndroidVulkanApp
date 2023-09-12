//
// Created by s_y_r_u_s_e on 11.09.2023.
//

#include "VulkanCore.h"

using namespace Utils;

VKAPI_ATTR VkBool32 VKAPI_CALL
MyDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                      uint64_t object, size_t location, int32_t messageCode,
                      const char *pLayerPrefix, const char *pMessage, void *pUserData) {
    LOGI("ERR %s", pMessage);
    return VK_FALSE;
}

void VulkanCore::clean() {
#ifdef _DEBUG
    // Get the address to the vkCreateDebugReportCallbackEXT function
    auto func =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_inst,
                                                                                        "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(m_inst, 0, 0);
    }
#endif

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_inst, m_surface, nullptr);
    vkDestroyInstance(m_inst, nullptr);
}

VulkanCore::~VulkanCore() {
    clean();
}

void VulkanCore::init(ANativeWindow *window, AAssetManager *assetManager) {
    assert(window && assetManager);

    // reset if needed
    if (m_winController || m_assetManager) {
        clean();
    }

    m_winController.reset(window);
    m_assetManager = assetManager;

    std::vector<VkExtensionProperties> ExtProps;
    VulkanEnumExtProps(ExtProps);

    VulkanCheckValidationLayerSupport();

    createInstance();

    createSurface();

    VulkanGetPhysicalDevices(m_inst, m_surface, m_physDevices);
    selectPhysicalDevice();
    createLogicalDevice();
}

void VulkanCore::createSurface() {
    assert(m_winController);
    const VkAndroidSurfaceCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = m_winController.get()};

    VK_CHECK(vkCreateAndroidSurfaceKHR(m_inst, &create_info,
                                       nullptr, &m_surface));
    LOGD("Surface created");
}

VkPhysicalDevice VulkanCore::getPhysDevice() const {
    assert(m_gfxDevIndex >= 0);
    return m_physDevices.m_devices[m_gfxDevIndex];
}

const VkSurfaceFormatKHR &VulkanCore::getSurfaceFormat() const {
    assert(m_gfxDevIndex >= 0);
    return m_physDevices.m_surfaceFormats[m_gfxDevIndex][0];
}

VkSurfaceCapabilitiesKHR VulkanCore::getSurfaceCaps() {
    assert(m_gfxDevIndex >= 0);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            getPhysDevice(), m_surface,
            const_cast<VkSurfaceCapabilitiesKHR *>(&(m_physDevices.m_surfaceCaps[m_gfxDevIndex])));

    VkSurfaceCapabilitiesKHR capabilities = m_physDevices.m_surfaceCaps[m_gfxDevIndex];
    if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        // Swap to get identity width and height
        uint32_t width = capabilities.currentExtent.width;
        uint32_t height = capabilities.currentExtent.height;
        capabilities.currentExtent.height = width;
        capabilities.currentExtent.width = height;
    }

    return capabilities;
}

void VulkanCore::selectPhysicalDevice() {
    for (size_t i = 0u; i < m_physDevices.m_devices.size(); ++i) {
        for (size_t j = 0u; j < m_physDevices.m_qFamilyProps[i].size(); ++j) {
            VkQueueFamilyProperties &QFamilyProp = m_physDevices.m_qFamilyProps[i][j];

            LOGI("Family %d Num queues: %d\n", j, QFamilyProp.queueCount);
            VkQueueFlags flags = QFamilyProp.queueFlags;
            LOGI("GFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
                 (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                 (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
                 (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                 (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

            if ((flags & VK_QUEUE_GRAPHICS_BIT) && (m_gfxDevIndex == -1)) {
                if (!m_physDevices.m_qSupportsPresent[i][j]) {
                    LOGI("Present is not supported");
                    continue;
                }

                m_gfxDevIndex = i;
                m_gfxQueueFamily = j;
                LOGI("Using GFX device %d and queue family %d\n", m_gfxDevIndex,
                     m_gfxQueueFamily);
            }
        }
    }

    if (m_gfxDevIndex == -1) {
        LOGE("No GFX device found!");
        abort();
    }
}

void VulkanCore::createInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "MyVulkanApp";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char *pInstExt[] = {
#ifdef _DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
            "VK_KHR_surface", "VK_KHR_android_surface"};

#ifdef _DEBUG
    const char *pInstLayers[] = {"VK_LAYER_KHRONOS_validation"};
#endif

    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
#ifdef _DEBUG
    instInfo.enabledLayerCount = ARRAY_SIZE(pInstLayers);
    instInfo.ppEnabledLayerNames = pInstLayers;
#endif
    instInfo.enabledExtensionCount = ARRAY_SIZE(pInstExt);
    instInfo.ppEnabledExtensionNames = pInstExt;

    VkResult res = vkCreateInstance(&instInfo, nullptr, &m_inst);
    LOGD("vkCreateInstance %d\n", res);
    VK_CHECK(res);

#ifdef _DEBUG
    // Get the address to the vkCreateDebugReportCallbackEXT function
    PFN_vkCreateDebugReportCallbackEXT my_vkCreateDebugReportCallbackEXT =
            reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_inst,
                                                                                       "vkCreateDebugReportCallbackEXT"));

    // Register the debug callback
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext = nullptr;
    callbackCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &MyDebugReportCallback;
    callbackCreateInfo.pUserData = nullptr;

    VkDebugReportCallbackEXT callback;
    res = my_vkCreateDebugReportCallbackEXT(m_inst, &callbackCreateInfo, nullptr, &callback);
    LOGI("my_vkCreateDebugReportCallbackEXT %d", res);
    VK_CHECK(res);
#endif
}

void VulkanCore::createLogicalDevice() {
    VkDeviceQueueCreateInfo qInfo = {};
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    float qPriorities = 1.0f;
    qInfo.queueCount = 1;
    qInfo.pQueuePriorities = &qPriorities;
    qInfo.queueFamilyIndex = m_gfxQueueFamily;

    const char *pDevExt[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.enabledExtensionCount = ARRAY_SIZE(pDevExt);
    devInfo.ppEnabledExtensionNames = pDevExt;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &qInfo;
    devInfo.pEnabledFeatures = &deviceFeatures;

    VkResult res = vkCreateDevice(getPhysDevice(), &devInfo, nullptr, &m_device);

    LOGD("vkCreateDevice %d\n", res);
    VK_CHECK(res);
}
