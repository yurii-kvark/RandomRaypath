#pragma once
#include "../graphic_libs.h"
#include "glm/fwd.hpp"
#include "glm/vec2.hpp"

#include <atomic>
#include <memory>
#include <mutex>

namespace ray::graphics {

// TODO: fix or remove RAY_GRAPHICS_ENABLE disabling
#if RAY_GRAPHICS_ENABLE

struct g_app_driver {
        static constexpr glm::u32 k_frames_in_flight = 2;

        struct driver_handler {
                g_app_driver* g_handler = nullptr;
                void try_init_with_surface(VkSurfaceKHR surface);
        };
        struct driver_handler_deleter {
                void operator()(driver_handler* handler) const;
        };

        static g_app_driver& thread_safe();

        g_app_driver& operator=(g_app_driver&&) = delete;

        // Explicit init, will block once everybody until done.
        std::shared_ptr<driver_handler> init_driver_handler(bool headless = false);

        ~g_app_driver();

        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physical = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

        glm::u32 gfx_family = UINT32_MAX;
        glm::u32 present_family = UINT32_MAX;
        VkQueue gfx_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;

private:
        std::atomic<int> current_drivers_num = 0;
        std::atomic<bool> init_done = false;
        mutable std::mutex init_mtx;

        bool headless_mode = false;

        void graphic_init();
        void graphic_init_surface(VkSurfaceKHR surface);
        void graphic_destroy();
};
#endif
};