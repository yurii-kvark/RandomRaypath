#include "renderer.h"

#include "g_app_driver.h"

#include <print>
#include <vector>
#include <cstring>
#include <fstream>
#include <chrono>
#include <set>


using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

// TODO: add vulkan debug module


renderer::renderer(std::weak_ptr<GLFWwindow> basis_win)
        : gl_window(std::move(basis_win)) {

        if (gl_window.expired()) {
                ray_log(e_log_type::fatal, "renderer: basis_win == nullptr");
                return;
        }

        driver_lifetime = g_app_driver::thread_safe().init_driver_handler();

        if (!driver_lifetime) {
                ray_log(e_log_type::fatal, "error renderer: can't create driver.");
                return;
        }

        const bool success_creation = create();
        if (!success_creation) {
                ray_log(e_log_type::fatal, "error renderer: can't properly init Vulkan runtime.");
                return;
        }
}


renderer::~renderer() {
        destroy();
        driver_lifetime.reset();
}


bool renderer::draw_frame() {
        if (!swapchain) {
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

        pipe.renderer_perform_draw(command_buffer, frame_index);

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

        frame_index = (frame_index + 1) % g_app_driver::k_frames_in_flight;

        return true;
}


bool renderer::create() {
        if (!create_surface()) {
                return false;
        }

        if (!create_swapchain()) {
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
        pipe.renderer_shutdown();
        destroy_swapchain();
        destroy_surface();
}

bool renderer::create_surface() {
        if (surface != VK_NULL_HANDLE) {
                ray_log(e_log_type::fatal, "renderer: create_surface surface already exist.");
                return false;
        }

        VkInstance instance = g_app_driver::thread_safe().instance;

        if (instance == VK_NULL_HANDLE) {
                ray_log(e_log_type::fatal, "renderer: create_surface instance is null.");
                return false;
        }

        if (auto window_lock = gl_window.lock()) {
                if (glfwCreateWindowSurface(instance, window_lock.get(), nullptr, &surface) != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "renderer: glfwCreateWindowSurface failed");
                        return false;
                }

                if (driver_lifetime) {
                        driver_lifetime->try_init_with_surface(surface);
                }
        } else {
                ray_log(e_log_type::fatal, "renderer: create_surface gl_window is null.");
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
                ray_log(e_log_type::fatal, "renderer: surface has no formats or present modes");
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
                        ray_log(e_log_type::fatal, "renderer: gl_window({}), glfwGetFramebufferSize return 0, 0 surface size.", gl_window.expired() ? "null" : "present");
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
                ray_log(e_log_type::fatal, "renderer: vkCreateSwapchainKHR failed");
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
                        ray_log(e_log_type::fatal, "renderer: vkCreateImageView failed");
                        return false;
                }
        }

        pipe.renderer_set_swapchain_format(swapchain_format, glm::uvec2 {swapchain_extent.width, swapchain_extent.height});

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
                ray_log(e_log_type::fatal, "renderer: vkCreateCommandPool failed");
                return false;
        }

        VkCommandBufferAllocateInfo buffer_allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        buffer_allocate_info.commandPool = cmd_pool;
        buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        buffer_allocate_info.commandBufferCount = g_app_driver::k_frames_in_flight;

        if (vkAllocateCommandBuffers(device, &buffer_allocate_info, cmd) != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "renderer: vkAllocateCommandBuffers failed");
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

        for (uint32_t i = 0; i < g_app_driver::k_frames_in_flight; ++i) {
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

        for (glm::u32 i = 0; i < g_app_driver::k_frames_in_flight; ++i) {
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
        destroy_swapchain();

        if (!create_swapchain()) {
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