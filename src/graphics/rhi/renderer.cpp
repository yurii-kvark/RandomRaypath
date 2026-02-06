#include "renderer.h"

#include "g_app_driver.h"

#include <atomic>
#include <print>
#include <vector>
#include <cstring>
#include <fstream>
#include <chrono>
#include <mutex>
#include <set>


using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

// TODO: add vulkan debug module

static uint64_t now_ticks_ns() {
        return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
        ).count();
}

renderer::renderer(std::weak_ptr<GLFWwindow> basis_win)
        : gl_window(std::move(basis_win)) {

        if (gl_window.expired()) {
                std::println("renderer: basis_win == nullptr");
                return;
        }

        driver_lifetime = g_app_driver::thread_safe().init_driver_handler();

        if (!driver_lifetime) {
                std::println("error renderer: can't create driver.");
                return;
        }

        const bool success_creation = create();
        if (!success_creation) {
                std::println("error renderer: can't properly init Vulkan runtime.");
                return;
        }

        start_ticks = now_ticks_ns();
}


renderer::~renderer() {
        destroy();
        driver_lifetime.reset();
}


bool renderer::draw_frame() {
        if (!swapchain || !pipeline) {
                return false;
        }

        VkDevice device = g_app_driver::thread_safe().device;
        VkQueue gfx_queue = g_app_driver::thread_safe().gfx_queue;
        VkQueue present_queue = g_app_driver::thread_safe().present_queue;

        if (device == VK_NULL_HANDLE || gfx_queue == VK_NULL_HANDLE || present_queue == VK_NULL_HANDLE) {
                return false;
        }

        if (frame_submitted[frame_index]) {
                vkWaitForFences(device, 1, &in_flight[frame_index], VK_TRUE, UINT64_MAX);
        }
        vkResetFences(device, 1, &in_flight[frame_index]);
        frame_submitted[frame_index] = false;

        uint32_t imageIndex = 0;
        VkResult acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available[frame_index], VK_NULL_HANDLE, &imageIndex);
        if (acquire != VK_SUCCESS) {
                if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
                        recreate_swapchain();
                }
                return true;
        }

        VkCommandBuffer command_buffer = cmd[frame_index];
        vkResetCommandBuffer(command_buffer, 0);

        VkCommandBufferBeginInfo cb_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(command_buffer, &cb_begin_info);

        VkImageMemoryBarrier2 barrier_color{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        barrier_color.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier_color.srcAccessMask = VK_ACCESS_2_NONE;
        barrier_color.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier_color.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier_color.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier_color.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier_color.image = swapchain_images[imageIndex];
        barrier_color.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier_color.subresourceRange.levelCount = 1;
        barrier_color.subresourceRange.layerCount = 1;

        VkDependencyInfo dep1{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dep1.imageMemoryBarrierCount = 1;
        dep1.pImageMemoryBarriers = &barrier_color;
        vkCmdPipelineBarrier2(command_buffer, &dep1);

        VkClearValue clear{};
        clear.color = { { 0.02f, 0.02f, 0.02f, 1.0f } };

        VkRenderingAttachmentInfo color_render_att{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        color_render_att.imageView = swapchain_image_views[imageIndex];
        color_render_att.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_render_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_render_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_render_att.clearValue = clear;

        VkRenderingInfo render_info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        render_info.renderArea.offset = { 0, 0 };
        render_info.renderArea.extent = swapchain_extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pColorAttachments = &color_render_att;

        vkCmdBeginRendering(command_buffer, &render_info);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport_data{};
        viewport_data.x = 0.0f;
        viewport_data.y = 0.0f;
        viewport_data.width = (float)swapchain_extent.width;
        viewport_data.height = (float)swapchain_extent.height;
        viewport_data.minDepth = 0.0f;
        viewport_data.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport_data);

        VkRect2D rect_scissor{};
        rect_scissor.offset = { 0, 0 };
        rect_scissor.extent = swapchain_extent;
        vkCmdSetScissor(command_buffer, 0, 1, &rect_scissor);

        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vbuf, &off);
        vkCmdBindIndexBuffer(command_buffer, ibuf, 0, VK_INDEX_TYPE_UINT16);

        float t = (float)((now_ticks_ns() - start_ticks) / 1.0e9);
        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &t);

        vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

        vkCmdEndRendering(command_buffer);

        VkImageMemoryBarrier2 barrier_present{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        barrier_present.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier_present.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier_present.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier_present.dstAccessMask = VK_ACCESS_2_NONE;
        barrier_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier_present.image = swapchain_images[imageIndex];
        barrier_present.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier_present.subresourceRange.levelCount = 1;
        barrier_present.subresourceRange.layerCount = 1;

        VkDependencyInfo dep2{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dep2.imageMemoryBarrierCount = 1;
        dep2.pImageMemoryBarriers = &barrier_present;
        vkCmdPipelineBarrier2(command_buffer, &dep2);

        vkEndCommandBuffer(command_buffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submmit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submmit_info.waitSemaphoreCount = 1;
        submmit_info.pWaitSemaphores = &image_available[frame_index];
        submmit_info.pWaitDstStageMask = &waitStage;
        submmit_info.commandBufferCount = 1;
        submmit_info.pCommandBuffers = &command_buffer;
        submmit_info.signalSemaphoreCount = 1;
        submmit_info.pSignalSemaphores = &render_finished[frame_index];

        vkQueueSubmit(gfx_queue, 1, &submmit_info, in_flight[frame_index]);
        frame_submitted[frame_index] = true;

        VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_finished[frame_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &imageIndex;

        vkQueuePresentKHR(present_queue, &present_info);

        frame_index = (frame_index + 1) % k_frames_in_flight;

        return true;
}


bool renderer::create() {
        if (!create_surface()) {
                return false;
        }

        if (!create_swapchain()) {
                return false;
        }

        if (!create_pipeline()) {
                return false;
        }

        if (!create_buffers()) {
                return false;
        }

        if (!create_commands()) {
                return false;
        }

        if (!create_sync()) {
                return false;
        }

        return true;
}

void renderer::destroy() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device != VK_NULL_HANDLE) {
                vkDeviceWaitIdle(device);
        }

        destroy_sync();
        destroy_commands();
        destroy_buffers();
        destroy_pipeline();
        destroy_swapchain();
        destroy_surface();
}

bool renderer::create_surface() {
        if (surface != VK_NULL_HANDLE) {
                std::println("renderer: create_surface surface already exist.");
                return false;
        }

        VkInstance instance = g_app_driver::thread_safe().instance;

        if (instance == VK_NULL_HANDLE) {
                std::println("renderer: create_surface instance is null.");
                return false;
        }

        if (auto window_lock = gl_window.lock()) {
                if (glfwCreateWindowSurface(instance, window_lock.get(), nullptr, &surface) != VK_SUCCESS) {
                        std::println("renderer: glfwCreateWindowSurface failed");
                        return false;
                }

                if (driver_lifetime) {
                        driver_lifetime->try_init_with_surface(surface);
                }
        } else {
                std::println("renderer: create_surface gl_window is null.");
        }

        return surface != VK_NULL_HANDLE;
}


void renderer::destroy_surface(){
        VkInstance instance = g_app_driver::thread_safe().instance;
        if (instance == VK_NULL_HANDLE) {
                return;
        }

        if (surface != VK_NULL_HANDLE) {
                vkDestroySurfaceKHR(instance, surface, nullptr);
                surface = VK_NULL_HANDLE;
        }
}


bool renderer::create_swapchain() {
        VkPhysicalDevice physical = g_app_driver::thread_safe().physical;
        VkDevice device = g_app_driver::thread_safe().device;
        glm::u32 gfx_family = g_app_driver::thread_safe().gfx_family;
        glm::u32 present_family = g_app_driver::thread_safe().present_family;

        if (physical == VK_NULL_HANDLE || device == VK_NULL_HANDLE
                || gfx_family == UINT32_MAX || present_family == UINT32_MAX) {
                return false;
        }

        VkSurfaceCapabilitiesKHR caps{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);

        glm::u32 format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, formats.data());

        glm::u32 present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &present_mode_count, nullptr);
        std::vector<VkPresentModeKHR> present_modes(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &present_mode_count, present_modes.data());

        if (format_count == 0 || present_mode_count == 0) {
                std::println("renderer: surface has no formats or present modes");
                return false;
        }

        VkSurfaceFormatKHR chosen_format = formats[0];
        for (auto& format : formats) { // VK_FORMAT_B8G8R8A8_SRGB // VK_FORMAT_B8G8R8A8_UNORM
                if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
                        chosen_format = format;

                        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                                break;
                        }
                }
        }

        VkPresentModeKHR chosen_present = VK_PRESENT_MODE_FIFO_KHR;
        for (auto mode : present_modes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        chosen_present = mode;
                        break;
                }
        }

        VkExtent2D extent = caps.currentExtent;
        if (extent.width == 0xFFFFFFFFu) {
                int w = 0, h = 0;
                if (auto window_lock = gl_window.lock()) {
                        glfwGetFramebufferSize(window_lock.get(), &w, &h);
                }

                if (w == 0 || h == 0) {
                        std::println("renderer: gl_window({}), glfwGetFramebufferSize return 0, 0 surface size.", gl_window.expired() ? "null" : "present");
                        return false;
                }

                extent.width = static_cast<glm::u32>(w);
                extent.height = static_cast<glm::u32>(h);
                extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, extent.width));
                extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, extent.height));
        }

        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
                imageCount = caps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchain_create_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_create_info.surface = surface;
        swapchain_create_info.minImageCount = imageCount;
        swapchain_create_info.imageFormat = chosen_format.format;
        swapchain_create_info.imageColorSpace = chosen_format.colorSpace;
        swapchain_create_info.imageExtent = extent;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.preTransform = caps.currentTransform;
        swapchain_create_info.presentMode = chosen_present;
        swapchain_create_info.clipped = VK_TRUE;

        if (gfx_family != present_family) {
                glm::u32 q_idx[2] = { gfx_family, present_family };
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchain_create_info.queueFamilyIndexCount = 2;
                swapchain_create_info.pQueueFamilyIndices = q_idx;
        } else {
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (!(caps.supportedCompositeAlpha & alpha)) {
                if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
                        alpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
                } else if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
                        alpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
                } else {
                        alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
                }
        }
        swapchain_create_info.compositeAlpha = alpha;

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        swapchain_create_info.imageUsage = usage;

        if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS) {
                std::println("renderer: vkCreateSwapchainKHR failed");
                return false;
        }

        swapchain_format = chosen_format.format;
        swapchain_extent = extent;

        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
        assert(swapchain_images.empty());
        swapchain_images.resize(swapchain_image_count, {VK_NULL_HANDLE});
        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

        assert(swapchain_image_views.empty());
        swapchain_image_views.resize(swapchain_image_count, {VK_NULL_HANDLE});

        for (glm::u32 i = 0; i < swapchain_image_count; ++i) {
                VkImageViewCreateInfo image_view_ci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
                image_view_ci.image = swapchain_images[i];
                image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D; // need 2d only
                image_view_ci.format = swapchain_format;
                image_view_ci.components = {
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY };
                image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_view_ci.subresourceRange.baseMipLevel = 0;
                image_view_ci.subresourceRange.levelCount = 1;
                image_view_ci.subresourceRange.baseArrayLayer = 0;
                image_view_ci.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device, &image_view_ci, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
                        std::println("renderer: vkCreateImageView failed");
                        return false;
                }
        }

        return true;
}


void renderer::destroy_swapchain() {
        VkDevice device = g_app_driver::thread_safe().device;

        if (device == VK_NULL_HANDLE) {
                swapchain_image_views.resize(0);
                swapchain_images.resize(0);
                return;
        }

        for (auto& image_view : swapchain_image_views) {
                if (image_view != VK_NULL_HANDLE) {
                        vkDestroyImageView(device, image_view, nullptr);
                }
        }
        swapchain_image_views.resize(0);

        // Swapchain images are owned by the swapchain and must not be destroyed manually.
        swapchain_images.resize(0);

        if (swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                swapchain = VK_NULL_HANDLE;
        }
}


VkShaderModule renderer::create_shader_module_from_file(const char* path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
                std::println("renderer: failed to open shader file: {}", path);
                return VK_NULL_HANDLE;
        }

        size_t size = (size_t)file.tellg();
        file.seekg(0);

        std::vector<uint32_t> code((size + 3) / 4);
        file.read((char*)code.data(), (std::streamsize)size);

        VkShaderModuleCreateInfo smci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        smci.codeSize = code.size() * sizeof(uint32_t);
        smci.pCode = code.data();

        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return VK_NULL_HANDLE;
        }

        VkShaderModule mod = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &smci, nullptr, &mod) != VK_SUCCESS) {
                std::println("renderer: vkCreateShaderModule failed: {}", path);
                return VK_NULL_HANDLE;
        }
        return mod;
}

bool renderer::create_pipeline() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return false;
        }

        VkShaderModule vert = create_shader_module_from_file("../shaders/quad.vert.spv");
        VkShaderModule frag = create_shader_module_from_file("../shaders/rainbow.frag.spv");

        if (!vert || !frag) {
                return false;
        }

        VkPipelineShaderStageCreateInfo pipeline_stages[2]{};
        pipeline_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_stages[0].module = vert;
        pipeline_stages[0].pName = "main";

        pipeline_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_stages[1].module = frag;
        pipeline_stages[1].pName = "main";

        VkPushConstantRange pust_constant {};
        pust_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pust_constant.offset = 0;
        pust_constant.size = sizeof(float);

        VkPipelineLayoutCreateInfo pipeline_layout_cinf { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_cinf.pushConstantRangeCount = 1;
        pipeline_layout_cinf.pPushConstantRanges = &pust_constant;

        if (vkCreatePipelineLayout(device, &pipeline_layout_cinf, nullptr, &pipeline_layout) != VK_SUCCESS) {
                std::println("renderer: vkCreatePipelineLayout failed");
                return false;
        }

        VkVertexInputBindingDescription vertex_input_state_bind_desc {};
        vertex_input_state_bind_desc.binding = 0;
        vertex_input_state_bind_desc.stride = sizeof(vertex);
        vertex_input_state_bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertex_input_attr_desc[2] {};
        vertex_input_attr_desc[0].location = 0;
        vertex_input_attr_desc[0].binding = 0;
        vertex_input_attr_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attr_desc[0].offset = offsetof(vertex, pos);

        vertex_input_attr_desc[1].location = 1;
        vertex_input_attr_desc[1].binding = 0;
        vertex_input_attr_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attr_desc[1].offset = offsetof(vertex, uv);

        VkPipelineVertexInputStateCreateInfo vertex_input_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex_input_state_cinf.vertexBindingDescriptionCount = 1;
        vertex_input_state_cinf.pVertexBindingDescriptions = &vertex_input_state_bind_desc;
        vertex_input_state_cinf.vertexAttributeDescriptionCount = 2;
        vertex_input_state_cinf.pVertexAttributeDescriptions = vertex_input_attr_desc;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        input_assembly_state_cinf.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_state_cinf.viewportCount = 1;
        viewport_state_cinf.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterization_state_cinf.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_cinf.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_cinf.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_cinf.depthClampEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisample_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample_state_cinf.sampleShadingEnable = VK_FALSE;
        multisample_state_cinf.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attch_state {};
        color_blend_attch_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blend_state_cinf.logicOpEnable = VK_FALSE;
        color_blend_state_cinf.attachmentCount = 1;
        color_blend_state_cinf.pAttachments = &color_blend_attch_state;

        VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state_cinf.dynamicStateCount = 2;
        dynamic_state_cinf.pDynamicStates = dynamic_states;

        VkPipelineRenderingCreateInfo render_cinf { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        render_cinf.colorAttachmentCount = 1;
        render_cinf.pColorAttachmentFormats = &swapchain_format;

        VkGraphicsPipelineCreateInfo pipeline_create_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_create_info.pNext = &render_cinf;
        pipeline_create_info.stageCount = 2;
        pipeline_create_info.pStages = pipeline_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state_cinf;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_cinf;
        pipeline_create_info.pViewportState = &viewport_state_cinf;
        pipeline_create_info.pRasterizationState = &rasterization_state_cinf;
        pipeline_create_info.pMultisampleState = &multisample_state_cinf;
        pipeline_create_info.pColorBlendState = &color_blend_state_cinf;
        pipeline_create_info.pDynamicState = &dynamic_state_cinf;
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.layout = pipeline_layout;

        const auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);

        vkDestroyShaderModule(device, vert, nullptr);
        vkDestroyShaderModule(device, frag, nullptr);

        if (result != VK_SUCCESS) {
                std::println("renderer: vkCreateGraphicsPipelines failed");
                return false;
        }

        return true;
}

void renderer::destroy_pipeline() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
        }

        if (pipeline_layout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
                pipeline_layout = VK_NULL_HANDLE;
        }
}

glm::u32 renderer::find_memory_type(glm::u32 typeBits, VkMemoryPropertyFlags props) {
        VkPhysicalDevice physical = g_app_driver::thread_safe().physical;
        if (physical == VK_NULL_HANDLE) {
                return UINT32_MAX;
        }

        VkPhysicalDeviceMemoryProperties mp{};
        vkGetPhysicalDeviceMemoryProperties(physical, &mp);

        for (glm::u32 i = 0; i < mp.memoryTypeCount; ++i) {
                if ((typeBits & (1u << i)) && ((mp.memoryTypes[i].propertyFlags & props) == props)) {
                        return i;
                }
        }
        return UINT32_MAX;
}

bool renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& outBuf, VkDeviceMemory& outMem) {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return false;
        }

        VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bci.size = size;
        bci.usage = usage;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bci, nullptr, &outBuf) != VK_SUCCESS) {
                std::println("renderer: vkCreateBuffer failed");
                return false;
        }

        VkMemoryRequirements mr{};
        vkGetBufferMemoryRequirements(device, outBuf, &mr);

        uint32_t memType = find_memory_type(mr.memoryTypeBits, props);
        if (memType == UINT32_MAX) {
                std::println("renderer: find_memory_type failed");
                return false;
        }

        VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = memType;

        if (vkAllocateMemory(device, &mai, nullptr, &outMem) != VK_SUCCESS) {
                std::println("renderer: vkAllocateMemory failed");
                return false;
        }

        vkBindBufferMemory(device, outBuf, outMem, 0);

        return true;
}

bool renderer::create_buffers() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return false;
        }

        static const vertex verts[] = {
                {{-0.8f, -0.6f}, {0.0f, 1.0f}},
                {{ 0.8f, -0.6f}, {1.0f, 1.0f}},
                {{ 0.8f,  0.6f}, {1.0f, 0.0f}},
                {{-0.8f,  0.6f}, {0.0f, 0.0f}},
            };
        static const uint16_t idx[] = { 0, 1, 2, 2, 3, 0 };

        const bool vert_success = create_buffer(sizeof(verts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vbuf, vmem);
        const bool idx_success = create_buffer(sizeof(idx), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ibuf, imem);

        if (!vert_success || !idx_success) {
                return false;
        }
        void* p = nullptr;
        vkMapMemory(device, vmem, 0, sizeof(verts), 0, &p);
        std::memcpy(p, verts, sizeof(verts));
        vkUnmapMemory(device, vmem);

        vkMapMemory(device, imem, 0, sizeof(idx), 0, &p);
        std::memcpy(p, idx, sizeof(idx));
        vkUnmapMemory(device, imem);

        return true;
}

void renderer::destroy_buffers() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        if (vbuf) {
                vkDestroyBuffer(device, vbuf, nullptr);
        }
        if (vmem) {
                vkFreeMemory(device, vmem, nullptr);
        }
        if (ibuf) {
                vkDestroyBuffer(device, ibuf, nullptr);
        }
        if (imem) {
                vkFreeMemory(device, imem, nullptr);
        }

        vbuf = VK_NULL_HANDLE;
        vmem = VK_NULL_HANDLE;
        ibuf = VK_NULL_HANDLE;
        imem = VK_NULL_HANDLE;
}

bool renderer::create_commands() {
        VkDevice device = g_app_driver::thread_safe().device;
        glm::u32 gfx_family = g_app_driver::thread_safe().gfx_family;
        if (device == VK_NULL_HANDLE || gfx_family == UINT32_MAX) {
                return false;
        }

        VkCommandPoolCreateInfo command_pool_cinf{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        command_pool_cinf.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_cinf.queueFamilyIndex = gfx_family;

        if (vkCreateCommandPool(device, &command_pool_cinf, nullptr, &cmd_pool) != VK_SUCCESS) {
                std::println("renderer: vkCreateCommandPool failed");
                return false;
        }

        VkCommandBufferAllocateInfo buffer_allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        buffer_allocate_info.commandPool = cmd_pool;
        buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        buffer_allocate_info.commandBufferCount = k_frames_in_flight;

        if (vkAllocateCommandBuffers(device, &buffer_allocate_info, cmd) != VK_SUCCESS) {
                std::println("renderer: vkAllocateCommandBuffers failed");
                return false;
        }

        return true;
}

void renderer::destroy_commands() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        if (cmd_pool) {
                vkDestroyCommandPool(device, cmd_pool, nullptr);
                cmd_pool = VK_NULL_HANDLE;
        }
}

bool renderer::create_sync() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return false;
        }

        VkSemaphoreCreateInfo semaphore_cinf{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fence_cinf{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_cinf.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < k_frames_in_flight; ++i) {
                vkCreateSemaphore(device, &semaphore_cinf, nullptr, &image_available[i]);
                vkCreateSemaphore(device, &semaphore_cinf, nullptr, &render_finished[i]);
                vkCreateFence(device, &fence_cinf, nullptr, &in_flight[i]);
        }

        return true;
}

void renderer::destroy_sync() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        for (glm::u32 i = 0; i < k_frames_in_flight; ++i) {
                if (image_available[i]) {
                        vkDestroySemaphore(device, image_available[i], nullptr);
                }
                if (render_finished[i]) {
                        vkDestroySemaphore(device, render_finished[i], nullptr);
                }
                if (in_flight[i]) {
                        vkDestroyFence(device, in_flight[i], nullptr);
                }

                image_available[i] = VK_NULL_HANDLE;
                render_finished[i] = VK_NULL_HANDLE;
                in_flight[i] = VK_NULL_HANDLE;
        }
}

void renderer::recreate_swapchain() {
        int w = 0, h = 0;

        if (auto window_lock = gl_window.lock()) {
                glfwGetFramebufferSize(window_lock.get(), &w, &h);
        }

        if (w == 0 || h == 0) {
                return;
        }

        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        vkDeviceWaitIdle(device);

        destroy_sync();
        destroy_commands();
        destroy_buffers();
        destroy_pipeline();
        destroy_swapchain();

        if (!create_swapchain()) {
                return;
        }
        if (!create_pipeline()) {
                return;
        }
        if (!create_buffers()) {
                return;
        }
        if (!create_commands()) {
                return;
        }
        if (!create_sync()) {
                return;
        }
}

#endif