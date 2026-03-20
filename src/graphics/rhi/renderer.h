#pragma once
#include "g_app_driver.h"
#include "config/client_renderer.h"
#include "graphics/graphic_libs.h"
#include "glm/glm.hpp"
#include "pipeline/pipeline_manager.h"
#include <memory>


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

// This render does not apply any graphical smoothing like antialiasing or anisotropic filtering intentionally.
// The goal is to achieve 2d editor crisp feeling.
class renderer {
public:
        renderer(std::weak_ptr<GLFWwindow> basis_win, config::visual_style in_style);
        ~renderer();

        renderer(const renderer&) = delete;
        renderer& operator=(const renderer&) = delete;
        renderer(renderer&&) noexcept = default;
        renderer& operator=(renderer&&) noexcept = default;

        bool draw_frame();

        pipeline_manager pipe;

private:
        bool create();
        void destroy();

        bool create_surface();
        void destroy_surface();

        bool create_swapchain();
        void destroy_swapchain();

        bool create_commands();
        void destroy_commands();

        bool create_sync();
        void destroy_sync();

        void recreate_swapchain();

private:
        std::weak_ptr<GLFWwindow> gl_window;
        config::visual_style style;

        std::shared_ptr<g_app_driver::driver_handler> driver_lifetime;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
        VkExtent2D swapchain_extent {};

        std::vector<VkImage> swapchain_images;
        glm::u32 swapchain_image_count = 0;
        std::vector<VkImageView> swapchain_image_views;

        VkCommandPool cmd_pool = VK_NULL_HANDLE;

        VkCommandBuffer cmd[g_app_driver::k_frames_in_flight] {};

        VkSemaphore image_available[g_app_driver::k_frames_in_flight] {};
        VkSemaphore render_finished[g_app_driver::k_frames_in_flight] {};
        VkFence in_flight[g_app_driver::k_frames_in_flight]{};
        glm::u8 frame_submitted[g_app_driver::k_frames_in_flight] = {0};
        glm::u32 frame_index = 0;
};

#endif
};