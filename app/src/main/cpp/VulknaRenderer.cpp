//
// Created by s_y_r_u_s_e on 11.09.2023.
//

#include "VulkanRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace Utils;

void VulkanRenderer::init(ANativeWindow *newWindow, AAssetManager *newManager) {
    assert(newWindow && newManager);
    m_core.init(newWindow, newManager);
    if (m_initialized) {
        cleanup();
    }
    vkGetDeviceQueue(m_core.getDevice(), m_core.getQueueFamily(), 0, &m_queue);
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createUniformBuffers();
    createDescriptorPool();
    createTexture();
    createDescriptorSets();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
    m_initialized = true;
}

// mapping required memory property into a VK memory type
// memory type is an index of 32 entries; or the bit index
// for the memory type ( each BIT of an 32 bit integer is a type ).
VkResult VulkanRenderer::allocateMemoryTypeFromProperties(uint32_t typeBits,
                                          VkFlags requirements_mask,
                                          uint32_t* typeIndex) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_core.getPhysDevice(), &memProps);
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memProps.memoryTypes[i].propertyFlags &
                 requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return VK_SUCCESS;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return VK_ERROR_MEMORY_MAP_FAILED;
}

void VulkanRenderer::loadTextureFromFile(const char* filePath,
                                             Texture* texture,
                                             VkImageUsageFlags usage, VkFlags required_props) {
    // Check for linear supportability
    VkFormatProperties props;
    bool needBlit = true;

    auto vkGetPhysicalDeviceFormatProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(
                    vkGetInstanceProcAddr(m_core.getInstance(),
                                          "vkGetPhysicalDeviceFormatProperties"));
    assert(vkGetPhysicalDeviceFormatProperties);

    vkGetPhysicalDeviceFormatProperties(m_core.getPhysDevice(), TEXTURE_FORMAT, &props);
    assert((props.linearTilingFeatures | props.optimalTilingFeatures) &
           VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
        // linear format supporting the required texture
        needBlit = false;
    }

    // Read the file:
    AAsset* file = AAssetManager_open(m_core.getAssetManager(),
                                      filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    stbi_uc* fileContent = new unsigned char[fileLength];
    AAsset_read(file, fileContent, fileLength);
    AAsset_close(file);

    int imgWidth, imgHeight, n;
    unsigned char* imageData = stbi_load_from_memory (
            fileContent, fileLength, &imgWidth,
            &imgHeight, &n, 4);
    assert(n == 4);

    texture->width = imgWidth;
    texture->height = imgHeight;

    // Allocate the linear texture so texture could be copied over
    VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = TEXTURE_FORMAT,
            .extent = {static_cast<uint32_t>(imgWidth),
                       static_cast<uint32_t>(imgHeight), 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = (needBlit ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                               : VK_IMAGE_USAGE_SAMPLED_BIT),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            //.queueFamilyIndexCount = 1,
            //.pQueueFamilyIndices = {m_core.getQueueFamily()},
            .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
    };
    VkMemoryAllocateInfo mem_alloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = 0,
            .memoryTypeIndex = 0,
    };

    VkMemoryRequirements mem_reqs;
    VK_CHECK(vkCreateImage(m_core.getDevice(), &image_create_info, nullptr,
                          &texture->image));
    vkGetImageMemoryRequirements(m_core.getDevice(), texture->image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    VK_CHECK(allocateMemoryTypeFromProperties(mem_reqs.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                              &mem_alloc.memoryTypeIndex));
    VK_CHECK(vkAllocateMemory(m_core.getDevice(), &mem_alloc, nullptr, &texture->mem));
    VK_CHECK(vkBindImageMemory(m_core.getDevice(), texture->image, texture->mem, 0));

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void* data;

        vkGetImageSubresourceLayout(m_core.getDevice(), texture->image, &subres,
                                    &layout);
        VK_CHECK(vkMapMemory(m_core.getDevice(), texture->mem, 0,
                            mem_alloc.allocationSize, 0, &data));

        for (int32_t y = 0; y < imgHeight; y++) {
            unsigned char* row = (unsigned char*)((char*)data + layout.rowPitch * y);
            for (int32_t x = 0; x < imgWidth; x++) {
                row[x * 4] = imageData[(x + y * imgWidth) * 4];
                row[x * 4 + 1] = imageData[(x + y * imgWidth) * 4 + 1];
                row[x * 4 + 2] = imageData[(x + y * imgWidth) * 4 + 2];
                row[x * 4 + 3] = imageData[(x + y * imgWidth) * 4 + 3];
            }
        }

        vkUnmapMemory(m_core.getDevice(), texture->mem);
        stbi_image_free(imageData);
    }
    delete[] fileContent;

    texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkCommandPoolCreateInfo cmdPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = static_cast<uint32_t>(m_core.getQueueFamily()),
    };

    VkCommandPool cmdPool;
    VK_CHECK(vkCreateCommandPool(m_core.getDevice(), &cmdPoolCreateInfo, nullptr,
                                &cmdPool));

    VkCommandBuffer gfxCmd;
    const VkCommandBufferAllocateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = cmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    VK_CHECK(vkAllocateCommandBuffers(m_core.getDevice(), &cmd, &gfxCmd));
    VkCommandBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr};
    VK_CHECK(vkBeginCommandBuffer(gfxCmd, &cmd_buf_info));

    // If linear is supported, we are done
    VkImage stageImage = VK_NULL_HANDLE;
    VkDeviceMemory stageMem = VK_NULL_HANDLE;
    if (!needBlit) {
        setImageLayout(gfxCmd, texture->image, VK_IMAGE_LAYOUT_PREINITIALIZED,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    } else {
        // save current image and mem as staging image and memory
        stageImage = texture->image;
        stageMem = texture->mem;
        texture->image = VK_NULL_HANDLE;
        texture->mem = VK_NULL_HANDLE;

        // Create a tile texture to blit into
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK(vkCreateImage(m_core.getDevice(), &image_create_info, nullptr,
                              &texture->image));
        vkGetImageMemoryRequirements(m_core.getDevice(), texture->image, &mem_reqs);

        mem_alloc.allocationSize = mem_reqs.size;
        VK_CHECK(allocateMemoryTypeFromProperties(
                mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &mem_alloc.memoryTypeIndex));
        VK_CHECK(
                vkAllocateMemory(m_core.getDevice(), &mem_alloc, nullptr, &texture->mem));
        VK_CHECK(vkBindImageMemory(m_core.getDevice(), texture->image, texture->mem, 0));

        // transitions image out of UNDEFINED type
        setImageLayout(gfxCmd, stageImage, VK_IMAGE_LAYOUT_PREINITIALIZED,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        setImageLayout(gfxCmd, texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageCopy bltInfo{
                .srcSubresource {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
                .srcOffset { .x = 0, .y = 0, .z = 0 },
                .dstSubresource {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
                .dstOffset { .x = 0, .y = 0, .z = 0},
                .extent { .width = static_cast<uint32_t>(imgWidth), .height = static_cast<uint32_t>(imgHeight), .depth = 1,},
        };
        vkCmdCopyImage(gfxCmd, stageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                       &bltInfo);

        setImageLayout(gfxCmd, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    VK_CHECK(vkEndCommandBuffer(gfxCmd));
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    VkFence fence;
    VK_CHECK(vkCreateFence(m_core.getDevice(), &fenceInfo, nullptr, &fence));

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &gfxCmd,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
    };
    VK_CHECK(vkQueueSubmit(m_queue, 1u, &submitInfo, fence) != VK_SUCCESS);
    VK_CHECK(vkWaitForFences(m_core.getDevice(), 1, &fence, VK_TRUE, 100000000) !=
            VK_SUCCESS);
    vkDestroyFence(m_core.getDevice(), fence, nullptr);

    vkFreeCommandBuffers(m_core.getDevice(), cmdPool, 1, &gfxCmd);
    vkDestroyCommandPool(m_core.getDevice(), cmdPool, nullptr);
    if (stageImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_core.getDevice(), stageImage, nullptr);
        vkFreeMemory(m_core.getDevice(), stageMem, nullptr);
    }
}

void VulkanRenderer::createTexture() {
        loadTextureFromFile(TEXTURE_NAME.data(), &m_texture, VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        const VkSamplerCreateInfo sampler = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias = 0.0f,
                .maxAnisotropy = 1,
                .compareOp = VK_COMPARE_OP_NEVER,
                .minLod = 0.0f,
                .maxLod = 0.0f,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .unnormalizedCoordinates = VK_FALSE,
        };
        VkImageViewCreateInfo view = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = VK_NULL_HANDLE,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = TEXTURE_FORMAT,
                .components =
                        {
                                VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
                        },
                .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };

    VK_CHECK(vkCreateSampler(m_core.getDevice(), &sampler, nullptr,
                                &m_texture.sampler));
        view.image = m_texture.image;
    VK_CHECK(
                vkCreateImageView(m_core.getDevice(), &view, nullptr, &m_texture.view));
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                                  VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(m_core.getDevice(), &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_core.getDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(m_core.getDevice(), &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(m_core.getDevice(), buffer, bufferMemory, 0);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_core.getPhysDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                        properties) == properties) {
            return i;
        }
    }

    abort();
}

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UBO_Data);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(m_core.getDevice(), &layoutInfo, nullptr,
                                         &m_descriptorSetLayout));
}

void VulkanRenderer::recreateSwapChain() {
    vkDeviceWaitIdle(m_core.getDevice());
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createFramebuffers();
}

void VulkanRenderer::render() {
    if (!m_initialized) {
        return;
    }
    if (m_orientationChanged) {
        onOrientationChange();
        return;
    }

    vkWaitForFences(m_core.getDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE,
                    UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
            m_core.getDevice(), m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame],
            VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    assert(result == VK_SUCCESS ||
           result == VK_SUBOPTIMAL_KHR);  // failed to acquire swap chain image
    updateUniformBuffer(m_currentFrame);

    vkResetFences(m_core.getDevice(), 1, &m_inFlightFences[m_currentFrame]);
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_queue, 1, &submitInfo,
                           m_inFlightFences[m_currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_queue, &presentInfo);
    if (result == VK_SUBOPTIMAL_KHR) {
        m_orientationChanged = true;
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
    } else {
        assert(result == VK_SUCCESS);  // failed to present swap chain image!
    }
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VK_CHECK(vkCreateDescriptorPool(m_core.getDevice(), &poolInfo, nullptr, &m_descriptorPool));
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    VK_CHECK(vkAllocateDescriptorSets(m_core.getDevice(), &allocInfo, m_descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UBO_Data);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_texture.view;
        imageInfo.sampler = m_texture.sampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_core.getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    UBO_Data ubo{};
    getPrerotationMatrix(m_core.getSurfaceCaps(), m_pretransformFlag,
                         ubo.MVP);
    void *data;
    vkMapMemory(m_core.getDevice(), m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0,
                &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_core.getDevice(), m_uniformBuffersMemory[currentImage]);
}

void VulkanRenderer::onOrientationChange() {
    recreateSwapChain();
    m_orientationChanged = false;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                         uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    const auto &extent = m_core.getSurfaceCaps().currentExtent;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    VkViewport viewport{};
    viewport.width = (float) extent.width;
    viewport.height = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkClearValue clearColor = {{{0.5f, 0.5f, 0.0f, 1.0f}}};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame],
                            0, nullptr);

    vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(PushConstant_Data), &m_hsvFactors);

    vkCmdDraw(commandBuffer, 12, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanRenderer::cleanupSwapChain() {
    for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(m_core.getDevice(), m_swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        vkDestroyImageView(m_core.getDevice(), m_swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(m_core.getDevice(), m_swapChain, nullptr);
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(m_core.getDevice());
    cleanupSwapChain();

    vkDestroyImageView(m_core.getDevice(), m_texture.view, nullptr);
    vkDestroyImage(m_core.getDevice(), m_texture.image, nullptr);
    vkFreeMemory(m_core.getDevice(), m_texture.mem, nullptr);

    vkDestroyDescriptorPool(m_core.getDevice(), m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_core.getDevice(), m_descriptorSetLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(m_core.getDevice(), m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_core.getDevice(), m_uniformBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_core.getDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_core.getDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_core.getDevice(), m_inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(m_core.getDevice(), m_commandPool, nullptr);
    vkDestroyPipeline(m_core.getDevice(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_core.getDevice(), m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_core.getDevice(), m_renderPass, nullptr);
    vkDestroyDevice(m_core.getDevice(), nullptr);
    vkDestroySurfaceKHR(m_core.getInstance(), m_core.getSurface(), nullptr);
    vkDestroyInstance(m_core.getInstance(), nullptr);
    m_initialized = false;
}

void VulkanRenderer::createSwapChain() {
    const VkSurfaceCapabilitiesKHR &surfaceCaps = m_core.getSurfaceCaps();

    assert(surfaceCaps.currentExtent.width != -1);

    m_pretransformFlag = surfaceCaps.currentTransform;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_core.getSurface();
    createInfo.minImageCount = surfaceCaps.minImageCount;
    createInfo.imageFormat = m_core.getSurfaceFormat().format;
    createInfo.imageColorSpace = m_core.getSurfaceFormat().colorSpace;
    createInfo.imageExtent = surfaceCaps.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = m_pretransformFlag;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_core.getDevice(), &createInfo, nullptr, &m_swapChain));

    uint32_t numSwapChainImages = 0;
    vkGetSwapchainImagesKHR(m_core.getDevice(), m_swapChain, &numSwapChainImages, nullptr);

    m_swapChainImages.resize(numSwapChainImages);
    m_swapChainImageViews.resize(numSwapChainImages);
    m_swapChainFramebuffers.resize(numSwapChainImages);

    vkGetSwapchainImagesKHR(m_core.getDevice(), m_swapChain, &numSwapChainImages,
                            m_swapChainImages.data());
}

void VulkanRenderer::createImageViews() {
    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_core.getSurfaceFormat().format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(m_core.getDevice(), &createInfo, nullptr,
                                   &m_swapChainImageViews[i]));
    }
}

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_core.getSurfaceFormat().format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(m_core.getDevice(), &renderPassInfo, nullptr, &m_renderPass));
}

void VulkanRenderer::createGraphicsPipeline() {
    auto vertShaderCode =
            LoadBinaryFileToVector("shaders/shader.vert.spv", m_core.getAssetManager());
    auto fragShaderCode =
            LoadBinaryFileToVector("shaders/shader.frag.spv", m_core.getAssetManager());

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(PushConstant_Data);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &push_constant;

    VK_CHECK(vkCreatePipelineLayout(m_core.getDevice(), &pipelineLayoutInfo, nullptr,
                                    &m_pipelineLayout));
    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
    dynamicStateCI.dynamicStateCount =
            static_cast<uint32_t>(dynamicStateEnables.size());

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateCI;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(m_core.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                       nullptr, &m_graphicsPipeline));
    vkDestroyShaderModule(m_core.getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_core.getDevice(), vertShaderModule, nullptr);
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<uint8_t> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();

    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_core.getDevice(), &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

void VulkanRenderer::createFramebuffers() {
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {m_swapChainImageViews[i]};

        const auto &extent = m_core.getSurfaceCaps().currentExtent;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(m_core.getDevice(), &framebufferInfo, nullptr,
                                     &m_swapChainFramebuffers[i]));
    }
}

void VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_core.getQueueFamily();
    VK_CHECK(vkCreateCommandPool(m_core.getDevice(), &poolInfo, nullptr, &m_commandPool));
}

void VulkanRenderer::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_commandBuffers.size();

    VK_CHECK(vkAllocateCommandBuffers(m_core.getDevice(), &allocInfo, m_commandBuffers.data()));
}

void VulkanRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(m_core.getDevice(), &semaphoreInfo, nullptr,
                                   &m_imageAvailableSemaphores[i]));

        VK_CHECK(vkCreateSemaphore(m_core.getDevice(), &semaphoreInfo, nullptr,
                                   &m_renderFinishedSemaphores[i]));

        VK_CHECK(vkCreateFence(m_core.getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]));
    }
}